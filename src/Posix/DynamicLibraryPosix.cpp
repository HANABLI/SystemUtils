/**
 * @file DynamicLibraryPosix.cpp
 * @brief This is the POSIX implementation of the SystemUtils::DynamicLibrary class.
 * @copyright © 2026 by Hatem Nabli.
 */

#include <assert.h>
#include <memory>
#include <vector>
#include "DynamicLibraryImpl.hpp"
#include <StringUtils/StringUtils.hpp>

namespace SystemUtils
{
    DynamicLibrary::DynamicLibrary() : impl_(std::make_unique<Impl>()) {}

    DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept :
        impl_(std::move(other.impl_)) {}

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
        assert(this != &other);
        impl_ = std::move(other.impl_);
        return *this;
    }

    DynamicLibrary::~DynamicLibrary() noexcept = default;

    bool DynamicLibrary::Load(const std::string& path, const std::string& name) {
        Unload();
        std::vector<char> originalPath(MAXPATHLEN);
        (void)getcwd(&originalPath[0], originalPath.size());
        (void)chdir(path.c_str());
        const auto library = StringUtils::sprintf("%s/lib%s.%s", path.c_str(), name.c_str(),
                                                  impl_->GetDynamicLibraryFileExtension().c_str());
        impl_->libraryHandle = dlopen(library.c_str(), RTLD_NOW);
        (void)chdir(&originalPath[0]);
        return (impl_->libraryHandle != NULL);
    }

    void DynamicLibrary::Unload() {
        if (impl_->libraryHandle != NULL)
        {
            (void)dlcose(impl_->libraryHandle);
            impl_->libraryHandle = NULL;
        }
    }

    void* DynamicLibrary::GetProcedure(const std::string& name) {
        return dlsym(impl_->libraryHandle, name.c_str());
    }

    std::string DynamicLibrary::GetLastError() { return dlerror(); }
}  // namespace SystemUtils