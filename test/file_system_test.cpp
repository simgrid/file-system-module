#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "../include/PathUtil.hpp"
#include "../include/FileSystem.hpp"
#include "../include/OneDiskStorage.hpp"

namespace sgfs=simgrid::module::fs;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(fs_test, "File System Test");

class FileSystemTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Engine *engine_;

    FileSystemTest() {
        int argc = 1;
        char **argv = (char **) calloc(argc, sizeof(char *));
        argv[0] = strdup("unit_test");

        // Setup up engine, platform, and file system
        engine_ = new sg4::Engine(&argc, argv);

        XBT_INFO("Creating a platform with one host and one disk...");
        auto *my_zone = sg4::create_full_zone("zone");
        host_ = my_zone->create_host("my_host", "100Gf");
        auto my_disk = host_->create_disk("my_disk", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", my_disk);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100kB partition...");
        fs_->mount_partition("/dev/a/", ods, "100kB");
    }

};

TEST_F(FileSystemTest, FileCreate)  {

    // Create one actor (for this test we could likely do it all in the maestro but what the hell)
    sg4::Actor::create("FileCreator", host_, [this]() {
        XBT_INFO("Creating a 10MB file, which should fail");
        ASSERT_THROW(this->fs_->create_file("/dev/a/foo.txt", "10MB"), std::exception);
        XBT_INFO("Creating a 10kB file, which should work");
        ASSERT_NO_THROW(this->fs_->create_file("/dev/a/foo.txt", "10kB"));
        XBT_INFO("Checking remaining space");
        ASSERT_DOUBLE_EQ(this->fs_->partition_by_name("/dev/a")->get_free_space(), 90*1000);
    });

    // Run the simulation
    ASSERT_NO_THROW(engine_->run());
    
    // Redundant check, just for kicks
    ASSERT_DOUBLE_EQ(this->fs_->partition_by_name("/dev/a")->get_free_space(), 90*1000);

}