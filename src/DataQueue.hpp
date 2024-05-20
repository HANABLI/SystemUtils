#ifndef SYSTEM_UTILS_DATA_QUEUE_HPP
#define SYSTEM_UTILS_DATA_QUEUE_HPP

/**
 * @file DataQueue.hpp
 * 
 * This module declares the SystemUtils::DataQueue class
 * 
 * © 2024 by Hatem Nabli
*/

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <memory>


namespace SystemUtils {

    class DataQueue
    {
    public:

        /**
         * This is a buffer of data either being enqueued
         * or dequeued.
        */
        typedef std::vector< uint8_t > Buffer;

    public:
        // Life cycle management 
        ~DataQueue() noexcept;
        DataQueue(const DataQueue&) = delete;
        DataQueue(DataQueue&&) noexcept;
        DataQueue& operator=(const DataQueue& other) = delete;
        DataQueue& operator=(DataQueue&& other) noexcept;
        // Methods
    public:
        /**
         * This is an instance constructor.
        */
        DataQueue();

        /**
         * This method puts a copy the given data onto the end
         * of the queue.
         * 
         * @param[in] data
         *      This is the data to copy and store at the end of the queue.
        */
        void Enqueue(const Buffer& data);

                /**
         * This method moves the given data onto the end
         * of the queue.
         * 
         * @param[in] data
         *      This is the data to move and store at the end of the queue.
        */
        void Enqueue(Buffer&& data);

        /**
         * This method remove the given number of bytes from
         * the queue. Fewer bytes may be retured if there are fewer
         * bytes in the queue than given.
         * 
         * @param[in] numBytes
         *      This is the number of bytes to try to remove from the queue.
         * 
         * @return
         *      The bytes actually removed from the queue are returned.
        */
        Buffer Dequeue(size_t numBytes);

        /**
         * This method used to copy a given number of bytes from
         * the queue. Fewer bytes may be retured if there are fewer
         * bytes in the queue than given.
         * 
         * @param[in] numBytes
         *      This is the number of bytes to peek from the queue.
         * @return 
         *      return the bytes peeked from the queue
        */
        Buffer Peek(size_t numBytes);

        /**
         * This methed is used to remove the given number of bytes
         * form the queue. Fewer bytes may be removed if there are fewer
         * bytes in the queue than given.
         * 
         * @param[in] numBytes
         *      This is the number of bytes to try to remove from the queue.
        */
        void Drop(size_t numBytes);

        /**
         * This method returns the number of distinct buffers of data
         * currently held in the queue.
         * 
         * @note
         *      This method is useful for unit tests. since the internal 
         *      organisation of buffers in the queue is subject to change
         *      and not intended to be part of the interface.
         * @return
         *      The number of the distinct buffers of data 
         *      currently held in the queue is returned.
         *      
        */
        size_t GetBuffersQueued() const noexcept;

        /**
         * This method returns the number of bytes currently held in the
         * queue.
         * 
         * @return
         *      Returns he number of bytes of data currently held in the 
         *      queue.
        */
        size_t GetBytesQueued() const noexcept;

        //private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
        */
        struct Impl;

        /**
        * This contains the private properties of the instance.
        */
        std::unique_ptr< Impl > impl_;       
    };

}


#endif /* SYSTEM_UTILS_DATA_QUEUE_HPP */