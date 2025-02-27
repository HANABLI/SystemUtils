/**
 * @file DiagnosticsContextTests.cpp
 *
 * This module contain the unit tests of the
 * SystemUtils::DiagnosticsContext class.
 *
 * © 2024 by Hatem Nabli
 */

#include <gtest/gtest.h>
#include <SystemUtils/DiagnosticsContext.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>

struct ReceivedMessage
{
    // Properties

    /**
     * This indentifies the origin of the diagnostic information.
     */
    std::string senderName;

    /**
     * This is used to filter out less-important information.
     * The level is heigher the more important the information is.
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
     *      The level is hogher the more important the information is.
     *
     * @param[in] newMessage
     *      This is the content of the message.
     */
    ReceivedMessage(std::string newSenderName, size_t newLevel, std::string newMessage) :
        senderName(newSenderName), level(newLevel), message(newMessage) {}

    /**
     * This is the equality operator of the class
     *
     * @param[in] other
     *      This is the other instance to which to compare this instance.
     * @return
     *      an indication of whether or not the two instances are equals
     *      is returned.
     */
    bool operator==(const ReceivedMessage& other) const noexcept {
        return ((senderName == other.senderName) && (level == other.level) &&
                (message == other.message));
    }
};

TEST(DiagnosticsContextTests, DiagnosticsContextTests_PushAnPopContex_Test) {
    SystemUtils::DiagnosticsSender sender("Hatem");
    std::vector<ReceivedMessage> receivedMessages;
    sender.SubscribeToDiagnostics(
        [&receivedMessages](std::string senderName, size_t level, std::string message)
        { receivedMessages.emplace_back(senderName, level, message); });
    sender.SendDiagnosticInformationString(0, "hello");
    {
        SystemUtils::DiagnosticsContext testContext(sender, "coucou");
        sender.SendDiagnosticInformationString(0, "world");
    }
    sender.SendDiagnosticInformationString(0, "last message");
    ASSERT_EQ((std::vector<ReceivedMessage>{
                  {"Hatem", 0, "hello"},
                  {"Hatem", 0, "coucou: world"},
                  {"Hatem", 0, "last message"},
              }),
              receivedMessages);
}
