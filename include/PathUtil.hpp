#ifndef SIMGRID_MODULE_FS_PATHUTIL_H_
#define SIMGRID_MODULE_FS_PATHUTIL_H_

#include <xbt.h>
#include <xbt/config.h>
#include <memory>
#include <vector>

#include "Partition.hpp"

namespace simgrid::module::fs {

    class XBT_PUBLIC PathUtil {
    public:
        static std::string simplify_path_string(const std::string &path);
        static bool goes_up(const std::string &simplified_path);
        static std::vector<std::string>::const_iterator find_mount_point(const std::string &simplified_absolute_path,
                                                                         const std::vector<std::string> &mount_points);
        static bool is_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point);
        static std::string path_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point);

    };

} // namespace simgrid::module::fs

#endif