/**
 * @file TimeLinux.cpp
 * @brief This is the Linux implementation of the SystemUtils::Time class.
 * @copyright © 2026 by Hatem Nabli.
 */

 #include <SystemUtils/Time.hpp>
 #include <time.h>
 #include <inttypes.h>
 namespace SystemUtils {

    struct Time::Impl {

    };
    Time::Time() : impl_(std::make_unique<Impl>()) {}
    Time::Time(Time&&) noexcept = default;
    Time& Time::operator=(Time&&) noexcept = default;
    Time::~Time() noexcept {}

    double Time::GetTime() {
        struct timespec timeSp;
        if (clock_gettime(CLOCK_REALTIME, &timeSp) != 0) {
            return 0.0;
        }
        return static_cast<double>(timeSp.tv_sec) + static_cast<double>(timeSp.tv_nsec) / 1e9;
    }

 }