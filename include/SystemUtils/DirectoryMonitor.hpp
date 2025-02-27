#ifndef SYSTEM_UTILS_DIRECTORY_MONITOR_HPP
#define SYSTEM_UTILS_DIRECTORY_MONITOR_HPP

/**
 * @file DirectoryMonitor.hpp
 *
 * This module declares the SystemUtils::DirectoryMonitor class.
 *
 * © 2024 by Hatem Nabli
 */

#include <memory>
#include <string>
#include <functional>

namespace SystemUtils
{
    /**
     * This class is used to monitor for changes to a directory
     * in the file system, invoking a callback whenever any cahnge
     * is detectde.
     */
    class DirectoryMonitor
    {
    public:
        /**
         * This is provided by the owner of the object and called
         * whenever a change is detected to the monitored directory.
         */
        typedef std::function<void()> Callback;

        // Lifecycle Managment
    public:
        ~DirectoryMonitor() noexcept;
        DirectoryMonitor(const DirectoryMonitor&) = delete;
        DirectoryMonitor(DirectoryMonitor&& other) noexcept;
        DirectoryMonitor& operator=(const DirectoryMonitor&) = delete;
        DirectoryMonitor& operator=(DirectoryMonitor&& other) noexcept;

        // Public Methods
    public:
        /**
         * This is an instance constructor.
         */
        DirectoryMonitor();

        /**
         * This start the monitoring of the given directory, If any file
         * in the directory is added, changed, or removed, the given
         * callback function will be called.
         *
         * @param[in] callback
         *       This is the function to call whenever there are eny changes
         *       detected to any files in the given directory.
         * @param[in] path
         *       This is the path to the directory to monitor.
         *
         * @return
         *       An indication of whether or not the directory monitor
         *       was successfully start monitoring the given
         *       directory is returned.
         */
        bool Start(Callback callback, const std::string& path);

        /**
         * This method stop anu ongoing monitoring being done.
         */
        void Stop();

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
        std::unique_ptr<Impl> impl_;
    };
}  // namespace SystemUtils

#endif /*SYSTEM_UTILS_DIRECTORY_MONITOR_HPP*/