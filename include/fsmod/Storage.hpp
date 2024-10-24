/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/MessageQueue.hpp>

#include <atomic>
#include <boost/intrusive_ptr.hpp>
#include <utility>

#include "Partition.hpp"

namespace simgrid::fsmod {

    /**
     * @brief A class that implements a storage abstraction
     */
    class XBT_PUBLIC Storage {

    public:
        [[nodiscard]] const std::string& get_name() const;
        [[nodiscard]] const char* get_cname() const;
        [[nodiscard]] s4u::Host* get_controller_host() const;
        [[nodiscard]] s4u::ActorPtr get_controller() const;
        [[nodiscard]] std::vector<s4u::Disk*> get_disks() const;
        [[nodiscard]] size_t get_num_disks() const;
        [[nodiscard]] s4u::Disk* get_first_disk() const;
        [[nodiscard]] s4u::Disk* get_disk_at(unsigned long position) const;
        [[nodiscard]] virtual s4u::ActorPtr start_controller(s4u::Host* host, const std::function<void()> &func);

    protected:
        explicit Storage(std::string name): name_(std::move(name)) {}
        virtual ~Storage() = default;

        void set_disk(s4u::Disk* disk) { disks_.push_back(disk); }
        void set_disks(const std::vector<s4u::Disk*>& disks) { disks_ = disks; }

        friend class File;
        virtual s4u::IoPtr read_async(sg_size_t size) = 0;
        virtual void read(sg_size_t size) = 0;

        virtual s4u::IoPtr write_async(sg_size_t size) = 0;
        virtual void write(sg_size_t size) = 0;

    private:
        std::string name_;
        std::vector<s4u::Disk*> disks_;
        s4u::Host* controller_host_ = nullptr;
        s4u::ActorPtr controller_ = nullptr;
    };

} // namespace simgrid::fsmod

#endif
