#ifndef FSMOD_ONEREMOTEDISKSTORAGE_HPP
#define FSMOD_ONEREMOTEDISKSTORAGE_HPP

/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "Storage.hpp"

namespace simgrid::fsmod {

    /**
     * @brief A class that implements a one-disk storage
     */
    class XBT_PUBLIC OneRemoteDiskStorage : public Storage {
    public:
        OneRemoteDiskStorage(const std::string &name, simgrid::s4u::Disk *disk);
        ~OneRemoteDiskStorage() override = default;
        static std::shared_ptr<OneRemoteDiskStorage> create(const std::string &name, simgrid::s4u::Disk *disk);

    protected:
        s4u::IoPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::IoPtr write_async(sg_size_t size, bool detached = false) override;
        void write(sg_size_t size) override;
    };
} // namespace simgrid::fsmod

#endif //FSMOD_ONEREMOTEDISKSTORAGE_HPP
