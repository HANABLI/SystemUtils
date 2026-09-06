#ifndef SYSTEM_UTILS_NETWORK_CONNECTION_POSIX_HPP
#define SYSTEM_UTILS_NETWORK_CONNECTION_POSIX_HPP
/**
 * @file NetworkConnectionPosix.hpp
 * @brief this is the posix platform attributes declaration
 *        of the SystemUtils::NetworkConnection class.
 * @copyright © 2026 by Hatem Nabli
 */
#include "../DataQueue.hpp"
#include "PipeSignal.hpp"

#include <SystemUtils/NetworkConnection.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <stdint.h>

namespace SystemUtils {

    struct NetworkConnection::Platform {

        /**
         * This is the operating system handle to the network port
         * bound by the worker.
         */
        int networkSock = -1;

        /**
         * This flag indicates whether or not the peer of the connection
         * has signaled a close.
         */
        bool peerClosed = false;

        /**
         * This flag indicates whether or not the connection is being
         * closed.
         */
        bool isClosing = false;

        /**
         * This flag indicates whether or not the socket has been
         * shut down (FD_CLOSE indication sent).
         */
        bool shutdownSent = false;

        /**
         * This is the thread worker which performs all the actual
         * sending and receibing of data over the network.
         */
        std::thread worker;

        /**
         * This signal is used to wake up the worker if the output
         * queue pile a new message or to tell the worker to stop.
         */
        PipeSignal workerSignal;

        /**
         * This flag indicates whether or not the worker thread should stop.
         */
        bool stopWorker = false;

        /**
         * This is used to synchronize access to the object.
         */
        std::recursive_mutex workingMutex;

        /**
         * This queue pile data to be sent across the network by the thread worker.
         */
        DataQueue dataQueue;

        /**
         * This is a factory method used to create a new NetworkConnection object out
         * of already established one.
         *
         * @param[in] networkSock
         *      This is the network socket of the established connection.
         * @param[in] boundAddress
         *      This represent the IPv4 address of the network interface bound for
         *      the established connection.
         * @param[in] boundPort
         *      This is the port number bound for the established connection.
         * @param[in] peerAddress
         *      This represent the IPv4 address of the remote peer to whish to establish
         *      the new connection.
         * @param[in] peerPort
         *      This is the port number of the remote peer to whish to establish
         *      the new connection.
         */
        static std::shared_ptr< NetworkConnection > MakeConnectionFromExistingSocket(
            int networkSock,
            uint32_t boundAddress,
            uint16_t boundPort,
            uint32_t peerAddress,
            uint16_t peerPort
        );

        /**
         * This method helper should be called to standardize what the class does when
         * it wants to close the connection.
         */
        void CloseImmediately();

    };
}

#endif /* SYSTM_UTILS_NETWORK_CONNECTION_POSIX_HPP */