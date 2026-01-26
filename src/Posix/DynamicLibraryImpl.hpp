#ifndef SYSTEM_UTILS_DYNAMIC_LIBRARY_IMPL_HPP
#define SYSTEM_UTILS_DYNAMIC_LIBRARY_IMPL_HPP
/**
 * @file DynamicLibraryImpl.hpp
 * @brief This contains the declaration of the POSIX DynamicLibrary properties.
 * @copyright © 2026 by Hatem Nabli.
 */

#include <SystemUtils/DynamicLibrary.hpp>

namespace SystemUtils
{
    /**
     * This structure contains the declaration of the POSIX
     * DynamicLibrary properties.
     */
    struct DynamicLibrary::Impl
    {
        // Properties
        void* libraryHandle = NULL;

        // Methods
        static std::string GetDynamicLibraryFileExtension();
    };
}  // namespace SystemUtils

#endif