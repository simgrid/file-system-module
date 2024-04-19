#include <algorithm>
#include <memory>

#include <simgrid/s4u/Engine.hpp>

#include "FileSystem.hpp"
#include "File.hpp"
#include "PathUtil.hpp"
#include "Partition.hpp"
#include "FileSystemException.h"

namespace simgrid::module::fs {
    /**
     * @brief Private method to find the partition and path at mount point for an absolute path
     * @param full_path: an absolute simplified file path
     * @return
     */
    std::pair<std::shared_ptr<Partition>, std::string>
    FileSystem::find_path_at_mount_point(const std::string &simplified_path) const {

        // Identify the mount point and path at mount point partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                               [simplified_path](const std::pair<std::string, std::shared_ptr<Partition>> &element) {
                                   return PathUtil::is_at_mount_point(simplified_path, element.first);
                               });
        if (it == this->partitions_.end()) {
            throw FileSystemException(XBT_THROW_POINT, "No partition found for path " + simplified_path + " at file system " + name_);
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;
        return std::make_pair(partition, path_at_mount_point);
    }

    /*********************** PUBLIC INTERFACE *****************************/

    std::shared_ptr<FileSystem> FileSystem::create(const std::string &name, int max_num_open_files) {
        return std::shared_ptr<FileSystem>(new FileSystem(name, max_num_open_files));
    }

    /**
     * @brief A method to add a partition to the file system
     * @param mount_point: the partition's mount point
     * @param partition: the partition
     */
    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage,
                                     const std::string &size) {
        mount_partition(mount_point, storage, static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")));
    }

    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, sg_size_t size) {

        if (PathUtil::simplify_path_string(mount_point) != mount_point or not PathUtil::is_absolute(mount_point)) {
            throw std::invalid_argument(
                    "simgrid::module::fs::FileSystem::add_partition(): Mount point should be a simple absolute path");
        }
        for (auto const &mp: this->partitions_) {
            if ((mp.first.rfind(mount_point, 0) == 0) or (mount_point.rfind(mp.first, 0) == 0)) {
                throw std::invalid_argument(
                        "simgrid::module::fs::FileSystem::add_partition(): New mount point '" + mount_point +
                        "' would conflict with existing mount point '" + mp.first +
                        "') - mount points cannot be prefixes of each other.");
            }
        }
        this->partitions_[mount_point] = std::shared_ptr<Partition>(new Partition(mount_point, storage, size));
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name(const std::string &name) const {
        auto partition = partition_by_name_or_null(name);
        if (not partition)
            throw std::invalid_argument("Partition not found: '" + name + "'");
        return partition;
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name_or_null(const std::string &name) const {

        auto partition = partitions_.find(name);
        if (partition != partitions_.end())
            return partition->second;
        else
            return nullptr;
    }

    /**
     * @brief Create a new file on the file system in zero time
     * @param full_path: the absolute path to the file
     * @param size: the file size
     */
    void FileSystem::create_file(const std::string &full_path, const std::string &size) {
        create_file(full_path, static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")));
    }

    void FileSystem::create_file(const std::string &full_path, sg_size_t size) {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that the file doesn't already exist
        if (partition->get_content().find(path_at_mount_point) != partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE ALREADY EXISTS"); // TODO
        }

        // Check that there is enough space
        if (partition->get_free_space() < size) {
            throw std::runtime_error("EXCEPTION: NOT ENOUGH SPACE"); // TODO
        }

        // Create FileMetaData
        auto metadata = std::make_unique<FileMetadata>(size);

        // Add the file to the content
        partition->get_content()[path_at_mount_point] = std::move(metadata);

        // Decrease free space on partition
        partition->decrease_free_space(size);
    }

    /**
      * @brief Open a file
      * @param full_path: an absolute file path
      * @return
      */
    std::shared_ptr<File> FileSystem::open(const std::string &full_path) {
        // Get a file descriptor
        if (this->num_open_files_ >= this->max_num_open_files_) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }

        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that file is there
        if (partition->get_content().find(path_at_mount_point) == partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE NOT FOUND"); // TODO
        }

        // Get the FileMetadata raw pointer
        auto metadata = partition->get_content().at(path_at_mount_point).get();
        metadata->increase_file_refcount();

        // Create the file object
        auto file = std::shared_ptr<File>(new File(simplified_path, metadata, partition.get()));

        this->num_open_files_++;
        return file;
    }

    /**
     * @brief
     * @param full_path: an absolute file path
     * @return the file size in bytes
     */
    sg_size_t FileSystem::file_size(const std::string &full_path) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that the file exist
        if (partition->get_content().find(path_at_mount_point) == partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }

        return partition->get_content().at(path_at_mount_point)->get_current_size();
    }

    void FileSystem::unlink_file(const std::string &full_path) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        FileMetadata *metadata_ptr = nullptr;
        try {
            metadata_ptr = partition->get_content().at(path_at_mount_point).get();
        } catch (std::out_of_range &e) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }

        if (metadata_ptr->get_file_refcount() > 0) {
            throw std::runtime_error("EXCEPTION: CANNOT UNLINK A FILE THAT IS OPEN"); // TODO
        }
        // Free space on partition and remove from content
        partition->increase_free_space(metadata_ptr->get_current_size());
        partition->get_content().erase(path_at_mount_point);
    }

    /**
     * @brief Move a file
     * @param src_full_path: the source path
     * @param dst_full_path: the destination path
     */
    void FileSystem::move_file(const std::string &src_full_path, const std::string &dst_full_path) const {

        std::string simplified_src_path = PathUtil::simplify_path_string(src_full_path);
        auto [src_partition, src_path_at_mount_point] = this->find_path_at_mount_point(simplified_src_path);

        std::string simplified_dst_path = PathUtil::simplify_path_string(dst_full_path);
        auto [dst_partition, dst_path_at_mount_point] = this->find_path_at_mount_point(simplified_dst_path);

        // No mv across partitions (just like in the real world)
        if (src_partition != dst_partition) {
            throw std::runtime_error("EXCEPTION: CANNOT DO A MOVE OPERATION ACROSS PARTITIONS"); // TODO
        }
        auto partition = src_partition;

        // Get the src metadata, which must exit
        FileMetadata *src_metadata;
        try {
            src_metadata = partition->get_content().at(src_path_at_mount_point).get();
        } catch (std::out_of_range &e) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }

        // Get the dst metadata, if any
        FileMetadata *dst_metadata = nullptr;
        try {
            dst_metadata = partition->get_content().at(dst_path_at_mount_point).get();
        } catch (std::out_of_range &ignore) {}

        // No-op mv?
        if (src_metadata == dst_metadata) {
            return; // just like in the real world
        }

        // Sanity checks
        if (src_metadata->get_file_refcount() > 0) {
            throw std::runtime_error("EXCEPTION: CANNOT MOV A FILE THAT IS OPEN"); // TODO
        }
        if (dst_metadata and dst_metadata->get_file_refcount()) {
            throw std::runtime_error("EXCEPTION: CANNOT MOV A FILE TO A DESTINATION FILE THAT IS OPEN"); // TODO
        }

        // Update free space if needed
        if (dst_metadata) {
            auto src_size = src_metadata->get_current_size();
            auto dst_size = dst_metadata->get_current_size();
            if (dst_size > src_size) {
                partition->increase_free_space(dst_size - src_size);
            } else {
                if (src_size - dst_size <= partition->get_free_space()) {
                    partition->decrease_free_space(src_size - dst_size);
                } else {
                    throw std::runtime_error("EXCEPTION: NOT ENOUGH SPACE"); // TODO
                }
            }
        }

        // Do the move (reusing the original unique ptr, just in case)
        std::unique_ptr<FileMetadata> uniq_ptr = std::move(partition->get_content().at(src_path_at_mount_point));
        partition->get_content().erase(src_path_at_mount_point);
        partition->get_content()[dst_path_at_mount_point] = std::move(uniq_ptr);

    }

}