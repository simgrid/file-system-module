/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "fsmod/PathUtil.hpp"
#include "fsmod/FileSystem.hpp"
#include "fsmod/JBODStorage.hpp"
#include "fsmod/FileSystemException.hpp"

#include "./test_util.hpp"

namespace sgfs=simgrid::fsmod;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(jds_io_test, "JBODStorage I/O Test");

class JBODStorageTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::JBODStorage> jds_;
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host* fs_client_;
    sg4::Host* fs_server_;
    std::vector<sg4::Disk*> disks_;

    JBODStorageTest() = default;

    void setup_platform() {
        sg4::Engine::set_config("network/crosstraffic:0");
        XBT_INFO("Creating a platform with two hosts and a 4-disk JBOD...");
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        fs_client_ = my_zone->add_host("fs_client", "100Mf");
        fs_server_ = my_zone->add_host("fs_server", "200Mf");
        for (int i = 0 ; i < 4 ; i++ )
            disks_.push_back(fs_server_->add_disk("jds_disk" + std::to_string(i), "2MBps", "1MBps"));

        const auto* link = my_zone->add_link("link", 120e6 / 0.97)->set_latency(0);
        my_zone->add_route(fs_client_, fs_server_, {link});
        my_zone->seal();

        XBT_INFO("Creating a JBOD storage on fs_server...");
        jds_ = sgfs::JBODStorage::create("my_storage", disks_);
        jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID5);
        ASSERT_EQ(jds_->get_raid_level(), sgfs::JBODStorage::RAID::RAID5);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100MB partition...");
        fs_->mount_partition("/dev/a/", jds_, "100MB");
    }
};

