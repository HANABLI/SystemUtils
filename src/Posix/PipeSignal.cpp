/**
 * @file PipeSignal.cpp
 * @brief This contains the implementation of the SystemUtils::PipeSignal class.
 * @copyright © 2026 by Hatem Nabli.
 */
#include "PipeSignal.hpp"
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

namespace SystemUtils
{
    struct PipeSignal::PipeSignalImpl
    {
        /**
         * This represent the pipe file descriptor to cary
         * when pipe is created.
         */
        int pipe[2] = {-1, -1};

        /**
         * This is the humen readable string that indicate the last error
         * occurred form instance methods.
         */
        std::string lastError;
    };

    PipeSignal::~PipeSignal() {
        if (impl_->pipe[0] >= 0)
        { (void)close(impl_->pipe[0]); }
        if (impl_->pipe[1] >= 0)
        { (void)close(impl_->pipe[1]); }
    }

    bool PipeSignal::Initialize() {
        if (impl_->pipe[0] >= 0 && impl_->pipe[1] >= 0)
        { return true; }
        if (pipe(impl_->pipe) != 0)
        {
            impl_->lastError = strerror(errno);
            impl_->pipe[0] = -1;
            impl_->pipe[1] = -1;
            return false;
        }

        for (int i = 0; i < 2; ++i)
        {
            int flags = fcntl(impl_->pipe[i], F_GETFL, 0);
            if ( flags < 0)
            {
                (void)close(impl_->pipe[0]);
                (void)close(impl_->pipe[1]);
                impl_->pipe[0] = -1;
                impl_->pipe[1] = -1;
                return false;
            }
            flags |= O_NONBLOCK;
            if (fcntl(impl_->pipe[i], F_SETFL, flags) < 0)
            {
                (void)close(impl_->pipe[0]);
                (void)close(impl_->pipe[1]);
                impl_->pipe[0] = -1;
                impl_->pipe[1] = -1;
                return false;
            }
        }
        return true;
    }

    std::string PipeSignal::GetLastError() const { return impl_->lastError; }

    void PipeSignal::Set() {
        uint8_t token = 46;
        (void)write(impl_->pipe[1], &token, 1);
    }

    void PipeSignal::Clear() {
        uint8_t token;
        (void)read(impl_->pipe[0], &token, 1);
    }

    bool PipeSignal::IsSet() const {
        fd_set readfds;
        struct timeval timeout = {0};
        FD_ZERO(&readfds);
        FD_SET(impl_->pipe[0], &readfds);
        return (select(impl_->pipe[0] + 1, &readfds, NULL, NULL, &timeout) != 0);
    }

    int PipeSignal::GetSelectHandler() const { return impl_->pipe[0]; }
}  // namespace SystemUtils