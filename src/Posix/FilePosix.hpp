#ifndef SYSTEM_UTILS_FILE_POSIX_HPP
#define SYSTEM_UTILS_FILE_POSIX_HPP

#include <SystemUtils/File.hpp>

namespace SystemUtils
{
    struct File::Platform
    {
        /**
         * This is the operating system handle to the file.
         */
        int handle = -1;
        /**
         * This flag indicates whether or not the file was
         * opened with write capability.
         */
        bool writeAccess = false;
    };
}  // namespace SystemUtils

#endif