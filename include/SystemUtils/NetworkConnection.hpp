#ifndef SYSTEM_UTILS_NETWORK_CONNECTION_HPP
#define SYSTEM_UTILS_NETWORK_CONNECTION_HPP

/**
 * @file NetworkConnection.hpp
 *
 * This module declares the SystemUtils::NetworkConnection class.
 *
 * © 2024 by Hatem Nabli
 */

#include "DiagnosticsSender.hpp"
#include "INetworkConnection.hpp"

#include <vector>
#include <memory>
#include <stdint.h>

namespace SystemUtils
{
    /**
     *
     */
    class NetworkConnection : public INetworkConnection
    {
        // Rules of five Life cycle managment
    public:
        ~NetworkConnection() noexcept;
        NetworkConnection(const NetworkConnection&) = delete;
        NetworkConnection(NetworkConnection&& other) noexcept = delete;
        NetworkConnection& operator=(const NetworkConnection&) = delete;
        NetworkConnection& operator=(NetworkConnection&& other) noexcept = delete;

        // public methods
    public:
        /**
         * This is an instance constructor
         */
        NetworkConnection();

        /**
         * This is a function which determine the IPv4 address
         * of a host by the given name
         *
         * @return
         *       The Ipv4 address of the host having the given name is returned.
         * @retval 0
         *       This is returned if the IPv4 address of the host having
         *       the given name could not be determined.
         */
        static uint32_t GetAddressOfHost(const std::string& host);

        // INetworkConnection interface
    public:
        virtual DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel = 0) override;
        virtual bool Connect(uint32_t peerAddress, uint16_t peerPort) override;
        virtual bool Process(MessageReceivedDelegate messageReceivedDelegate,
                             BrokenDelegate brokenDelegate) override;
        virtual uint32_t GetPeerAddress() const override;
        virtual uint16_t GetPeerPort() const override;
        virtual bool IsConnected() const override;
        virtual uint32_t GetBoundAddress() const override;
        virtual uint16_t GetBoundPort() const override;
        virtual void SendMessage(const std::vector<uint8_t>& message) override;
        virtual void Close(bool clean = false) override;

    public:
        /**
         * This is the type of structure that contains the platform-specific
         * private properties of the instance. It is defined in the
         * platform-specific part of the implementation and declared here to
         * ensure that it is scoped inside the class.
         */
        struct Platform;
        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        // Private properties
    private:
        /**
         * This contains the private properties of the instance.
         */
        std::shared_ptr<Impl> impl_;
    };
}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_NETWORK_CONNECTION_HPP */
