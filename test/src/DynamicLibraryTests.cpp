/**
 * @file DynamicLibraryTests.cpp
 *
 * This module contains the unit tests of
 * the SystemUtils::DynamicLibrary class
 *
 * © 2024 by Hatem Nabli
 */

#include <gtest/gtest.h>
#include <SystemUtils/DynamicLibrary.hpp>
#include <SystemUtils/File.hpp>

TEST(DynamicLibraryTests, DynamicLibraryTests_LoadAndGetFunction_Test) {
    SystemUtils::DynamicLibrary lib;
    ASSERT_TRUE(lib.Load(SystemUtils::File::GetExeParentDirectory() + "/", "MockDynamicLibrary"));

    const auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int (*procedure)(int) = (int (*)(int))procedureAddress;
    ASSERT_EQ(25, procedure(5));
}

TEST(DynamicLibraryTests, DynamicLibraryTests_UnloadFunc_Test) {
    SystemUtils::DynamicLibrary lib;
    ASSERT_TRUE(lib.Load(SystemUtils::File::GetExeParentDirectory() + "/", "MockDynamicLibrary"));

    const auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int (*procedure)(int) = (int (*)(int))procedureAddress;
    lib.Unload();
    ASSERT_DEATH(procedure(5), "");
}