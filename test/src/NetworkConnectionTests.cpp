/**
 * @file NetworkConnectionTests.cpp
 * 
 * this module contains the unit tests of
 * the NetworkConnection class.
*/
#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include <vector>
#include <condition_variable>
#include <SystemUtils/NetworkConnection.hpp>
#include <SystemUtils/NetworkEndPoint.hpp>
#include <StringUtils/StringUtils.hpp>

#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "IPHlpApi")
#undef ERROR
#undef SendMessage
#undef min
#undef max
#else /* POSIX */
#include <sys/socket.h>
#endif

namespace {
    /**
     * 
    */
    struct Packet
    {
        /**
         *  This is a copy of the data from the received datagram 
         */
        std::vector< uint8_t > body = {};

        /**
         * This is the IPv4 address of the datagram sender.
        */
        uint32_t address = 0;

        /**
        * This is the port number of the datagram sender
        */
        uint16_t port = 0;

        Packet(
            const std::vector< uint8_t >& newBody,
            uint32_t newAddress,
            uint16_t newPort
        ) 
            : body(newBody)
            , address(newAddress)
            , port(newPort)
        {
        }
    };

    struct Owner {

        /**
         * This is used to wait for, or signal, a condition
         * upon which that the owner might be waiting.
        */
        std::condition_variable_any condition;

        /**
         * This is used to synchronize acces to the class.
        */
        std::mutex mutex;

        /**
         * This holds a copy of each packet received.
        */
        std::vector< Packet > packetsReceived;

        /**
         * This holds the data received from a connection-oriented stream.
        */
        std::vector< uint8_t > streamReceived;

        /**
         * These are connections that have been established between
         * the unit test and remote clients.
        */
        std::vector< std::shared_ptr< SystemUtils::NetworkConnection> > connections;

        /**
         * This is an indication of whether or not a connection
         * is brocken.
        */
        bool connectionBroken = false;

        /**
         * This is an indication of whether or not a connection
         * was brocken gracefully
        */
        bool connectionBrokenGracefully = false;

        /**
         * This is a function to call when the connection is closed
         * 
         * @param[in]
         *      This indicates whether or not the peer of connection
         *      has closed the connection gracefully. 
        */
        std::function< void(bool graceful) > connectionBrokenDelegate;

        /**
         * This method waits up to a second for a packet
         * to be received from the network endpoint.
         * 
         * @return
         *      An indication of whether or not a packet
         *      is received form the network endpoint is returned.
        */
        bool AwaitPacket() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !packetsReceived.empty();
                }
            );
        }

        /**
         * This method waits up to a second for a connection
         * to be received from a network endpoint.
         * 
         * @return
         *      An indication of whether or not a connection 
         *      is received from the networkend point is returned.
        */
        bool AwaitConnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !connections.empty();
                }
            );
        }

        /**
         * This method waits up to a second for a number of bytes
         * to be received from a client connected to the network endpoint.
         * 
         * @param[in] numBytes
         *      This is the number of bytes we expect to receive.
         * 
         * @return
         *      an indication of whether or not a number of bytes are
         *      received from a client connected to the network endpoint.
        */
        bool AwaitStream(size_t numBytes) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numBytes]{
                    return (streamReceived.size() >= numBytes);
                }
            );
        }

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
        */
        void NetworkEndPointNewConnection(std::shared_ptr< SystemUtils::NetworkConnection > newConnection) {
            std::unique_lock< decltype(mutex) > lock(mutex) ;
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process(
                [this](const std::vector< uint8_t >& message){
                    NetworkConnectionMessageReceived(message);
                },
                [this](bool graceful){
                    NetworkConnectionBroken(graceful);
                }
            );
        }


        /**
         * This is the callback issued whenever more data
         * is received from the peer of the connection.
        */
        void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            streamReceived.insert(
                streamReceived.end(),
                message.begin(),
                message.end()
            );
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever the connection
         * is broken.
        */
        void NetworkConnectionBroken(bool graceful) {
            if (connectionBrokenDelegate != nullptr) {
                connectionBrokenDelegate(graceful);
            }
            std::unique_lock< decltype(mutex) > lock(mutex);
            connectionBroken = true;
            connectionBrokenGracefully = graceful;
            condition.notify_all();
        }

        void NetworkEndPointPacketReceived(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            packetsReceived.emplace_back(body, address, port);
            condition.notify_all();
        }
    };
    
}

struct NetworkConnectionTests : public ::testing::Test {
    //
    /**
     * This keeps track of whether or not WSAStartup succeeded,
     * because if we need to call WSACleanup upon teardown.
    */
    bool wsaStarted = false;

    /**
    * This is the unit inder test client
    */
    SystemUtils::NetworkConnection client;

