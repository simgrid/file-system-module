/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>

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
    // Create the static field that the extension mechanism needs
    xbt::Extension<kernel::routing::NetZoneImpl, FileSystemNetZoneImplExtension>
        FileSystemNetZoneImplExtension::EXTENSION_ID;

    void FileSystemNetZoneImplExtension::register_file_system(const std::shared_ptr<FileSystem>& fs) {
        file_systems_[fs->get_name()] = fs;
    }

    /**
     * @brief Private method to find the partition and path at mount point for an absolute path
     * @param simplified_path: an absolute simplified file path
     * @return A pair that consists of a Partition and the path at the partition's mount point
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
            throw InvalidPathException(XBT_THROW_POINT, "No path prefix matches a partition's mount point (" + simplified_path + ")");
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;
        return std::make_pair(partition, path_at_mount_point);
    }

    /*********************** PUBLIC INTERFACE *****************************/

    /**
     * @brief Method to create a FileSystem instance
     * @param name: the file system's name (can be any string)
     * @param max_num_open_files: the file system's bound on the number of simultaneous opened files
     * @return A shared pointer to a FileSystem instance
     */
    std::shared_ptr<FileSystem> FileSystem::create(const std::string &name, int max_num_open_files) {
        return std::make_shared<FileSystem>(name, max_num_open_files);
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
     * @param mount_point: the partition's mount point (e.g., "/dev/a/")
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
        for (auto const &[mp, p]: this->partitions_) {
            if ((mp.rfind(cleanup_mount_point, 0) == 0) || (cleanup_mount_point.rfind(mp, 0) == 0)) {
                throw std::invalid_argument("Mount point already exists or is prefix of existing mount point");
            }
        }

        switch (caching_scheme) {
            case Partition::CachingScheme::FIFO:
                this->partitions_[cleanup_mount_point] =
                        std::make_shared<PartitionFIFOCaching>(cleanup_mount_point, this, std::move(storage), size);
                break;
            case Partition::CachingScheme::LRU:
                this->partitions_[cleanup_mount_point] =
                        std::make_shared<PartitionLRUCaching>(cleanup_mount_point, this, std::move(storage), size);
                break;
            default: // actually Partition::CachingScheme::NONE
                this->partitions_[cleanup_mount_point] =
                        std::make_shared<Partition>(cleanup_mount_point, this, std::move(storage), size);
                break;
        }
    }

   /**
    * @brief Register a file system in the NetZone it belongs.
    *
    * @param netzone The SimGrid NetZone
    * @param fs The FSMOD file system
    */
    void FileSystem::register_file_system(const s4u::NetZone* netzone, std::shared_ptr<FileSystem> fs) {
        if (not FileSystemNetZoneImplExtension::EXTENSION_ID.valid()) {
            // This is the first time we register a FileSystem, register the NetZoneImpls extension properly
            FileSystemNetZoneImplExtension::EXTENSION_ID =
                kernel::routing::NetZoneImpl::extension_create<FileSystemNetZoneImplExtension>();
        }
        auto pimpl = netzone->get_impl();
        // If this NetZoneImpl doesn't have an extension yet, create one
        if (not pimpl->extension<FileSystemNetZoneImplExtension>())
           pimpl->extension_set(new FileSystemNetZoneImplExtension());

        pimpl->extension<FileSystemNetZoneImplExtension>()->register_file_system(fs);
    }

    /**
     * @brief Get all the file systems an actor has access to.
     *
     * This corresponds to the file systems in the NetZone wherein the Host on which the Actor runs is.
     *
     * @param actor The actor asking for all the file systems it can access
     * @return A file system map, using names as keys
     */
    const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>&
    FileSystem::get_file_systems_by_actor(s4u::ActorPtr actor) {
        const kernel::routing::NetZoneImpl* netzone_impl;
        if (not actor || simgrid::s4u::Actor::is_maestro()) {
            netzone_impl = s4u::Engine::get_instance()->get_netzone_root()->get_impl();
        } else {
            netzone_impl = actor->get_host()->get_englobing_zone()->get_impl();
        }

        const auto* extension = netzone_impl->extension<FileSystemNetZoneImplExtension>();
        if (extension) {
             return extension->get_all_file_systems();
        } else {
            static const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>& empty = {};
            return empty;
        }
    }

    const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>&
    FileSystem::get_file_systems_by_netzone(const s4u::NetZone* netzone) {
        const auto* extension = netzone->get_impl()->extension<FileSystemNetZoneImplExtension>();
        if (extension) {
             return extension->get_all_file_systems();
        } else {
            static const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>& empty = {};
            return empty;
        }
    }

    /**
     * @brief Retrieve a partition by name (i.e., mount point), and throw an exception if no such partition exists
     * @param name: A name (i.e., mount point)
     * @return A Partition instance
     */
    std::shared_ptr<Partition> FileSystem::partition_by_name(const std::string &name) const {
        auto partition = partition_by_name_or_null(name);
        if (not partition)
            throw std::invalid_argument("Partition not found");
        return partition;
    }

    /**
     * @brief Retrieve a partition by name (i.e., mou t point)
     * @param name: A name (i.e., mount point)
     * @return A Partition instance or nullptr if no such partition exists
     */
    std::shared_ptr<Partition> FileSystem::partition_by_name_or_null(const std::string &name) const {
        auto partition = partitions_.find(PathUtil::simplify_path_string(name));
        if (partition != partitions_.end())
            return partition->second;
        else
            return nullptr;
    }

    /**
     * @brief Retrieve all partitions in the file systems
     * @return A list of Partition instances
     */
    std::vector<std::shared_ptr<Partition>> FileSystem::get_partitions() const {
        std::vector<std::shared_ptr<Partition>> to_return;
        to_return.reserve(this->partitions_.size());
        for (auto const & [mount_point, partition] : this->partitions_) {
            to_return.push_back(partition);
        }
        return to_return;
    }

    /**
     * @brief Retrieve the Partition that corresponds to an absolute full path
     * @param full_path: a full absolute path
     * @return A Partition instance or nullptr if the (invalid) path matches no known partition
     */
    std::shared_ptr<Partition> FileSystem::get_partition_for_path_or_null(const std::string& full_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        try {
            auto [partition, mount_point] = this->find_path_at_mount_point(simplified_path);
            return partition;
        } catch (InvalidPathException&) {
            return nullptr;
        }
    }


    /**
     * @brief Create a new file on the file system in zero time
     * @param full_path: the absolute path to the file
     * @param size: the file size
     */
    void FileSystem::create_file(const std::string &full_path, const std::string &size) const {
        create_file(full_path, static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")));
    }

    void FileSystem::create_file(const std::string &full_path, sg_size_t size) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that the path doesn't match an existing directory
        // TODO: This is weak, since if directory "a/b/c/d" exists, director "a/b" does not!
        // TODO: (we don't _really_ have directories, just prefixes before the file name)
        if (partition->directory_exists(path_at_mount_point)) {
            throw InvalidPathException(XBT_THROW_POINT, "Provided file path is that of an existing directory (" + path_at_mount_point + ")");
        }

        // Split the path
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        // Add the file to the content
        partition->create_new_file(dir, file_name, size);
    }


    /**
     * @brief Set the evictability of a file so that it can or cannot be evicted
     *        if stored on a partition that implements caching
     * @param full_path: an absolute file path
     * @param evictable: true if the file should be evictable, false if not
     */
    void FileSystem::make_file_evictable(const std::string& full_path, bool evictable) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Split the path
        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        partition->make_file_evictable(dir, file_name, evictable);
    }


    /**
      * @brief Open a file. If no file corresponds to the given full path, a new file of size 0 is created.
      * @param full_path: an absolute file path
      * @param access_mode: access mode ("r", "w", or "a")
      * @return
      */
    std::shared_ptr<File> FileSystem::open(const std::string &full_path, const std::string& access_mode) {
        // "Get a file descriptor"
        if (this->num_open_files_ >= this->max_num_open_files_) {
            throw TooManyOpenFilesException(XBT_THROW_POINT);
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
                throw FileNotFoundException(XBT_THROW_POINT, full_path);
            create_file(full_path, "0B");
            metadata = partition->get_file_metadata(dir, file_name);
        } else {
            if (access_mode == "w") {
                // Opening a file in "w" mode resets its size to 0. Update metadata and partition free space accordingly
                partition->increase_free_space(metadata->get_current_size());
                metadata->set_current_size(0);
                metadata->set_future_size(0);
            }
        }

        // Increase the refcount
        metadata->increase_file_refcount();

        // Create the file object
        auto file = std::make_shared<File>(simplified_path, access_mode, metadata, partition.get());

        if (access_mode == "a")
            file->current_position_ = metadata->get_current_size();

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

        auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);

        // Check that the file exist
        auto file_metadata = partition->get_file_metadata(dir, file_name);
        if (not file_metadata) {
            throw FileNotFoundException(XBT_THROW_POINT, full_path);
        }

        return file_metadata->get_current_size();
    }

    /**
     * @brief Unlink a file
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
            throw InvalidMoveException(XBT_THROW_POINT, "Cannot move file across partitions");
        }

        auto [src_dir, src_file_name] = PathUtil::split_path(src_path_at_mount_point);
        auto [dst_dir, dst_file_name] = PathUtil::split_path(dst_path_at_mount_point);

        auto partition = src_partition;
        partition->move_file(src_dir, src_file_name, dst_dir, dst_file_name);
    }

    /**
     * @brief Check that a file exists at a given path
     * @param full_path: the file path
     * @return true if the file exists, false otherwise
     */
    bool FileSystem::file_exists(const std::string& full_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        try {
            auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
            auto [dir, file_name] = PathUtil::split_path(path_at_mount_point);
            return (partition->get_file_metadata(dir, file_name) != nullptr);
        } catch (simgrid::Exception&) {
            return false;
        }
    }

    /**
     * @brief Create a directory
     * @param full_dir_path: the directory path
     */
    void FileSystem::create_directory(const std::string& full_dir_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_dir_path);
        // This check cannot be performed at the partition-level
        if (this->file_exists(simplified_path)) {
            throw InvalidPathException(XBT_THROW_POINT, "Path is that of an existing file: " + simplified_path);
        }
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        partition->create_new_directory(path_at_mount_point);
    }


    /**
     * @brief Check that a directory exists at a given path
     * @param full_path: the directory path
     * @return true if the directory exists, false otherwise
     */
    bool FileSystem::directory_exists(const std::string& full_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        return partition->directory_exists(path_at_mount_point);
    }

    /**
     * @brief Retrieve the list of names of files in a directory
     * @param full_dir_path: the path to the directory
     * @return
     */
    std::set<std::string, std::less<>> FileSystem::list_files_in_directory(const std::string &full_dir_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_dir_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        return partition->list_files_in_directory(path_at_mount_point);
    }

    /**
     * @brief Remove a directory and the files it contains
     * @param full_dir_path: the path to the directory
     * @return
     */
    void FileSystem::unlink_directory(const std::string &full_dir_path) const {
        std::string simplified_path = PathUtil::simplify_path_string(full_dir_path);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);
        partition->delete_directory(path_at_mount_point);
    }

}
