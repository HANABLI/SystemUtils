/**
 * @file NetworkEndPoint.cpp
 *
 * this module represent the implementation of the
 * SystemUtils::NetworkEndPoint Class
 *
 * © 2024 by Hatem Nabli
 */

#include "NetworkEndPointImpl.hpp"
#include <SystemUtils/NetworkEndPoint.hpp>

namespace SystemUtils
{
    NetworkEndPoint::~NetworkEndPoint() noexcept = default;
    NetworkEndPoint::NetworkEndPoint(NetworkEndPoint&& other) noexcept = default;
    NetworkEndPoint& NetworkEndPoint::operator=(NetworkEndPoint&& other) noexcept = default;

    NetworkEndPoint::NetworkEndPoint() : impl_(new Impl()) {}

    DiagnosticsSender::UnsubscribeDelegate NetworkEndPoint::SubscribeToDiagnostics(
        DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return impl_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }

    void NetworkEndPoint::SendPacket(uint32_t address, uint16_t port,
                                     const std::vector<uint8_t>& body) {
        impl_->SendPacket(address, port, body);
    }

    bool NetworkEndPoint::Open(NetworkConnectionDelegate networkConnectionDelegate,
                               PacketReceivedDelegate packetReceivedDelegate, Mode mode,
                               uint32_t localAddress, uint32_t groupAddress, uint16_t port) {
        impl_->newConnectionDelegate = networkConnectionDelegate;
        impl_->packetReceivedDelegate = packetReceivedDelegate;
        impl_->mode = mode;
        impl_->localAddress = localAddress;
        impl_->groupAddress = groupAddress;
        impl_->port = port;
        return impl_->Open();
    }

    uint16_t NetworkEndPoint::GetBoundPort() const { return impl_->port; }

    void NetworkEndPoint::Close() { impl_->Close(true); }

    std::vector<uint32_t> NetworkEndPoint::GetInterfaceAddresses() {
        return Impl::GetInterfaceAddresses();
    }
}  // namespace SystemUtils