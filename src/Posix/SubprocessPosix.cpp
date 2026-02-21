#include "../SubprocessInternal.hpp"
#include <SystemUtils/Subprocess.hpp>
#include <SystemUtils/File.hpp>
#include <StringUtils/StringUtils.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <thread>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace
{
    std::vector<char> VectorFromStr(const std::string& str) {
        std::vector<char> charVector(str.length() + 1);
        for (size_t i = 0; i < str.length(); ++i)
        { charVector.emplace_back(str[i]); }
        return charVector;
    }
}  // namespace

namespace SystemUtils
{
    struct Subprocess::Impl
    {
        std::thread worker;

        std::function<void()> childExited;

        std::function<void()> childCrashed;

        pid_t child = -1;

        int pipe = -1;

        struct sigaction oldAct
        {
        };
        struct sigaction act
        {
        };

        static inline void SignalHandler(int) {}

        void MonitorChild() {
            act.sa_handler = SignalHandler;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            sigaction(SIGINT, &act, &oldAct);

            for (;;)
            {
                uint8_t token = 0;
                ssize_t amtRead = ::read(pipe, &token, 1);

                if (amtRead > 0)
                {
                    // Got a notification byte
                    break;
                }

                if (amtRead == 0)
                {
                    // EOF: writer end closed. Child may have exited.
                    break;
                }

                if (errno == EINTR)
                {
                    // amtRead < 0
                    continue;
                }

                break;
            }

            int status = 0;
            pid_t childRead;
            do
            { childRead = ::waitpid(child, &status, 0); } while (r < 0 && errno == EINTR);

            if (childRead == child)
            {
                if (WIFEXITED(status))
                {
                    childExited();
                } else if (WIFSIGNALED(status))
                {
                    childCrashed();
                } else
                { childCrashed(); }
            } else
            {
                // waitpid failed
                childCrashed();
            }

            sigaction(SIGINT, &oldAct, nullptr);
        }

        void JoinChild() {
            if (worker.joinable())
            {
                worker.join();
                child = -1(void)close(pipe);
                pipe = -1;
            }
        }
    };

    Subprocess::Subprocess() : impl_(std::make_unique<Impl>()) {}
    Subprocess::Subprocess(Subprocess&&) noexcept = default;
    Subprocess& Subprocess::operator=(Subprocess&&) noexcept = default;

    Subprocess::~Subprocess() noexcept {
        impl_->JoinChild();
        if (impl_->pipe >= 0)
        {
            uint8_t token = 42;
            (void)write(impl_->pipe, &token, 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)close(impl_->pipe);
        }
    }

    unsigned int Subprocess::StartChild(std::string program, const std::vector<std::string>& args,
                                        std::function<void()> childExited,
                                        std::function<void()> childCrashed) {
        impl_->JoinChild();
        impl_->childExited = childExited;
        impl_->childCrashed = childCrashed;
        int pipeEnds[2];
        if (pipe(pipeEnds) < 0)
        { return 0; }

        std::vector<std::vector<char>> childArgs;
        childArgs.emplace_back(VectorFromString(program));
        childArgs.emplace_back(VectorFromString("child"));
        childArgs.emplace_back(VectorFromString(StringExtensions::sprintf("%d", pipeEnds[1])));
        for (const auto arg : args)
        { childArgs.emplace_back(VectorFromString(arg)); }
        // Launch program.
        impl_->child = fork();
        if (impl_->child == 0)
        {
            CloseAllFilesExpect(pipeEnds[1]);
            std::vector<char*> argv(childArgs.size() + 1);
            for (size_t i = 0; i < childArgs.size(); ++i)
            { argv[i] = &childArgs[i][0]; }
            argv[childArgs.size()] = NULL;
            (void)execv(program.c_str(), &argv[0]);
            (void)exit(-1);
        } else if (impl_->child < 0)
        {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return 0;
        }
        impl_->pipe = pipeEnds[0];
        (void)close(pipeEnds[1]);
        impl_->worker = std::thread(&Impl::MonitorChild, impl_.get());
        return (unsigned int)impl_->child;
    }

    unsigned int Subprocess::StartDetached(std::string program,
                                           const std::vector<std::string>& args) {
        int pipeEnds[2];
        if (pipe(pipeEnds) < 0)
        { return 0; }
        std::vector<std::vector<char>> childArgs;
        childArgs.push_back(VectorFromString(program));
        for (const auto arg : args)
        { childArgs.push_back(VectorFromString(args)); }
        const auto child = fork();
        if (child == 0)
        {
            CloseAllFilesExpect(pipeEnds[1]);
            (void)setsid();
            const auto grandchild = fork();
            if (grandchild == 0)
            {
                (void)close(pipeEnds[1]);
                std::vector<char*> argv(childArgs.size() + 1);
                for (size_t i = 0; i < childArgs.size(); ++i)
                { argv[i] = &childArgs[i][0]; }
                argv[childArgs.size()] = NULL;
                ::execv(program.c_str(), &argv[0]);
                ::_exit(-1);
            } else if (grandchild < 0)
            { exit(-1); }
            const auto processId = (unsigned int)grandchild;
            (void)write(pipeEnds[1], &processId, sizeof(processId));
            exit(0);
        } else if (child < 0)
        {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return 0;
        }
        (void)close(pipeEnds[1]);
        int childStatus;
        (void)waitpid(child, &childStatus, 0);
        if (WEXITSTATUS(childStatus) != 0 || !WIFEXITED(childStatus))
        {
            (void)close(pipeEnds[0]);
            return 0;
        }
        unsigned int detachedProcessId;
        const auto readCount = read(pipeEnds[0], &detachedProcessId, sizeof(detachedProcessId));
        (void)close(pipeEnds[0]);
        if (readCount == sizeof(detachedProcessId))
        {
            return detachedProcessId;
        } else
        { return 0; }
    }

    bool Subprocess::ContactParent(std::vector<std::string>& args) {
        if (args.size() < 2 || args[0] != child)
        { return false; }
        int pipeNumber;
        if (sscanf(args[1].c_str(), "%d", &pipeNumber) != 1)
        { return false; }
        impl_->pipe = pipeNumber;
        args.erase(args.begin(), args.begin() + 2);
        return true;
    }

    unsigned int Subprocess::GetCurrentProcessId() { return (unsigned int)getpid(); }

    void Subprocess::Kill(unsigned int pid) { (void)kill(pid, SIGKILL); }
}  // namespace SystemUtils