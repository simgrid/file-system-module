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

XBT_LOG_NEW_DEFAULT_CATEGORY(fs_test, "File System Test");

class FileSystemTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_one_;
    sg4::Disk * disk_two_;

    FileSystemTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and two disks...");
        auto *my_zone = sg4::Engine::get_instance()->get_netzone_root()->add_netzone_full("zone");
        host_ = my_zone->add_host("my_host", "100Gf");
        disk_one_ = host_->add_disk("disk_one", "1kBps", "2kBps");
        disk_two_ = host_->add_disk("disk_two", "1kBps", "2kBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's first disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_one_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100kB partition...");
        fs_->mount_partition("/dev/a/", ods, "100kB");
    }
};

TEST_F(FileSystemTest, MountPartition)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            XBT_INFO("Creating a one-disk storage on the host's second disk...");
            auto ods = sgfs::OneDiskStorage::create("my_storage", disk_two_);
            // Coverage
            ASSERT_EQ("my_storage", ods->get_name());
            ASSERT_EQ("my_storage", std::string(ods->get_cname()));
            ASSERT_EQ(nullptr, ods->get_controller_host());
            ASSERT_EQ(nullptr, ods->get_controller());
            ASSERT_NO_THROW(auto actor = ods->start_controller(disk_one_->get_host(), []() {}));

            XBT_INFO("Mount a new partition with a name that's not a clean path, which shouldn't work");
            ASSERT_THROW(fs_->mount_partition("/dev/../dev/a", ods, "100kB"), std::invalid_argument);
            XBT_INFO("Mount a new partition with a name conflict, which shouldn't work");
            ASSERT_THROW(fs_->mount_partition("/dev/a", ods, "100kB"), std::invalid_argument);
            XBT_INFO("Mount a new partition incorrectly");
            ASSERT_THROW(fs_->mount_partition("/dev/", ods, "100kB"), std::invalid_argument);
            XBT_INFO("Mount a new partition correctly");
            ASSERT_NO_THROW(fs_->mount_partition("/dev/b", ods, "100kB"));
            XBT_INFO("Try to access a non-existing partition by name, which shouldn't work");
            ASSERT_THROW(auto part = fs_->partition_by_name("/dev/"), std::invalid_argument);
            XBT_INFO("Retrieving the file system's partitions");
            std::vector<std::shared_ptr<sgfs::Partition>> partitions;
            ASSERT_NO_THROW(partitions = fs_->get_partitions());
            ASSERT_EQ(partitions.size(), 2);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}


TEST_F(FileSystemTest, FileCreate)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();

        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            XBT_INFO("Create a 10MB file at /foo/foo.txt, which should fail");
            ASSERT_THROW(fs_->create_file("/foo/foo.txt", "10MB"), sgfs::InvalidPathException);
            try {
                fs_->create_file("/foo/foo.txt", "10MB");
            } catch (sgfs::InvalidPathException &e) {
                auto msg = e.what(); // coverage
                XBT_ERROR("%s", msg);
            }
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt, which should fail");
            ASSERT_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"), sgfs::NotEnoughSpaceException);
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt, which should work");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            std::shared_ptr<sgfs::Partition> partition;
            ASSERT_NO_THROW(partition = fs_->partition_by_name("/dev/a"););
            ASSERT_EQ(fs_->get_partition_for_path_or_null("/dev/a/foo.txt"), partition);
            ASSERT_EQ(fs_->get_partition_for_path_or_null("/dev/bogus/foo.txt"), nullptr);
            ASSERT_EQ(fs_->partition_by_name("/dev/a")->get_name(), "/dev/a");
            ASSERT_EQ(0, strcmp(partition->get_cname(), "/dev/a"));
            ASSERT_EQ(100*1000, partition->get_size());
            ASSERT_EQ(1, fs_->partition_by_name("/dev/a")->get_num_files());
            XBT_INFO("Create the same file again at /dev/a/foo.txt, which should fail");
            ASSERT_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"), sgfs::FileAlreadyExistsException);
            XBT_INFO("Check remaining space");
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a")->get_free_space(), 90 * 1000);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
        ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a")->get_free_space(), 90 * 1000);
    });
}

