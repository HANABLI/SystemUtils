#ifndef SYSTEM_UTILS_SUBPROCESS_INTERNAL_HPP
#define SYSTEM_UTILS_SUBPROCESS_INTERNAL_HPP

/**
 * @file SubprocessInternal.hpp
 * 
 * This module declares functions used internally by the Subprocess module
 * of the SystmeUtils library.
 * 
 * © 2024 by Hatem Nabli
*/


namespace SystemUtils {

    /**
     * This function closes all file handles currently open in the process,
     * expect for the given one.
     * 
     * @param[in] keepOpen
     *      This is the hnadle to keep open.
    */
    void CloseAllFilesExpect(int keepOpen);
}

#endif /* SYSTEM_UTILS_SUBPROCESS_INTERNAL_HPP */