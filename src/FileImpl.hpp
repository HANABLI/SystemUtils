#ifndef SYSTEM_UTILS_FILE_IMPL_HPP
#define SYSTEM_UTILS_FILE_IMPL_HPP
/**
 * @file FileImpl.hpp
 * This module contain the definition of platform-generique part 
 * of the implementation of the SystmeUtils::File class
 * 
 * © 2024 by Hatem Nabli
*/
#include <memory>
#include <SystemUtils\File.hpp>

namespace SystemUtils {

    struct File::Impl {

        /**
         * This is the path to the file in the file system.
        */
        std::string path;

        /**
         * This contains any platform-specific private properties
         * of the class.
        */
       std::unique_ptr< Platform > platform_;

       ~Impl() noexcept;
       Impl(const Impl&) = delete;
       Impl(Impl&&) noexcept;
       Impl& operator=(const Impl&) = delete;
       Impl& operator=(Impl&&);

        /**
        * This is the default constructor.
        */
        Impl();

        /**
         * This is a helper function that creates all the directories
         * in the given path that don't already exist.
         * 
         * @param[in] path
         *    This is the path for which to ensure all the directories
         *    in it are ceated if they don't already exist.
         */
        static bool CreatePath(std::string path);
    };

}

#endif /*SYSTEM_UTILS_FILE_IMPL_HPP*/