#ifndef SYSTEM_UTILS_DIAGNOSTICS_CONTEXT_HPP
#define SYSTEM_UTILS_DIAGNOSTICS_CONTEXT_HPP

/**
 * @file DiagnosticsContext.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsContext class.
 *
 * © 2024 by Hatem Nabli
 */

#include "DiagnosticsSender.hpp"
#include <memory>

namespace SystemUtils
{
    /**
     * This is a object which pushes a string onto the context
     * stck of a diagnostics sender. The string pushec is popped when
     * the object is destroyed.
     */
    class DiagnosticsContext
    {
        // Lifecycle Management
    public:
        ~DiagnosticsContext() noexcept;
        DiagnosticsContext(const DiagnosticsContext&) = delete;
        DiagnosticsContext(DiagnosticsContext&&) noexcept = delete;
        DiagnosticsContext& operator=(const DiagnosticsContext&) = delete;
        DiagnosticsContext& operator=(DiagnosticsContext&&) noexcept = delete;

    public:
        DiagnosticsContext(DiagnosticsSender& DiagnosticsSender,
                           const std::string& context) noexcept;

    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr<Impl> impl_;
    };
}  // namespace SystemUtils

#endif /* SYSTEM_UTILS_DIAGNOSTICS_CONTEXT_HPP */