/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "fsmod/OneRemoteDiskStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::fsmod {

    /**
     * @brief Create an instance of a remote one-disk storage
     * @param name: the storage's name
     * @param disk: the storage's disk
     * @return a remote one-disk storage instance
     */
    std::shared_ptr<OneRemoteDiskStorage> OneRemoteDiskStorage::create(const std::string& name, s4u::Disk* disk) {
        return std::make_shared<OneRemoteDiskStorage>(name, disk);
    }

    OneRemoteDiskStorage::OneRemoteDiskStorage(const std::string& name, s4u::Disk* disk) : Storage(name) {
        set_disk(disk);
    }

    s4u::IoPtr OneRemoteDiskStorage::read_async(sg_size_t size) {
        auto source_host = get_controller_host();
        if (source_host == nullptr)
            source_host = this->get_first_disk()->get_host();
        return s4u::Io::streamto_async(source_host, get_first_disk(), s4u::Host::current(), nullptr, size);
    }

    void OneRemoteDiskStorage::read(sg_size_t size) {
        this->read_async(size)->wait();
    }

    s4u::IoPtr OneRemoteDiskStorage::write_async(sg_size_t size) {
       auto destination_host = get_controller_host();
       if (destination_host == nullptr)
           destination_host= this->get_first_disk()->get_host();
       return s4u::Io::streamto_async(s4u::Host::current(), nullptr, destination_host, get_first_disk(), size);
    }

    void OneRemoteDiskStorage::write(sg_size_t size) {
        this->write_async(size)->wait();
    }

}
