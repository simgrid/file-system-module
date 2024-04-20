#ifndef FILE_SYSTEM_MODULE_PATH_H
#define FILE_SYSTEM_MODULE_PATH_H

#include <filesystem>
#include "../include/PathUtil.hpp"

namespace simgrid::module::fs {

    /**
     * @brief A method to simplify a path string (which is either absolute or relative to /)
     * @param path_string: an arbitrary path string
     * @return A new path string that has been sanitized
     * (i.e., remove redundant slashes, resolved ".."'s and "."'s)
     */
    std::string PathUtil::simplify_path_string(const std::string &path_string) {
        // Prepend with "/" as that's always the working directory
        auto lexically_normal =  std::string(std::filesystem::path("/"+path_string).lexically_normal());
        PathUtil::remove_trailing_slashes(lexically_normal);
        return lexically_normal;
    }

    /**
     * @brief A method to remove extraneous trailing slashes
     * @param path_string: an arbitrary path string (which is either absolute or relative to /)
     * @return A new path string where trailing slashes have been removed
     */
    void PathUtil::remove_trailing_slashes(std::string &path) {
        while (path.size() > 1 && path.at(path.size()-1) == '/') {
            path.pop_back();
        }
    }

    /**
     * @brief A method to split a path (which is either absolute or relative to /) into a prefix (the "directory")
     *        and a suffix (the "file")
     * @param path_string: a simplified path string
     * @return a <prefix , suffix> pair, where either one of them could be empty
     */
    std::pair<std::string, std::string> PathUtil::split_path(std::string &path) {
        std::string dir;
        std::string file;
        auto last_slash = path.find_last_of('/');
        if (last_slash == std::string::npos) {
            dir = "";
            file = path;
        } else {
            dir = path.substr(0, last_slash);
            if (dir.empty()) dir = "/"; // ugly special case
            file = path.substr(last_slash + 1, std::string::npos);
        }
        return std::make_pair(dir, file);
    }

    /**
     * @brief A method to find out whether an absolute path is at a given mount point
     * @param simplified_absolute_path: a SIMPLIFIED absolute path that DOES NOT GO UP
     * @param mount_point: a mount point
     * @return True if mount_point is a prefix of  simplified_absolute_path, false otherwise
     */
    bool PathUtil::is_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point) {
        return simplified_absolute_path.rfind(mount_point, 0) == 0;
    }

    /**
     * @brief A method to extract the path at a mount point given an absolute path
     * @param simplified_absolute_path: a SIMPLIFIED absolute path that DOES NOT GO UP
     * @param mount_point: a mount point
     * @return The path at the mount point
     * @throws std::logic_error If the path is not at that mount point
     */
    std::string PathUtil::path_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point) {
        if (simplified_absolute_path.rfind(mount_point, 0) != 0) {
            throw std::logic_error("Path '" + simplified_absolute_path + "' is not at mount point");
        } else {
            return simplified_absolute_path.substr(mount_point.length());
        }
    }

}

#endif //FILE_SYSTEM_MODULE_PATH_H
