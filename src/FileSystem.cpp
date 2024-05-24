#include <algorithm>
#include <memory>

#include <utility>

#include "fsmod/FileSystem.hpp"
#include "fsmod/File.hpp"
#include "fsmod/PathUtil.hpp"
#include "fsmod/Partition.hpp"
#include "fsmod/PartitionFIFOCaching.hpp"
#include "fsmod/PartitionLRUCaching.hpp"
#include "fsmod/FileSystemException.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_filesystem, "File System module: File system management related logs");

namespace simgrid::fsmod {

    /**
     * @brief Private method to find the partition and path at mount point for an absolute path
     * @param simplified_path: an absolute simplified file path
     * @return
     */
    std::pair<std::shared_ptr<Partition>, std::string>
    FileSystem::find_path_at_mount_point(const std::string &simplified_path) const {
        // Identify the mount point and path at mount point partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                               [&simplified_path](const std::pair<std::string, std::shared_ptr<Partition>> &element) {
                                   return PathUtil::is_at_mount_point(simplified_path, element.first);
                               });
        if (it == this->partitions_.end()) {
            throw FileSystemException(XBT_THROW_POINT, "No partition found");
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;
        return std::make_pair(partition, path_at_mount_point);
    }

    /*********************** PUBLIC INTERFACE *****************************/

    std::shared_ptr<FileSystem> FileSystem::create(const std::string &name, int max_num_open_files) {
        return std::make_shared<FileSystem>(FileSystem(name, max_num_open_files));
    }

    /**
     * @brief A method to add a partition to the file system
     * @param mount_point: the partition's mount point
     * @param storage: the storage
     * @param size: the partition size as a unit string (e.g., "100MB")
     * @param caching_scheme: the caching scheme (default: Partition::CachingScheme::NONE)
     */
    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage,
                                     const std::string &size, Partition::CachingScheme caching_scheme) {
        mount_partition(mount_point, std::move(storage), static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")), caching_scheme);
    }

    /**
     * @brief A method to add a partition to the file system
     * @param mount_point: the partition's mount point
     * @param storage: the storage
     * @param size: the partition size in bytes
     * @param caching_scheme: the caching scheme (default: Partition::CachingScheme::NONE)
     */
    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, sg_size_t size,
                                     Partition::CachingScheme caching_scheme) {
        auto cleanup_mount_point = mount_point;
        PathUtil::remove_trailing_slashes(cleanup_mount_point);
        if (PathUtil::simplify_path_string(mount_point) != cleanup_mount_point) {
            throw std::invalid_argument("Invalid partition path");
        }
        for (auto const &mp: this->partitions_) {
            if ((mp.first.rfind(cleanup_mount_point, 0) == 0) or (cleanup_mount_point.rfind(mp.first, 0) == 0)) {
                throw std::invalid_argument("Mount point already exists or is prefix of existing mount point");
            }
        }

        switch (caching_scheme) {
            case Partition::CachingScheme::NONE:
                this->partitions_[cleanup_mount_point] =
                   std::make_shared<Partition>(cleanup_mount_point, std::move(storage), size);
                break;
            case Partition::CachingScheme::FIFO:
                this->partitions_[cleanup_mount_point] =
                   std::make_shared<PartitionFIFOCaching>(cleanup_mount_point, std::move(storage), size);
                break;
            case Partition::CachingScheme::LRU:
                this->partitions_[cleanup_mount_point] =
                   std::make_shared<PartitionLRUCaching>(cleanup_mount_point, std::move(storage), size);
                break;
        }
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name(const std::string &name) const {
        auto partition = partition_by_name_or_null(name);
        if (not partition)
            throw std::invalid_argument("Partition not found");
        return partition;
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name_or_null(const std::string &name) const {
        auto partition = partitions_.find(PathUtil::simplify_path_string(name));
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

        // Check that the path doesn't match an existing directory
        // TODO: This is weak, since if directory "a/b/c/d" exists, director "a/b" does not!
        // TODO: (we don't _really_ have directories, just prefixes before the file name)
        if (partition->directory_exists(path_at_mount_point)) {
            throw FileSystemException(XBT_THROW_POINT, "Provided file path is that of an existing directory");
        }

        // Split the path
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        // Add the file to the content
        partition->create_new_file(dir, file_name, size);
    }

    /**
      * @brief Open a file. If no file corresponds to the given fullpath, a new file of size 0 is created.
      * @param full_path: an absolute file path
      * @param access_mode: access mode ("r", "w", or "a")
      * @return
      */
    std::shared_ptr<File> FileSystem::open(const std::string &full_path, const std::string& access_mode) {
        // "Get a file descriptor"
        if (this->num_open_files_ >= this->max_num_open_files_) {
            throw FileSystemException(XBT_THROW_POINT, "Too many open file descriptors");
        }
        if (access_mode != "r" && access_mode != "w" && access_mode != "a") {
            throw std::invalid_argument("Invalid access mode. Authorized values are: 'r', 'w', or 'a'");
        }

        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Split the path
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        // Get the file metadata
        auto metadata = partition->get_file_metadata(dir, file_name);
        if (not metadata) {
            if (access_mode == "r")
                throw FileSystemException(XBT_THROW_POINT, "File not found. Cannot be opened in 'r' mode");
            create_file(full_path, "0B");
            metadata = partition->get_file_metadata(dir, file_name);
        } else {
            if (access_mode == "w") {
                //TODO update metadata to reset size to 0
            }
        }

        // Increase the refcount
        metadata->increase_file_refcount();

        // Create the file object
        auto file = std::shared_ptr<File>(new File(simplified_path, access_mode, metadata, partition.get()));

        XBT_INFO("%s %d", access_mode.c_str(), SEEK_END);
        if (access_mode == "a")
            file->current_position_ = metadata->get_current_size();

        this->num_open_files_++;
        return file;
    }

   /**
     * @brief Closes the file. After closing, using the file has undefined
     * behavior.
     * @param file a shared pointer on a File
     */
    void FileSystem::close(const std::shared_ptr<File> &file) {
        this->num_open_files_--;
        file->metadata_->decrease_file_refcount();
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

        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        // Check that the file exist
        auto file_metadata = partition->get_file_metadata(dir, file_name);
        if (not file_metadata) {
            throw FileSystemException(XBT_THROW_POINT, "File not found");
        }

        return file_metadata->get_current_size();
    }

    /**
     * @brief Unlike a file
     * @param full_path: the file path
     */
    void FileSystem::unlink_file(const std::string &full_path) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        partition->delete_file(dir, file_name);
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
            throw FileSystemException(XBT_THROW_POINT, "Cannot move file across partitions");
        }

        auto [src_dir, src_file_name] = PathUtil::split_path(src_path_at_mount_point);
        auto [dst_dir, dst_file_name] = PathUtil::split_path(dst_path_at_mount_point);

        auto partition = src_partition;
        partition->move_file(src_dir, src_file_name, dst_dir, dst_file_name);
    }

    /**
     * @brief Method to check that a file exists at a given path
     * @param full_path: the file path
     * @return true if the file exists, false otherwise
     */
    bool FileSystem::file_exists(const std::string& full_path) {
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);
        return (partition->get_file_metadata(dir, file_name) != nullptr);
    }

    /**
     * @brief Method to check that a directory exists at a given path
     * @param full_path: the directory path
     * @return true if the directory exists, false otherwise
     */
    bool FileSystem::directory_exists(const std::string& full_path) {
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        return partition->directory_exists(path_at_mount_point);
    }

    /**
     * @brief Method to get the list of names of files in a directory
     * @param full_dir_path: the path to the directory
     * @return
     */
    std::set<std::string, std::less<>> FileSystem::list_files_in_directory(const std::string &full_dir_path) {
        std::string simplified_path = PathUtil::simplify_path_string(full_dir_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        return partition->list_files_in_directory(path_at_mount_point);
    }

    /**
     * @brief Method to remove a directory and the files it contains
     * @param full_dir_path: the path to the directory
     * @return
     */
    void FileSystem::unlink_directory(const std::string &full_dir_path) {
        std::string simplified_path = PathUtil::simplify_path_string(full_dir_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        partition->delete_directory(path_at_mount_point);
    }

}
