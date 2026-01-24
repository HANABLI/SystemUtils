/**
 * @file FileTest.cpp
 *
 * This module contains the unit tests of the
 * SystemUtils::File class
 *
 * © 2024 by Hatem Nabli
 */

#include <set>
#include <gtest/gtest.h>
#include <SystemUtils/File.hpp>

struct FileTests : public ::testing::Test
{
    std::string testDirectoryPath;

    virtual void SetUp() {
        testDirectoryPath = SystemUtils::File::GetExeParentDirectory() + "/testFileDirectory";
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(testDirectoryPath));
    }

    virtual void TearDown() { ASSERT_TRUE(SystemUtils::File::DeleteDirectory(testDirectoryPath)); }
};

TEST_F(FileTests, FileTests_Basic_Test) {
    std::string testFilePath = testDirectoryPath + "/testFile.txt";
    SystemUtils::File testArea(testDirectoryPath);
    SystemUtils::File file(testFilePath);
    ASSERT_FALSE(file.IsExisting());
    ASSERT_FALSE(file.IsDirectory());
    ASSERT_FALSE(file.OpenReadOnly());
    ASSERT_TRUE(testArea.IsExisting());
    ASSERT_TRUE(testArea.IsDirectory());

    // Create file and verify it exists
    ASSERT_TRUE(file.OpenReadWrite());
    ASSERT_TRUE(file.IsExisting());
    ASSERT_FALSE(file.IsDirectory());
    file.Close();

    // Open file test when it exists
    ASSERT_TRUE(file.OpenReadOnly());
    file.Close();

    // Now Destroy the file and verify it not longer exists
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Move the file while it's open.
    ASSERT_TRUE(file.OpenReadWrite());
    ASSERT_TRUE(file.IsExisting());
    ASSERT_EQ(testFilePath, file.GetPath());
    {
        SystemUtils::File fileCheck(testFilePath);
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    {
        SystemUtils::File fileCheck(testFilePath + "2");
        ASSERT_FALSE(fileCheck.IsExisting());
    }

    ASSERT_TRUE(file.Move(testFilePath + "2"));
    ASSERT_TRUE(file.IsExisting());
    ASSERT_NE(testFilePath, file.GetPath());
    ASSERT_EQ(testFilePath + "2", file.GetPath());
    {
        SystemUtils::File fileCheck(testFilePath);
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    {
        SystemUtils::File fileCheck(testFilePath + "2");
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    file.Close();
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Move the file while it's not open.
    file = SystemUtils::File(testFilePath);
    ASSERT_TRUE(file.OpenReadWrite());
    file.Close();
    ASSERT_TRUE(file.IsExisting());
    ASSERT_EQ(testFilePath, file.GetPath());
    {
        SystemUtils::File fileCheck(testFilePath);
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    {
        SystemUtils::File fileCheck(testFilePath + "2");
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    ASSERT_TRUE(file.Move(testFilePath + "2"));
    ASSERT_TRUE(file.IsExisting());
    ASSERT_NE(testFilePath, file.GetPath());
    ASSERT_EQ(testFilePath + "2", file.GetPath());
    {
        SystemUtils::File fileCheck(testFilePath);
        ASSERT_FALSE(fileCheck.IsExisting());
    }
    {
        SystemUtils::File fileCheck(testFilePath + "2");
        ASSERT_TRUE(fileCheck.IsExisting());
    }
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());

    // Copy file test
    file = SystemUtils::File(testFilePath);
    ASSERT_TRUE(file.OpenReadWrite());
    const std::string helloWorldTest = "Hello, World!\r\n";
    ASSERT_EQ(helloWorldTest.length(), file.Write(helloWorldTest.data(), helloWorldTest.length()));
    {
        SystemUtils::File file2(testFilePath + "2");
        ASSERT_FALSE(file2.IsExisting());
        ASSERT_TRUE(file.Copy(file2.GetPath()));
        ASSERT_TRUE(file2.IsExisting());
        ASSERT_TRUE(file2.OpenReadOnly());
        SystemUtils::IFile::Buffer buffer((size_t)file2.GetSize());
        ASSERT_EQ(buffer.size(), file2.Read(buffer));
        ASSERT_EQ(helloWorldTest, std::string(buffer.begin(), buffer.end()));
        file2.Destroy();
        ASSERT_FALSE(file2.IsExisting());
    }
    file.Close();
    file.Destroy();
    ASSERT_FALSE(file.IsExisting());
}

TEST_F(FileTests, FileTests_DirectoryMethods_Test) {
    //
    SystemUtils::File testDirectory(testDirectoryPath);
    const std::string testFilePath = testDirectoryPath + "/testFile.txt";
    SystemUtils::File file(testFilePath);
    ASSERT_TRUE(file.OpenReadWrite());
    const std::string helloWorldString = "Hello, World!\r\n";
    ASSERT_EQ(helloWorldString.length(),
              file.Write(helloWorldString.data(), helloWorldString.length()));
    SystemUtils::File file2(testFilePath + "2");
    ASSERT_TRUE(file.Copy(file2.GetPath()));
    file.Close();

    const std::string subDirectoryPath = testDirectoryPath + "/sub";
    SystemUtils::File::CreateDirectory(subDirectoryPath);
    SystemUtils::File sub(subDirectoryPath);
    ASSERT_TRUE(sub.IsDirectory());
    const std::string testSubFilePath = subDirectoryPath + "/subTest.txt";
    SystemUtils::File file3(testSubFilePath);
    ASSERT_TRUE(file3.OpenReadWrite());
    const std::string testString = "Some words!\r\n";
    ASSERT_EQ(testString.length(), file3.Write(testString.data(), testString.length()));
    file3.Close();

    // Get a list of files.
    std::vector<std::string> list;
    SystemUtils::File::ListDirectory(testDirectoryPath, list);
    std::set<std::string> set(list.begin(), list.end());
    for (const std::string& expectedElement : {"testFile.txt", "testFile.txt2", "sub"})
    {
        const auto element = set.find(testDirectoryPath + "/" + expectedElement);
        ASSERT_FALSE(element == set.end()) << expectedElement;
        (void)set.erase(element);
    }
    ASSERT_TRUE(set.empty());

    // Copy subdirectory and verify its one file inside got copied.
    const std::string subDirectoryPath2 = testDirectoryPath + "/sub2";
    ASSERT_TRUE(SystemUtils::File::CopyDirectory(subDirectoryPath, subDirectoryPath2));
    SystemUtils::File sub2(subDirectoryPath2);
    ASSERT_TRUE(sub2.IsDirectory());
    ASSERT_TRUE(sub2.IsExisting());
    const std::string testSubFilePath2 = subDirectoryPath2 + "/subTest.txt";
    SystemUtils::File file4(testSubFilePath2);
    ASSERT_TRUE(file4.OpenReadWrite());
    SystemUtils::IFile::Buffer buffer((size_t)file4.GetSize());
    ASSERT_EQ(buffer.size(), file4.Read(buffer));
    ASSERT_EQ(testString, std::string(buffer.begin(), buffer.end()));
    file4.Close();

    ASSERT_TRUE(SystemUtils::File::DeleteDirectory(subDirectoryPath2));
    ASSERT_FALSE(sub2.IsExisting());
    ASSERT_FALSE(file4.IsExisting());
}

TEST_F(FileTests, FileTests_RepurposeFileObjec_Test) {
    const std::string testFilePath1 = testDirectoryPath + "/toto.txt";
    const std::string testFilePath2 = testDirectoryPath + "/titi.txt";
    SystemUtils::File file(testFilePath1);

    file = testFilePath2;
    ASSERT_EQ(testFilePath2, file.GetPath());
}

TEST_F(FileTests, FileTests_WriteAndReadBack__Test) {
    const std::string testFilePath = testDirectoryPath + "/toto.txt";
    SystemUtils::File file(testFilePath);
    ASSERT_TRUE(file.OpenReadWrite());
    const std::string testString = "Hello, World!\r\n";
    ASSERT_EQ(testString.length(), file.Write(testString.data(), testString.length()));
    file.SetPosition(0);
    SystemUtils::IFile::Buffer buffer(testString.length());
    ASSERT_EQ(testString.length(), file.Read(buffer));
    ASSERT_EQ(testString, std::string(buffer.begin(), buffer.end()));
}
