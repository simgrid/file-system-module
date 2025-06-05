/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "fsmod/OneDiskStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::fsmod {

    /**
     * @brief Retrieve the storage's name
     * @return a name string
     */
    const std::string& Storage::get_name() const {
        return name_;
    }

    /**
     * @brief Retrieve the storage's name
     * @return a C-style name string
     */
    const char* Storage::get_cname() const {
        return name_.c_str();
    }

    /**
     * @brief Retrieve the controller's host
     * @return The host on which the controller is running (or nullptr if no controller is running)
     */
    s4u::Host* Storage::get_controller_host() const {
        return controller_host_;
    }

    /**
     * @brief Retrieve the controller actor
     * @return The controller actor (or nullptr if no controller is running)
     */
    s4u::ActorPtr Storage::get_controller() const {
        return controller_;
    }

    /**
     * @brief Return the list of disks used by the storage
     * @return A list of disks
     */
    std::vector<s4u::Disk*> Storage::get_disks() const {
        return disks_;
    }

    /**
     * @brief Return the number of disks used by the storage
     * @return A number of disks
     */
    size_t Storage::get_num_disks() const {
        return disks_.size();
    }

    /**
     * @brief Return the first disk used by the storage
     * @return A disk
     */
    s4u::Disk* Storage::get_first_disk() const {
        return disks_.front();
    }

    /**
     * @brief Return a particular disk used by the storage
     * @param position: the index of the disk in the list of disks
     * @return A disk
     */
    s4u::Disk* Storage::get_disk_at(unsigned long position) const {
        return disks_.at(position);
    }

    /**
     * @brief Start a controller actor on a host
     * @param host: A host
     * @param func: A lambda that implements the controller actor's code
     * @return An actor
     */
    s4u::ActorPtr Storage::start_controller(s4u::Host* host, const std::function<void()> &func) {
        controller_host_ = host;
        controller_ = controller_host_->add_actor(name_+"_controller", func);
        return controller_;
    }

}