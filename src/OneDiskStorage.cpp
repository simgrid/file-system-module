/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "fsmod/OneDiskStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::fsmod {

    /**
     * @brief Method to create an instance of a one-disk storage
     * @param name: the storage's name
     * @param disk: the storage's disk
     * @return a one-dist storage instance
     */
    std::shared_ptr<OneDiskStorage> OneDiskStorage::create(const std::string& name, s4u::Disk* disk) {
        return std::make_shared<OneDiskStorage>(name, disk);
    }

    OneDiskStorage::OneDiskStorage(const std::string& name, s4u::Disk* disk) : Storage(name) {
        set_disk(disk);
        set_controller_host(disk->get_host());
        // Create a no-op controller
        set_controller(s4u::Actor::create(name+"_controller", get_controller_host(),
                                          [this]() { get_message_queue()->get<void*>(); })->daemonize());
    }

    s4u::IoPtr OneDiskStorage::read_async(sg_size_t size) {
        return get_first_disk()->read_async(size);
    }

    void OneDiskStorage::read(sg_size_t size) {
        get_first_disk()->read(size);
    }

    s4u::IoPtr OneDiskStorage::write_async(sg_size_t size) {
        return get_first_disk()->write_async(size);
    }

    void OneDiskStorage::write(sg_size_t size) {
        get_first_disk()->write(size);
    }

}
