/**
 * @file NetworkEndPointWin32.cpp
 *
 * This module contains the windows implementation of
 * the SystemUtils::NetworkEndPoint platform class.
 *
 * © 2024 by Hatem Nabli
 */

/**
 * WinSock2.h should be included first because if Windows.h is
 * included before it, WinSock.h gets included which conflicts
 * with WinSock2.h.
 *
 * Windows.h should always be included next because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h beforhand.
 */
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "iphlpapi")
#undef ERROR
#undef SendMessage
#undef min
#undef max

#include <inttypes.h>
#include <memory>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <string>
#include <assert.h>

#include <SystemUtils/NetworkConnection.hpp>
#include "../NetworkConnectionImpl.hpp"
#include "../NetworkEndPointImpl.hpp"
#include "NetworkEndPointWin32.hpp"
#include "NetworkConnectionWin32.hpp"

namespace
{
    /**
     * This is the maximum number of bytes to try to read
     * from a network socket at once.
     */
    constexpr size_t MAXIMUM_READ_SIZE = 65536;

}  // namespace

namespace SystemUtils
{
    NetworkEndPoint::Impl::Impl() : platform(new Platform()), diagnosticsSender("NetworkEndPoint") {
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData))
        { platform->wsaStarted = true; }
    }

    NetworkEndPoint::Impl::~Impl() {
        Close(true);
        if (platform->socketEvent != NULL)
        { (void)CloseHandle(platform->socketEvent); }
        if (platform->processorStateChangeevent != NULL)
        { (void)CloseHandle(platform->processorStateChangeevent); }
        if (platform->wsaStarted)
        { (void)WSACleanup(); }
    }

    bool NetworkEndPoint::Impl::Open() {
        // Close endpoint if it was previously open.
        Close(true);

        // socket.
        platform->socket = socket(
            AF_INET, (mode == NetworkEndPoint::Mode::Connection) ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (platform->socket == INVALID_SOCKET)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error creating socket (%d)",
                WSAGetLastError());
            return false;
        }

        if (mode == NetworkEndPoint::Mode::MulticastSend)
        {
            struct in_addr multicastInterface;
            multicastInterface.S_un.S_addr = htonl(localAddress);
            if (setsockopt(platform->socket, IPPROTO_IP, IP_MULTICAST_IF,
                           (const char*)&multicastInterface,
                           sizeof(multicastInterface)) == SOCKET_ERROR)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error setting socket option IP_MULTICAST_IF (%d)", WSAGetLastError());
            }
            Close(false);
            return false;
        } else
        {
            struct sockaddr_in socketAddress;
            (void)memset(&socketAddress, 0, sizeof(socketAddress));
            socketAddress.sin_family = AF_INET;
            if (mode == NetworkEndPoint::Mode::MulticastReceive)
            {
                BOOL option = TRUE;
                if (setsockopt(platform->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option,
                               sizeof(option)) == SOCKET_ERROR)
                {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemUtils::DiagnosticsSender::Levels::ERROR,
                        "error setting socket option SO_REUSEADDR (%d)", WSAGetLastError());
                    Close(false);
                    return false;
                }
                socketAddress.sin_addr.S_un.S_addr = INADDR_ANY;
            } else
            { socketAddress.sin_addr.S_un.S_addr = htonl(localAddress); }
            socketAddress.sin_port = htons(port);
            if (bind(platform->socket, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) !=
                0)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR, "error in bind (%d)",
                    WSAGetLastError());
                Close(false);
                return false;
            }
            if (mode == NetworkEndPoint::Mode::MulticastReceive)
            {
                for (auto localAddress : NetworkEndPoint::GetInterfaceAddresses())
                {
                    struct ip_mreq multicastGroup;
                    multicastGroup.imr_multiaddr.S_un.S_addr = htonl(groupAddress);
                    multicastGroup.imr_interface.S_un.S_addr = htonl(localAddress);
                    if (setsockopt(platform->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                   (const char*)&multicastGroup,
                                   sizeof(multicastGroup)) == SOCKET_ERROR)
                    {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "error setting socket option IP_ADD_MEMBERSHIP (%d) for local "
                            "interface %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
                            WSAGetLastError(), (uint8_t)((localAddress >> 24) & 0xFF),
                            (uint8_t)((localAddress >> 16) & 0xFF),
                            (uint8_t)((localAddress >> 8) & 0xFF), (uint8_t)(localAddress & 0xFF));
                        Close(false);
                        return false;
                    }
                }
            } else
            {
                int socketAddressLength = sizeof(socketAddress);
                if (getsockname(platform->socket, (struct sockaddr*)&socketAddress,
                                &socketAddressLength) == 0)
                {
                    port = ntohs(socketAddress.sin_port);
                } else
                {
                    diagnosticsSender.SendDiagnosticInformationFormatted(
                        SystemUtils::DiagnosticsSender::Levels::ERROR,
                        "error in getsocketname (%d)", WSAGetLastError());
                    Close(false);
                    return false;
                }
            }
        }

        // Prepare events used in processing.
        if (platform->processorStateChangeevent == NULL)
        {
            platform->processorStateChangeevent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->processorStateChangeevent == NULL)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error creating processor state change event (%d)", (int)GetLastError());
                Close(false);
                return false;
            }
        } else
        { (void)ResetEvent(platform->processorStateChangeevent); }
        platform->processorStop = false;
        if (platform->socketEvent == NULL)
        {
            platform->socketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->socketEvent == NULL)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR,
                    "error creating incoming client event (%d)", (int)GetLastError());
                Close(false);
                return false;
            }
        }
        long socketEvents = 0;
        if (mode == NetworkEndPoint::Mode::Connection)
        { socketEvents |= FD_ACCEPT; }
        if ((mode == NetworkEndPoint::Mode::Datagram) ||
            (mode == NetworkEndPoint::Mode::MulticastReceive))
        { socketEvents |= FD_READ; }
        if ((mode == NetworkEndPoint::Mode::Datagram) ||
            (mode == NetworkEndPoint::Mode::MulticastSend))
        { socketEvents |= FD_WRITE; }
        if (WSAEventSelect(platform->socket, platform->socketEvent, socketEvents) != 0)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemUtils::DiagnosticsSender::Levels::ERROR, "error in WSAEventSelect (%d)",
                WSAGetLastError());
            Close(false);
            return false;
        }

        if (mode == NetworkEndPoint::Mode::Connection)
        {
            if (listen(platform->socket, SOMAXCONN) != 0)
            {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemUtils::DiagnosticsSender::Levels::ERROR, "error in listen (%d)",
                    WSAGetLastError());
                Close(false);
                return false;
            }
        }
        diagnosticsSender.SendDiagnosticInformationFormatted(0, "endpoint opened for port %" PRIu16,
                                                             port);
        platform->processor = std::move(std::thread(&NetworkEndPoint::Impl::Processor, this));
        return true;
    }

    void NetworkEndPoint::Impl::Processor() {
        const HANDLE handles[2] = {platform->processorStateChangeevent, platform->socketEvent};
        std::vector<uint8_t> buffer;
        std::unique_lock<std::recursive_mutex> processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop)
        {
            if (wait)
            {
                processingLock.unlock();
                (void)WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
            }
            wait = true;
            buffer.resize(MAXIMUM_READ_SIZE);
            struct sockaddr_in peerAddress;
            int peerAddressSize = sizeof(peerAddress);
            if (mode == NetworkEndPoint::Mode::Connection)
            {
                const SOCKET client =
                    accept(platform->socket, (struct sockaddr*)&peerAddress, &peerAddressSize);
                if (client == INVALID_SOCKET)
                {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError != WSAEWOULDBLOCK)
                    {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::WARNING, "error in accept (%d)",
                            WSAGetLastError());
                    }
                } else
                {
                    LINGER linger;
                    linger.l_onoff = 1;
                    linger.l_linger = 0;
                    (void)setsockopt(client, SOL_SOCKET, SO_LINGER, (const char*)&linger,
                                     sizeof(linger));
                    uint32_t boundIPv4Address = 0;
                    uint16_t boundPort = 0;
                    struct sockaddr_in boundAddress;
                    int boundAddressSize = sizeof(boundAddress);
                    if (getsockname(client, (struct sockaddr*)&boundAddress, &boundAddressSize) ==
                        0)
                    {
                        boundIPv4Address = ntohl(boundAddress.sin_addr.S_un.S_addr);
                        boundPort = ntohs(boundAddress.sin_port);
                    }
                    auto connection = NetworkConnection::Platform::MakeConnectionFromExistingSocket(
                        client, boundIPv4Address, boundPort,
                        ntohl(peerAddress.sin_addr.S_un.S_addr), ntohs(peerAddress.sin_port));
                    newConnectionDelegate(connection);
                }
            } else if ((mode == NetworkEndPoint::Mode::Datagram) ||
                       (mode == NetworkEndPoint::Mode::MulticastReceive))
            {
                const int dataReceived =
                    recvfrom(platform->socket, (char*)&buffer[0], (int)buffer.size(), 0,
                             (struct sockaddr*)&peerAddress, &peerAddressSize);
                if (dataReceived == SOCKET_ERROR)
                {
                    const auto errorCode = WSAGetLastError();
                    if (errorCode != WSAEWOULDBLOCK)
                    {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR, "error in recvfrom (%d)",
                            WSAGetLastError());
                        Close(false);
                        break;
                    }
                } else if (dataReceived > 0)
                {
                    buffer.resize(dataReceived);
                    packetReceivedDelegate(ntohl(peerAddress.sin_addr.S_un.S_addr),
                                           ntohs(peerAddress.sin_port), buffer);
                }
            }
            if (!platform->outputQueue.empty())
            {
                NetworkEndPoint::Platform::Packet& packet = platform->outputQueue.front();
                (void)memset(&peerAddress, 0, sizeof(peerAddress));
                peerAddress.sin_family = AF_INET;
                peerAddress.sin_addr.S_un.S_addr = htonl(packet.address);
                peerAddress.sin_port = htons(packet.port);
                const int amountSent =
                    sendto(platform->socket, (const char*)&packet.body[0], (int)packet.body.size(),
                           0, (const sockaddr*)&peerAddress, sizeof(peerAddress));
                if (amountSent == SOCKET_ERROR)
                {
                    const auto errorCode = WSAGetLastError();
                    if (errorCode != WSAEWOULDBLOCK)
                    {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR, "error in sendto (%d)",
                            WSAGetLastError());
                        Close(false);
                        break;
                    }
                } else
                {
                    if (amountSent != (int)packet.body.size())
                    {
                        diagnosticsSender.SendDiagnosticInformationFormatted(
                            SystemUtils::DiagnosticsSender::Levels::ERROR,
                            "send truncatted (%d < %d)", amountSent, (int)packet.body.size());
                    }
                    platform->outputQueue.pop_front();
                    if (!platform->outputQueue.empty())
                    { wait = false; }
                }
            }
        }
    }

    void NetworkEndPoint::Impl::SendPacket(uint32_t address, uint16_t port,
                                           const std::vector<uint8_t>& body) {
        std::unique_lock<std::recursive_mutex> processingLock(platform->processingMutex);
        NetworkEndPoint::Platform::Packet packet;
        packet.address = address;
        packet.port = port;
        packet.body = body;
        platform->outputQueue.push_back(std::move(packet));
        (void)SetEvent(platform->processorStateChangeevent);
    }

    void NetworkEndPoint::Impl::Close(bool stopProcessing) {
        if (stopProcessing && platform->processor.joinable())
        {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeevent);
            platform->processor.join();
            platform->outputQueue.clear();
        }
        if (platform->socket != INVALID_SOCKET)
        {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0, "closing endpoint for port %" PRIu16, port);
            (void)closesocket(platform->socket);
            platform->socket = INVALID_SOCKET;
        }
    }

    std::vector<uint32_t> NetworkEndPoint::Impl::GetInterfaceAddresses() {
        // Start up winSock library.
        bool wsaStarted = false;
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData))
        { wsaStarted = true; }

        // Get address of all networ adapters.
        //
        // Recommendation of 15KB pre-allocated buffer from:
        // https://msdn.microsoft.com/en-us/library/aa365915%28v=vs.85%29.aspx
        std::vector<uint8_t> buffer(15 * 1024);
        ULONG bufferSize = (ULONG)buffer.size();
        ULONG result =
            GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)&buffer[0], &bufferSize);
        if (result == ERROR_BUFFER_OVERFLOW)
        {
            buffer.resize(bufferSize);
            result = GetAdaptersAddresses(AF_INET, 0, NULL, (PIP_ADAPTER_ADDRESSES)&buffer[0],
                                          &bufferSize);
        }
        std::vector<uint32_t> addresses;
        if (result == ERROR_SUCCESS)
        {
            for (PIP_ADAPTER_ADDRESSES adapter = (PIP_ADAPTER_ADDRESSES)&buffer[0]; adapter != NULL;
                 adapter = adapter->Next)
            {
                if (adapter->OperStatus != IfOperStatusUp)
                { continue; }
                for (PIP_ADAPTER_UNICAST_ADDRESS unicastAddress = adapter->FirstUnicastAddress;
                     unicastAddress != NULL; unicastAddress = unicastAddress->Next)
                {
                    struct sockaddr_in* ipAddress =
                        (struct sockaddr_in*)unicastAddress->Address.lpSockaddr;
                    addresses.push_back(ntohl(ipAddress->sin_addr.S_un.S_addr));
                }
            }
        }

        if (wsaStarted)
        { (void)WSACleanup(); }

        return addresses;
    }
}  // namespace SystemUtils