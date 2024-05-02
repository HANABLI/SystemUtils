/**
 * @file StringFileTests.cpp
 * 
 * This module contains the unit tests of the
 * SystemAbstraction::StringFile class.
 * 
 * © 2024 by Hatem Nabli
*/

#include <gtest/gtest.h> 
#include <stdint.h>
#include <SystemUtils/StringFile.hpp>

TEST(StringFileTests, Placeholder) {
    ASSERT_TRUE(true);
}

TEST(StringFileTests, StringFileTests_WriteAndReadBack_Tests) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTest = "Hello, World!\r\n";
    ASSERT_EQ(helloWorldTest.length(), sf.Write(helloWorldTest.data(), helloWorldTest.length()));
    sf.SetPosition(0);
    SystemUtils::IFile::Buffer buffer(12);
    ASSERT_EQ(5, sf.Read(buffer, 5, 7));
    ASSERT_EQ((std::vector< uint8_t >{0, 0, 0, 0, 0, 0, 0, 'H', 'e', 'l', 'l', 'o'}), buffer);
}

TEST(StringFileTests, StringFileTests_ReadAdvancesFilePointer_Test) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTest = "Hello, World!\r\n";
    ASSERT_EQ(helloWorldTest.length(), sf.Write(helloWorldTest.data(), helloWorldTest.length()));
    sf.SetPosition(0);
    SystemUtils::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ("Hello", std::string(buffer.begin(), buffer.end()));
    ASSERT_EQ(5, sf.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ(", Wor", std::string(buffer.begin(), buffer.end()));
    ASSERT_EQ(10, sf.GetPosition());
}

TEST(StringFileTests, StringFileTests_PeakNotAdvancesFilePointer_Test) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTest = "Hello, World!\r\n";
    ASSERT_EQ(helloWorldTest.length(), sf.Write(helloWorldTest.data(), helloWorldTest.length()));
    sf.SetPosition(0);
    SystemUtils::IFile::Buffer buffer(5);
    ASSERT_EQ(
        buffer.size(),
        sf.Read(buffer)
    );
    ASSERT_EQ("Hello", std::string(buffer.begin(), buffer.end()));
    ASSERT_EQ(5, sf.GetPosition());
    ASSERT_EQ(
        buffer.size(),
        sf.Peek(buffer)
    );
    ASSERT_EQ(", Wor", std::string(buffer.begin(), buffer.end()));
    ASSERT_EQ(5, sf.GetPosition());
}

TEST(StringFileTests, StringFileTests_GetSize_Test) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTest = "Hello, World!\r\n";
    ASSERT_EQ(0, sf.GetSize());
    (void)sf.Write(helloWorldTest.data(), helloWorldTest.length());
    ASSERT_EQ(helloWorldTest.length(), sf.GetSize());
}

TEST(StringFileTests, StringFileTests_SetSize_Test) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTests = "Hello, World!\r\n";
    
    (void)sf.Write(helloWorldTests.data(), helloWorldTests.length());
    ASSERT_EQ(helloWorldTests.length(), sf.GetSize());
    ASSERT_TRUE(sf.SetSize(5));
    ASSERT_EQ(5, sf.GetSize());
    SystemUtils::IFile::Buffer buffer(5);
    ASSERT_EQ(0, sf.Peek(buffer));
    ASSERT_EQ(0, sf.Read(buffer));
    sf.SetPosition(0);
    ASSERT_EQ(5, sf.Read(buffer));
    ASSERT_EQ((std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o'}), buffer);
    ASSERT_EQ("Hello", std::string(buffer.begin(), buffer.end()));
    ASSERT_TRUE(sf.SetSize(20));
    ASSERT_EQ(20, sf.GetSize());
    buffer.resize(20);
    sf.SetPosition(0);
    ASSERT_EQ(20, sf.Read(buffer));
    ASSERT_EQ((std::vector< uint8_t >{'H', 'e', 'l', 'l', 'o', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}), buffer);
}

TEST(StringFileTests, StringFileTests_Clone_Test) {
    SystemUtils::StringFile sf;
    const std::string helloWorldTest = "Hello, World!\r\n";
    (void)sf.Write(helloWorldTest.data(), helloWorldTest.length());
    sf.SetPosition(0);
    const auto clone = sf.Clone();
    sf.SetPosition(5);
    sf.Write("FeelsBadMan", 11);
    SystemUtils::IFile::Buffer buffer(helloWorldTest.length());
    ASSERT_EQ(0, clone->GetPosition());
    ASSERT_EQ(
        helloWorldTest.length(),
        clone->Read(buffer)
    );
    ASSERT_EQ(
        helloWorldTest,
        std::string(buffer.begin(), buffer.end())
    );
}






