#ifndef FSMOD_ONEDISKSTORAGE_HPP
#define FSMOD_ONEDISKSTORAGE_HPP

/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "Storage.hpp"

namespace simgrid::fsmod {

    /**
     * @brief A class that implements a one-disk storage
     */
    class XBT_PUBLIC OneDiskStorage : public Storage {
    public:
        OneDiskStorage(const std::string &name, simgrid::s4u::Disk *disk);
        ~OneDiskStorage() override = default;
        static std::shared_ptr<OneDiskStorage> create(const std::string &name, simgrid::s4u::Disk *disk);

        s4u::IoPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::IoPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;
    };
} // namespace simgrid::fsmod

#endif //FSMOD_ONEDISKSTORAGE_HPP
