#ifndef SYSTEM_UTILS_PIPE_SIGNAL_HPP
#define SYSTEM_UTILS_PIPE_SIGNAL_HPP

/**
 * @file PipeSignal.hpp
 * @brief This file containes the Posix declaration of the SystemUtils::PipeSignal class.
 * @copyright © 2026 by Hatem Nabli.
 */

#include <memory>
#include <string>

namespace SystemUtils
{
    class PipeSignal
    {
    public:
        /**
         * This is the constructor
         */
        explicit PipeSignal();

        /**
         * This is the destructor.
         */
        ~PipeSignal();

        /**
         * This method initializes the instance.
         *
         * @return
         *      A flag indication of whether or not the instance initialization
         * succeeded is returned.
         */
        bool Initialize();

        /**
         * This return a readable string to indicate the last
         * error occurred in other methods of the instance.
         */
        std::string GetLastError() const;

        /**
         * This method sets the signal.
         */
        void Set();

        /**
         * This method clears the signal.
         */
        void Clear();

        /**
         * This method indicates whether or not the signael is set.
         */
        bool IsSet() const;

        /**
         * This method returns a file descriptor which may
         * be used with the read set in when select to wait
         * for the signal.
         */
        int GetSelectHandler() const;

    private:
        struct PipeSignalImpl;

        std::unique_ptr<struct PipeSignalImpl> impl_;
    };
}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_PIPE_SIGNAL_HPP */