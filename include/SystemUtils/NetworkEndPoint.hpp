#ifndef SYSTEM_UTILS_NETWORK_END_POINT_HPP
#define SYSTEM_UTILS_NETWORK_END_POINT_HPP

/**
 * @file NetworkEndPoint.hpp
 * 
 * this module declare the SystmeUtils::NetworkEndPoint class.
 * 
 * © 2024 by Hatem Nabli
*/

#include <stddef.h>
#include <functional>

#include "NetworkConnection.hpp"
#include "DiagnosticsSender.hpp"


namespace SystemUtils {
    
    /**
     * This class listens for incoming connections from remote objects,
     * constructing network connection objects from them and passing
     * them along to the owner. 
    */
    class NetworkEndPoint {
    public:
        /**
         * This is the type of callback function to be called whenever
         * a new client connects to the network endpoint.
        */
        typedef std::function< void(std::shared_ptr< NetworkConnection > networkConnection) > NetworkConnectionDelegate;

        /**
         * This is the type of callback function to be called whenever
         * a new datagram-oriented message is received by the network endpoint.
         * 
         * @param[in] address
         *      This is the IPv4 address of the client who sent the message.
         * 
         * @param[in] port
         *      This is the port number of the client who sent the message.
         * 
         * @param[in] body
         *      This is the contents of the datagram sent by the client.
        */
        typedef std::function< void(uint32_t address, uint16_t port, const std::vector< uint8_t >& body) > PacketReceivedDelegate;

        /**
        * These are the different sts of behavior that can be 
        * configured for a network endpoint.
        */
        enum class Mode {

            /**
             * In this mode, the network endpoint is not connection-oriented,
             * and only sends and receives unicast packets.
            */
            Datagram,

            /**
             * In this mode, the network endpoint listen for connections
             * from clients, and produces NetworkConnection objects for
             * each client connectionestablished.
            */
            Connection,

            /**
             * In this mode, the network endpoint is set up to only
             * send multicast or broadcast packets.
            */
            MulticastSend,

            /**
             * In this mode, the network endpoint is set up to only
             * receive multicast or brodcast packets.
            */
            MulticastReceive,
        };

        // Lifecycle Management
    public:
        ~NetworkEndPoint() noexcept;
        NetworkEndPoint(const NetworkEndPoint&) = delete;
        NetworkEndPoint(NetworkEndPoint&& other) noexcept;
        NetworkEndPoint& operator=(const NetworkEndPoint&) = delete;
        NetworkEndPoint& operator=(NetworkEndPoint&& other) noexcept;
        
        // Public methods 
    public: 
        /**
         * This is the instance constructor.
        */
        NetworkEndPoint();

        /**
         * This method forms a new subscription to diagnostic
         * messages published by the sender.
         *  
         * @param[in] delegate
         *       This is the function to call to deliver messages
         *       to this subscriber.
         * 
         * @param[in] minLevel
         *       This is the minimum level of message that this subscriber
         *       desires to receive.
         * 
         * @return
         *       A function is returned which may be called
         *       to terminate the subscription.
         */
        DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );

        /**
         * This method starts message or connection processing on the endpoint,
         * depending on the given mode.
         * 
         * @param[in] newConnectionDelegate
         *       This is the callback function to be called whenever
         *       a new client connects to the network endpoint.
         * 
         * @param[in] packetReceivedDelegate
         *       This is the callback function to be called whenever
         *        a new datagram-oriented message is received by the
         *        network endpoint.
         * 
         * @param[in] mode
         *        This selects the kind of processing to perform with
         *        the endpoint.
         * 
         * @param[in] localAddress
         *        This is the address to use on the network for the endpoint.
         *        It is only required for the multicast sed mode. It is not
         *        used at all for multicast received mode, since in this mode
         *        the socket request membership in multicast group on all
         *        interfaces. For datagram and connection modes, if an address
         *        is specified, it limits the traffic to a single interface.
         * 
         * @param[in] groupAddress
         *        This is the address to select for multicasting, if a multicast
         *        mode is selected.
         * 
         * @param[in] port
         *        This is the port number to use on the network. For multicast
         *        modes, it is required and is the multicast port number.
         *        For datagram and connection modes, it is optional, and if set,
         *        specifies the local port number to bind; otherwise an arbitrary
         *        ephemeral port is bound.
         * @return
         *        An indication of whether or not the method was successful is returned.
         */
        bool Open(
            NetworkConnectionDelegate newConnectionDelegate,
            PacketReceivedDelegate packetReceivedDelegate,
            Mode mode,
            uint32_t localAddress,
            uint32_t groupAddress,
            uint16_t port
        );

        /**
         * This method returns the network port that the endpoint
         * has bound for its use
         * 
         * @return
         *      The network port that the endpoint
         *      has bound for its use is returned
         */
        uint16_t GetBoundPort() const;

        /**
         * This method is used when the network endpoint is configured
         * to send datagram messages (not connection-oriented).
         * It is called to send a message to one or more recipients.
         * 
         * @param[in] address
         *      This is the IPv4 address of the receipients of the message.
         * 
         * @param[in] port 
         *      This is the port of the receipient of the message.
         * 
         * @param[in] body
         *      This is the desired payload of the message.
         */
        void SendPacket(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        );

        /**
         * This method is the opposite of the Open method. It stops
         * any and all network activity associated with the endpoint,
         * and releases any network resources previously acquired.
         */
        void Close();

        /**
         * This is a helper free function which determines the IPv4
         * addresses of all active network interfaces on the local host.
         * 
         * @return
         *       The IPv4 addresses of all active network interfaces on
         *       the local hst are returned.
         */
        static std::vector< uint32_t > GetInterfaceAddresses();

        // Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This is the type of structure that contains the platform-specific
         * private properties of the instance.  It is defined in the
         * platform-specific part of the implementation and declared here to
         * ensure that it is scoped inside the class.
         */
        struct Platform;

        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr< Impl > impl_;

    };    
    
}

#endif /* SYSTEM_UTILS_NETWORK_END_POINT_HPP */