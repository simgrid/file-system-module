/* Copyright (c) 2024-2026. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "fsmod/FileSystem.hpp"
#include "fsmod/OneDiskStorage.hpp"
#include "fsmod/FileSystemException.hpp"

#include "./test_util.hpp"

namespace sgfs=simgrid::fsmod;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(seek_test, "Seek Test");

class SeekTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    SeekTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and one disk...");
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        host_ = my_zone->add_host("my_host", "100Gf");
        disk_ = host_->add_disk("disk", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 1MB partition...");
        fs_->mount_partition("/dev/a/", ods, "1MB");
    }
};

TEST_F(SeekTest, SeekandTell)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 100kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "100kB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "a"));
            XBT_INFO("Check current position, should be 100k ('append' mode)");
            ASSERT_DOUBLE_EQ(file->tell(), 100*1000);
            XBT_INFO("Seek back to beginning of file");
            ASSERT_NO_THROW(file->seek(SEEK_SET));
            XBT_INFO("Check current position, should be 0");
            ASSERT_DOUBLE_EQ(file->tell(), 0);
            XBT_INFO("Seek 1kB forward");
            ASSERT_NO_THROW(file->seek(1000));
            XBT_INFO("Check current position, should be 1k");
            ASSERT_DOUBLE_EQ(file->tell(), 1000);
            XBT_INFO("Try to seek before the beginning of the file, should not work.");
            ASSERT_THROW(file->seek(-1000, SEEK_SET), sgfs::InvalidSeekException);
            XBT_INFO("Try to seek backwards from the end of the file, should work");
            ASSERT_NO_THROW(file->seek(-1000, SEEK_END));
            XBT_INFO("Check current position, should be 99k");
            ASSERT_DOUBLE_EQ(file->tell(), 99*1000);
            XBT_INFO("Seek backwards from the current position in file, should work");
            ASSERT_NO_THROW(file->seek(-1000, SEEK_CUR));
            XBT_INFO("Check current position, should be 98k");
            ASSERT_DOUBLE_EQ(file->tell(), 98*1000);
            XBT_INFO("Try to seek beyond the end of the file, should work.");
            ASSERT_NO_THROW(file->seek(5000, SEEK_CUR));
            XBT_INFO("Check current position, should be 103k");
            ASSERT_DOUBLE_EQ(file->tell(), 103*1000);
            XBT_INFO("Check file size, it should still be 100k");
            ASSERT_DOUBLE_EQ(fs_->file_size("/dev/a/foo.txt"), 100000);
            XBT_INFO("Write 1kB to /dev/a/foo.txt");
            ASSERT_NO_THROW(file->write("1kB"));
            XBT_INFO("Check file size, it should now be 104k");
            ASSERT_DOUBLE_EQ(fs_->file_size("/dev/a/foo.txt"), 104000);
            XBT_INFO("Seek from an arbitrary position in file, should work");
            ASSERT_NO_THROW(file->seek(1000, 2000));
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Checking file size of a non-existing file, which should fail");
            ASSERT_THROW(auto size = fs_->file_size("/dev/a/foo_bogus.txt"), sgfs::FileNotFoundException);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
