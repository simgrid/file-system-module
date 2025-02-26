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

XBT_LOG_NEW_DEFAULT_CATEGORY(fifo_caching_test, "FIFO Caching Test");

class CachingTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    CachingTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and one disk...");
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        host_ = my_zone->create_host("my_host", "100Gf");
        disk_ = host_->create_disk("disk", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100MB FIFO partition...");
        fs_->mount_partition("/dev/fifo/", ods, "100MB", sgfs::Partition::CachingScheme::FIFO);
        XBT_INFO("Mounting a 100MB LRU partition...");
        fs_->mount_partition("/dev/lru/", ods, "100MB", sgfs::Partition::CachingScheme::LRU);
   }
};

TEST_F(CachingTest, LogicTest){
      DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
            XBT_INFO("Try to mount partition with empty name");
            ASSERT_THROW(fs_->mount_partition("", ods, "100MB"),
                         std::invalid_argument);
            ASSERT_THROW(fs_->mount_partition("", ods, "100MB", sgfs::Partition::CachingScheme::FIFO),
                         std::invalid_argument);
            ASSERT_THROW(fs_->mount_partition("", ods, "100MB", sgfs::Partition::CachingScheme::LRU),
                         std::invalid_argument);
            sg4::this_actor::sleep_for(1);
        });
        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, FIFOBasics)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 20MB file at /dev/fifo/20mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/20mb.txt", "20MB"));
            XBT_INFO("Create a 60MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/60mb.txt", "60MB"));
            XBT_INFO("Create a 30MB file at /dev/fifo/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_FALSE(fs_->file_exists("/dev/fifo/20mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/60mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/30mb.txt"));
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, FIFOBasicsWithUnevictableFiles)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 20MB file at /dev/fifo/20mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/20mb.txt", "20MB"));
            XBT_INFO("Making the 20MB file at /dev/fifo/20mb.txt unevictable");
            ASSERT_THROW(fs_->make_file_evictable("/dev/fifo/bogus.txt", false), sgfs::FileNotFoundException);
            ASSERT_NO_THROW(fs_->make_file_evictable("/dev/fifo/20mb.txt", false));
            XBT_INFO("Create a 60MB file at /dev/a/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/60mb.txt", "60MB"));
            XBT_INFO("Create a 30MB file at /dev/fifo/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/20mb.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/fifo/60mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/30mb.txt"));
            ASSERT_NO_THROW(fs_->make_file_evictable("/dev/fifo/20mb.txt", true));
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/60mb.txt", "60MB"));
            ASSERT_FALSE(fs_->file_exists("/dev/fifo/20mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/60mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/30mb.txt"));
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, FIFODontEvictOpenFiles) {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            XBT_INFO("Create a 20MB file at /dev/fifo/20mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/20mb.txt", "20MB"));
            XBT_INFO("Create a 60MB file at /dev/fifo/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/60mb.txt", "60MB"));
            XBT_INFO("Opening the 20MB file");
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(file = fs_->open("/dev/fifo/20mb.txt", "r"));
            XBT_INFO("Create a 30MB file at /dev/fifo/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/fifo/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/20mb.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/fifo/60mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/fifo/30mb.txt"));
            XBT_INFO("Opening the 30MB file");
            std::shared_ptr<sgfs::File> file2;
            ASSERT_NO_THROW(file2 = fs_->open("/dev/fifo/30mb.txt", "r"));
            XBT_INFO("Create a 60MB file at /dev/fifo/60mb.txt");
            ASSERT_THROW(fs_->create_file("/dev/fifo/60mb.txt", "60MB"), sgfs::NotEnoughSpaceException);
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, FIFOExtensive)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            std::vector<std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>>> experiments =
                    {
                            {{"create", "f1", "20MB"}, {"f1"}, {}},
                            {{"create", "f2", "50MB"}, {"f1", "f2"}, {}},
                            {{"create", "f3", "30MB"}, {"f1", "f2", "f3"}, {}},
                            {{"delete", "f1"},         {"f2", "f3"}, {"f1"}},
                            {{"create", "f4", "30MB"}, {"f3", "f4"}, {"f2"}},
                            {{"create", "f5", "50MB"}, {"f4", "f5"}, {"f3"}},
                            {{"create", "f6", "10MB"}, {"f4", "f5", "f6"}, {}},
                            {{"create", "f7", "10MB"}, {"f4", "f5", "f6", "f7"}, {}},
                            {{"create", "f8", "10MB"}, {"f5", "f6", "f7", "f8"}, {"f4"}},
                    };

            for (auto const &xp : experiments) {
                // Perform action
                auto action = std::get<0>(xp);
                auto expected_files_there = std::get<1>(xp);
                auto expected_files_not_there = std::get<2>(xp);
                if (action.at(0) == "create") {
                    auto file_name = action.at(1);
                    auto file_size = action.at(2);
                    fs_->create_file("/dev/fifo/" + file_name, file_size);
                } else if (action.at(0) == "delete") {
                    auto file_name = action.at(1);
                    fs_->unlink_file("/dev/fifo/" + file_name);
                }
                // Check state
                for (auto const &f : expected_files_there) {
                    ASSERT_TRUE(fs_->file_exists("/dev/fifo/" + f));
                }
                for (auto const &f : expected_files_not_there) {
                    ASSERT_FALSE(fs_->file_exists("/dev/fifo/" + f));
                }
            }
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, LRUBasics)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            XBT_INFO("Create a 20MB file at /dev/lru/20mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/lru/20mb.txt", "20MB"));
            XBT_INFO("Create a 60MB file at /dev/lru/60mb.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/lru/60mb.txt", "60MB"));
            XBT_INFO("Open file 20mb.txt and access it");
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(file = fs_->open("/dev/lru/20mb.txt", "r"));
            ASSERT_NO_THROW(file->read(10));
            ASSERT_NO_THROW(file->close());
            ASSERT_NO_THROW(fs_->create_file("/dev/lru/30mb.txt", "30MB"));
            XBT_INFO("Check that files are as they should be");
            ASSERT_TRUE(fs_->file_exists("/dev/lru/20mb.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/lru/60mb.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/lru/30mb.txt"));
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(CachingTest, LRUExtensive)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        engine->add_actor("TestActor", host_, [this]() {
            std::vector<std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>>> experiments =
                    {
                            {{"create", "f1", "20MB"}, {"f1"}, {}},
                            {{"create", "f2", "50MB"}, {"f1", "f2"}, {}},
                            {{"create", "f3", "30MB"}, {"f1", "f2", "f3"}, {}},
                            {{"access", "f1"},         {"f1", "f2", "f3"}, {}},
                            {{"create", "f4", "30MB"}, {"f1", "f3", "f4"}, {"f2"}},
                            {{"delete", "f1"},         {"f3", "f4"}, {"f1"}},
                            {{"create", "f5", "10MB"}, {"f3", "f4", "f5"}, {}},
                            {{"create", "f6", "80MB"}, {"f5", "f6"}, {"f3", "f4"}},
                            {{"access", "f5"},         {"f5", "f6"}, {}},
                            {{"create", "f7", "20MB"}, {"f5", "f7"}, {"f6"}},
                    };

            for (auto const &xp : experiments) {
                // Perform action
                auto action = std::get<0>(xp);
                auto expected_files_there = std::get<1>(xp);
                auto expected_files_not_there = std::get<2>(xp);
                if (action.at(0) == "create") {
                    auto file_name = action.at(1);
                    auto file_size = action.at(2);
                    fs_->create_file("/dev/lru/" + file_name, file_size);
                } else if (action.at(0) == "delete") {
                    auto file_name = action.at(1);
                    fs_->unlink_file("/dev/lru/" + file_name);
                } else if (action.at(0) == "access") {
                    auto file_name = action.at(1);
                    auto file = fs_->open("/dev/lru/" + file_name, "r");
                    file->read(1);
                    file->close();
                }
                // Check state
                for (auto const &f : expected_files_there) {
                    ASSERT_TRUE(fs_->file_exists("/dev/lru/" + f));
                }
                for (auto const &f : expected_files_not_there) {
                    ASSERT_FALSE(fs_->file_exists("/dev/lru/" + f));
                }
            }
        });

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}
