#include <gtest/gtest.h>
#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "../include/PathUtil.hpp"
#include "../include/FileSystem.hpp"
#include "../include/OneDiskStorage.hpp"
#include "../include/FileSystemException.hpp"

#include "./test_util.hpp"

namespace sgfs=simgrid::module::fs;
namespace sg4=simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(ods_io_test, "OneDiskStorage I/O Test");

class OneDiskStorageTest : public ::testing::Test {
public:
    std::shared_ptr<sgfs::FileSystem> fs_;
    sg4::Host * host_;
    sg4::Disk * disk_;

    OneDiskStorageTest() = default;

    void setup_platform() {
        XBT_INFO("Creating a platform with one host and one disk...");
        auto *my_zone = sg4::create_full_zone("zone");
        host_ = my_zone->create_host("my_host", "100Gf");
        disk_ = host_->create_disk("disk_one", "1MBps", "2MBps");
        my_zone->seal();

        XBT_INFO("Creating a one-disk storage on the host's disk...");
        auto ods = sgfs::OneDiskStorage::create("my_storage", disk_);
        XBT_INFO("Creating a file system...");
        fs_ = sgfs::FileSystem::create("my_fs");
        XBT_INFO("Mounting a 100MB partition...");
        fs_->mount_partition("/dev/a/", ods, "100MB");
    }
};


TEST_F(OneDiskStorageTest, SingleRead)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        sg4::Actor::create("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            XBT_INFO("Create a 10kB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10kB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt"));
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

TEST_F(OneDiskStorageTest, SingleAsyncRead)  {
    DO_TEST_WITH_FORK([this]() {
        this->setup_platform();
        sg4::Actor::create("TestActor", host_, [this]() {
            std::shared_ptr<sgfs::File> file;
            sg4::IoPtr my_read;
            sg_size_t num_bytes_read = 0;
            XBT_INFO("Create a 10MB file at /dev/a/foo.txt");
            ASSERT_NO_THROW(fs_->create_file("/dev/a/foo.txt", "10MB"));
            XBT_INFO("Open File '/dev/a/foo.txt'");
            ASSERT_NO_THROW(file = fs_->open("/dev/a/foo.txt"));
            XBT_INFO("Asynchronously read 2MB at /dev/a/foo.txt");
            ASSERT_NO_THROW(my_read = file->read_async("2MB"));
            XBT_INFO("Sleep for 1 second");
            ASSERT_NO_THROW(sg4::this_actor::sleep_for(1));
            XBT_INFO("Sleep complete. Clock should be at1s");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 1.0);
            XBT_INFO("Wait for read completion");
            ASSERT_NO_THROW(num_bytes_read = file->read_wait(my_read));
            XBT_INFO("Read complete. Clock should be at 2s");
            ASSERT_DOUBLE_EQ(sg4::Engine::get_clock(), 2.0);
            ASSERT_DOUBLE_EQ(num_bytes_read, 2000000);
            XBT_INFO("Close the file");
            ASSERT_NO_THROW(file->close());
        });
        // Run the simulation
        ASSERT_NO_THROW(sg4::Engine::get_instance()->run());
    });
}
