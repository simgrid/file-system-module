/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <iostream>
#include <utility>

#include "fsmod.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fs_test, "File System Test");

namespace sg4 = simgrid::s4u;
namespace sgfs = simgrid::fsmod;

class FileWriterActor {
    std::shared_ptr<sgfs::FileSystem> fs_;
    std::string file_path_;
    sg_size_t num_bytes_;

public:
    explicit FileWriterActor(std::shared_ptr<sgfs::FileSystem> fs,
                             std::string file_path,
                            sg_size_t num_bytes) :
            fs_(std::move(fs)),
            file_path_(std::move(file_path)),
            num_bytes_(num_bytes)
    {}

    void operator()() {
        XBT_INFO("Opening the file...");
        auto file = fs_->open(file_path_, "w");
        XBT_INFO("Writing %llu to it...", num_bytes_);
        file->write(num_bytes_);
        XBT_INFO("Closing file...");
        fs_->close(file);
        XBT_INFO("The file size it: %llu", fs_->file_size(file_path_));
    }
};

int main(int argc, char **argv) {
    sg4::Engine engine(&argc, argv);

    XBT_INFO("Creating a platform with two hosts, one having a bunch of disks...");
    auto *my_zone = sg4::create_full_zone("zone");
    auto fs_client = my_zone->create_host("fs_client", "100Gf");
    auto fs_server = my_zone->create_host("fs_server", "100Gf");
    std::vector<sg4::Disk*> my_disks;
    for (int i = 0 ; i < 4 ; i++ )
       my_disks.push_back(fs_server->create_disk("my_disk" + std::to_string(i), "1kBps", "2kBps"));

    const auto* link = my_zone->create_link("link", "1Gbps")->set_latency(0);
    my_zone->add_route(fs_client, fs_server, {link});
    my_zone->seal();

    XBT_INFO("Creating a JBOD storage on the server host");
    auto jds = sgfs::JBODStorage::create("my_storage", my_disks, sgfs::JBODStorage::RAID::RAID5);

    XBT_INFO("Creating a file system...");
    auto fs = sgfs::FileSystem::create("my_fs");
    XBT_INFO("Mounting a 100kB partition...");
    fs->mount_partition("/dev/bogus/", jds, "100kB");

    std::string file_path = "/dev//bogus/../bogus/file.txt";
    XBT_INFO("Creating a 10kB file...");
    fs->create_file(file_path, "10kB");
    XBT_INFO("The file size it: %llu", fs->file_size(file_path));

    auto partition = fs->partition_by_name("/dev/bogus/");
    XBT_INFO("%llu bytes free on %s", partition->get_free_space(), partition->get_cname());

    XBT_INFO("Creating file writer actor...");
    sg4::Actor::create("MyActor", fs_client, FileWriterActor(fs, file_path, 6*1000));

    XBT_INFO("Launching the simulation...");
    engine.run();
}
