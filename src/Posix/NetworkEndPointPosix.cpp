#include "NetworkEndPointPosix.hpp"
#include "../NetworkEndPointImpl.hpp"
#include "NetworkConnectionPosix.hpp"
#include "../NetworkConnectionImpl.hpp"
#include <SystemUtils/NetworkConnection.hpp>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>
#include <vector>

namespace {
    static const size_t MAXIMUM_READ_SIZE = 65536;
    static const size_t MAXIMUM_WRITE_SIZE = 65536;
}

namespace SystemUtils {

    NetworkEndPoint::Impl::Impl() : platform(std::make_unique<Platform>()), diagnosticsSender("NetworkEndPoint") {  }

    NetworkEndPoint::Impl::~Impl() noexcept {
        Close(true);
    }

    bool NetworkEndPoint::Impl::Open() {
        //Close endpoint
        Close(true);

        //Obtain Socket
        platform->networkSocket = socket(AF_INET, (mode == NetworkEndPoint::Mode::Connection) ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (platform->networkSocket < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error creating socket: %s",
                strerror(errno)
            );
            return false;
        }

        /**
         * In multicast send mode, we have to use a local address as
         * interface socket option. Otherwise, we should bind a local
         * address to the socket, either we have to configure group
         * membership if in multicast receive mode or obtain locally
         * bound port.
         */
        if (mode == NetworkEndPoint::Mode::MulticastSend) {
            struct in_addr multicastInterface;
            multicastInterface.s_addr = htonl(localAddress);
            if (setsockopt(platform->networkSocket, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&multicastInterface, sizeof(multicastInterface)) < 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error setting multicast socket: %s",
                    strerror(errno)
                );
                Close(false);
                return false;
            }
        } else {
            struct sockaddr_in peerAddress;
            (void)memset(&peerAddress, 0, sizeof(peerAddress));
            peerAddress.sin_family = AF_INET;
            if (mode == NetworkEndPoint::Mode::MulticastReceive) {
                int option = 1;
                if (setsockopt(platform->networkSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemUtils::DiagnosticsSender::Levels::ERROR,
                        "error setting option SO_REUSEADDR: %s",
                        strerror(errno)
                    );
                    Close(false);
                    return false;
                }
                peerAddress.sin_addr.s_addr = INADDR_ANY;
            } else {
                peerAddress.sin_addr.s_addr = htonl(localAddress);
            }
            peerAddress.sin_port = htons(port);
            if (bind(platform->networkSocket, (struct sockaddr*)&peerAddress, sizeof(peerAddress)) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error in bing: %s",
                    strerror(errno)
                );
                Close(false);
                return false;
            }
            if (mode == NetworkEndPoint::Mode::MulticastReceive) {
                for (auto localAddress: NetworkEndPoint::GetInterfaceAddresses()) {
                    struct ip_mreq multicastGroup;
                    multicastGroup.imr_multiaddr.s_addr = htonl(groupAddress);
                    multicastGroup.imr_interface.s_addr = htonl(localAddress);
                    if (setsockopt(platform->networkSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&multicastGroup, sizeof(multicastGroup)) < 0) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "error setting socket option IP_ADD_MEMBERSHIP: %s",
                            strerror(errno)
                        );
                        Close(false);
                        return false;
                    }
                }
            } else {
                socklen_t peerAddressLength = sizeof(peerAddress);
                if (getsockname(platform->networkSocket, (struct sockaddr*)&peerAddress, &peerAddressLength) == 0) {
                    port = ntohs(peerAddress.sin_port);
                } else {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemUtils::DiagnosticsSender::Levels::ERROR,
                        "error in getsockname: %s",
                        strerror(errno)
                    );
                    Close(false);
                    return false;
                }

            }
        }

        /**
         * Initialize signal events used in working
         */
        if (!platform->workerSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error initializing worker events signal: %s",
                platform->workerSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->workerSignal.Clear();

        if (mode == NetworkEndPoint::Mode::Connection) {
            if (listen(platform->networkSocket, SOMAXCONN) != 0) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error in listening: %s",
                    strerror(errno)
                );
                return false;
            }
        } else {
            int flags = fcntl(platform->networkSocket, F_GETFL, 0);
            flags |= O_NONBLOCK;
            (void)fcntl(platform->networkSocket, F_SETFL, flags);
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(
            SystemUtils::DiagnosticsSender::Levels::INFO,
            "endpoint opened for %" PRIu16,
            port
        );
        platform->stopWorker = false;
        platform->worker = std::thread(&NetworkEndPoint::Impl::Work, this);
        return true;
    }

    void NetworkEndPoint::Impl::Work() {
        const int workerStateChangeSelectHandle = platform->workerSignal.GetSelectHandler();
        const int nfds = std::max(workerStateChangeSelectHandle, platform->networkSocket) + 1;
        fd_set readfds, writefds;
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > workingLock(platform->workingMutex);
        bool wait = true;
        while(!platform->stopWorker) {
            if (wait) {
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                FD_SET(platform->networkSocket, &readfds);
                if (platform->outputQueue.size() > 0) {
                    FD_SET(platform->networkSocket, &writefds);
                }
                FD_SET(workerStateChangeSelectHandle, &readfds);
                workingLock.unlock();
                (void)select(nfds, &readfds, &writefds, NULL, NULL);
                workingLock.lock();
                if (FD_ISSET(workerStateChangeSelectHandle, &readfds) != 0) {
                    platform->workerSignal.Clear();
                }
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            struct sockaddr_in peerAddress;
            socklen_t peerAddressLength = (socklen_t)sizeof(peerAddress);
            if (FD_ISSET(platform->networkSocket, &readfds)) {
                if (mode == NetworkEndPoint::Mode::Connection) {
                    const int client = accept(platform->networkSocket, (struct sockaddr*)&peerAddress, &peerAddressLength);
                    if (client < 0) {
                        if (errno != EWOULDBLOCK) {
                            diagnosticsSender.SendDiagnosticInformationFormatted(
                                SystemUtils::DiagnosticsSender::Levels::WARNING,
                                "error in accept: %s",
                                strerror(errno)
                            );
                        }
                    } else {
                        struct linger linger;
                        linger.l_onoff = 1;
                        linger.l_linger = 0;
                        (void)setsockopt(client, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
                        int flags = fcntl(client, F_GETFL, 0);
                        flags |= O_NONBLOCK;
                        (void)fcntl(client, F_SETFL, flags);
                        uint32_t boundIpv4Address;
                        uint16_t boundPort = 0;
                        struct sockaddr_in boundAddress;
                        socklen_t boundAddressSize = sizeof(boundAddress);
                        if (getsockname(client, (struct sockaddr*)&boundAddress, &boundAddressSize) == 0) {
                            boundIpv4Address = ntohl(boundAddress.sin_addr.s_addr);
                            boundPort = ntohs(boundAddress.sin_port);
                        }
                        auto connection = NetworkConnection::Platform::MakeConnectionFromExistingSocket(
                            client,
                            boundIpv4Address,
                            boundPort,
                            ntohl(peerAddress.sin_addr.s_addr),
                            ntohs(peerAddress.sin_port)
                        );
                        newConnectionDelegate(connection);
                    }
                } else if (
                    (mode == NetworkEndPoint::Mode::Datagram)
                    || (mode == NetworkEndPoint::Mode::MulticastReceive)) {
                    const ssize_t dataReceived = recvfrom(
                        platform->networkSocket,
                        &buffer[0],
                        buffer.size(),
                        MSG_NOSIGNAL,
                        (struct sockaddr*)&peerAddress,
                        &peerAddressLength
                    );
                    if (dataReceived < 0) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "error in recvfrom: %s",
                            strerror(errno)
                        );
                        Close(false);
                        break;
                    } else if (dataReceived > 0) {
                        buffer.resize((size_t)dataReceived);
                        packetReceivedDelegate(
                            ntohl(peerAddress.sin_addr.s_addr),
                            ntohs(peerAddress.sin_port),
                            buffer
                        );
                    }
                }
            }
            if (!platform->outputQueue.empty()) {
                NetworkEndPoint::Platform::Packet& packet = platform->outputQueue.front();
                (void)memset(&peerAddress, 0, sizeof(peerAddress));
                peerAddress.sin_family = AF_INET;
                peerAddress.sin_addr.s_addr = htonl(packet.address);
                peerAddress.sin_port = htons(packet.port);
                const ssize_t dataSent = sendto(
                    platform->networkSocket,
                    &packet.data[0],
                    packet.data.size(),
                    MSG_NOSIGNAL,
                    (const sockaddr*)&peerAddress,
                    sizeof(peerAddress)
                );
                if (dataSent < 0) {
                    if (errno != EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "error in sendto: %s",
                            strerror(errno)
                        );
                        close(false);
                        break;
                    }
                } else {
                    if ((size_t)dataSent != packet.data.size()) {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "send truncated (%d < %d)",
                            (int)dataSent,
                            (int)packet.data.size()
                        );
                    }
                    platform->outputQueue.pop_front();
                    if(!platform->outputQueue.empty()) {
                        wait = false;
                    }
                }
            }
        }
    }

    void NetworkEndPoint::Impl::SendPacket(uint32_t address, uint16_t port, const std::vector<uint8_t>& data) {
        std::unique_lock<std::recursive_mutex> workingLock(platform->workingMutex);
        NetworkEndPoint::Platform::Packet packet;
        packet.address = address;
        packet.port = port;
        packet.data = data;
        platform->outputQueue.emplace_back(std::move(packet));
        platform->workerSignal.Set();
    }

    void NetworkEndPoint::Impl::Close(bool stopWorking) {
        if (stopWorking && platform->worker.joinable()) {
            platform->stopWorker = true;
            platform->workerSignal.Set();
            platform->worker.join();
        }
        if (platform->networkSocket >= 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing endpoint for port %" PRIu16,
                port
            );
            (void)close(platform->networkSocket);
            platform->networkSocket = -1;
        }
    }

    std::vector< uint32_t > NetworkEndPoint::Impl::GetInterfaceAddresses() {
        std::vector< uint32_t > addresses;
        struct ifaddrs* ifaddrHead;
        if (getifaddrs(&ifaddrHead) < 0) {
            return addresses;
        }
        for (
            struct ifaddrs* ifaddr = ifaddrHead;
            ifaddr != NULL;
            ifaddr = ifaddr->ifa_next
        ) {
            if ((ifaddr->ifa_flags & IFF_UP) == 0) {
                continue;
            }
            if (
                (ifaddr->ifa_flags & IFF_UP) == 0
            ) {continue;}
            if (
                (ifaddr->ifa_addr != NULL) &&
                (ifaddr->ifa_addr->sa_family == AF_INET)
            ) {
                struct sockaddr_in* ipAddress = (struct sockaddr_in*)ifaddr->ifa_addr;
                addresses.push_back(ntohl(ipAddress->sin_addr.s_addr));
            }
        }
        freeifaddrs(ifaddrHead);
        return addresses;
    }
}