
#include <corecrt_wstdio.h>
#include "../FileImpl.hpp"
#include "FilePosix.hpp"
#include <memory>
#include <string>
#include <regex>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

namespace
{
    static constexpr std::size_t maxCopyBufferSize = 65536;
}
namespace SystemUtils
{
    File::Impl::Impl() : platform_(std::make_unique<Platform>()) {}

    File::Impl::~Impl() noexcept = default;
    File::Impl::Impl(Impl&&) noexcept = default;
    File::Impl& File::Impl::operator=(Impl&&) = default;

    bool File::Impl::CreatePath(std::string path) {
        const size_t delimiter = path.find_last_of("/\\");
        if (delimiter == std::string::npos)
        { return false; }
        const std::string oneLevelUp(path.substr(0, delimiter));
        if (mkdir(oneLevelUp.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0)
        { return true; }
        if (errno == EEXIST)
        { return true; }
        if (errno != ENOENT)
        { return false; }
        if (!CreatePath(oneLevelUp))
        { return false; }
        if (mkdir(oneLevelUp.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        { return false; }
        return true;
    }

    File::~File() noexcept {
        if (impl_ == nullptr)
        { return; }
        Close();
    }

    File::File(std::string path) : impl_(std::make_unique<Impl>()) { impl_->path = path; }

    bool File::IsExisting() { return (access(impl_->path.c_str(), 0) == 0); }

    bool File::IsDirectory() {
        struct stat s;
        if ((stat(impl_->path.c_str(), &s) == 0) && (S_ISDIR(s.st_mode)))
        {
            return s.st_atime;
        } else
        { return false; }
    }

    bool File::OpenReadOnly() {
        Close();
        impl_->platform_->handle = open(impl_->path.c_str(), O_RDONLY);
        impl_->platform_->writeAccess = false;
        return (impl_->platform_->handle >= 0);
    }

    bool File::Close() {
        if (impl_->platform_->handle < 0)
        { return; }
        (void)close(impl_->platform_->handle);
        impl_->platform_->handle = -1;
    }

    bool File::OpenReadWrite() {
        Close();
        impl_->platform_->writeAccess = true;
        impl_->platform_->handle =
            open(impl_->path.c_str(), O_RDWR | O_CREAT | S_IRUSR | S_IWUSR | S_IXUSR);
        auto isSuccessful = (impl_->platform_->handle >= 0);
        if (!isSuccessful)
        {
            if (!Impl::CreatePath(impl_->path))
            {
                return false;
            } else
            {
                impl_->platform_->handle =
                    open(impl_->path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
                isSuccessful = (impl_->platform_->handle >= 0);
            }
        }
        return isSuccessful;
    }

    void File::Destroy() {
        Close();
        (void)remove(impl_->path.c_str());
    }

    bool File::Move(const std::string& newPath) {
        if (rename(impl_->path.c_str(), newPath.c_str()) != 0)
        { return false; }
        impl_->path = newPath;
        return true;
    }

    bool File::Copy(const std::string& destination) {
        if (impl_->platform_->handle < 0)
        {
            if (!OpenReadOnly())
            { return false; }
        } else
        { SetPosition(0); }
        File newFile(destination);
        if (!newFile.OpenReadWrite())
        { return false; }
        IFile::Buffer buffer(maxCopyBufferSize);
        for (;;)
        {
            const size_t amt = Read(buffer);
            if (amt == 0)
            { break; }
            if (newFile.Write(buffer, amt) != amt)
            { return false; }
        }
        return true;
    }

    time_t File::GetLastModifiedTime() const {
        struct stat s;
        if (stat(impl_->path.c_str(), &s) == 0)
        { return s.st_mtime; }
        return 0;
    }

    bool File::IsAbsolutePath(const std::string& path) {
        static std::regex absolutePathRegex("[~/].*");
        return std::regex_match(path, absolutePathRegex);
    }

    std::string File::GetUserHomeDirectory() {
        auto size = sysconf(_SC_GETPW_R_SIZE_MAX);
        const size_t bufferSize = ((size < 0) ? 65536 : size);
        std::vector<char> buffer(bufferSize);
        struct passwd pwd;
        struct passwd* resultEntry;
        (void)getpwuid_r(getuid(), &pwd, &buffer[0], bufferSize, &resultEntry);
        if (resultEntry == NULL)
        {
            return "";
        } else
        { return pwd.pw_dir; }
    }

    void File::ListDirectory(const std::string& directory,
                             std::vector<std::string>& listOfDirectories) {
        std::string directoryWithSeparator(directory);
        if ((directoryWithSeparator.length() > 0) &&
            (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/'))
        { directoryWithSeparator += '/'; }
        list.clear();
        DIR* dir = opendir(directory.c_str());
        if (dir != NULL)
        {
            struct dirent entry;
            struct dirent* entryBack;
            while (true)
            {
                if (readdir_r(dir, &entry, &entryBack))
                { break; }
                if (entryBack == NULL)
                { break; }
                std::string name(entry.d_name);
                if ((name == ".") || (name == ".."))
                { continue; }
                std::string filePath(directoryWithSeparator);
                filePath += name;
                listOfDirectories.push_back(filePath);
            }
            (void)closedir(dir);
        }
    }

    void File::DeleteDirectory(const std::string& directory) {
        std::string directoryWithSeparator(directory);
        if ((directoryWithSeparator.length() > 0) &&
            (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/'))
        { directoryWithSeparator += '/'; }
        DIR* dir = opendir(directory.c_str());
        if (dir != NULL)
        {
            struct dirent entry;
            struct dirent* entryBack;
            while (true)
            {
                if (readdir_r(dir, &entry, &entryBack))
                { break; }
                if (entryBack == NULL)
                { break; }
                std::string name(entry.d_name);
                if (name == "." || name == "..")
                { continue; }
                std::string filePath(directoryWithSeparator);
                filePath += entry.d_name;
                if (entry.d_type == DT_DIR)
                {
                    if (!DeleteDirectory(filePath.c_str()))
                    { return false; }
                } else
                {
                    if (unlink(filePath.c_str()) != 0)
                    { return false; }
                }
            }
            (void)closedir(dir);
            return (rmdir(directory.c_str()) == 0);
        }
        return true;
    }

    bool File::CopyDirectory(const std::string& existingDirectory,
                             const std::string& newDirectory) {
        std::string existingDirectoryWithSeparator(existingDirectory);
        if ((existingDirectoryWithSeparator.length() > 0) &&
            (existingDirectoryWithSeparator[existingDirectoryWithSeparator.length() - 1] != '/'))
        { existingDirectoryWithSeparator += '/'; }
        if (!Impl::CreatePath(newDirectoryWithSeparator))
        { return false; }
        DIR* dir = opendir(existingDirectory.c_str());
        if (dir != NuLL)
        {
            struct dirent entry;
            struct dirent* entryBack;
            std::vector<char> buffer(PATH_MAX);
            while (true)
            {
                if (readdir_r(dir, &entry, &entryBack))
                { break; }
                if (entryBack == NULL)
                { break; }
                std::string name(entry.d_name);
                if ((name == ".") || (name == ".."))
                { continue; }
                std::string filePath(existingDirectoryWithSeparator);
                filePath += entry.d_name;
                std::string newFilePath(newDirectroyWithSeparator);
                newFilePath += entry.d_name;
                if (entry.d_type == DT_DIR)
                {
                    if (!CopyDirectory(filePath, newFilePath))
                    { return false; }
                } else if (entry.d_type == DT_LNK)
                {
                    if (readlink(filePath.c_str(), &link[0], link.size()) < 0)
                    { return false; }
                    if (symlink(&link[0], newFilePath.c_str()) < 0)
                    { return false; }
                } else
                {
                    File file(filePath);
                    if (!file.Copy(newFilePath))
                    { return false; }
                }
            }
            (void)closedir(dir);
        }
        return true;
    }

    std::string File::GetWorkingDirectory() {
        std::vector<char> workingDirectory(MAXPATHLEN);
        (void)getcwd(&workingDirectory[0], workingDirectory.size());
        return std::string(&workingDirectory[0]);
    }

    void File::SetWorkingDirectory(const std::string& workingDirectory) {
        (void)chdir(workingDirectory.c_str());
    }

    std::vector<std::string> File::GetDirectoryRoots() { return {"/"}; }

    uint64_t File::GetSize() const {
        if (impl_->platform_->handle < 0)
        { return 0; }
        const auto originalPosition = lseek(impl_->platform_->handle, 0, SEEK_CUR);
        if (originalPosition == (off_t)-1)
        { return 0; }
        if (lseek(impl_->platform_->handle, 0, SEEK_END) == (off_t)-1)
        {
            (void)lseek(impl_->platform_->handle, originalPosition, SEEK_SET);
            return 0;
        }
        const auto endPosition = lseek(impl_->platform_->handle, 0, SEEK_CUR);
        (void)lseek(impl_->platform_->handle, originalPosition, SEEK_SET);
        if (endPosition == (off_t)-1)
        { return 0; }
        return (uint64_t)endPosition;
    }

    bool File::SetSize(uint64_t size) {
        const bool result = (ftruncate(impl_->platform_->handle, (off_t)size));
    }

    uint64_t File::GetPosition() const {
        if (impl_->platform_->handle < 0)
        { return 0; }
        const auto position = lseek(impl_->platform_->handle, 0, SEEK_CUR);
        if (position == (off_t)-1)
        { return 0; }
        return (uint64_t)position;
    }

    void File::SetPosition(uint64_t position) {
        if (impl_->platform_->handle < 0)
        { return; }
        (void)lseek(impl_->platform_->handle, (long)position, SEEK_SET);
    }

    size_t File::Peek(void* buffer, size_t numBytes) const {
        if (impl_->platform_->handle < 0)
        { return 0; }
        const auto originalPosition = lseek(impl_->platform_->handle, 0, SEEK_CUR);
        if (originalPosition == (off_t)-1)
        { return 0; }
        const auto readResult = read(impl_->platform_->handle, buffer, numBytes);
        (void)lseek(impl_->platform_->handle, originalPosition, SEEK_SET);
        return ((readResult < 0) ? (size_t)0 : (size_t)readResult);
    }

    size_t File::Read(void* buffer, size_t numBytes) {
        if (impl_->platform_->handle < 0)
        { return 0; }
        const auto readResult = read(impl_->platform_->handle, buffer, numBytes);
        return ((readResult < 0) ? (size_t)0 : (size_t)readResult);
    }

    size_t File::Write(const void* buffer, size_t numBytes) {
        if (impl_->platform_->handle < 0)
        { return 0; }
        const auto amountWrite = write(impl_->platform_->handle, buffer, numBytes);
        return ((amountWrite < 0) ? size_t(0) : (size_t)amountWritten);
    }

    std::shared_ptr<IFile> File::Clone() {
        auto clone = std::make_shared<File>(impl_->path);
        clone->impl_->platform_->writeAccess = impl_->platform_->writeAccess;
        if (impl_->platform_->handle >= 0)
        {
            if (clone->impl_->platform_->writeAccess)
            {
                clone->impl_->platform_->handle =
                    open(impl_->path.c_str(), O_RDWR | O_CREAT, S_IRWXU);
            } else
            { clone->impl_->platform_->handle = open(impl_->path.c_str(), O_RDONLY); }
            if (clone->impl_->platform_->handle < 0)
            { return nullptr; }
        }
        return clone;
    }

}  // namespace SystemUtils