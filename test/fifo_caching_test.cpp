#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "fsmod/FileSystem.hpp"
#include "fsmod/OneDiskStorage.hpp"
#include "fsmod/FileSystemException.hpp"

#include "./test_util.hpp"

namespace sgfs=simgrid::module::fs;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(fifo_caching_test, "FIFO Caching Test");

class FIFOCachingTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    FIFOCachingTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and two disks...");
        auto *my_zone = sg4::create_full_zone("zone");
        host_ = my_zone->create_host("my_host", "100Gf");
        disk_ = host_->create_disk("disk", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100MB partition...");
        fs_->mount_partition("/dev/a/", ods, "100MB", sgfs::Partition::CachingScheme::FIFO);
    }
};

TEST_F(FIFOCachingTest, Basics)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        sg4::Actor::create("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 20MB file at /dev/a/20mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/20mb.txt", "20MB"));
            XBT_INFO("Create a 60MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/60mb.txt", "60MB"));
            XBT_INFO("Create a 30MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_FALSE(this->fs_->file_exists("/dev/a/20mb.txt"));
            ASSERT_TRUE(this->fs_->file_exists("/dev/a/60mb.txt"));
            ASSERT_TRUE(this->fs_->file_exists("/dev/a/30mb.txt"));
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FIFOCachingTest, DontEvictOpenFiles)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        sg4::Actor::create("TestActor", host_, [this]() {
            XBT_INFO("Create a 20MB file at /dev/a/20mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/20mb.txt", "20MB"));
            XBT_INFO("Create a 60MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/60mb.txt", "60MB"));
            XBT_INFO("Opening the 20MB file");
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(file = this->fs_->open("/dev/a/20mb.txt"));
            XBT_INFO("Create a 30MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(this->fs_->create_file("/dev/a/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_TRUE(this->fs_->file_exists("/dev/a/20mb.txt"));
            ASSERT_FALSE(this->fs_->file_exists("/dev/a/60mb.txt"));
            ASSERT_TRUE(this->fs_->file_exists("/dev/a/30mb.txt"));
            XBT_INFO("Opening the 30MB file");
            std::shared_ptr<sgfs::File> file2;
            ASSERT_NO_THROW(file2 = this->fs_->open("/dev/a/30mb.txt"));
            XBT_INFO("Create a 60MB file at /dev/a/60mb.txt");
            ASSERT_THROW(this->fs_->create_file("/dev/a/60mb.txt", "60MB"), sgfs::FileSystemException);

        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