TEST_F(FileSystemTest, Directories)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();

        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            XBT_INFO("Check that directory /dev/a/ exists");
            ASSERT_TRUE(fs_->directory_exists("/dev/a/"));
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            XBT_INFO("Create a 10kB file at /dev/a/b/c/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/b/c/foo.txt", "10kB"));
            ASSERT_TRUE(fs_->file_exists("/dev/a/b/c/foo.txt"));
            XBT_INFO("Create a 10kB file at /dev/a/b/c, which should fail");
            ASSERT_THROW(fs_->create_file("/dev/a/b/c/", "10kB"), sgfs::InvalidPathException);
            XBT_INFO("Check that what should exist does");
            ASSERT_FALSE(fs_->file_exists("/dev/a/b/c"));
            XBT_INFO("Check free space");
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a//////")->get_free_space(), 80*1000);
            XBT_INFO("Create a 10kB file at /dev/a/b/c/faa.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/b/c/faa.txt", "10kB"));
            std::set<std::string, std::less<>> found_files;
            ASSERT_THROW(found_files = fs_->list_files_in_directory("/dev/a/b/c_bogus"), sgfs::DirectoryDoesNotExistException);
            ASSERT_NO_THROW(found_files = fs_->list_files_in_directory("/dev/a/b/c"));
            ASSERT_TRUE(found_files.find("foo.txt") != found_files.end());
            ASSERT_TRUE(found_files.find("faa.txt") != found_files.end());
            XBT_INFO("Try to unlink non-existing directory. This shouldn't work");
            ASSERT_THROW(fs_->unlink_directory("/dev/a/b/d"), sgfs::DirectoryDoesNotExistException);
            ASSERT_FALSE(fs_->directory_exists("/dev/a/b/d"));


            ASSERT_THROW(fs_->create_directory("/whatever/bogus"), sgfs::InvalidPathException);
            ASSERT_THROW(fs_->create_directory("/dev/a/foo.txt"), sgfs::InvalidPathException);
            ASSERT_NO_THROW(fs_->create_directory("/dev/a/new_dir"));
            ASSERT_THROW(fs_->create_directory("/dev/a/new_dir"), sgfs::DirectoryAlreadyExistsException);
            ASSERT_NO_THROW(fs_->unlink_directory("/dev/a/new_dir"));


            XBT_INFO("Try to unlink a directory in which one file is opened. This shouldn't work");
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(file = fs_->open("/dev/a/b/c/foo.txt", "r"));
            ASSERT_EQ(file->get_path(), "/dev/a/b/c/foo.txt");
            ASSERT_EQ(file->get_access_mode(), "r");
            ASSERT_EQ(file->get_file_system(), fs_.get());
            ASSERT_THROW(fs_->unlink_directory("/dev/a/b/c"), sgfs::FileIsOpenException);
            ASSERT_NO_THROW(file->close());
            ASSERT_NO_THROW(fs_->unlink_directory("/dev/a/b/c"));
            ASSERT_FALSE(fs_->directory_exists("/dev/a/b/c"));
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FileSystemTest, FileMove)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();

        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            XBT_INFO("Try to move a non-existing file. This shouldn't work");
            ASSERT_THROW(fs_->move_file("/dev/a/foo.txt", "/dev/a/b/c/foo.txt"), sgfs::FileNotFoundException);
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a/")->get_free_space(), 90*1000);
            ASSERT_DOUBLE_EQ(fs_->get_free_space_at_path("/dev/a/"), 90*1000);
            ASSERT_DOUBLE_EQ(fs_->get_free_space_at_path("/dev/a/foo.txt"), 90*1000);
            XBT_INFO("Try to move file /dev/a/foo.txt to the same path. This should work (no-op)");
            ASSERT_NO_THROW(fs_->move_file("/dev/a/foo.txt", "/dev/a/foo.txt"));
            XBT_INFO("Move file /dev/a/foo.txt to /dev/a/b/c/foo.txt");
            ASSERT_NO_THROW(fs_->move_file("/dev/a/foo.txt", "/dev/a/b/c/foo.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/a/b/c/foo.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/a/foo.txt"));

            XBT_INFO("Create a 20kB file at /dev/a/stuff.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/stuff.txt", "20kB"));
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a/")->get_free_space(), 70*1000);

            // Moving a smaller file to a bigger file
            XBT_INFO("Move file /dev/a/b/c/foo.txt to /dev/a/stuff.txt");
            ASSERT_NO_THROW(fs_->move_file("/dev/a/b/c/foo.txt", "/dev/a/stuff.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/a/b/c/foo.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/a/stuff.txt"));
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a/")->get_free_space(), 90*1000);

            // Moving a bigger file to a smaller file
            XBT_INFO("Create a 20kB file at /dev/a/big.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/big.txt", "20kB"));
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a/")->get_free_space(), 70*1000);
            XBT_INFO("Move file /dev/a/stuff.txt to /dev/a/big.txt");
            ASSERT_NO_THROW(fs_->move_file("/dev/a/stuff.txt", "/dev/a/big.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/a/stuff.txt"));
            ASSERT_TRUE(fs_->file_exists("/dev/a/big.txt"));
            ASSERT_DOUBLE_EQ(fs_->partition_by_name("/dev/a/")->get_free_space(), 90*1000);

            auto ods = sgfs::OneDiskStorage::create("my_storage", disk_two_);
            XBT_INFO("Mount a new partition");
            ASSERT_NO_THROW(fs_->mount_partition("/dev/b/", ods, "100kB"));
            XBT_INFO("Move file /dev/a/stuff.txt to /dev/b/stuff.txt which is forbidden");
            ASSERT_THROW(fs_->move_file("/dev/a/stuff.txt", "/dev/b/stuff.txt"), sgfs::InvalidMoveException);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FileSystemTest, FileOpenClose)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();

        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            XBT_INFO("Create a 10kB file at /dev/a/stuff/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/stuff/foo.txt", "10kB"));
            XBT_INFO("Create a 10kB file at /dev/a/stuff/other.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/stuff/other.txt", "10kB"));

            XBT_INFO("Opening the file");
            std::shared_ptr<sgfs::File> file;
            ASSERT_NO_THROW(file = fs_->open("/dev/a/stuff/foo.txt", "r"));

            XBT_INFO("Trying to move the file");
            ASSERT_THROW(fs_->move_file("/dev/a/stuff/foo.txt", "/dev/a/bar.txt"), sgfs::FileIsOpenException);
            XBT_INFO("Trying to unlink the file");
            ASSERT_THROW(fs_->unlink_file("/dev/a/stuff/foo.txt"), sgfs::FileIsOpenException);
            XBT_INFO("Trying to overwrite the file");
            ASSERT_THROW(fs_->move_file("/dev/a/stuff/other.txt", "/dev/a/stuff/foo.txt"), sgfs::FileIsOpenException);

            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Trying to unlink the file");
            ASSERT_NO_THROW(fs_->unlink_file("/dev/a/stuff/foo.txt"));
            ASSERT_FALSE(fs_->file_exists("/dev/a/stuff/foo.txt"));
            XBT_INFO("Trying to unlink the file again");
            ASSERT_THROW(fs_->unlink_file("/dev/a/stuff/foo.txt"), sgfs::FileNotFoundException);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FileSystemTest, TooManyFilesOpened) {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            std::shared_ptr<sgfs::File> file2;
            std::shared_ptr<sgfs::File> file3;

            XBT_INFO("Creating a one-disk storage on the host's second disk...");
            auto ods = sgfs::OneDiskStorage::create("my_storage", disk_two_);
            XBT_INFO("Creating another file system ...on which you can only open 2 files at a time");
            auto limited_fs = sgfs::FileSystem::create("my_limited_fs", 2);
            XBT_INFO("Mounting a 100kB partition...");
            limited_fs->mount_partition("/dev/a/", ods, "100kB");

            XBT_INFO("Opening a first file, should be fine");
            ASSERT_NO_THROW(file = limited_fs->open("/dev/a/stuff/foo.txt", "w"));
            XBT_INFO("Opening a second file, should be fine");
            ASSERT_NO_THROW(file2 = limited_fs->open("/dev/a/stuff/bar.txt", "w"));
            XBT_INFO("Opening a third file, should not work");
            ASSERT_THROW(file3 = limited_fs->open("/dev/a/stuff/baz.txt", "a"), sgfs::TooManyOpenFilesException);
            XBT_INFO("Close the first file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Opening a third file should now work");
            ASSERT_NO_THROW(file3 = limited_fs->open("/dev/a/stuff/baz.txt", "w"));
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FileSystemTest, BadAccessMode) {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            XBT_INFO("Open the file in read mode ('r')");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r"));
            XBT_INFO("Try to write in a file opened in read mode, which should fail");
            ASSERT_THROW(file->write("5kB"), std::invalid_argument);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open the file in write mode ('w')");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "w"));
            XBT_INFO("Try to read from a file opened in write mode, which should fail");
            ASSERT_THROW(file->read("5kB"), std::invalid_argument);
            XBT_INFO("Asynchronous read from a file opened in write mode should also fail");
            ASSERT_THROW(file->read_async("5kB"), std::invalid_argument);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Open the file in unsupported mode ('w+'), which should fail");
            ASSERT_THROW(file = fs_->open("/dev/a/foo.txt", "w+"), std::invalid_argument);
            XBT_INFO("Open a non-existing file in read mode ('r'), which should fail");
            ASSERT_THROW(file = fs_->open("/dev/a/bar.txt", "r"), sgfs::FileNotFoundException);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}

TEST_F(FileSystemTest, ReadPlusMode) {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        // Create one actor (for this test we could likely do it all in the maestro but what the hell)
        host_->add_actor("TestActor", [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            XBT_INFO("Open the file in read+ mode ('r+')");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt", "r+"));
            XBT_INFO("Seek to the mid point of the file");
            ASSERT_NO_THROW(file->seek(-5000, SEEK_END));
            XBT_INFO("Write 10Kb to the file");
            ASSERT_NO_THROW(file->write("10kB"));
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
            XBT_INFO("Check the file size");
            ASSERT_EQ(fs_->file_size("/dev/a/foo.txt"), 15000);
        });

        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
