/**
 * @file DiagnosticsContext.cpp
 *
 * This module contain the implementation of the
 * SystemUtils::DiagnosticsContext class.
 *
 * © 2024 by Hatem Nabli
 */

#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/DiagnosticsContext.hpp>

namespace SystemUtils
{
    struct DiagnosticsContext::Impl
    {
        /**
         * This is the sender upon which this class is pushing a context
         * as long as the class instance exists.
         */
        DiagnosticsSender& diagnosticsSender;

        /**
         *
         */
        Impl(DiagnosticsSender& newDiagnosticsSender) : diagnosticsSender(newDiagnosticsSender) {}
    };

    DiagnosticsContext::DiagnosticsContext(DiagnosticsSender& diagnosticsSender,
                                           const std::string& context) noexcept :
        impl_(new Impl(diagnosticsSender)) {
        diagnosticsSender.PushContext(context);
    }

    DiagnosticsContext::~DiagnosticsContext() noexcept { impl_->diagnosticsSender.PopContext(); }
}  // namespace SystemUtils