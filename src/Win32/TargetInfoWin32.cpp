/**
 * @file TargetInfoWin32.cpp
 * 
 * This module contains the windows implementation 
 * of the SystemUtils::TargetInfo functions
 */

#include <SystemUtils/TargetInfo.hpp>

namespace SystemUtils {

    std::string GetTargetArchtecture() {
#if defined(_WIN64)
        return "x64";
#else
        return "x86";
#endif
    }

    std::string GetTargetVariant() {
#if defined(_DEBUG)
        return "Debug";
#else 
        return "Release";
#endif
    }
}