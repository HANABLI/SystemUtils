/**
 * @file SubprocessTests
 * 
 * This module contains the unit tests of 
 * teh SystemUtil::Subprocess class.
 * 
 * © 2024 by Hatem nabli 
*/

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <SystemUtils/DirectoryMonitor.hpp>
#include <SystemUtils/Subprocess.hpp>
#include <SystemUtils/File.hpp>

namespace {

    /**
     * This is used to receive callbacks from unit under tests.
    */
    struct Owner
    {
        /**
         * This is used to synchronize access to the class
        */
        std::mutex mutex;

        /**
         * This is used to wait for, or signal, a condition upon
         * which that the owner might be waiting.
        */
        std::condition_variable_any condition;     

        /**
         * This flag indicates whether or not the subprocess exited
        */
        bool exited = false;

        /**
         * This flag indicates whether or not the subprocess crashed
        */
        bool crashed = false;

        //Methods

        /**
         * This method waits up to a second for the subprocess to exit.
         * 
         * @return
         *      An indication of whether or not the subprocess exited
         *      is returned.
        */
        bool AwaitExited() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return exited;
                }

            );
        }

        /**
         * This method waits up to a second for the subprocess to crash.
         * 
         * @return
         *      An indication of whether or not the subprocess crashed
         *      is returned.
        */
        bool AwaitCrashed() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return crashed;
                }

            );
        }

        void SubprocessChildExited() {
            std::unique_lock< std::mutex > lock(mutex);
            exited = true;
            condition.notify_all();
        }

        void SubprocessChildCrashed() {
            std::unique_lock< std::mutex > lock(mutex);
            crashed = true;
            condition.notify_all();
        }
    };

}


struct SubprocessTests : public ::testing::Test
{

    /**
     * This is the temporary directory to use to test
     * the File class.
    */
    std::string testAreaPath;

    /**
     * This is used to monitor the temporary directory for changes.
    */
    SystemUtils::DirectoryMonitor monitor;

    /**
    * This is set if any change happens in the temporary directory.
    */
    bool testAreaChanged = false;

    /**
     * This is used to wait for, a signal , or a condition
     * upon which that the owner might be waiting.
    */
    std::condition_variable_any condition;

    /**
     * This is used to synchronize access to the class.
    */
    std::mutex mutex;


    bool AwaitTestAreaChanged() {
        std::unique_lock< decltype(mutex) > lock(mutex);
        return condition.wait_for(
            lock,
            std::chrono::seconds(1),
            [this]{
                return testAreaChanged;
            }
        );
    }

    virtual void SetUp() {
        testAreaPath = SystemUtils::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(testAreaPath));
        monitor.Start(
            [this]{
                std::lock_guard< std::mutex > lock(mutex);
                testAreaChanged = true;
                condition.notify_all();
            },
            testAreaPath
        );
    }

    virtual void TearDown() {
        monitor.Stop();
        ASSERT_TRUE(SystemUtils::File::DeleteDirectory(testAreaPath));
    }
};


TEST_F(SubprocessTests, SubprocessTests_StartSubprocess_Test) {
    Owner owner;
    SystemUtils::Subprocess child;
    const auto reportedPid = child.StartChild(
        SystemUtils::File::GetExeParentDirectory() + "/MockSubprocess",
        {"Hello, World", "exit"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_NE(0, reportedPid);
    ASSERT_TRUE(AwaitTestAreaChanged());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    const std::string pidFilePath = testAreaPath + "/pid";
    FILE* pidFile = fopen(pidFilePath.c_str(), "r");
    ASSERT_FALSE(pidFile == NULL);
    unsigned int pid;
    const auto pidScanResult = fscanf(pidFile, "%u", &pid);
    (void)fclose(pidFile);
    ASSERT_EQ(1, pidScanResult);
    EXPECT_EQ(pid, reportedPid);
}

#ifdef _WIN32
TEST_F(SubprocessTests, StartSubprocessWithFileExtension_Test) {
    Owner owner;
    SystemUtils::Subprocess child;
    const auto reportedPid = child.StartChild(
        SystemUtils::File::GetExeParentDirectory() + "/MockSubprocess.exe",
        {"Hello, World", "exit"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_NE(0, reportedPid);
    ASSERT_TRUE(AwaitTestAreaChanged());
}
#endif

TEST_F(SubprocessTests, SubprocessTests_Exit__Test) {
    SystemUtils::Subprocess child;
    Owner owner;
    const auto reportedPid = child.StartChild(
        SystemUtils::File::GetExeParentDirectory() + "/MockSubprocess",
        {"Hello, World", "exit"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_TRUE(owner.AwaitExited());
    ASSERT_FALSE(owner.crashed);
}

TEST_F(SubprocessTests, SubprocessTests_Crash_Test) {
    SystemUtils::Subprocess child;
    Owner owner;
    const auto reportedPid = child.StartChild(
        SystemUtils::File::GetExeParentDirectory() + "/MockSubprocess",
        {"Hello, World", "crash"},
        [&owner]{ owner.SubprocessChildExited(); },
        [&owner]{ owner.SubprocessChildCrashed(); }
    );
    ASSERT_TRUE(owner.AwaitCrashed());
    ASSERT_FALSE(owner.exited);
}

