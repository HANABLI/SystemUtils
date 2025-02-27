/**
 * @file NetworkEndPointTests.cpp
 *
 * This module contains the unit tests of the
 * SystemUtils::NetworkEndPoint class.
 *
 * © 2024 by Hatem Nabli
 */

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <SystemUtils/NetworkEndPoint.hpp>

#ifdef _WIN32
/**
 * WinSock2.h should be included first because if Windows.h is
 * included before it, WinSock.h gets included which conflicts
 * with WinSock2.h.
 *
 * Windows.h should always be included next because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h beforhand.
 */
#    define IPV4_ADDRESS_IN_SOCKADDR sin_addr.S_un.S_addr
#    define SOCKADDR_LENGTH_TYPE int
#    include <WinSock2.h>
#    include <Windows.h>
#    include <WS2tcpip.h>
#    include <iphlpapi.h>
#    pragma comment(lib, "ws2_32")
#    pragma comment(lib, "IPHlpApi")
#    undef ERROR
#    undef SendMessage
#    undef min
#    undef max
#else
#    include <sys/socket.h>
#endif /* _WIN32 or POSIX */

namespace
{
    /**
     * This is used to hold all the information about a received
     * datagram that we care about.
     */
    struct Packet
    {
        /**
         *  This is a copy of the data from the received datagram
         */
        std::vector<uint8_t> body = {};

        /**
         * This is the IPv4 address of the datagram sender.
         */
        uint32_t address = 0;

        /**
         * This is the port number of the datagram sender
         */
        uint16_t port = 0;

        Packet(const std::vector<uint8_t>& newBody, uint32_t newAddress, uint16_t newPort) :
            body(newBody), address(newAddress), port(newPort) {}
    };

    /**
     * This is used to receive callbacks from unit under tests.
     */
    struct Owner
    {
        /**
         * This is used to synchronize access to the class
         */
        std::mutex mutex;

        /**
         * This is used to wait for, or signal, a condition upon
         * which that the owner might be waiting.
         */
        std::condition_variable_any condition;

        /**
         * This is holds a copy of each packet received.
         */
        std::vector<Packet> packetsReceived;

        /**
         * This holds the data received from a connection-oriented stream.
         */
        std::vector<uint8_t> streamReceived;

        /**
         * These are connections that have been established
         * between the unit under test and remote clients.
         */
        std::vector<std::shared_ptr<SystemUtils::NetworkConnection>> connections;

        /**
         * This flag indicates whether or not a connection
         * to the network endPoint has been broken.
         */
        bool connectionBroken = false;

        /**
         * This method waits up to a second for a packet
         * to be received from the network endpoint.
         *
         * @return
         *      An indication of whether or not a packet
         *      is received form the network endpoint is returned.
         */
        bool AwaitPacket() {
            std::unique_lock<decltype(mutex)> lock(mutex);
            return condition.wait_for(lock, std::chrono::seconds(1),
                                      [this] { return !packetsReceived.empty(); });
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
            std::unique_lock<decltype(mutex)> lock(mutex);
            return condition.wait_for(lock, std::chrono::seconds(1),
                                      [this] { return !connections.empty(); });
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
            std::unique_lock<decltype(mutex)> lock(mutex);
            return condition.wait_for(lock, std::chrono::seconds(1),
                                      [this, numBytes]
                                      { return (streamReceived.size() >= numBytes); });
        }

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         */
        void NetworkEndPointNewConnection(
            std::shared_ptr<SystemUtils::NetworkConnection> newConnection) {
            std::unique_lock<decltype(mutex)> lock(mutex);
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process([this](const std::vector<uint8_t>& message)
                                         { NetworkConnectionMessageReceived(message); },
                                         [this](bool) { NetworkConnectionBrocken(); });
        }

