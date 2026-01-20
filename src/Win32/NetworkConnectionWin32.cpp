/**
 * @file NetworkConnectionWin32.cpp
 *
 * This module contains the windows implementation
 * of the SystemUtils::NetworkConnectionWin32 class.
 */

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#undef ERROR
#undef SendMessage
#undef min
#undef max

#include <functional>
#include <algorithm>
#include <thread>
#include <mutex>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "NetworkConnectionWin32.hpp"
#include "../NetworkConnectionImpl.hpp"

namespace
{
    static const size_t MAXIMUM_READ_SIZE = 65536;

    static const size_t MAXIMUM_WRITE_SIZE = 65536;
}  // namespace

namespace SystemUtils
{
    NetworkConnection::Impl::~Impl() {
        if (platform->processor.joinable())
        {
            if (std::this_thread::get_id() == platform->processor.get_id())
            {
                platform->processor.detach();
            } else
            { platform->processor.join(); }
        }
        if (platform->wsaStarted)
        { (void)WSACleanup(); }
        if (platform->socketEvent != NULL)
        { (void)CloseHandle(platform->socketEvent); }
        if (platform->processorStateChangeevent != NULL)
        { (void)CloseHandle(platform->processorStateChangeevent); }
    }

    NetworkConnection::Impl::Impl() :
        platform(new NetworkConnection::Platform()), diagnosticsSender("NetworkConnection") {
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData))
        { platform->wsaStarted = true; }
    }

    bool NetworkConnection::Impl::Connect() {
        if (Close(CloseProcedure::ImmediateAndStopProcessor))
        { brokenDelegate(false); }
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        platform->socket = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (platform->socket == INVALID_SOCKET)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error creating socket (%d)",
                WSAGetLastError());
            return false;
        }
        LINGER linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        (void)setsockopt(platform->socket, SOL_SOCKET, SO_LINGER, (const char*)&linger,
                         sizeof(linger));
        if (bind(platform->socket, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error in bind (%d)",
                WSAGetLastError());
            (void)Close(CloseProcedure::ImmediateDoNotStopProcessor);
            return false;
        };
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.S_un.S_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->socket, (const sockaddr*)&socketAddress, sizeof(socketAddress)) != 0)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error in connect (%d)",
                WSAGetLastError());
            (void)Close(CloseProcedure::ImmediateDoNotStopProcessor);
            return false;
        }
        int socketAddressLength = sizeof(socketAddress);
        if (getsockname(platform->socket, (struct sockaddr*)&socketAddress, &socketAddressLength) ==
            0)
        {
            boundAddress = ntohl(socketAddress.sin_addr.S_un.S_addr);
            boundPort = ntohs(socketAddress.sin_port);
        };
        return true;
    }

    bool NetworkConnection::Impl::Process() {
        if (platform->socket == INVALID_SOCKET)
        {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "not connected");
            return false;
        }
        if (platform->processor.joinable())
        {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemUtils::DiagnosticsSender::Levels::WARNING, "already connected");
            return true;
        }
        platform->processorStop = false;
        if (platform->processorStateChangeevent == NULL)
        {
            platform->processorStateChangeevent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->processorStateChangeevent == NULL)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error creating processor stat change event (%d)", (int)GetLastError());
                return false;
            }
        }
        if (platform->socketEvent == NULL)
        {
            platform->socketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->socketEvent == NULL)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error creating socket event (%d)", (int)GetLastError());
                return false;
            }
        }
        if (WSAEventSelect(platform->socket, platform->socketEvent,
                           FD_READ | FD_WRITE | FD_CLOSE) != 0)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error in WSAEventSelect (%d)",
                WSAGetLastError());
            return false;
        }
        const auto self = shared_from_this();
        platform->processor = std::thread([self] { self->Processor(); });
        return true;
    }

    void NetworkConnection::Impl::Processor() {
        const HANDLE handles[2] = {platform->processorStateChangeevent, platform->socketEvent};
        std::vector<uint8_t> buffer;
        std::unique_lock<std::recursive_mutex> processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop && (platform->socket != INVALID_SOCKET))
        {
            if (wait)
            {
                diagnosticsSender.SendDiagnosticInformationString(0, "processor going to sleep");
                processingLock.unlock();
                (void)WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
            }
            diagnosticsSender.SendDiagnosticInformationString(0, "processor woke up");
            if (platform->peerClosed)
            {
                wait = true;
            } else
            {
                buffer.resize(MAXIMUM_READ_SIZE);
                diagnosticsSender.SendDiagnosticInformationString(0, "processor trying to read");
                const int receivedData =
                    recv(platform->socket, (char*)&buffer[0], (int)buffer.size(), 0);
                if (receivedData == SOCKET_ERROR)
                {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError == WSAEWOULDBLOCK)
                    {
                        wait = true;
                    } else
                    {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1, "connection closed abruptly by the peer");
                        if (Close(CloseProcedure::ImmediateDoNotStopProcessor))
                        {
                            processingLock.unlock();
                            brokenDelegate(false);
                            processingLock.lock();
                        }
                        break;
                    }
                } else if (receivedData > 0)
                {
                    diagnosticsSender.SendDiagnosticInformationString(0,
                                                                      "processor read something");
                    wait = false;
                    buffer.resize((size_t)receivedData);
                    processingLock.unlock();
                    messageReceivedDelegate(buffer);
                    processingLock.lock();
                } else
                {
                    diagnosticsSender.SendDiagnosticInformationString(
                        1, "connection closed gracefully by peer");
                    platform->peerClosed = true;
                    processingLock.unlock();
                    brokenDelegate(true);
                    processingLock.lock();
                }
            }
            if (platform->socket == INVALID_SOCKET)
            { break; }
            const auto outputQueueLength = platform->outputQueue.GetBytesQueued();
            if (outputQueueLength > 0)
            {
                diagnosticsSender.SendDiagnosticInformationString(0, "processor trying to write");
                const auto writeSize = (int)std::min(outputQueueLength, MAXIMUM_WRITE_SIZE);
                buffer = platform->outputQueue.Peek(writeSize);
                const int dataSent = send(platform->socket, (const char*)&buffer[0], writeSize, 0);
                if (dataSent == SOCKET_ERROR)
                {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError == WSAEWOULDBLOCK)
                    {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1, "connection closed abruptly by peer");
                        if (Close(CloseProcedure::ImmediateDoNotStopProcessor))
                        {
                            processingLock.unlock();
                            brokenDelegate(false);
                            processingLock.lock();
                        }
                        diagnosticsSender.SendDiagnosticInformationString(
                            0, "processor breaking due to send error");
                        break;
                    }
                } else if (dataSent > 0)
                {
                    diagnosticsSender.SendDiagnosticInformationString(0,
                                                                      "processor wrote something ");
                    (void)platform->outputQueue.Drop(dataSent);
                    if ((dataSent == writeSize) && (platform->outputQueue.GetBytesQueued() > 0))
                    {
                        diagnosticsSender.SendDiagnosticInformationString(
                            0, "processor has more to write");
                        wait = false;
                    }
                } else
                {
                    if (Close(CloseProcedure::ImmediateDoNotStopProcessor))
                    {
                        processingLock.unlock();
                        brokenDelegate(false);
                        processingLock.lock();
                    }
                    diagnosticsSender.SendDiagnosticInformationString(
                        0, "processor breaking du to send returning 0");
                    break;
                }
            }
            if ((platform->outputQueue.GetBytesQueued() == 0) && platform->closing)
            {
                if (!platform->shutdownSent)
                {
                    diagnosticsSender.SendDiagnosticInformationString(
                        0, "processor closing and done sending");
                    shutdown(platform->socket, SD_SEND);
                    platform->shutdownSent = true;
                }
                if (platform->peerClosed)
                {
                    diagnosticsSender.SendDiagnosticInformationString(
                        0, "processor closing connection immediately");
                    CloseImmediately();
                    if (brokenDelegate != nullptr)
                    {
                        processingLock.unlock();
                        brokenDelegate(false);
                        processingLock.lock();
                    }
                }
            }
        }
        diagnosticsSender.SendDiagnosticInformationString(
            0, "processor returning due to being told to stop");
    }

    bool NetworkConnection::Impl::IsConnected() const {
        return (platform->socket != INVALID_SOCKET);
    }

    void NetworkConnection::Impl::SendMessage(const std::vector<uint8_t>& message) {
        std::unique_lock<std::recursive_mutex> processingLock(platform->processingMutex);
        platform->outputQueue.Enqueue(message);
        (void)SetEvent(platform->processorStateChangeevent);
    }

    bool NetworkConnection::Impl::Close(CloseProcedure procedure) {
        if ((procedure == CloseProcedure::ImmediateAndStopProcessor) &&
            (std::this_thread::get_id() != platform->processor.get_id()) &&
            platform->processor.joinable())
        {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeevent);
        }
        std::unique_lock<std::recursive_mutex> processingLock(platform->processingMutex);
        if (platform->socket != INVALID_SOCKET)
        {
            if (procedure == CloseProcedure::Graceful)
            {
                platform->closing = true;
                diagnosticsSender.SendDiagnosticInformationString(1, "closing connection");
                (void)SetEvent(platform->processorStateChangeevent);
            } else
            {
                // Close immediately
                CloseImmediately();
                return (brokenDelegate != nullptr);
            }
        }
        return false;
    }

    void NetworkConnection::Impl::CloseImmediately() {
        platform->CloseImmediately();
        diagnosticsSender.SendDiagnosticInformationString(1, "closed connection");
    }

    uint32_t NetworkConnection::Impl::GetAddressOfHost(const std::string& hostName) {
        bool wsaStarted = false;
        const std::unique_ptr<WSADATA, std::function<void(WSADATA*)>> WSAData(
            new WSADATA,
            [&wsaStarted](WSADATA* p)
            {
                if (wsaStarted)
                { (void)WSACleanup(); }
                delete p;
            });
        wsaStarted = !WSAStartup(MAKEWORD(2, 0), WSAData.get());
        struct addrinfo hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        struct addrinfo* rawResults;
        if (getaddrinfo(hostName.c_str(), NULL, &hints, &rawResults) != 0)
        { return 0; }
        std::unique_ptr<struct addrinfo, std::function<void(struct addrinfo*)>> results(
            rawResults, [](struct addrinfo* p) { freeaddrinfo(p); });
        if (results == NULL)
        {
            return 0;
        } else
        {
            struct sockaddr_in* ipAddress = (struct sockaddr_in*)results->ai_addr;
            return ntohl(ipAddress->sin_addr.S_un.S_addr);
        }
    }

    std::shared_ptr<NetworkConnection>
    NetworkConnection::Platform::MakeConnectionFromExistingSocket(SOCKET sock,
                                                                  uint32_t boundAddress,
                                                                  uint16_t boundPort,
                                                                  uint32_t peerAddress,
                                                                  uint16_t peerPort) {
        const auto connection = std::make_shared<NetworkConnection>();
        connection->impl_->platform->socket = sock;
        connection->impl_->boundAddress = boundAddress;
        connection->impl_->boundPort = boundPort;
        connection->impl_->peerAddress = peerAddress;
        connection->impl_->peerPort = peerPort;
        return connection;
    }

    void NetworkConnection::Platform::CloseImmediately() {
        (void)closesocket(socket);
        socket = INVALID_SOCKET;
    }
}  // namespace SystemUtils
