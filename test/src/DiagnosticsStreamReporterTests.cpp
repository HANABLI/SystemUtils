/**
 * @file DiagnosticsStreamReporterTest.cpp
 * 
 * This module contains the unit tests of the
 * SystemUtils::DiagnosticsStreamReporter functions
 * 
 * © 2024 by Hatem Nabli
*/

#include<vector>

#include <gtest/gtest.h>
#include <SystemUtils/File.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/DiagnosticsStreamReporter.hpp>

struct ReceivedMessage 
{
    // Properties

    /**
     * This identifies the origin of the diagnostic information
    */
    std::string senderName;

    /**
    * This is used to filter out less-important information.
    * The level is higher the more important the information is.
    */
    size_t level;

    /**
     * This is the content of the message.
    */
    std::string message;

   // Methods

   /**
    * This constructor is used to initialize all the fields
    * of the structure.
    * 
    * @param[in] newSenderName
    *      This identifies the origin of the diagnostic information.
    * 
    * @param[in] newLevel
    *      This is used to filter out less-important information.   
    *      the level is higher the more important the information is.
    * 
    * @param[in] newMessage
    *       This is the content of the message.   
   */
    ReceivedMessage(
        std::string newSenderName,
        size_t newLevel,
        std::string newMessage
    ) : senderName(newSenderName)
    , level(newLevel)
    , message(newMessage)
    {

    }

    /**
     * This is the equality operator of the class.
     * 
     * @param[in] other
     *      This is the other instance to which to compare this instance.
     * 
     * @return
     *      An indication of whether or not the two instances are equal
     *      is returned.
    */
    bool operator==(const ReceivedMessage& other) const noexcept {
        return (
            (senderName == other.senderName)
            && (level == other.level)
            && (message == other.message)
        );
    }
};

void CheckLogMessage(
    FILE* f,
    const std::string& expected
) {
    std::vector<char> lineBuffer(256);
    const auto lineCString = fgets(lineBuffer.data(), (int)lineBuffer.size(), f);
    ASSERT_FALSE(lineCString == NULL);
    const std::string lineCppString(lineCString);
    ASSERT_GE(lineCppString.length(), 2);
    ASSERT_EQ('[', lineCppString[0]);
    const auto firstSpace = lineCppString.find(' ');
    ASSERT_FALSE(firstSpace == std::string::npos);
    ASSERT_EQ(expected, lineCppString.substr(firstSpace + 1));
}


/**
 * This is a helper function for the unit tests, that checks to
 * make sure we are at the end of the given log file.
 * 
 * @param[in] f
 *      This is the log file.
*/
void CheckIsEndOfFile(FILE* f) {
    ASSERT_EQ(EOF, fgetc(f));
}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
*/
struct DiagnosticsStreamReporterTests : public ::testing::Test
{
    // Properties

    /**
     * This is the test fixture for these tests, providing common
     * setup and teardown for each test.
    */
   std::string testAreaPath;

   // Methods

   virtual void SetUp() {
        testAreaPath = SystemUtils::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(testAreaPath));
   }

   virtual void TearDown() {
        ASSERT_TRUE(SystemUtils::File::DeleteDirectory(testAreaPath));
   }
};

TEST_F(DiagnosticsStreamReporterTests, SaveDiagnosticMessagesToLogFiles) {
    SystemUtils::DiagnosticsSender sender("foo");
    auto output = fopen((testAreaPath + "/out.txt").c_str(), "wt");
    auto error = fopen((testAreaPath + "/error.txt").c_str(), "wt");
    const auto unsubscribeDelegate = sender.SubscribeToDiagnostics(
        SystemUtils::DiagnosticsStreamReporter(output, error)
    );
    sender.SendDiagnosticInformationString(0, "hello");
    sender.SendDiagnosticInformationString(10, "world");
    sender.SendDiagnosticInformationString(2, "last message");
    sender.SendDiagnosticInformationString(5, "be careful");
    unsubscribeDelegate();
    sender.SendDiagnosticInformationString(0, "really the last message");
    (void)fclose(output);
    (void)fclose(error);
    output = fopen((testAreaPath + "/error.txt").c_str(), "rt");
    CheckLogMessage(output, "foo:0] hello\n");
    CheckLogMessage(output, "foo:2] last message\n");
    CheckIsEndOfFile(output);
    (void)fclose(output);
    error = fopen((testAreaPath + "/error.txt").c_str(), "rt");
    CheckLogMessage(error, "foo:10] error: world\n");
    CheckLogMessage(error, "foo:5] warning: be careful\n");
    CheckIsEndOfFile(error);
    (void)fclose(error);
}
