/**
 * @file DataQueue.cpp
 *
 * This module contains the implementation of the
 * SystemeUtils::DataQueue class.
 *
 * © 2024 by Hatem Nabli.
 */

#include "DataQueue.hpp"
#include <deque>
#include <stddef.h>
#include <algorithm>

namespace
{
    /**
     * This represents one sequential piece of data being
     * held in a DataQueue
     */
    struct Element
    {
        /* data */
        /**
         * This hold the actual bytes in the queue element.
         */
        SystemUtils::DataQueue::Buffer data;

        /**
         * This is the number of bytes that have already been
         * consumed from this element.
         */
        size_t consumed = 0;
    };

}  // namespace
namespace SystemUtils
{
    /**
     * This holds the private properties of the DataQueue class.
     */
    struct DataQueue::Impl
    {
        // Properties

        /**
         * This is the structure of actual data in the queue.
         * It has two levels
         * Each element holds a buffer and an indication of
         * the number of bytes have already been consumed from it.
         */
        std::deque<Element> elements;

        /**
         * This keeps track of the total number of bytes
         * across the queue.
         */
        size_t totalBytes = 0;

        // Methods

        /**
         * This method tries to remove the given number
         * of bytes from the queue, based on the given mode flags.
         * Fewer bytes may be returned/removed if there are fewer bytes
         * in the queue than requested.
         *
         * @param[in] numBytes
         *      This is the number of bytes to try to remove from the queue.
         *
         * @param[in] returnData
         *      This flag indicates whether or not the data should be
         *      copied or moved to the returned buffer.
         *
         * @param[in] removeData
         *      This flag indicates shether or not the data should be
         *      removed from the queue.
         */
        auto Dequeue(size_t numBytes, bool returnData, bool removeData) -> Buffer {
            Buffer buffer;
            auto nextElement = elements.begin();
            auto numBytesLeftFromQueue = std::min(numBytes, totalBytes);
            while (numBytesLeftFromQueue > 0)
            {
                if ((nextElement->consumed == 0) &&
                    (nextElement->data.size() == numBytesLeftFromQueue) && buffer.empty())
                {
                    if (returnData)
                    {
                        if (removeData)
                        {
                            buffer = std::move(nextElement->data);
                        } else
                        { buffer = nextElement->data; }
                    }
                    if (removeData)
                    {
                        nextElement = elements.erase(nextElement);
                        totalBytes -= numBytesLeftFromQueue;
                    }
                    break;
                }
                const auto bytesToConsume = std::min(
                    numBytesLeftFromQueue, nextElement->data.size() - nextElement->consumed);
                if (returnData)
                {
                    (void)buffer.insert(
                        buffer.end(), nextElement->data.begin() + nextElement->consumed,
                        nextElement->data.begin() + nextElement->consumed + bytesToConsume);
                }
                numBytesLeftFromQueue -= bytesToConsume;
                if (removeData)
                {
                    nextElement->consumed += bytesToConsume;
                    totalBytes -= bytesToConsume;
                    if (nextElement->consumed >= nextElement->data.size())
                    { nextElement = elements.erase(nextElement); }
                } else
                {
                    if (nextElement->consumed + bytesToConsume >= nextElement->data.size())
                    { ++nextElement; }
                }
            }
            return buffer;
        }
    };

    DataQueue::~DataQueue() noexcept = default;
    DataQueue::DataQueue(DataQueue&& other) noexcept = default;
    DataQueue& DataQueue::operator=(DataQueue&& other) noexcept = default;

    DataQueue::DataQueue() : impl_(new Impl()) {}

    void DataQueue::Enqueue(const Buffer& data) {
        impl_->totalBytes += data.size();
        Element element;
        element.data = data;
        impl_->elements.push_back(std::move(element));
    }

    void DataQueue::Enqueue(Buffer&& data) {
        impl_->totalBytes += data.size();
        Element element;
        element.data = std::move(data);
        impl_->elements.push_back(std::move(element));
    }

    auto DataQueue::Dequeue(size_t numBytes) -> Buffer {
        return impl_->Dequeue(numBytes, true, true);
    }

    auto DataQueue::Peek(size_t numBytes) -> Buffer {
        return impl_->Dequeue(numBytes, true, false);
    }

    void DataQueue::Drop(size_t numBytes) { impl_->Dequeue(numBytes, false, true); }

    size_t DataQueue::GetBuffersQueued() const noexcept { return impl_->elements.size(); }

    size_t DataQueue::GetBytesQueued() const noexcept { return impl_->totalBytes; }
}  // namespace SystemUtils