TEST_F(JBODStorageTest, NotEnoughDisks)  {
    DO_TEST_WITH_FORK([this]() {
        XBT_INFO("Creating a platform with two hosts and 3 disks");
        std::vector<sg4::Disk*> disks;
        std::shared_ptr<sgfs::JBODStorage> jds3;
        std::shared_ptr<sgfs::JBODStorage> jds2;
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        auto host = my_zone->add_host("host", "100Mf");
        disks.push_back(host->add_disk("jds_disk1", "2MBps", "1MBps"));
        disks.push_back(host->add_disk("jds_disk2", "2MBps", "1MBps"));
        disks.push_back(host->add_disk("jds_disk3", "2MBps", "1MBps"));
        my_zone->seal();
        XBT_INFO("Create a 3-Disk JBOD storage using RAID0 by default");
        ASSERT_NO_THROW(jds3 = sgfs::JBODStorage::create("my_storage", disks));
        XBT_INFO("Try to set RAID level to RAID6 which should fail");
        ASSERT_THROW(jds3->set_raid_level(sgfs::JBODStorage::RAID::RAID6), std::invalid_argument);
        XBT_INFO("Remove one disk");
        ASSERT_NO_THROW(disks.pop_back());
        XBT_INFO("Create a 2-Disk JBOD storage using RAID0 by default");
        ASSERT_NO_THROW(jds2 = sgfs::JBODStorage::create("my_storage", disks));
        XBT_INFO("Try to set RAID level to RAID4 which should fail");
        ASSERT_THROW(jds2->set_raid_level(sgfs::JBODStorage::RAID::RAID4), std::invalid_argument);
        XBT_INFO("Try to set RAID level to RAID5 which should also fail");
        ASSERT_THROW(jds2->set_raid_level(sgfs::JBODStorage::RAID::RAID5), std::invalid_argument);
        XBT_INFO("Try to set RAID level to RAID1 which should work");
        ASSERT_NO_THROW(jds2->set_raid_level(sgfs::JBODStorage::RAID::RAID1));
        XBT_INFO("Try to set RAID level to RAID3 which is not supported but should work");
        ASSERT_NO_THROW(jds2->set_raid_level(sgfs::JBODStorage::RAID::RAID3));

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}



TEST_F(JBODStorageTest, SingleRead)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            XBT_INFO("Check access mode");
            ASSERT_EQ(file->get_access_mode(), "r");
            XBT_INFO("read 0B at /dev/a/foo.txt, which should return 0");
            ASSERT_DOUBLE_EQ(file->read(0), 0);
            XBT_INFO("read 100kB at /dev/a/foo.txt, which should return only 10kB");
            ASSERT_DOUBLE_EQ(file->read("100kB"), 10000);
            XBT_INFO("read 10kB at /dev/a/foo.txt, which should return O as we are at the end of the file");
            ASSERT_DOUBLE_EQ(file->read("10kB"), 0);
            XBT_INFO("Seek back to SEEK_SET and read 9kB at /dev/a/foo.txt");
            ASSERT_NO_THROW(file->seek(SEEK_SET));
            ASSERT_DOUBLE_EQ(file->read("9kB"), 9000);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, SingleAsyncRead)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            sg4::IoPtr my_read;
            sg_size_t num_bytes_read = 0;
            XBT_INFO("Create a 60MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "60MB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            XBT_INFO("Asynchronously read 12MB at /dev/a/foo.txt");
            ASSERT_NO_THROW(my_read = file->read_async("12MB"));
            XBT_INFO("Sleep for 1 second");
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(1));
            XBT_INFO("Sleep complete. Clock should be at 1s");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 1.0);
            XBT_INFO("Wait for read completion");
            ASSERT_NO_THROW(my_read->wait());
            XBT_INFO("Read complete. Clock should be at 2.1s (2s to read, 0.1 to transfer)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 2.1);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, SingleWrite)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 1MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "1MB"));
            XBT_INFO("Check remaining space");
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a")->get_free_space(), 99 * 1000 *1000);
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Opening file in 'w' mode reset size to 0. Check remaining space");
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a")->get_free_space(), 100 * 1000 *1000);
            XBT_INFO("Write 0B at /dev/a/foo.txt, which should return 0");
            ASSERT_DOUBLE_EQ(file->write(0), 0);
            XBT_INFO("Write 200MB at /dev/a/foo.txt, which should not work");
            ASSERT_THROW(file->write("200MB"), sgfs::NotEnoughSpaceException);
            XBT_INFO("Write 2MB at /dev/a/foo.txt");
            ASSERT_DOUBLE_EQ(file->write("2MB"), 2000000);
            XBT_INFO("Check remaining space");
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a")->get_free_space(), 98 * 1000 * 1000);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, SingleAsyncWrite)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            sg4::IoPtr my_write;
            sg_size_t num_bytes_written = 0;
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Asynchronously write 12MB at /dev/a/foo.txt");
            ASSERT_NO_THROW(my_write = file->write_async("12MB"));
            XBT_INFO("Sleep for 1 second");
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(1));
            XBT_INFO("Sleep complete. Clock is at 1s");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 1.0);
            XBT_INFO("Wait for write completion");
            ASSERT_NO_THROW(my_write->wait());
            XBT_INFO("Write complete. Clock is at 4.12s (.1s to transfer, 0.02 to compute parity, 4s to write)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 4.12);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, SingleDetachedWrite)  {
    DO_TEST_WITH_FORK([this]() {
        xbt_log_control_set("root.thresh:info");
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            sg4::IoPtr my_write;
            sg_size_t num_bytes_written = 0;
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Asynchronously write 12MB at /dev/a/foo.txt");
            ASSERT_NO_THROW(my_write = file->write_async("12MB", true));
            XBT_INFO("Sleep for 4 seconds");
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(4));
            XBT_INFO("Sleep complete. Clock should be at 4s and nothing should be written yet");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 4.0);
            ASSERT_EQ(file->get_num_bytes_written(my_write), 0);
            XBT_INFO("Sleep for 0.12 more second");
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(0.12));
            XBT_INFO("Clock should be at 4.12s (.1s to transfer, 0.02 to compute parity, 4s to write), and write be complete");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 4.12);
            // FIXME: On a JBOD we can't retrieve the number of bytes written (or read)
            //ASSERT_DOUBLE_EQ(file->get_num_bytes_written(my_write), 2000000);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, ReadWriteUnsupportedRAID)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID2));
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt' in write mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Write 2MB at /dev/a/foo.txt, which should fail");
            ASSERT_THROW(file->write("12MB"), std::invalid_argument);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open File '/dev/a/foo.txt' in read mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            XBT_INFO("Read 12MB at /dev/a/foo.txt, which should fail too");
            ASSERT_THROW(file->read("12MB"), std::invalid_argument);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, ReadWriteRAID0)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID0));
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt' in write mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Write 12MB at /dev/a/foo.txt");
            ASSERT_DOUBLE_EQ(file->write("12MB"), 12000000);
            XBT_INFO("Write complete. Clock is at 3.1s (.1s to transfer, 3s to write)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 3.1);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open File '/dev/a/foo.txt' in read mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            ASSERT_DOUBLE_EQ(file->read("12MB"), 12000000);
            XBT_INFO("Read complete. Clock is at 4.7s (1.5s to read, .1s to transfer)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 4.7);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, ReadWriteRAID1)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID1));
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt' in write mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Write 6MB at /dev/a/foo.txt");
            ASSERT_DOUBLE_EQ(file->write("6MB"), 6000000);
            XBT_INFO("Write complete. Clock is at 6.05s (.05s to transfer, 6s to write)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 6.05);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open File '/dev/a/foo.txt' in read mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            ASSERT_DOUBLE_EQ(file->read("6MB"), 6000000);
            XBT_INFO("Read complete. Clock is at 9.1s (3s to read, .05s to transfer)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 9.1);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, ReadWriteRAID4)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID4));
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt' in write mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Write 4MB at /dev/a/foo.txt");
            ASSERT_DOUBLE_EQ(file->write("6MB"), 6000000);
            XBT_INFO("Write complete. Clock is at 2.06s (.05s to transfer, 0.01 to compute parity, 2s to write)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 2.06);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open File '/dev/a/foo.txt' in read mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            ASSERT_DOUBLE_EQ(file->read("6MB"), 6000000);
            XBT_INFO("Read complete. Clock is at 3.11s (1s to read, .05s to transfer)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 3.11);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(JBODStorageTest, ReadWriteRAID6)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        fs_client_->add_actor("TestActor", [this]() {
            std::vector<std::string> expected_debug_outputs =
                {"Parity disks are #1 and #2. Reading From: jds_disk0 jds_disk3 ",
                 "Parity disks are #0 and #1. Reading From: jds_disk2 jds_disk3 ",
                 "Parity disks are #3 and #0. Reading From: jds_disk1 jds_disk2 "};
            std::string captured_debug_output;
            std::shared_ptr<sgfs::File> file;

            ASSERT_NO_THROW(jds_->set_raid_level(sgfs::JBODStorage::RAID::RAID6));
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt' in write mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Write 6MB at /dev/a/foo.txt");
            ASSERT_DOUBLE_EQ(file->write("6MB"), 6000000);
            XBT_INFO("Write complete. Clock is at 3.08s (.05s to transfer, 0.02 to compute parity, 3s to write)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 3.08);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open File '/dev/a/foo.txt' in read mode");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            ASSERT_DOUBLE_EQ(file->read("6MB"), 6000000);
            XBT_INFO("Read complete. Clock is at 4.63s (1.5s to read, .05s to transfer)");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 4.63);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Test the evolution of parity disks ");
            xbt_log_control_set("fsmod_jbod.thresh:debug fsmod_jbod.fmt:'%m'");
            for (int i = 0; i < 3; i++ ){
                xbt_log_control_set("no_loc");
                ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
                ASSERT_NO_THROW(file->write("6MB"));
                ASSERT_NO_THROW(file->close());
                ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
                ASSERT_NO_THROW(testing::internal::CaptureStderr());
                ASSERT_NO_THROW(file->read("6MB"));
                ASSERT_NO_THROW(captured_debug_output = testing::internal::GetCapturedStderr());
                XBT_INFO("%s", captured_debug_output.c_str());
                ASSERT_EQ(captured_debug_output, expected_debug_outputs.at(i));
            }
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
