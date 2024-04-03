#ifndef FILE_SYSTEM_MODULE_PATH_H
#define FILE_SYSTEM_MODULE_PATH_H

#include <filesystem>
#include "../include/fsmod.hpp"

namespace simgrid::module::fs {

    /**
     * @brief A method to simplify a path string
     * @param path_string: an arbitrary path string
     * @return A new path string that has been sanitized
     * (i.e., remove redundant slashes, resolved ".."'s and "."'s)
     */
    std::string Path::simplify_path_string(const std::string &path_string) {
        return std::filesystem::path(path_string).lexically_normal();
    }

    /**
     * @brief A method to determine whether a path goes "up" (i.e., it starts with "../")
     * @param simplified_path: a SIMPLIFIED path
     * @return True if the path goes up, false otherwise
     */
    bool Path::goes_up(const std::string &simplified_path) {
        return simplified_path.rfind("..",0) == 0;
    }

    /**
     * @brief A method to identify the mount point for an absolute path
     * @param simplified_absolute_path: a SIMPLIFIED absolute path that DOES NOT GO UP
     * @param mount_points: a list of possible mount points
     * @return An iterator to the found mount point or to std::vector::end
     */
    std::vector<std::string>::const_iterator Path::find_mount_point(const std::string &simplified_absolute_path,
                                                                    const std::vector<std::string> &mount_points) {
        for (auto it = mount_points.begin(); it != mount_points.end(); it++) {
            if (simplified_absolute_path.rfind((*it), 0) == 0) {
                return it;
            }
        }
        return mount_points.end();
    }

    /**
     * @brief A method to find out whether an absolute path is at a given mount point
     * @param simplified_absolute_path: a SIMPLIFIED absolute path that DOES NOT GO UP
     * @param mount_point: a mount point
     * @return True if mount_point is a prefix of  simplified_absolute_path, false otherwise
     */
    bool Path::is_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point) {
        return simplified_absolute_path.rfind(mount_point, 0) == 0;
    }

    /**
     * @brief A method to extract the path at a mount point given an absolute path
     * @param simplified_absolute_path: a SIMPLIFIED absolute path that DOES NOT GO UP
     * @param mount_point: a mount point
     * @return The path at the mount point
     * @throws std::logic_error If the path is not at that mount point
     */
    std::string Path::path_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point) {
        if (simplified_absolute_path.rfind(mount_point, 0) != 0) {
            throw std::logic_error("Path '" + simplified_absolute_path + "' is not at mount point");
        } else {
            return simplified_absolute_path.substr(mount_point.length());
        }
    }

}

#endif //FILE_SYSTEM_MODULE_PATH_H
