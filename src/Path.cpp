
#ifndef FILE_SYSTEM_MODULE_PATH_H
#define FILE_SYSTEM_MODULE_PATH_H

#include <filesystem>
#include "../include/fsmod.hpp"

namespace simgrid {
    namespace module {
        namespace fs {

            /**
             * @brief A method to simplify a path string
             * @param path_string: an arbitrary path string
             * @return a new path string that has been sanitized
             * (i.e., remove redundant slashes, resolved ".."'s and "."'s)
             */
            std::string Path::simplify_path_string(const std::string &path_string) {
                return std::filesystem::path(path_string).lexically_normal();
            }

            bool Path::path_goes_up(const std::string &path) {
                // TODO
                return false;
            }

            sg_size_t Path::find_mount_point(const std::string &user_path,
                                                    const std::vector<std::string> &mount_points) {
                // TODO
                return 0;

            }

            bool Path::is_at_mount_point(const std::string &user_path, const std::string &mount_point) {
                // TODO
                return false;
            }

            std::string Path::path_at_mount_point(const std::string &user_path, const std::string &mount_point) {
                // TODO
                return "";
            }
        }
    }
}

#endif //FILE_SYSTEM_MODULE_PATH_H
