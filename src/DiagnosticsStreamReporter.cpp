/**
 * @file DiagnosticsStreamReporter.cpp
 *
 * This module contains the implementation of the
 * SystemUtils::DiagnosticsStreamReporter class.
 *
 * © 2024 by Hatem Nabli
 */
#include <SystemUtils/DiagnosticsStreamReporter.hpp>
#include <SystemUtils/Time.hpp>
#include <mutex>

namespace SystemUtils
{
    DiagnosticsSender::DiagnosticMessageDelegate DiagnosticsStreamReporter(FILE* output,
                                                                           FILE* error) {
        auto time = std::make_shared<Time>();
        auto mutex = std::make_shared<std::mutex>();
        double timeReference = time->GetTime();
        return [output, error, time, mutex, timeReference](std::string senderName, size_t level,
                                                           std::string message)
        {
            std::lock_guard<std::mutex> lock(*mutex);
            FILE* destination;
            std::string prefix;
            if (level >= DiagnosticsSender::Levels::ERROR)
            {
                destination = error;
                prefix = "error: ";
            } else if (level >= DiagnosticsSender::Levels::WARNING)
            {
                destination = error;
                prefix = "warning: ";
            } else
            { destination = output; }
            fprintf(destination, "[%.6lf %s:%zu] %s%s\n", time->GetTime() - timeReference,
                    senderName.c_str(), level, prefix.c_str(), message.c_str());
        };
    }
}  // namespace SystemUtils