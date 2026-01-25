/**
 * @file CryptoRandomPosix.cpp
 * @brief This module contain the POSIX implementation of the SystemUtils::CryptoRandomPosix
 * class.
 * @copyright 2026 by Hatem Nabli
 */
#include <fcntl.h>
#include <stdio.h>

#include <SystemUtils/CryptoRandom.hpp>

namespace SystemUtils
{
    /**
     * This is the Posix struct of CryptoRandom class.
     */
    struct CryptoRandom::Impl
    {
        /**
         * This is used to contain file descriptor of the urandom file
         * from which to read a random number.
         */
        int rn;
    };

    CryptoRandom::CryptoRandom() : impl_(std::make_unique<Impl>()) {
        impl_->rn = open("/dev/urandom", O_RDONLY);
    }

    void CryptoRandom::Generate(void* buffer, size_t length) {
        (void)read(impl_->rn, buffer, length);
    }
}  // namespace SystemUtils