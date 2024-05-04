/**
 * @file DirectoryMonitorTests.cpp
 * 
 * This module contains the unit tests of the
 * SystemUtils::DirectoryMonitor class.
 * 
 * © 2024 by Hatem Nabli 
 */
#include <functional>
#include <mutex>
#include <fstream>
#include <condition_variable>

#include <gtest/gtest.h>
#include <SystemUtils/DirectoryMonitor.hpp>
#include <SystemUtils/File.hpp>

/**
 * This is a helper used with a directory monitor to receive
 * the "changed" callback and wait for it to happen without 
 * racing the directory monitor.
*/
struct DirectoryMonitorCallBack {
    // Properties

    /**
     * This flag is set when the directory monitor
     * publishes an event to signal a change in the directory.
    */
    bool changeDetected = false;

    /**
    * This is used to wait for the change callback to happen.
    */
    std::condition_variable changeCondition;

    /**
     * This is used to synchronize access to this object.
     */
    std::mutex changeMutex;

    /**
     * This is the delegate to be given to the directory
     * monitor in order to hook this helper up.
    */
    std::function< void() > dmCallback = [this]{
        std::lock_guard< std::mutex > lock(changeMutex);
        changeDetected = true;
        changeCondition.notify_all();
    };

    //Methods

    /**
     * This method is called by the unit tests in orde to
     * wait for the directory monitor callback to happen.
     * 
     * @return
     *      return an indication of whether or not the callback 
     *      happend before a reasonable amount of time has 
     *      elapsed.
    */
    bool AwaitChanged() {
        std::unique_lock< decltype(changeMutex) > lock(changeMutex);
        const auto changed = changeCondition.wait_for(lock, std::chrono::milliseconds(50), [this]{return changeDetected;});
        changeDetected = false;
        return changed;
    }

};
/**
 * This is the test ficture for these tests, providing common
 * setup and teardown for each test.
*/
struct DirectoryMonitorTests: public ::testing::Test
    {
    
    // Properties
    SystemUtils::DirectoryMonitor dm;

    /**
     * This used to wait for callbacks.
    */
    DirectoryMonitorCallBack dmCallbackMonitoring;
    /**
     * This is the temporary directory to use to test
     * the directoryMonitor class
    */
    std::string outerpath;

    /**
     * This is the directory to be monitored for changes.
     * It will be a subdirectory of outerPath
    */
    std::string innerPath;
    
    // ::testing::Test
    virtual void SetUp() {
        outerpath = SystemUtils::File::GetExeParentDirectory() + "/TestArea";
        innerPath = outerpath + "/DirectroryToMonitor";
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(outerpath));
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(innerPath));
    }

    virtual void TearDown() {
        dm.Stop();
        ASSERT_TRUE(SystemUtils::File::DeleteDirectory(outerpath));
    }
};

TEST_F(DirectoryMonitorTests, NoCallbackAfterStarting_Test) {
    ASSERT_TRUE(dm.Start(dmCallbackMonitoring.dmCallback, innerPath));
    ASSERT_FALSE(dmCallbackMonitoring.AwaitChanged());   
}

TEST_F(DirectoryMonitorTests, DirectoryMonitoring_Test) {

    ASSERT_TRUE(dm.Start(dmCallbackMonitoring.dmCallback, innerPath));

    //Create a file in the monitored directory.
    std::string testFilePath = innerPath + "/myFile.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackMonitoring.AwaitChanged());
    }

    // Edit the file in the monitored directory
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_TRUE(dmCallbackMonitoring.AwaitChanged());
    }

    //Delete the file in the monitored directory
    {
        SystemUtils::File file(testFilePath);
        file.Destroy();
        ASSERT_TRUE(dmCallbackMonitoring.AwaitChanged());
    }

    //Create a file outside the monitored directory
    testFilePath = outerpath + "/myFile.txt";
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackMonitoring.AwaitChanged());
    }

    //Edit the file outside the monitored directory.
    {
        std::fstream file(testFilePath, std::ios_base::out | std::ios_base::ate);
        ASSERT_FALSE(file.fail());
        file << "Hello, World\r\n";
        ASSERT_FALSE(file.fail());
        file.close();
        ASSERT_FALSE(dmCallbackMonitoring.AwaitChanged());
    }

    //Delete file outside the monitored directory.
    {
        SystemUtils::File file(testFilePath);
        file.Destroy();
        ASSERT_FALSE(dmCallbackMonitoring.AwaitChanged());
    }
}



