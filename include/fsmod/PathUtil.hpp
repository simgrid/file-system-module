#ifndef SIMGRID_MODULE_FS_PATHUTIL_H_
#define SIMGRID_MODULE_FS_PATHUTIL_H_

#include <xbt.h>
/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.h>
#include <memory>
#include <vector>

#include "Partition.hpp"

namespace simgrid::fsmod {

    /** \cond EXCLUDE_FROM_DOCUMENTATION */

    class XBT_PUBLIC PathUtil {
    public:
        static std::string simplify_path_string(const std::string& path);
        static void remove_trailing_slashes(std::string &path);
        static std::pair<std::string, std::string> split_path(std::string_view path);
        static bool is_at_mount_point(std::string_view simplified_absolute_path, std::string_view mount_point);
        static std::string path_at_mount_point(const std::string& simplified_absolute_path, std::string_view mount_point);
    };

    /** \endcond */

} // namespace simgrid::fsmod

#endif