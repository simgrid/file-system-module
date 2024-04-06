#include "FileSystem.hpp"
#include "File.hpp"
#include "PathUtil.hpp"
#include "Partition.hpp"
#include <algorithm>
#include <iostream>
#include <memory>

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
            throw std::runtime_error("EXCEPTION: WRONG MOUNT POINT"); // TODO
        }

        auto mount_point = it->first;
        auto partition = it->second.get();

        // Get the path at the mount point
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, mount_point);

        // Check that file is there
        if (partition->get_content().find(path_at_mount_point) == partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE NOT FOUND"); // TODO
        }

        // Get the FileMetadata raw pointer
        auto metadata = partition->get_content().at(path_at_mount_point).get();

        // Create the file object
        auto file =  std::shared_ptr<File>(new File(fullpath, metadata, partition));

        this->num_open_files_++;
        return file;
    }

    /**
     * @brief A method to add a partition to the file system
     * @param mount_point: the partition's mount point
     * @param partition: the partition
     */
    void FileSystem::mount_partition(const std::string &mount_point, sg_size_t size) {
        if (PathUtil::simplify_path_string(mount_point) != mount_point or not PathUtil::is_absolute(mount_point)) {
            throw std::invalid_argument("simgrid::module::fs::FileSystem::add_partition(): Mount point should be a simple absolute path");
        }
        for (auto const &mp : this->partitions_) {
            if ((mp.first.rfind(mount_point, 0) == 0) or (mount_point.rfind(mp.first, 0) == 0)) {
                throw std::invalid_argument("simgrid::module::fs::FileSystem::add_partition(): New mount point '" + mount_point +
                                            "' would conflict with existing mount point '" + mp.first +
                                            "') - mount points cannot be prefixes of each other.");
            }
        }
        this->partitions_[mount_point] = std::shared_ptr<Partition>(new Partition(mount_point, size));
    }


    /**
     * @brief Create a new file on the file system in zero time
     * @param fullpath: the absolute path to the file
     * @param size: the file size
     */
    void FileSystem::create(const std::string& fullpath, sg_size_t size) {
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);

        // Identify the mount point and path at mount point partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                               [simplified_path](const std::pair<std::string, std::shared_ptr<Partition>>& element) {
                                   return PathUtil::is_at_mount_point(simplified_path, element.first);
                               });
        if (it == this->partitions_.end()) {
            throw std::runtime_error("EXCEPTION: WRONG MOUNT POINT"); // TODO
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;

        // Check that the file doesn't already exist
        if (it->second->get_content().find(path_at_mount_point) != it->second->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE ALREADY EXISTS"); // TODO
        }

        // Create FileMetaData
        auto metadata = std::make_unique<FileMetadata>(size);

        // Add the file to the content
        partition->get_content()[path_at_mount_point] = std::move(metadata);
    }

    /**
     * @brief
     * @param fullpath: an absolute file path
     * @return the file size in bytes
     */
    sg_size_t FileSystem::size(const std::string& fullpath) const {
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);

        // Identify the mount point and path at mount point partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                               [simplified_path](const std::pair<std::string, std::shared_ptr<Partition>>& element) {
                                   return PathUtil::is_at_mount_point(simplified_path, element.first);
                               });
        if (it == this->partitions_.end()) {
            throw std::runtime_error("EXCEPTION: WRONG MOUNT POINT"); // TODO
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;

        // Check that the file exist
        if (it->second->get_content().find(path_at_mount_point) == it->second->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }

        return it->second->get_content().at(path_at_mount_point)->get_current_size();
    }


}