    /**
     * This is used to capture callbacks from the unit under test.
     */
    std::shared_ptr< Owner > clientOwner = std::make_shared< Owner >();

    /**
     * These are the diagnostic messages that have been
     * received from the unit under test
     */
    std::vector< std::string > diagnosticMessages;

    /**
     * This is the delegate obtained when subscribing
     * to receive diagnostic messages from the unit test.
     * It's called to terminate the subscribtion.
    */
   SystemUtils::DiagnosticsSender::UnsubscribeDelegate diagnosticUnsubscribeDelegate;

   /**
    * If this flag is set, we will print all received diagnostic
    * messages, in addition to storing them.
   */
    bool printDiagnosticMessages = false;

    virtual void SetUp() {
#if _WIN32
        WSADATA WSAData;
        if (!WSAStartup(MAKEWORD(2, 0), &WSAData)) {
            wsaStarted = true;
        }
#endif /* _WIN32 */
        diagnosticUnsubscribeDelegate = client.SubscribeToDiagnostics(
            [this](
                std::string senderName,
                size_t level,
                std::string message 
            ){
                diagnosticMessages.push_back(
                    StringUtils::sprintf(
                        "%s[%zu]: %s",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    )
                );
                if (printDiagnosticMessages) {
                    printf(
                        "%s[%zu]: %s\n",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    );
                }
            },
            1
        );
    }

    virtual void TearDown() {
        diagnosticUnsubscribeDelegate();
#if _WIN32
        if (wsaStarted) {
            (void)WSACleanup();
        }
#endif /* _WIN32 */
    }
};

TEST_F(NetworkConnectionTests, NetworkConnectionTests_EstablishConnection__Test) {
    SystemUtils::NetworkEndPoint server;
    std::vector< std::shared_ptr< SystemUtils::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [&clients, &callbackCondition, &callbackMutex](
        std::shared_ptr< SystemUtils::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        callbackCondition.notify_all();
    };
    const auto packetReceiveDelegate = [](
        
            uint32_t address,
            uint16_t port,
            const std::vector<uint8_t>& body    
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceiveDelegate,
            SystemUtils::NetworkEndPoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
}

TEST_F(NetworkConnectionTests, NetworkConnectionTests_SendingMessage_Test) {
    SystemUtils::NetworkEndPoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemUtils::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [&clients, &callbackCondition, &callbackMutex, &serverConnectionOwner](
        std::shared_ptr< SystemUtils::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(
            newConnection);
            ASSERT_TRUE(newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceiveDelegate = [](
        
            uint32_t address,
            uint16_t port,
            const std::vector<uint8_t>& body    
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceiveDelegate,
            SystemUtils::NetworkEndPoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientConnectionOwner = clientOwner;
    ASSERT_TRUE(client.Process(
        [clientConnectionOwner](const std::vector< uint8_t >& message ){
            clientConnectionOwner->NetworkConnectionMessageReceived(message);
        },
        [clientConnectionOwner](bool graceful){
            clientConnectionOwner->NetworkConnectionBroken(graceful);
        }
    ));
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageOfbytes(messageAsString.begin(), messageAsString.end());
    client.SendMessage(messageOfbytes);
    ASSERT_TRUE(serverConnectionOwner.AwaitStream(messageOfbytes.size()));
    ASSERT_EQ(messageOfbytes, serverConnectionOwner.streamReceived);
}

TEST_F(NetworkConnectionTests, NetworkConnectionTests_ReceivingMessage_Test) {
        SystemUtils::NetworkEndPoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemUtils::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [&clients, &callbackCondition, &callbackMutex, &serverConnectionOwner](
        std::shared_ptr< SystemUtils::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(
            newConnection);
            ASSERT_TRUE(newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceiveDelegate = [](
        
            uint32_t address,
            uint16_t port,
            const std::vector<uint8_t>& body    
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceiveDelegate,
            SystemUtils::NetworkEndPoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientConnectionOwner = clientOwner;
    ASSERT_TRUE(client.Process([clientConnectionOwner](const std::vector< uint8_t >& message ){
        clientConnectionOwner->NetworkConnectionMessageReceived(message);
    },
    [clientConnectionOwner](bool graceful){
        clientConnectionOwner->NetworkConnectionBroken(graceful);
    })); 
    const std::string messageAsString("Hello, World");
    const std::vector< uint8_t > messageOfBytes(messageAsString.begin(), messageAsString.end());
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    clients[0]->SendMessage(messageOfBytes);
    ASSERT_TRUE(clientOwner->AwaitStream(messageOfBytes.size()));
    ASSERT_EQ(messageOfBytes, clientOwner->streamReceived);
}
