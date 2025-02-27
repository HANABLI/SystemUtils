#ifndef SYSTEM_UTILS_NETWORK_END_POINT_IMPL_HPP
#define SYSTEM_UTILS_NETWORK_END_POINT_IMPL_HPP

/**
 * @file NetworkEndPointImpl.hpp
 *
 * This module declares the SystemUtils::NetworkEndPoint::Impl
 * structure
 *
 * © 2024 by Hatem Nabli
 */

#include <memory>
#include <stdint.h>
#include <vector>

#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/NetworkEndPoint.hpp>

namespace SystemUtils
{
    struct NetworkEndPoint::Impl
    {
        // Properties

        /**
         * This contains any platform-specific private properties
         * of the class.
         */
        std::unique_ptr<Platform> platform;

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         */
        NetworkConnectionDelegate newConnectionDelegate;

        /**
         * This is the callback function to be called whenever
         * a new datagram-oriented message is received by the
         * network endpoint.
         */
        PacketReceivedDelegate packetReceivedDelegate;

        /**
         * This is the IPv4 address of the network interface
         * bound by this endpoint. If zero, then all network
         * interfaces are bound.
         */
        uint32_t localAddress = 0;

        /**
         * This is the multicast or brodcast address to which
         * the network endpoint is bound, if in one of the
         * multicast modes.
         */
        uint32_t groupAddress = 0;

        /**
         * This is the port number bound by this endpoint.
         */
        uint16_t port = 0;

        /**
         * This is the set of behaviors configured for
         * the network endpoint.
         */
        Mode mode = Mode::Datagram;

        /**
         * This is a helper object used to publish diagnostic messages.
         */
        DiagnosticsSender diagnosticsSender;

        // Lifecycle Management
        ~Impl() noexcept;
        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept;

        // Methods

        /**
         * This is the instance constructor.
         */
        Impl();

        /**
         * This method starts message or connection processing on
         * the endpoint, depending on the configured mode
         */
        bool Open();

        /**
         * This is the main function called for the worker thread
         * of the object. It support sending and receiving of messages,
         * using the underlying operating system network handle.
         */
        void Processor();

        /**
         * This method is used when the network endpoint is configured
         * to send datagram messages (not connection-oriented).
         * It is called to send a message to one or more receipients.
         *
         * @param[in] address
         *      This is the IPv4 address of the receipient of the message.
         *
         * @param[in] port
         *      This is the port of the receipient of the message.
         *
         * @param[in] body
         *      This is the desired payload of the message.
         */
        void SendPacket(uint32_t address, uint16_t port, const std::vector<uint8_t>& body);

        /**
         * This method is the opposite of the Open method. It stops
         * any and all network activity associated with the endpoint,
         * and releases any network resources previously acquired.
         *
         * @param[in] stopProcessing
         *      This flag indicates whether or not the network thread
         *      should be joined if it's running.
         */
        void Close(bool stopProcessing);

        /**
         * This is a helper free function which determines the IPv4
         * addresses of all active network interfaces on the local
         * host.
         *
         * @return
         *      The IPv4 addresses of all active network interfaces on
         *      the local host are returned.
         */
        static std::vector<uint32_t> GetInterfaceAddresses();
    };
}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_NETWORK_END_POINT_IMPL_HPP */