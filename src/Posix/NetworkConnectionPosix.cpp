/**
 * @file NetworkConnectionPosix.cpp
 * @brief This is the posix implementation of the SystemUtils::NetworkConnection class.
 * @copyright © 2026 by Hatem Nabli
 */

#include "NetworkConnectionPosix.hpp"
#include "../NetworkConnectionImpl.hpp"
#include <SystemUtils/DiagnosticsSender.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif /* MSG_NOSIGNAL */

namespace {
    static const size_t MAXIMUM_READ_SIZE = 65536;
    static const size_t MAXIMUM_WRITE_SIZE = 65536;
}

namespace SystemUtils {

    NetworkConnection::Impl::Impl() : platform(std::make_unique<Platform>()), diagnosticsSender("NetworkConnection") {

    }

    NetworkConnection::Impl::~Impl() noexcept {
        if (platform->worker.joinable()) {
            if (std::this_thread::get_id() == platform->worker.get_id()) {
                platform->worker.detach();
            } else {
                platform->worker.join();
            }
        }
    }

    bool NetworkConnection::Impl::Connect() {
        if (Close(CloseProcedure::ImmediateAndStopWorker)) {
            brokenDelegate(false);
        }
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        platform->networkSock = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (platform->networkSock < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error in socket creation: %s",
                strerror(errno)
            );
            return false;
        }
        struct linger linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        (void)setsockopt(platform->networkSock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
        if (bind(platform->networkSock, (struct sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error in bind: %s",
                strerror(errno)
            );
            (void)Close(CloseProcedure::ImmediateDoNotStopWorker);
            return false;
        }
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.s_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->networkSock, (const sockaddr*)&socketAddress, (socklen_t)sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error in connect: %s",
                strerror(errno)
            );
            (void)Close(CloseProcedure::ImmediateDoNotStopWorker);
            return false;
        }
        socklen_t socketAddressLength = sizeof(socketAddress);
        if (getsockname(platform->networkSock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
            boundAddress = ntohl(socketAddress.sin_addr.s_addr);
            boundPort = ntohs(socketAddress.sin_port);
        }
        int flags = fcntl(platform->networkSock, F_GETFL, 0);
        flags |= O_NONBLOCK;
        (void)fcntl(platform->networkSock, F_SETFL, flags);
        return true;
    }

