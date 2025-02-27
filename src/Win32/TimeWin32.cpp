/**
 * @file TimeWin32.cpp
 *
 * This module contains the Win32 specific part of the
 * implementation of the SystemAbstractions::Time class.
 *
 * © 2024 by Hatem Nabli
 */

#include <Windows.h>
#include <SystemUtils/Time.hpp>

namespace SystemUtils
{
    /**
     * This is the Win32-specific state of the Time calss.
     */
    struct Time::Impl
    {
        double scale = 0.0;
    };

    Time::~Time() = default;
    Time::Time(Time&&) noexcept = default;
    Time& Time::operator=(Time&&) noexcept = default;

    Time::Time() : impl_(new Impl()) {}

    double Time::GetTime() {
        if (impl_->scale == 0.0)
        {
            LARGE_INTEGER freq;
            (void)QueryPerformanceFrequency(&freq);
            impl_->scale = 1.0 / (double)freq.QuadPart;
        }
        LARGE_INTEGER now;
        (void)QueryPerformanceCounter(&now);
        return (double)now.QuadPart * impl_->scale;
    }

    struct tm Time::localTime(time_t time)
    {
        if (time == 0)
        { (void)::time(&time); }
        struct tm timeStruct;
        (void)localtime_s(&timeStruct, &time);
        return timeStruct;
    }

    struct tm Time::gmtime(time_t time)
    {
        if (time == 0)
        { (void)::time(&time); }
        struct tm timeStruct;
        (void)gmtime_s(&timeStruct, &time);
        return timeStruct;
    }
}  // namespace SystemUtils