/**
 * @file DynamicLibraryImpl.cpp
 * @brief This is the linux implementation of the Posix DynamicLibrary properties.
 * @copyright © 2026 by Hatem Nabli.
 */
#include "../Posix/DynamicLibraryImpl.hpp"

namespace SystemUtils
{
    std::string DynamicLibrary::Impl::GetDynamicLibraryFileExtension() { return "so"; }

}  // namespace SystemUtils