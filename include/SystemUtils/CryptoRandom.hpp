#ifndef SYSTEM_UTILS_CRYPTO_RANDOM
#define SYSTEM_UTILS_CRYPTO_RANDOM
/**
 * @file CryptoRandomWin32.hpp
 *
 * This module declares the SystemUtils::CryptoRandom class
 *
 * © 2024 by Hatem Nabli
 */
#include <memory>
#include <stddef.h>
namespace SystemUtils
{
    /**
     * This is the CryptoRandom class
     */
    class CryptoRandom
    {
    private:
        /* data */
        // LifeCycle
    public:
        ~CryptoRandom() noexcept;
        // Methods
    public:
        /**
         * Constructor
         */
        CryptoRandom(/* args */);
        /**
         * This method generates strong entropy random numbers and
         * put them into the given buffer
         *
         * @param[in, out] buffer
         *      This is the buffer in which to store random numbers.
         * @param[in] length
         *      This is the bytes length of the buffer
         */
        void Generate(void* buffer, size_t length);
        // private properties
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

#endif /* SYSTEM_UTILS_CRYPTO_RANDOM */