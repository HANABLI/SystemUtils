#ifndef SYSTEM_UTILS_NETWORK_CONNECTION_WIN_32_HPP
#define SYSTEM_UTILS_NETWORK_CONNECTION_WIN_32_HPP

/**
 * @file NetworkConnectionWin32.hpp
 *
 * This module declares the Windows implemenetation of
 * the SystemUtils::NetworkConnection::Platform structure.
 *
 * © 2024 by Hatem Nabli
 */

#include "../DataQueue.hpp"

#include <mutex>
#include <stdint.h>
#include <vector>

#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/NetworkConnection.hpp>

namespace SystemUtils
{
    struct NetworkConnection::Platform
    {
        /**
         * This propertie keeps track of whether or not WSAStartup succeeded,
         * because if so we need to call WSACleanup upon teardown.
         */
        bool wsaStarted = false;

        /**
         * This is the operating system hndle to the network
         * port bound by this object
         */
        SOCKET socket = INVALID_SOCKET;

        /**
         * This flag indicates whether or not the peer of
         * the connection has signaled a graceful close.
         */
        bool peerClosed = false;

        /**
         * This falhg indicates whether or not the connection
         * is in the process of being gracefully closed.
         */
        bool closing = false;

        /**
         * This flag indicates whether or not the socket has
         * been shut down (FD_CLOSE indication sent)
         */
        bool shutdownSent = false;

        /**
         * This is the thread which performs all the actual
         * sending and receiving of data over the network.
         */
        std::thread processor;

        /**
         * This is an event used with WSAEventSelect in order
         * for the worker thread to wait for interesting things
         * to happen with the bound network port.
         */
        HANDLE socketEvent = NULL;

        /**
         * This is an event used to wake up the worker thread
         * if a new message to send has been placed in the
         * output queue, or if we wnat the worker thread to stop.
         */
        HANDLE processorStateChangeevent = NULL;

        /**
         * This flag indicates whether or not the worker thread
         * should stop.
         */
        bool processorStop = false;

        /**
         * This is used to synchronize access to the object.
         */
        std::recursive_mutex processingMutex;

        /**
         * This temporarily holds messages to be sent across the network
         * by the worker thread. It is filled by the SentPacket method.
         */
        DataQueue outputQueue;

        // Methods
        /**
         * This is a factory method for creating a new NetworkConnection
         * object out of an already established connection.
         *
         * @param[in] sock
         *      This is the network socket for the established connection.
         *
         * @param[in] boundAddress
         *      This is the IPv4 address of the network interface
         *      bound for the established connection.
         *
         * @param[in] boundPort
         *      This is the port number bound for the established connection.
         *
         * @param[in] peerAddress
         *      This is the port number remote peer for the connection.
         *
         * @param[in] peerPort
         *      This is the port number remote peer of the connection.
         */
        static std::shared_ptr<NetworkConnection> MakeConnectionFromExistingSocket(
            SOCKET sock, uint32_t boundAddress, uint16_t boundPort, uint32_t peerAddress,
            uint16_t peerPort);

        /**
         * This helper method is called from various places to standardize
         * what the class does wen it wants to immedately close
         * the connection.
         */
        void CloseImmediately();
    };

}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_NETWORK_CONNECTION_WIN_32_HPP */