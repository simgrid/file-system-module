#include <iostream>

#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <utility>

#include "../include/FileSystem.hpp"
#include "../include/OneDiskStorage.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fs_test, "File System Test");

namespace sg4 = simgrid::s4u;
namespace sgfs = simgrid::module::fs;

class MyActor {
private:
    std::shared_ptr<sgfs::FileSystem> fs_;
public:
    explicit MyActor(std::shared_ptr<sgfs::FileSystem> fs) : fs_(std::move(fs)) {}

    void operator()() {



        XBT_INFO("Opening the file...");
        auto file = fs_->open("/dev/bogus/file.txt");
        XBT_INFO("Seek to offset 5kB...");
        file->seek(5000);
        XBT_INFO("Writing 6kB to it at offset 0...");
        file->write("6kB");
        XBT_INFO("Closing file...");
        file->close();
        XBT_INFO("The file size it: %llu", fs_->file_size("/dev/bogus/../bogus/file.txt"));

    }
};



int main(int argc, char **argv) {

    auto engine = new sg4::Engine(&argc, argv);

    XBT_INFO("Creating a platform with one host and one disk...");
    auto *my_zone = sg4::create_full_zone("AS");
    auto my_host = my_zone->create_host("my_host", "100Gf")->set_core_count(1);
    auto my_disk = my_host->create_disk("my_disk",
                         "1kBps",
                         "2kBps")->seal();
    my_host->seal();
    my_zone->seal();

    XBT_INFO("Creating a file system...");
    auto fs = sgfs::FileSystem::create("my_fs", 1024);
    XBT_INFO("Creating a one-disk storage on the host's disk...");
    auto ods = sgfs::OneDiskStorage::create("my_storage", my_disk);
    XBT_INFO("Mounting a 100kB partition...");
    fs->mount_partition("/dev/bogus/", ods, "100kB");

    XBT_INFO("Creating a 10kB file...");
    fs->create_file("/dev/bogus/file.txt", "10kB");
    XBT_INFO("The file size it: %llu", fs->file_size("/dev/bogus/../bogus/file.txt"));


    XBT_INFO("Creating one actor that will do everything...");
    sg4::Actor::create("MyActor", my_host, MyActor(fs));

    XBT_INFO("Launching the simulation...");
    engine->run();
}
