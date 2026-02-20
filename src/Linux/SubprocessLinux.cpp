#include <SystemUtils/Subprocess.hpp>
#include <SystemUtils/File.hpp>
#include <StringUtils/StringUtils.hpp>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <string>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace SystemUtils
{
    void CloseAllFilesExpect(int keepOpen) {
        std::vector<std::string> fds;
        const std::string fdsDir("/proc/self/fd");
        SystemUtils::File::ListDirectory(fdsDir, fds);
        for (const auto& fd : fds)
        {
            const auto fdNumStr = fd.substr(fdsDir.length());
            int fdNum;
            if ((sscanf(fdNumStr.c_str(), "%d", &fdNum) == 1) && (fdNum != keepOpen))
            { (void)close(fdNum); }
        }
    }

    std::vector<ProcessInfo> Subprocess::GetProcessList() {
        std::vector<std::string> process;
        const std::string procDir("/proc");
        SystemUtils::File::ListDirectory(procDir, process);
        std::vector<ProcessInfo> processes;
        for (const auto& proc : process)
        {
            ProcessInfo processInfo;
            const auto pidString = proc.substr(procDir.length());
            if (sscanf(pidString.c_str(), "%d", &processInfo.id) == 1)
            {
                const std::string exePath(proc + "/exe");
                std::vector<char> buffer(PATH_MAX + 1);
                if (realpath(exePath.c_str(), &buffer[0]) == NULL)
                { continue; }
                processInfo.image = std::string(buffer.data());
                processes.push_back(std::move(process));
            }
        }

        std::map<unsigned int, uint16_t> inodesToTcpPorts;
        std::ifstream tcpTable("/proc/net/tcp");
        {
            while (!tcpTable.fail() && !tcpTable.eof())
            {
                std::string line;
                (void)std::getline(tcpTable, line);
                unsigned int slot, localAddress, localPort;
                unsigned int remoteAddress, remotePort;
                unsigned int status, txQueue, rxQueue;
                unsigned int tr, when, retransmit, uid, timeout, inode;
                if (sscanf(line.c_str(), "%u:%X:%X %X:%X %X %X:%X %X:%X %X %u %u %u", &slot,
                           &localAddress, &localPort, &remoteAddress, &remotePort, &status,
                           &txQueue, &rxQueue, &tr, &when, &retransmit, &uid, &timeout,
                           &inode) == 14)
                {
                    if (status == 10)
                    { inodesToTcpPorts[inode] = localPort; }
                }
            }
        }

        for (auto& process : processes)
        {
            std::vector<std::string> fds;
            const std::string fdsDir(StringUtils::sprintf("/proc/%u/fd", process.id));
            SystemUtils::File::ListDirectory(fdsDir, fds);
            for (const auto& fd : fds)
            {
                std::string target;
                std::vector<char> buffer(64);
                while (target.empty())
                {
                    const auto used = readlink(fd.c_str(), buffer.data(), buffer.size());
                    if (used == buffer.size())
                    {
                        buffer.resize(buffer.size() * 2);
                    } else if (used >= 0)
                    {
                        target.assign(buffer.begin(), buffer.begin() + used);
                    } else
                    { break; }
                }
                unsigned int inode;
                if (sscanf(target.c_str(), "socket:[%u]", &inode) == 1)
                {
                    auto inodesToTcpPortEntry = inodesToTcpPorts.find(inode);
                    if (inodesToTcpPortEntry != inodesToTcpPorts.end())
                    { (void)process.tcpServerPorts.insert(inodesToTcpPortEntry->second) }
                }
            }
        }
        return processes;
    }
}  // namespace SystemUtils