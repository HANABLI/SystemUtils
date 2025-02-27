/**
 * @file NetworkConnection.cpp
 *
 * This module contains the implementation of the
 * SystemUtlis::NetworkConnection class.
 *
 * © 2024 by Hatem Nabli
 */

#include "NetworkConnectionImpl.hpp"
#include <SystemUtils/NetworkConnection.hpp>
#include <StringUtils/StringUtils.hpp>
namespace SystemUtils
{
    NetworkConnection::~NetworkConnection() noexcept { Close(false); }

    NetworkConnection::NetworkConnection() : impl_(new Impl()) {}

    DiagnosticsSender::UnsubscribeDelegate NetworkConnection::SubscribeToDiagnostics(
        DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    bool NetworkConnection::Connect(uint32_t peerAddress, uint16_t peerPort) {
        impl_->peerAddress = peerAddress;
        impl_->peerPort = peerPort;
        return impl_->Connect();
    }

    bool NetworkConnection::Process(MessageReceivedDelegate messageProcessDelegate,
                                    BrokenDelegate brokenDelegate) {
        impl_->messageReceivedDelegate = messageProcessDelegate;
        impl_->brokenDelegate = brokenDelegate;
        return impl_->Process();
    }

    uint32_t NetworkConnection::GetPeerAddress() const { return impl_->peerAddress; }

    uint16_t NetworkConnection::GetPeerPort() const { return impl_->peerPort; }

    bool NetworkConnection::IsConnected() const { return impl_->IsConnected(); }

    uint32_t NetworkConnection::GetBoundAddress() const { return impl_->boundAddress; }

    uint16_t NetworkConnection::GetBoundPort() const { return impl_->boundPort; }

    void NetworkConnection::SendMessage(const std::vector<uint8_t>& message) {
        impl_->SendMessage(message);
    }

    void NetworkConnection::Close(bool clean) {
        if (impl_->Close(clean ? Impl::CloseProcedure::Graceful
                               : Impl::CloseProcedure::ImmediateAndStopProcessor))
        { impl_->brokenDelegate(false); }
    }

    uint32_t NetworkConnection::GetAddressOfHost(const std::string& hostName) {
        return Impl::GetAddressOfHost(hostName);
    }
}  // namespace SystemUtils
