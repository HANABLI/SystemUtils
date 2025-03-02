/**
 * @file DiagnosticsSenderTests.cpp
 *
 * This module contains the unit tests of the
 * SystemUtils::DiagnosticsSender class.
 *
 * © 2024 by Hatem Nabli
 */

#include <string>
#include <gtest/gtest.h>
#include <SystemUtils/DiagnosticsSender.hpp>

/**
 * This is used to store a message received
 * from a diagnostics sender.
 */
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
    ReceivedMessage(std::string newSenderName, size_t newLevel, std::string newMessage) :
        senderName(newSenderName), level(newLevel), message(newMessage) {}

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
        return ((senderName == other.senderName) && (level == other.level) &&
                (message == other.message));
    }
};

TEST(DiagnosticsSenderTests, DiagnosticsSenderTests_SubsctriptionAndTransmission_Test) {
    SystemUtils::DiagnosticsSender sender("Me");
    sender.SendDiagnosticInformationString(100, "Bad information; FeelsBadMan");
    std::vector<ReceivedMessage> receivedMessages;
    const auto unsubscribeDelegate = sender.SubscribeToDiagnostics(
        [&receivedMessages](std::string senderName, size_t level, std::string message)
        { receivedMessages.emplace_back(senderName, level, message); },
        5);

    ASSERT_EQ(5, sender.GetMinLevel());
    ASSERT_TRUE(unsubscribeDelegate != nullptr);
    sender.SendDiagnosticInformationString(10, "blablabla");
    sender.SendDiagnosticInformationString(3, "Did you hear that?");
    sender.PushContext("spam");
    sender.SendDiagnosticInformationString(4, "Level 4 whisper...");
    sender.SendDiagnosticInformationString(5, "Level 5, can you dig it?");
    sender.PopContext();
    sender.SendDiagnosticInformationString(6, "Level 6 FOR THE WIN");
    unsubscribeDelegate();
    sender.SendDiagnosticInformationString(5, "Are you still there?");
    ASSERT_EQ(receivedMessages, (std::vector<ReceivedMessage>{
                                    {"Me", 10, "blablabla"},
                                    {"Me", 5, "spam: Level 5, can you dig it?"},
                                    {"Me", 6, "Level 6 FOR THE WIN"},
                                }));
}