    bool NetworkConnection::Impl::DoWork() {
        if (platform->networkSock < 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error in process: not connected"
            );
            return false;
        }
    #ifdef SO_NOSIGPIPE
        int opt = 1;
        if (setsockopt(platform->networkSock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt)) < 0) {
            diagnosticsSender.SendDiagnosticInformationForamtted(
                SystemUtils::DiagnosticsSender::Levels::WARNING,
                "error in setsockopt(SO_NOSIGPIPE): %s",
                strerror(errno)
            );
        }
    #endif /* SO_NOSIGPIPE */
        if (platform->worker.joinable()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::INFO,
                "already processing"
            );
            return true;
        }
        if (!platform->workerSignal.Initialize()) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR,
                "error creating worker state change event: %s",
                platform->workerSignal.GetLastError().c_str()
            );
            return false;
        }
        platform->workerSignal.Clear();
        const auto self = shared_from_this();
        platform->worker = std::thread([self]{ self->Work(); });
        return true;
    }

    void NetworkConnection::Impl::Work() {
        const int workerStateChangeSelectHandle = platform->workerSignal.GetSelectHandler();
        const int fds = std::max(workerStateChangeSelectHandle, platform->networkSock) + 1;
        fd_set readfds, writefds;
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > workingLock(platform->workingMutex);
        bool wait = true;
        while (
            !platform->stopWorker && (platform->networkSock >= 0)
        ) {
            if (wait) {
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                FD_SET(platform->stopWorker, &readfds);
                if (platform->dataQueue.GetBytesQueued() > 0) {
                    FD_SET(platform->networkSock, &writefds);
                }
                FD_SET(workerStateChangeSelectHandle, &readfds);
                workingLock.unlock();
                (void)select(fds, &readfds, &writefds, NULL, NULL);
                workingLock.lock();
                if (FD_ISSET(workerStateChangeSelectHandle, &readfds) != 0) {
                    platform->workerSignal.Clear();
                }
            }
            wait = true;
            if (platform->peerClosed) {
                wait = true;
            } else {
                buffer.resize(MAXIMUM_READ_SIZE);
                const auto amountReceived = recv(platform->networkSock, (char*)&buffer[0], (int)buffer.size(), MSG_NOSIGNAL);
                if (amountReceived < 0) {
                    if (errno == EWOULDBLOCK) {
                        wait = true;
                    } else {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection closed abruptly by peer"
                        );
                        if (Close(CloseProcedure::ImmediateDoNotStopWorker)) {
                            workingLock.unlock();
                            brokenDelegate(false);
                            workingLock.lock();
                        }
                        break;
                    }
                } else if (amountReceived > 0) {
                    buffer.resize((size_t)amountReceived);
                    wait = false;
                    workingLock.unlock();
                    messageReceivedDelegate(buffer);
                    workingLock.lock();
                } else {
                    diagnosticsSender.SendDiagnosticInformationString(
                        1,
                        "connection closed gracefully by peer"
                    );
                    platform->peerClosed = true;
                    workingLock.unlock();
                    brokenDelegate(true);
                    workingLock.lock();
                }
            }
            if (platform->networkSock < 0) {
                break;
            }
            const auto dataQueueLength = platform->dataQueue.GetBytesQueued();
            if (dataQueueLength > 0) {
                const auto writeSize = (int)std::min(dataQueueLength, MAXIMUM_WRITE_SIZE);
                buffer = platform->dataQueue.Peek(writeSize);
                const auto amountSent = send(platform->networkSock, (const char*)&buffer[0], writeSize, MSG_NOSIGNAL);
                if (amountSent < 0) {
                    if (errno != EWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection closed abruptly by peer"
                        );
                        if (Close(CloseProcedure::ImmediateDoNotStopWorker)) {
                            workingLock.unlock();
                            brokenDelegate(false);
                            workingLock.lock();
                        }
                        break;
                    }
                } else if (amountSent > 0) {
                    (void)platform->dataQueue.Drop(amountSent);
                    if (
                        (amountSent == writeSize)
                        && (platform->dataQueue.GetBytesQueued() > 0)
                    ) {
                        wait = false;
                    }
                } else {
                    if(Close(CloseProcedure::ImmediateDoNotStopWorker)) {
                        workingLock.unlock();
                        brokenDelegate(false);
                        workingLock.lock();
                    }
                    break;
                }
            }
            if (
                (platform->dataQueue.GetBytesQueued() == 0)
                && platform->isClosing
            ) {
                if (!platform->shutdownSent) {
                    shutdown(platform->networkSock, SHUT_WR);
                    platform->shutdownSent = true;
                }
                if (platform->peerClosed) {
                    CloseImmediately();
                    if (brokenDelegate != nullptr) {
                        workingLock.unlock();
                        brokenDelegate(false);
                        workingLock.lock();
                    }
                }
            }
        }
    }

    void NetworkConnection::Impl::SendMessage(const std::vector<uint8_t>& message) {
        std::lock_guard<decltype(platform->workingMutex)> lock(platform->workingMutex);
        platform->dataQueue.Enqueue(message);
        platform->workerSignal.Set();
    }

    bool NetworkConnection::Impl::IsConnected() const {
        return (platform->networkSock >= 0);
    }

    bool NetworkConnection::Impl::Close(CloseProcedure procedure) {
        if (
            (procedure == CloseProcedure::ImmediateAndStopWorker)
            && platform->worker.joinable()
        ) {
            platform->stopWorker = true;
            platform->workerSignal.Set();
        }
        std::lock_guard<decltype(platform->workingMutex)> lock(platform->workingMutex);
        if (platform->networkSock >= 0) {
            if (procedure == CloseProcedure::Graceful) {
                platform->isClosing = true;
                diagnosticsSender.SendDiagnosticInformationString(
                    1,
                    "closing connection"
                );
                platform->workerSignal.Set();
            } else {
                CloseImmediately();
                return (brokenDelegate != nullptr);
            }
        }
        return false;
    }

    void NetworkConnection::Impl::CloseImmediately() {
        platform->CloseImmediately();
        diagnosticsSender.SendDiagnosticInformationString(
            1,
            "connection was closed"
        );
    }

    uint32_t NetworkConnection::Impl::GetAddressOfHost(const std::string& host) {
        struct addrinfo hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        struct addrinfo* rawResults;
        if (getaddrinfo(host.c_str(), NULL, &hints, &rawResults) != 0) {
            return 0;
        }
        std::unique_ptr< struct addrinfo, std::function< void(struct addrinfo*) > > results(
            rawResults,
            [](struct addrinfo* p) {
                freeaddrinfo(p);
            }
        );
        if (results == NULL) {
            return 0;
        } else {
            struct sockaddr_in* ipAddress = (struct sockaddr_in*)results->ai_addr;
            return ntohl(ipAddress->sin_addr.s_addr);
        }
    }

    std::shared_ptr< NetworkConnection > NetworkConnection::Platform::MakeConnectionFromExistingSocket(
        int sock,
        uint32_t boundAddress,
        uint16_t boundPort,
        uint32_t peerAddress,
        uint16_t peerPort
    ) {
        const auto connection = std::make_shared< NetworkConnection >();
        connection->impl_->platform->networkSock = sock;
        connection->impl_->boundAddress = boundAddress;
        connection->impl_->boundPort = boundPort;
        connection->impl_->peerAddress = peerAddress;
        connection->impl_->peerPort = peerPort;
        return connection;
    }

    void NetworkConnection::Platform::CloseImmediately() {
        (void)close(networkSock);
        networkSock = -1;
    }
}