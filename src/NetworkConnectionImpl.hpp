#ifndef SYSTEM_UTILS_NETWORK_CONNECTION_IMPL_HPP
#define SYSTEM_UTILS_NETWORK_CONNECTION_IMPL_HPP

/**
 * @file NetworkConnectionImpl.hpp
 *
 * This module declares the SystemUtils::NetworkConnectionImpl
 * structure.
 *
 * © 2024 by Hatem Nabli
 */

#include <SystemUtils/NetworkConnection.hpp>

namespace SystemUtils
{
    struct NetworkConnection::Impl : public std::enable_shared_from_this<NetworkConnection::Impl>
    {
        enum class CloseProcedure
        {
            /**
             * This indicates that the connection should be terminated
             * immediately without stopping processor thread.
             */
            ImmediateDoNotStopProcessor,

            /**
             * This indicates that the connection should be terminated
             * immediately, and the processor thread should be joined.
             */
            ImmediateAndStopProcessor,
            /**
             * This indicates that the connection should be gracefully
             * closed, meaning all data queud to be sent should first be
             * sent by the processor thread, and then the socket should
             * be market as no longer sending data, and then it should be
             * closed.
             */
            Graceful,
        };

        /**
         * This contains any platform-specific private properties
         * of the class.
         */
        std::unique_ptr<Platform> platform;

        /**
         * This is the callback issyed whenever more data
         * is received from the peer of the connection.
         */
        MessageReceivedDelegate messageReceivedDelegate;

        /**
         * This is the callback issued whenever
         * the connection is brocken.
         */
        BrokenDelegate brokenDelegate;

        /**
         * This is the IPv4 address of the peer, if there is
         * a connection established.
         */
        uint32_t peerAddress = 0;

        /**
         * This is the port number of the peer, if there is
         * a connection established.
         */
        uint16_t peerPort = 0;

        /**
         * This is the IPv4 address that the connection object
         * is using, if there is a connection established.
         */
        uint32_t boundAddress = 0;

        /**
         * This is the port number that the connection object
         * is using, if there is a connection established.
         */
        uint16_t boundPort = 0;

        /**
         * This is a helper object used to publish diagnostic messages
         */
        DiagnosticsSender diagnosticsSender;

        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        // Methods

        /**
         * This is the instance constructor.
         */
        Impl();

        /**
         * This method attempts to establish a connection to the remote peer.
         *
         * @return
         *       An indication of whether or not the connection was successfully
         *       established is returned.
         */
        bool Connect();

        /**
         * This method starts message processing on the connection
         * listening for incoming messages and sending outgoig ones.
         *
         * @return
         *      An indication of whether or not the method was
         *      successful is returned.
         */
        bool Process();

        /**
         * This is the main function called for the worker thread
         * of the object. It does all the actual sending and
         * receiving of messages, using the underlying operating
         * system network handle.
         */
        void Processor();

        /**
         * This method returns an indication of whether or not there
         * is a connection currently established whith a peer.
         *
         * @return
         *      An indication of whether or not a connection
         *      is currently established with a peer is returned.
         */
        bool IsConnected() const;

        /**
         * This method appends the given data to the queu of data
         * currently being sent to the peer. The actual sending is
         * performed by the processor worked thread.
         *
         * @param[in] message
         *      This holds the data to be append to the send queue
         */
        void SendMessage(const std::vector<uint8_t>& message);

        /**
         * This method breaks the connection to the peer.
         *
         * @param[in] procedure
         *      This indicates the procedure to follow in order
         *      to close the connection.
         * @return
         *      An indication of whether or not the broken connection
         *      delegate should called is returned.
         */
        bool Close(CloseProcedure procedure);

        /**
         * This helper method is called from various places to standdarize
         * what the class does when is wants to immediately close
         * the connection.
         */
        void CloseImmediately();

        /**
         * This is a function which determinate the IPv4
         * address of a host bay the given host name.
         *
         * @return
         *      The Ipv4 address of the host having the given
         *      name is returned
         * @retval
         *      This is returned if the IPv4 address of the host
         *      having name could not be determined.
         */
        static uint32_t GetAddressOfHost(const std::string& hostName);
    };

}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_NETWORK_CONNECTION_IMPL_HPP */