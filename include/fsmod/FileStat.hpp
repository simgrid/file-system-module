/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_FILESTAT_H_
#define SIMGRID_MODULE_FS_FILESTAT_H_

#include <unordered_map>
#include <simgrid/forward.h>
#include <iostream>

namespace simgrid::fsmod {

    /**
     * @brief A class that implements a file stat data structure
     */
    class XBT_PUBLIC FileStat {
        public:
            /** @brief The file's size in bytes **/
            sg_size_t size_in_bytes;
            /** @brief The file's last access date **/
            double last_access_date;
            /** @brief The file's last modification date **/
            double last_modification_date;
            /** @brief The number of times the file is currently opened **/
            unsigned int refcount;
    };

} // namespace

#endif