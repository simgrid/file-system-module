#include <iostream>

#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <utility>

#include "../include/fsmod.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fs_test, "File System Test");

namespace sg4 = simgrid::s4u;
namespace sgfs = simgrid::module::fs;

class FileWriterActor {
private:
    std::shared_ptr<sgfs::FileSystem> fs_;
    std::string file_path_;
    double sleep_time_;
    sg_offset_t offset_;
    sg_size_t num_bytes_;

public:
    explicit FileWriterActor(std::shared_ptr<sgfs::FileSystem> fs,
                             std::string file_path,
                             double sleep_time,
                             sg_offset_t offset,
                             sg_size_t num_bytes) :
            fs_(std::move(fs)),
            file_path_(std::move(file_path)),
            sleep_time_(sleep_time),
            offset_(offset),
            num_bytes_(num_bytes)
    {}

    void operator()() {

        XBT_INFO("New FileWriter for file %s: sleep_time: %.2lf, offset=%llu, num_bytes=%llu", file_path_.c_str(), sleep_time_, offset_, num_bytes_);
        sg4::this_actor::sleep_for(sleep_time_);
        XBT_INFO("Opening the file...");
        auto file = fs_->open(file_path_);
        XBT_INFO("Seek to offset %llu...", offset_);
        file->seek(offset_);
        XBT_INFO("Writing %llu to it at offset %llu...", num_bytes_, offset_);
        file->write(num_bytes_);
        XBT_INFO("Closing file...");
        file->close();
        XBT_INFO("The file size it: %llu", fs_->file_size(file_path_));
    }
};



int main(int argc, char **argv) {

    sg4::Engine engine(&argc, argv);

    XBT_INFO("Creating a platform with one host and one disk...");
    auto *my_zone = sg4::create_full_zone("zone");
    auto my_host = my_zone->create_host("my_host", "100Gf");
    auto my_disk = my_host->create_disk("my_disk", "1kBps", "2kBps");
    my_zone->seal();

    XBT_INFO("Creating a one-disk storage on the host's disk...");
    auto ods = sgfs::OneDiskStorage::create("my_storage", my_disk);
    XBT_INFO("Creating a file system...");
    auto fs = sgfs::FileSystem::create("my_fs");
    XBT_INFO("Mounting a 100kB partition...");
    fs->mount_partition("/dev/bogus/", ods, "100kB");


    std::string file_path = "/dev//bogus/../bogus/file.txt";
    XBT_INFO("Creating a 10kB file...");
    fs->create_file(file_path, "10kB");
    XBT_INFO("The file size it: %llu", fs->file_size(file_path));
    auto partition = fs->partition_by_name("/dev/bogus/");
    XBT_INFO("%llu bytes free on %s", partition->get_free_space(), partition->get_cname());


    XBT_INFO("Creating file writer actors...");
    sg4::Actor::create("MyActor1", my_host, FileWriterActor(fs, file_path, 10, 5*1000, 6*1000));
    sg4::Actor::create("MyActor2", my_host, FileWriterActor(fs, file_path, 10.5, 5*1000, 8*1000));

    XBT_INFO("Launching the simulation...");
    engine.run();

    XBT_INFO("%llu bytes free on %s", partition->get_free_space(), partition->get_cname());
    XBT_INFO("Unlink %s", file_path.c_str());
    fs->unlink_file(file_path);
    XBT_INFO("%llu bytes free on %s", partition->get_free_space(), partition->get_cname());
}
