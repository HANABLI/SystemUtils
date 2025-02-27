/**
 * @file DynamicLibraryWin32.cpp
 *
 * This module contains the Window implementation of the
 * SystmeUtils::DynamicLibrary class.
 *
 * © 2024 by Hatem Nabli
 *
 */

#include <Windows.h>
#include <vector>
#include <assert.h>

#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/DynamicLibrary.hpp>

namespace SystemUtils
{
    struct DynamicLibrary::Impl
    {
        HMODULE libraryHandle = NULL;
    };

    DynamicLibrary::~DynamicLibrary() {
        if (impl_ == nullptr)
        { return; }
        Unload();
    }

    DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept :
        impl_(std::move(other.impl_)) {}

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }

    DynamicLibrary::DynamicLibrary() : impl_(new Impl()) {}

    bool DynamicLibrary::Load(const std::string& path, const std::string& name) {
        Unload();
        std::vector<char> originalPath(MAX_PATH);
        (void)GetCurrentDirectoryA((DWORD)originalPath.size(), &originalPath[0]);
        (void)SetCurrentDirectoryA(path.c_str());
        const auto library = StringUtils::sprintf("%s/%s.dll", path.c_str(), name.c_str());
        impl_->libraryHandle = LoadLibraryA(library.c_str());
        (void)SetCurrentDirectoryA(&originalPath[0]);
        return (impl_->libraryHandle != NULL);
    }

    void DynamicLibrary::Unload() {
        if (impl_->libraryHandle != NULL)
        { (void)FreeLibrary(impl_->libraryHandle); }
        impl_->libraryHandle = NULL;
    }

    void* DynamicLibrary::GetProcedure(const std::string& name) {
        return GetProcAddress(impl_->libraryHandle, name.c_str());
    }

    std::string DynamicLibrary::GetLastError() {
        return StringUtils::sprintf("%lu", (unsigned long)::GetLastError());
    }
}  // namespace SystemUtils
