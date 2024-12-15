/**
 * @file CryptoRandomWin32.cpp
 * 
 * This is the win32 implementation of the CryptoRandom class
 * 
 * © 2024 by Hatem Nabli
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * we don't include Windows.h first.
*/
#include <Windows.h>
#include <bcrypt.h>

#include <SystemUtils/CryptoRandom.hpp>

#pragma comment(lib, "Bcrypt")

namespace SystemUtils {

    struct CryptoRandom::Impl {

    };

    CryptoRandom::~CryptoRandom() = default;

    CryptoRandom::CryptoRandom(): impl_(new Impl){

    }

    void CryptoRandom::Generate(void* buffer, size_t length) {
        (void)BCryptGenRandom(
            NULL, // required when dwFlags includes BCRYPT_USE_SYSTEM_PREFERED_RNG
            (PUCHAR)buffer,
            (ULONG)length,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );
    }
}