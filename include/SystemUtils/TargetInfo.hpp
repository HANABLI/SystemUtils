#ifndef SYSTEM_UTILS_TARGET_INFO_HPP
#define SYSTEM_UTILS_TARGET_INFO_HPP

/**
 * @file TargetInfo.hpp
 * 
 * This module declares functions which obtain information about
 * the target ow which the program is running.
 * 
 * © 2024 by Hatem Nabli
 */

#include <string>


namespace SystemUtils {
    
    /**
     * This function returns an identifier corresponding to the machine 
     * architecture for which the currecntly running program was built.
     * 
     * @return
     *      Returns an identifier corresponding to the machine architecture for
     *      which the currently running program was build/
     */
    std::string GetTargetArchitecture();

    /**
     * This function returns an identifier corresponding to the build
     * variant that was selected to build the currently running
     * program.
     * 
     * @return
     *      Returns an identifier corresponding to the build variant
     *      that was selected to build the currently running
     *      program.
     */
    std::string GetTargetVariant();
}

#endif /* SYSTEM_UTILS_TARGET_INFO_HPP */