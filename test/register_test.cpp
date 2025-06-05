/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>

#include "fsmod/FileSystem.hpp"
#include "fsmod/OneDiskStorage.hpp"
#include "fsmod/FileSystemException.hpp"

#include "./test_util.hpp"

namespace sgfs=simgrid::fsmod;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(register_test, "Stat Test");

class RegisterTest : public ::testing::Test {
public:
    RegisterTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with 3 netzones with one host and one disk in each...");
        auto *root_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("root_zone");
        for (int i = 0; i < 3; i++) {
            std::string index = std::to_string(i);
            XBT_INFO("Creating Zone: my_zone_%d", i);
            auto* my_zone = root_zone->add_netzone_full("my_zone_" + index);
            auto host = my_zone->add_host("my_host_" + index, "100Gf");
            auto disk = host->add_disk("disk_" + index, "1kBps", "2kBps");
            my_zone->seal();
            XBT_INFO("Creating a one-disk storage on the host's disk...");
            auto ods = sgfs::OneDiskStorage::create("my_storage_" + index, disk);
            XBT_INFO("Creating a file system...");
            auto fs = sgfs::FileSystem::create("my_fs_" + index);
            XBT_INFO("Mounting a 1MB partition...");
            fs->mount_partition("/dev/a/", ods, "1MB");
            XBT_INFO("Register the file system in the NetZone...");
            sgfs::FileSystem::register_file_system(my_zone, fs);
            if (i == 0) {
                XBT_INFO("Creating a second file system in that zone...");
                auto extra_fs = sgfs::FileSystem::create("my_extra_fs");
                XBT_INFO("Register the file system in the NetZone...");
                sgfs::FileSystem::register_file_system(my_zone, extra_fs);
                XBT_INFO("Mounting a 10MB partition...");
                extra_fs->mount_partition("/tmp/", ods, "10MB");
            }
        }
        root_zone->seal();
    }
};

TEST_F(RegisterTest, RetrieveByActor)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        auto hosts = engine->get_all_hosts();
        int index = 0;
        for (const auto& h : hosts) {
            h->add_actor("TestActor", [index]() {
                std::map<std::string, std::shared_ptr<sgfs::FileSystem>, std::less<>> accessible_file_systems;
                std::shared_ptr<sgfs::FileSystem> fs;
                std::shared_ptr<sgfs::File> file;
                XBT_INFO("Retrieve the file systems this Actor can access");
                ASSERT_NO_THROW(accessible_file_systems = sgfs::FileSystem::get_file_systems_by_actor(sg4::Actor::self()));
                if (index == 0) {
                    XBT_INFO("There should be only two (%lu)", accessible_file_systems.size());
                    ASSERT_EQ(accessible_file_systems.size(), 2);
                    ASSERT_NO_THROW(fs = accessible_file_systems["my_fs_0"]);
                    ASSERT_EQ(fs->get_name(), "my_fs_0");
                    ASSERT_EQ(strcmp(fs->get_cname(), "my_fs_0"), 0);
                    ASSERT_NO_THROW(fs = accessible_file_systems["my_extra_fs"]);
                    ASSERT_EQ(fs->get_name(), "my_extra_fs");
                } else {
                    XBT_INFO("There should be only one,");
                    ASSERT_EQ(accessible_file_systems.size(), 1);
                    ASSERT_NO_THROW(fs = accessible_file_systems["my_fs_" + std::to_string(index)]);
                    ASSERT_EQ(fs->get_name(), "my_fs_" + std::to_string(index));
                }
            });
            index++;
        }

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(RegisterTest, RetrieveByZone)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        auto* engine = sg4::Engine::get_instance();
        auto netzones = engine->get_all_netzones();
        int index = -1;
        for (const auto& nz : netzones) {
            XBT_INFO("Looking for file systems in NetZone '%s'", nz->get_cname());
            std::map<std::string, std::shared_ptr<sgfs::FileSystem>, std::less<>> accessible_file_systems;
            ASSERT_NO_THROW(accessible_file_systems = sgfs::FileSystem::get_file_systems_by_netzone(nz));
            if (nz->get_name() == "_world_" || nz->get_name() == "root_zone") {
                ASSERT_EQ(accessible_file_systems.size(), 0);
            } else if (index == 0) {
                ASSERT_EQ(accessible_file_systems.size(), 2);
            } else {
                ASSERT_EQ(accessible_file_systems.size(), 1);
                auto [name, fs] = *accessible_file_systems.begin();
                ASSERT_EQ(name, "my_fs_" + std::to_string(index));
                ASSERT_EQ(fs->get_name(), "my_fs_" + std::to_string(index));
            }
            index++;
        }

        // Run the simulation
        ASSERT_NO_THROW(engine->run());
    });
}

TEST_F(RegisterTest, NoActor)  {
    DO_TEST_WITH_FORK([]() {
        XBT_INFO("Creating a very small platform");
        auto* engine = sg4::Engine::get_instance();
        auto* root_zone = engine->get_netzone_root();
        auto host = root_zone->add_host("my_host", "100Gf");
        auto disk = host->add_disk("disk", "1MBps", "2MBps");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk);
        root_zone->seal();

        XBT_INFO("Test we can has access to some file systems with no actor (we can't)");
        ASSERT_EQ(sgfs::FileSystem::get_file_systems_by_actor(nullptr).empty(), true);

        XBT_INFO("Create and register a file system in the Root NetZone...");
        ASSERT_NO_THROW(sgfs::FileSystem::register_file_system(root_zone, sgfs::FileSystem::create("root_fs")));

        XBT_INFO("Test if we can access to some file systems (we now should)");
        std::map<std::string, std::shared_ptr<sgfs::FileSystem>, std::less<>> accessible_file_systems;
        ASSERT_NO_THROW(accessible_file_systems = sgfs::FileSystem::get_file_systems_by_actor(nullptr));
        ASSERT_EQ(accessible_file_systems.size(), 1);

        std::shared_ptr<sgfs::FileSystem> root_fs;
        std::shared_ptr<sgfs::File> file;
        sg4::IoPtr my_write;

        XBT_INFO("Retrieve the 'root_fs' file system");
        ASSERT_NO_THROW(root_fs = accessible_file_systems["root_fs"]);
        XBT_INFO("Mounting a 10MB partition...");
        ASSERT_NO_THROW(root_fs->mount_partition("/root/", ods, "10MB"));
        XBT_INFO("Open File '/root/foo.txt'");
        ASSERT_NO_THROW(file = root_fs->open("/root/foo.txt", "w"));
        XBT_INFO("Write 1MB at /root/foo.txt");
        ASSERT_NO_THROW(my_write = file->write_async("1MB"));

        ASSERT_NO_THROW(engine->run());
        XBT_INFO("Simulation time %g", sg4::Engine::get_clock());
        ASSERT_NO_THROW(file->close());
        ASSERT_NO_THROW(engine->run());
    });
};