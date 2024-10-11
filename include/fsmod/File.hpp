/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_FS_FILE_H_
#define SIMGRID_MODULE_FS_FILE_H_

#include <simgrid/forward.h>
#include <simgrid/s4u/Io.hpp>
#include <utility>
#include <xbt/parse_units.hpp>

#include "FileMetadata.hpp"
#include "FileStat.hpp"
#include "Partition.hpp"

namespace simgrid::fsmod {

    /**
     * @brief A class that implemented a file abstraction
     */
    class XBT_PUBLIC File {
        friend class FileSystem;

        std::string path_;
        std::string access_mode_;
        sg_size_t current_position_ = SEEK_SET;
        int desc_id     = 0;
        FileMetadata* metadata_;
        Partition* partition_;

        void update_current_position(sg_offset_t pos);
        int write_init_checks(sg_size_t num_bytes);

    public:
        File(std::string full_path, std::string access_mode, FileMetadata *metadata,
             Partition *partition)
            : path_(std::move(full_path)),
              access_mode_(std::move(access_mode)),
              metadata_(metadata),
              partition_(partition) {};
        File(const File&) = delete;
        File& operator=(const File&) = delete;
        virtual ~File() = default;

        /** Get the number of bytes actually read by a given I/O Read activity */
        sg_size_t get_num_bytes_read(const s4u::IoPtr& read) const { return read->get_performed_ioops(); }

        /** Get the number of bytes actually written by a given I/O Write activity */
        sg_size_t get_num_bytes_written(const s4u::IoPtr& write) const { return write->get_performed_ioops(); }
        const std::string& get_access_mode() const { return access_mode_; }
        const std::string& get_path() const { return path_; }

        s4u::IoPtr read_async(const std::string& num_bytes);
        s4u::IoPtr read_async(sg_size_t num_bytes);
        sg_size_t read(const std::string& num_bytes, bool simulate_it=true);
        sg_size_t read(sg_size_t num_bytes, bool simulate_it=true);
        s4u::IoPtr write_async(const std::string& num_bytes);
        s4u::IoPtr write_async(sg_size_t num_bytes);
        sg_size_t write(const std::string& num_bytes, bool simulate_it=true);
        sg_size_t write(sg_size_t num_bytes, bool simulate_it=true);

        void close() const;

        /**
         * @brief Retrieve the file system that holds this file
         * @return A pointer to a FileSystem
         */
        [[nodiscard]] FileSystem *get_file_system() { return partition_->file_system_; }

        void seek(sg_offset_t pos, int origin = SEEK_SET); /** Sets the file head to the given position from a given origin. */
        /** Retrieves the current file position */
        [[nodiscard]] sg_size_t tell() const { return current_position_; }

        [[nodiscard]] std::unique_ptr<FileStat> stat() const;
    };

} // namespace simgrid::fsmod

#endif
