/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_PARTITION_LRU_CACHING_H_
#define SIMGRID_MODULE_FS_PARTITION_LRU_CACHING_H_

#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include "fsmod/PartitionFIFOCaching.hpp"
#include "fsmod/FileMetadata.hpp"

namespace simgrid::fsmod {

    /** \cond EXCLUDE_FROM_DOCUMENTATION    */

    class Storage;

    class XBT_PUBLIC PartitionLRUCaching : public PartitionFIFOCaching {
    public:
        PartitionLRUCaching(std::string name, FileSystem *file_system, std::shared_ptr<Storage> storage, sg_size_t size) :
                PartitionFIFOCaching(std::move(name), file_system, std::move(storage), size) {}

    protected:
        // Methods to perform caching
        void new_file_access_event(FileMetadata *file_metadata) override;
    };

    /** \endcond     */

} // namespace simgrid::fsmod

#endif
