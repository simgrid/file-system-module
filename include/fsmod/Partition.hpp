/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_PARTITION_H_
#define SIMGRID_MODULE_FS_PARTITION_H_

#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include "fsmod/FileMetadata.hpp"

namespace simgrid::fsmod {

    class Storage;

    class XBT_PUBLIC Partition {
    public:
        /**
         * @brief An enum that defines the possible caching schemes that can
         *        be used by a partition
         */
        enum class CachingScheme {
            /** @brief No caching behavior. When there is not sufficient space an exception is thrown */
            NONE = 0,
            /** @brief FIFO caching behavior. When there is not sufficient space, an attempt is made to evict
             * non-opened files in First-In-First-Out fashion (based on file creation timestamps)
             * to create space if possible, otherwise an exception is thrown
             */
            FIFO = 1,
            /** @brief LRU caching behavior. When there is not sufficient space, an attempt is made to evict
             * non-opened files in Least-Recently-Used fashion (based on latest file creation/read/write timestamp)
             * to create space if possible, otherwise an exception is thrown
             */
            LRU = 2
        };

        Partition(std::string name, std::shared_ptr<Storage> storage, sg_size_t size);
        virtual ~Partition() = default;

        /**
         * @brief Retrieves the partition's name
         * @return a name
         */
        [[nodiscard]] const std::string& get_name() const { return name_; }
        /**
         * @brief Retrieves the partition's name as a C-style string
         * @return a name
         */
        [[nodiscard]] const char* get_cname() const { return name_.c_str(); }
        /**
         * @brief Retrieves the partition's size in bytes
         * @return a number of bytes
         */
        [[nodiscard]] sg_size_t get_size() const { return size_; }

        /**
         * @brief Retrieves the partition's free space in bytes
         * @return a number of bytes
         */
        [[nodiscard]] sg_size_t get_free_space() const { return free_space_; }

        /**
         * @brief Retrieves the number of files stored in the partition
         * @return a number of files
         */
        [[nodiscard]] sg_size_t get_num_files() const {
            sg_size_t to_return = 0;
            for (auto const &[dir_path, files] : content_) {
                to_return += files.size();
            }
            return to_return;
        }

    protected:
        // Methods to perform caching
        virtual void create_space(sg_size_t num_bytes);
        virtual void new_file_creation_event(FileMetadata *file_metadata);
        virtual void new_file_access_event(FileMetadata *file_metadata);
        virtual void new_file_deletion_event(FileMetadata *file_metadata);

    private:
        friend class File;
        friend class FileMetadata;
        friend class FileSystem;

        std::string name_;
        std::shared_ptr<Storage> storage_;
        sg_size_t size_ = 0;
        sg_size_t free_space_ = 0;
        std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<FileMetadata>>> content_;

        void decrease_free_space(sg_size_t num_bytes) { free_space_ -= num_bytes; }
        void increase_free_space(sg_size_t num_bytes) { free_space_ += num_bytes; }

        [[nodiscard]] std::shared_ptr<Storage> get_storage() const { return storage_; }

        void create_new_directory(const std::string& dir_path);
        [[nodiscard]] bool directory_exists(const std::string& dir_path) const { return content_.find(dir_path) != content_.end(); }
        std::set<std::string, std::less<>> list_files_in_directory(const std::string &dir_path) const;
        void delete_directory(const std::string &dir_path);

        [[nodiscard]] FileMetadata* get_file_metadata(const std::string& dir_path, const std::string& file_name) const;
        void create_new_file(const std::string& dir_path,
                             const std::string& file_name,
                             sg_size_t size);
        void move_file(const std::string& src_dir_path, const std::string& src_file_name,
                       const std::string& dst_dir_path, const std::string& dst_file_name);

    protected:
        void delete_file(const std::string& dir_path, const std::string& file_name);
    };
} // namespace simgrid::fsmod

#endif
