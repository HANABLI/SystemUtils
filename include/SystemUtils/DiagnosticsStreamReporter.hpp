#ifndef SYSTEM_UTILS_DIAGNOSTICS_STREAM_REPORTER_HPP
#define SYSTEM_UTILS_DIAGNOSTICS_STREAM_REPORTER_HPP
/**
 * @file DiagnosticsStreamReporter.hpp
 * 
 * This module declares the SystemAbstractions::DiagnosticsStreamReporter class.
 * 
 * © 2024 by Hatem Nabli
*/

#include <stdio.h>

#include "DiagnosticsSender.hpp"

namespace SystemUtils {
    /**
     * This function returns a new diagnostic message delegate which
     * formats and prints all received diagnostic messages to the given
     * log files, according to the time received.
     * The level indicated, and the received message text.
     * 
     * @param[in] output 
     *      This is the output file to which to print all diagnostic messages
     *      that are under the "Levels::WARNING" level informally
     *      defined in the DiagnosticsSender class.
     * 
     * @param[in] error
     *      This is the file to which to print all diagnostic messages
     *      that are at or over the "Lvels::WARNING" level
     *      informally defined in the DiagnosticsSender class.
    */
   DiagnosticsSender::DiagnosticMessageDelegate DiagnosticsStreamReporter(
        FILE* output,
        FILE* error
   );
}

#endif /* SYSTEM_UTILS_DIAGNOSTICS_STREAM_REPORTER_HPP */