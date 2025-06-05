/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

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

XBT_LOG_NEW_DEFAULT_CATEGORY(stat_test, "Stat Test");

class StatTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    StatTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and two disks...");
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

TEST_F(StatTest, Stat)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 100kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "100kB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            XBT_INFO("Calling stat()");
            std::unique_ptr<sgfs::FileStat> stat_struct;
            ASSERT_NO_THROW(stat_struct = file->stat());
            XBT_INFO("Checking sanity");
            ASSERT_EQ(stat_struct->size_in_bytes, 100*1000);
            ASSERT_DOUBLE_EQ(stat_struct->last_modification_date, simgrid_get_clock());
            ASSERT_DOUBLE_EQ(stat_struct->last_access_date, simgrid_get_clock());
            ASSERT_EQ(stat_struct->refcount, 1);
            XBT_INFO("Modifying file state");
            std::shared_ptr<sgfs::File> file2;
            ASSERT_NO_THROW(file2 = fs_->open("/dev/a/foo.txt", "w"));
            ASSERT_NO_THROW(file2->seek(100*1000));
            ASSERT_NO_THROW(file2->write(12*1000, false));
            ASSERT_NO_THROW(file->close());
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(10));
            ASSERT_NO_THROW(file2 = fs_->open("/dev/a/foo.txt", "r"));
            ASSERT_NO_THROW(file->seek(0));
            ASSERT_NO_THROW(file->read(1000, false));
            XBT_INFO("Calling stat() again");
            ASSERT_NO_THROW(stat_struct = file->stat());
            XBT_INFO("Checking sanity");
            ASSERT_EQ(stat_struct->size_in_bytes, 112*1000);
            ASSERT_DOUBLE_EQ(stat_struct->last_modification_date, simgrid_get_clock() - 10);
            ASSERT_DOUBLE_EQ(stat_struct->last_access_date, simgrid_get_clock());
            ASSERT_EQ(stat_struct->refcount, 2);
            XBT_INFO("Modifying file state");
            ASSERT_NO_THROW(file2->close());
            XBT_INFO("Calling stat() again");
            ASSERT_NO_THROW(stat_struct = file->stat());
            XBT_INFO("Checking sanity");
            ASSERT_EQ(stat_struct->refcount, 1);
            ASSERT_NO_THROW(file->close());
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