        /**
         * This is the callback issued whenever more data
         * is received from the peer of the connection.
         */
        void NetworkConnectionMessageReceived(const std::vector<uint8_t>& message) {
            std::unique_lock<decltype(mutex)> lock(mutex);
            streamReceived.insert(streamReceived.end(), message.begin(), message.end());
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever the connection
         * is broken.
         */
        void NetworkConnectionBrocken() {
            std::unique_lock<decltype(mutex)> lock(mutex);
            connectionBroken = true;
            condition.notify_all();
        }

        void NetworkEndPointPacketReceived(uint32_t address, uint16_t port,
                                           const std::vector<uint8_t>& body) {
            std::unique_lock<decltype(mutex)> lock(mutex);
            packetsReceived.emplace_back(body, address, port);
            condition.notify_all();
        }
    };

}  // namespace

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct NetworkEndPointTests : public ::testing::Test
{
    /**
     * This keeps track of whether or not WSAStartup succeeded,
     * because if so we need to call WSACleanup upon teardown.
     */
    bool wsaStarted = false;

    virtual void SetUp() {
#if _WIN32
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData))
        { wsaStarted = true; }
#endif /* _WIN32 */
    }

    virtual void TearDown() {
#if _WIN32
        if (wsaStarted)
        { (void)WSACleanup(); }
#endif /* _WIN32 */
    }
};

TEST_F(NetworkEndPointTests, NetworkEndpointTests_DatagramSending_Test) {
    auto receiver = socket(AF_INET, SOCK_DGRAM, 0);
#if _WIN32
    ASSERT_FALSE(receiver == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(receiver < 0);
#endif /* _WIN32 or POSIX */

    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    int receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) ==
                0);
    port = ntohs(receiverAddress.sin_port);

    // Set up the NetworkEndPoint.
    SystemUtils::NetworkEndPoint endPoint;
    Owner owner;
    endPoint.Open([&owner](std::shared_ptr<SystemUtils::NetworkConnection> newConnection)
                  { owner.NetworkEndPointNewConnection(newConnection); },
                  [&owner](uint32_t address, uint16_t port, const std::vector<uint8_t>& body)
                  { owner.NetworkEndPointPacketReceived(address, port, body); },
                  SystemUtils::NetworkEndPoint::Mode::Datagram, 0, 0, 0);

    // Test sending a datagram from the unit under test
    const std::vector<uint8_t> testPacket{0x12, 0x34, 0x56, 0x78};
    endPoint.SendPacket(0x7F000001, port, testPacket);

    // Verify that we received the datagram.
    struct sockaddr_in senderAddress;
    int senderAddressSize = sizeof(senderAddress);
    std::vector<uint8_t> buffer(testPacket.size() * 2);
    const int amountReceived = recvfrom(receiver, (char*)buffer.data(), (int)buffer.size(), 0,
                                        (struct sockaddr*)&senderAddress, &senderAddressSize);
    ASSERT_EQ(testPacket.size(), amountReceived);
    buffer.resize(amountReceived);
    ASSERT_EQ(testPacket, buffer);
    ASSERT_EQ(0x7F000001, ntohl(senderAddress.sin_addr.S_un.S_addr));
    ASSERT_EQ(endPoint.GetBoundPort(), ntohs(senderAddress.sin_port));
}

TEST_F(NetworkEndPointTests, NetworkEndPointTests_DatagramReceiving_Test) {
    auto sender = socket(AF_INET, SOCK_DGRAM, 0);
#if _WIN32
    ASSERT_FALSE(sender == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(sender < 0);
#endif /* _WIN32 or POSIX */

    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    int senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndPoint.
    SystemUtils::NetworkEndPoint endPoint;
    Owner owner;
    endPoint.Open([&owner](std::shared_ptr<SystemUtils::NetworkConnection> newConnection)
                  { owner.NetworkEndPointNewConnection(newConnection); },
                  [&owner](uint32_t address, uint16_t port, const std::vector<uint8_t>& body)
                  { owner.NetworkEndPointPacketReceived(address, port, body); },
                  SystemUtils::NetworkEndPoint::Mode::Datagram, 0, 0, 0);

    // Test receiving a datagram at the unit under test
    const std::vector<uint8_t> testPacket{0x12, 0x34, 0x56, 0x78};
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endPoint.GetBoundPort());
    (void)sendto(sender, (const char*)testPacket.data(), (int)testPacket.size(), 0,
                 (const sockaddr*)&receiverAddress, sizeof(receiverAddress));

    // Verify that we received the datagram.
    ASSERT_TRUE(owner.AwaitPacket());
    ASSERT_EQ(testPacket, owner.packetsReceived[0].body);
    ASSERT_EQ(0x7F000001, owner.packetsReceived[0].address);
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.packetsReceived[0].port);
}

