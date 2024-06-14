/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FILESYSTEM_H_
#define SIMGRID_MODULE_FILESYSTEM_H_

#include <simgrid/forward.h> // sg_size_t
#include <xbt.h>
#include <xbt/config.h>
#include <xbt/parse_units.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "Partition.hpp"
#include "File.hpp"

namespace simgrid::fsmod {

    class FileSystem;

    class FileSystemNetZoneImplExtension {
        std::map<std::string, std::shared_ptr<FileSystem>, std::less<>> file_systems_;
    public:
        static xbt::Extension<kernel::routing::NetZoneImpl, FileSystemNetZoneImplExtension> EXTENSION_ID;

        void register_file_system(const std::shared_ptr<FileSystem>& fs);
        const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>& get_all_file_systems() const { return file_systems_;}
    };

    /**
     * @brief A class that implements a file system abstraction
     */
    class XBT_PUBLIC FileSystem {
        const std::string name_;
        int max_num_open_files_;

    public:
        explicit FileSystem(std::string name, int max_num_open_files) : name_(std::move(name)), max_num_open_files_(max_num_open_files) {};


        static std::shared_ptr<FileSystem> create(const std::string &name, int max_num_open_files = 1024);
        static const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>&
            get_file_systems_by_actor(s4u::ActorPtr actor);
        static const std::map<std::string, std::shared_ptr<FileSystem>, std::less<>>&
            get_file_systems_by_netzone(const s4u::NetZone* netzone);
        static void register_file_system(const s4u::NetZone* netzone, std::shared_ptr<FileSystem> fs);

        /**
         * @brief Retrieves the file system's name
         * @return a name
         */
        [[nodiscard]] const std::string& get_name() const { return name_; }
        /**
         * @brief Retrieves the file system's name as a C-style string
         * @return a name
         */
        [[nodiscard]] const char* get_cname() const { return name_.c_str(); }

        void mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, sg_size_t size,
                             Partition::CachingScheme caching_scheme  = Partition::CachingScheme::NONE);
        void mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, const std::string& size,
                             Partition::CachingScheme caching_scheme  = Partition::CachingScheme::NONE);

        void create_file(const std::string& full_path, sg_size_t size) const;
        void create_file(const std::string& full_path, const std::string& size) const;

        [[nodiscard]] bool file_exists(const std::string& full_path) const;

        void move_file(const std::string& src_full_path, const std::string& dst_full_path) const;
        void unlink_file(const std::string& full_path) const;

        void create_directory(const std::string& full_dir_path) const;
        [[nodiscard]] bool directory_exists(const std::string& full_dir_path) const;
        void unlink_directory(const std::string& full_dir_path) const;
        [[nodiscard]] std::set<std::string, std::less<>> list_files_in_directory(const std::string& full_dir_path) const;

        [[nodiscard]] sg_size_t file_size(const std::string& full_path) const;

        std::shared_ptr<File> open(const std::string& full_path, const std::string& access_mode);
        void close(const std::shared_ptr<File> &f);

        [[nodiscard]] std::shared_ptr<Partition> partition_by_name(const std::string& name) const;
        [[nodiscard]] std::shared_ptr<Partition> partition_by_name_or_null(const std::string& name) const;

        [[nodiscard]] std::vector<std::shared_ptr<Partition>> get_partitions() const;
        [[nodiscard]] std::shared_ptr<Partition> get_partition_for_path_or_null(const std::string& full_path) const;

    private:
        [[nodiscard]] std::pair<std::shared_ptr<Partition>, std::string> find_path_at_mount_point(const std::string &full_path) const;

        std::map<std::string, std::shared_ptr<Partition>, std::less<>> partitions_;

        int num_open_files_ = 0;
    };
} // namespace simgrid::fsmod

#endif
