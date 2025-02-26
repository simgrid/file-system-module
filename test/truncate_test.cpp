/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

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

XBT_LOG_NEW_DEFAULT_CATEGORY(truncate_test, "Truncate Test");

class TruncateTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    TruncateTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and two disks...");
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        host_ = my_zone->create_host("my_host", "100Gf");
        disk_ = host_->create_disk("disk", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 1MB partition...");
        fs_->mount_partition("/dev/a/", ods, "1MB");
    }
};

TEST_F(TruncateTest, TruncateAndTell)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 100kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "100kB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "a"));
            XBT_INFO("Check current position, should be 100k ('append' mode)");
            ASSERT_DOUBLE_EQ(file->tell(), 100*1000);
            XBT_INFO("Try to truncate an opened file, which should fail");
            ASSERT_THROW(fs_->truncate_file("/dev/a/foo.txt", 50*1000), sgfs::InvalidTruncateException);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Truncate the file to half its size");
            ASSERT_NO_THROW(fs_->truncate_file("/dev/a/foo.txt", 50*1000));
            XBT_INFO("Check file size");
            ASSERT_DOUBLE_EQ(fs_->file_size("/dev/a/foo.txt"), 50*1000);
            XBT_INFO("Check free space");
            ASSERT_DOUBLE_EQ(fs_->get_partitions().at(0)->get_free_space(), 1000*1000 - 50*1000);
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "a"));
            XBT_INFO("Write 1kB");
            ASSERT_NO_THROW(file->write(1000, false));
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Check file size");
            ASSERT_DOUBLE_EQ(fs_->file_size("/dev/a/foo.txt"), 51*1000);
            XBT_INFO("Check free space");
            ASSERT_DOUBLE_EQ(fs_->get_partitions().at(0)->get_free_space(), 1000*1000 - 51000);
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}
