#ifndef SYSTEM_UTILS_NETWORK_END_POINT_POSIX
#define SYSTEM_UTILS_NETWORK_END_POINT_POSIX

#include "PipeSignal.hpp"
#include <SystemUtils/NetworkEndPoint.hpp>

#include <stdint.h>
#include <vector>
#include <thread>
#include <mutex>
#include <list>



namespace SystemUtils {

    /**
     * This struct contains the Posix private properties of the
     * NetworkEndpoint class.
     */
    struct NetworkEndPoint::Platform {
        /**
         * This is the endpoint exchange packet structure.
         */
        struct Packet {
            uint32_t address;
            uint16_t port;
            std::vector< uint8_t > data;
        };

        /**
         * This is the operating system handle to the network port
         * bound by the worker.
         */
        int networkSocket = -1;

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
         * This queue pill Packet to be sent across the network
         * by the worker.
         */
        std::list< Packet > outputQueue;
    };
}


#endif /* SYSTEM_UTILS_NETWORK_END_PINT_POSIX */