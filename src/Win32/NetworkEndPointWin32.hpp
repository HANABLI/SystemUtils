#ifndef SYSTEM_UTILS_NETWORK_END_POINT_WIN32_HPP 
#define SYSTEM_UTILS_NETWORK_END_POINT_WIN32_HPP

/**
 * @file NetworkEndPointWin32.hpp
 * 
 * this module declares the Windows implementation of the
 * SystemUtils::NetworkEndPoint platform structure.
 * 
 * © 2024 by Hatem Nabli  
*/

#include <vector>
#include <thread>
#include <mutex>
#include <list>
#include <stdint.h>
#include <SystemUtils/NetworkEndPoint.hpp>

namespace SystemUtils {

    struct NetworkEndPoint::Platform 
    {
        /**
         * This is used to hold all information about
         * a datagram to be sent.
        */
        struct Packet {
            /**
             * This is the IPv4 address of the datagram recipient.
            */
            uint32_t address;

            /**
             * This is the port number of the datagram recipient
            */
            uint16_t port;

            /**
            * This is the message to sent in the datagram.
            */
            std::vector< uint8_t > body;
        };
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
    std::list< Packet > outputQueue;
    };
   
}

#endif /* SYSTEM_UTILS_NETWORK_END_POINT_WIN32_HPP */
