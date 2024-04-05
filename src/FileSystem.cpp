#include "FileSystem.hpp"
#include "File.hpp"
#include "PathUtil.hpp"
#include <algorithm>

namespace simgrid::module::fs {

    /**
     * @brief Open a file
     * @param fullpath: an absolute file path
     * @return
     */
    std::shared_ptr<File> FileSystem::open(const std::string &fullpath) {
        // Get a file descriptor
        if (this->num_open_files_ >= this->max_num_open_files_) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }

        // Compute the simplified path
        auto simplified_path = PathUtil::simplify_path_string(fullpath);

        // Identify the mount point and partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                                [simplified_path](const std::pair<std::string, std::shared_ptr<Partition>>& element) {
                                    return PathUtil::is_at_mount_point(simplified_path, element.first);
                                });
        if (it == this->partitions_.end()) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }

        auto mount_point = it->first;
        auto partition = it->second.get();

        // Get the path at the mount point
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, mount_point);

        // Get the FileMetadata raw pointer
        auto file_metadata = partition->get_content().at(path_at_mount_point).get();

        // Create the file object
        auto file = File::createInstance(simplified_path, file_metadata, partition);

        this->num_open_files_++;
        return file;
    }

};