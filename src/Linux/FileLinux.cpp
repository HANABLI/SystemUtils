#include "../Posix/FilePosix.hpp"
#include <SystemUtils/File.hpp>
#include <StringUtils/StringUtils.hpp>
#include <vector>
#include <string>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdint.h>

namespace SystemUtils
{
    std::string File::GetExeImagePath() {
        std::vector<char> buffer(PATH_MAX);
        (void)realpath("/proc/self/exe", &buffer[0]);
        return std::string(&buffer[0]);
    }

    std::string File::GetExeParentDirectory() {
        std::vector<char> buffer(PATH_MAX);
        (void)realpath("/proc/self/exe", &buffer[0]);
        auto length = strlen(&buffer[0]);
        while (--length > 0)
        {
            if (buffer[length] == '/')
            { break; }
        }
        if (length == 0)
        { ++length; }
        buffer[length] = '\0';
        return std::string(&buffer[0]);
    }

    std::string File::GetResourceFilePath(const std::string& name) {
        return StringUtils::sprintf("%s%s", GetExeParentDirectory().c_str(), name.c_str());
    }

    std::string File::GetLocalPerUserConfigDirectory(const std::string& nameKey) {
        return StringUtils::sprintf("%s%s", GetUserHomeDirectory().c_str(), nameKey.c_str());
    }

    std::string File::GetUserSavedProjectDirectory(const std::string& nameKey) {
        return StringUtils::sprintf("%s/ .%s/Saved Projects", GetUserHomeDirectory().c_str(),
                                    nameKey.c_str());
    }
}  // namespace SystsemUtils