TEST_F(NetworkEndPointTests, NetworkEndPointTests_ConnectionSending_Test) {
    auto receiver = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(receiver == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(receiver < 0);
#endif /* _WIN32 or POSIX */

    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = 0;
    receiverAddress.sin_port = 0;
    ASSERT_TRUE(bind(receiver, (struct sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);
    int receiverAddressLength = sizeof(receiverAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&receiverAddress, &receiverAddressLength) ==
                0);
    port = ntohs(receiverAddress.sin_port);

    // Set up the NetworkEndPoint.
    SystemUtils::NetworkEndPoint endPoint;
    Owner owner;
    endPoint.Open([&owner](std::shared_ptr<SystemUtils::NetworkConnection> newConnection)
                  { owner.NetworkEndPointNewConnection(newConnection); },
                  [&owner](uint32_t address, uint16_t port, const std::vector<uint8_t>& body)
                  { owner.NetworkEndPointPacketReceived(address, port, body); },
                  SystemUtils::NetworkEndPoint::Mode::Connection, 0, 0, 0);

    // Connect to the NetworkEndPoint.
    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    senderAddress.sin_port = htons(endPoint.GetBoundPort());
    ASSERT_TRUE(connect(receiver, (const sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);

    ASSERT_TRUE(owner.AwaitConnection());

    struct sockaddr_in socketAddress;
    SOCKADDR_LENGTH_TYPE socketAddressLength = sizeof(socketAddress);
    ASSERT_TRUE(getsockname(receiver, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0);
    ASSERT_EQ(ntohl(senderAddress.IPV4_ADDRESS_IN_SOCKADDR),
              owner.connections[0]->GetBoundAddress());
    ASSERT_EQ(ntohs(senderAddress.sin_port), owner.connections[0]->GetBoundPort());

    // Test sending a message from the unit under test.
    const std::vector<uint8_t> testPacket{0x12, 0x34, 0x56, 0x78};
    owner.connections[0]->SendMessage(testPacket);

    // Verify that we received the message from the unit test.
    std::vector<uint8_t> buffer(testPacket.size());
    const int amountReceived = recv(receiver, (char*)buffer.data(), (int)buffer.size(), 0);

    ASSERT_EQ(testPacket.size(), amountReceived);
    ASSERT_EQ(testPacket, buffer);
}

TEST_F(NetworkEndPointTests, NetworkEndPointTests_ConnectionReceiving_Test) {
    auto sender = socket(AF_INET, SOCK_STREAM, 0);
#if _WIN32
    ASSERT_FALSE(sender == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(sender < 0);
#endif /* _WIN32 or POSIX */

    struct sockaddr_in senderAddress;
    (void)memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddress.sin_family = AF_INET;
    senderAddress.sin_addr.S_un.S_addr = 0;
    senderAddress.sin_port = 0;
    ASSERT_TRUE(bind(sender, (struct sockaddr*)&senderAddress, sizeof(senderAddress)) == 0);
    int senderAddressLength = sizeof(senderAddress);
    uint16_t port;
    ASSERT_TRUE(getsockname(sender, (struct sockaddr*)&senderAddress, &senderAddressLength) == 0);
    port = ntohs(senderAddress.sin_port);

    // Set up the NetworkEndPoint.
    SystemUtils::NetworkEndPoint endPoint;
    Owner owner;
    endPoint.Open([&owner](std::shared_ptr<SystemUtils::NetworkConnection> newConnection)
                  { owner.NetworkEndPointNewConnection(newConnection); },
                  [&owner](uint32_t address, uint16_t port, const std::vector<uint8_t>& body)
                  { owner.NetworkEndPointPacketReceived(address, port, body); },
                  SystemUtils::NetworkEndPoint::Mode::Connection, 0, 0, 0);

    // Connect to the NetworkEndPoint.
    struct sockaddr_in receiverAddress;
    (void)memset(&receiverAddress, 0, sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.S_un.S_addr = htonl(0x7F000001);
    receiverAddress.sin_port = htons(endPoint.GetBoundPort());
    ASSERT_TRUE(connect(sender, (const sockaddr*)&receiverAddress, sizeof(receiverAddress)) == 0);

    ASSERT_TRUE(owner.AwaitConnection());

    // Test receiving a message at the unit under test.
    const std::vector<uint8_t> testPacket{0x12, 0x34, 0x56, 0x78};
    (void)send(sender, (char*)testPacket.data(), (int)testPacket.size(), 0);

    // Verify that we received the message at the unit under test.
    owner.AwaitStream(testPacket.size());
    ASSERT_EQ(testPacket, owner.streamReceived);
}
