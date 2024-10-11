/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <iostream>

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/Exception.hpp>

#include "fsmod/File.hpp"
#include "fsmod/Storage.hpp"
#include "fsmod/FileSystem.hpp"
#include "fsmod/FileSystemException.hpp"
#include "fsmod/FileStat.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_file, "File System module: File management related logs");

namespace simgrid::fsmod {

    /**
     * @brief Asynchronously read data from the file
     * @param num_bytes: the number of bytes to read as a string with units
     * @return A smart pointer on the corresponding I/O activity
     */
     s4u::IoPtr File::read_async(const std::string& num_bytes) {
        return read_async(static_cast<sg_size_t>(xbt_parse_get_size("", 0, num_bytes, "")));
    }

    /**
     * @brief Asynchronously read data from the file
     * @param num_bytes: the number of bytes to read
     * @return A smart pointer on the corresponding I/O activity
     */
    s4u::IoPtr File::read_async(sg_size_t num_bytes) {
        if (access_mode_ != "r")
            throw std::invalid_argument("Invalid access mode '" + access_mode_ + "'. Cannot read in 'w' or 'a' mode'");
        // if the current position is close to the end of the file, we may not be able to read the requested size
        sg_size_t num_bytes_to_read = std::min(num_bytes, metadata_->get_current_size() - current_position_);
        // Update
        current_position_ += num_bytes_to_read;
        metadata_->set_access_date(s4u::Engine::get_clock());
        return boost::dynamic_pointer_cast<s4u::Io>(partition_->get_storage()->read_async(num_bytes_to_read));
    }

   /**
     * @brief Read data from the file
     * @param num_bytes: the number of bytes to read as a string with units
     * @param simulate_it: if true simulate the I/O, if false the I/O takes zero time
     * @return the actual number of bytes read in the file
     */
    sg_size_t File::read(const std::string& num_bytes, bool simulate_it) {
        return read(static_cast<sg_size_t>(xbt_parse_get_size("", 0, num_bytes, "")), simulate_it);
    }

    /**
     * @brief Read data from the file
     * @param num_bytes: the number of bytes to read
     * @param simulate_it: if true simulate the I/O, if false the I/O takes zero time
     * @return the actual number of bytes read in the file
     */
    sg_size_t File::read(sg_size_t num_bytes, bool simulate_it) {
        if (access_mode_ != "r")
            throw std::invalid_argument("Invalid access mode '" + this->access_mode_ + "'. Cannot read in 'w' or 'a' mode'");
        if (num_bytes == 0) /* Nothing to read, return */
            return 0;
        // if the current position is close to the end of the file, we may not be able to read the requested size
        sg_size_t num_bytes_to_read = std::min(num_bytes, metadata_->get_current_size() - current_position_);
        // Update
        current_position_ += num_bytes_to_read;
        metadata_->set_access_date(s4u::Engine::get_clock());

        // Do the I/O simulation if need be
        if (simulate_it) {
            try {
                partition_->get_storage()->read(num_bytes_to_read);
            } catch (StorageFailureException&) {
                throw xbt::UnimplementedError("Handling of hardware resource failures not implemented");
            }
        }
        return num_bytes_to_read;
    }

    int File::write_init_checks(sg_size_t num_bytes) {
        static int sequence_number = -1;
        int my_sequence_number;

        if (access_mode_ != "w" && access_mode_ != "a" && access_mode_ != "r+")
            throw std::invalid_argument("Invalid access mode. Cannot write in 'r' mode'");

        if (access_mode_ == "a" && current_position_ < metadata_->get_future_size())
            current_position_ = metadata_->get_future_size();

        //TODO: Would be good to move some of the code below to FileMetadata, but that requires
        //      that FileMetadata know the partition....

        // Check whether there is enough space
        sg_size_t added_bytes = 0;
        if (current_position_ + num_bytes > metadata_->get_future_size())
            added_bytes = current_position_ + num_bytes - metadata_->get_future_size();

        if (added_bytes > partition_->get_free_space()) {
            partition_->create_space(added_bytes - partition_->get_free_space());
        }

        // Compute the new tentative file size
        sg_size_t new_file_size_if_i_succeed = metadata_->get_future_size() + added_bytes;
        // Decrease the available space on partition of what is going to be added by that write
        partition_->decrease_free_space(added_bytes);

        // Update metadata
        my_sequence_number = ++sequence_number;
        metadata_->notify_write_start(my_sequence_number, new_file_size_if_i_succeed);

        return my_sequence_number;
    }

    /**
     * @brief Asynchronously write data from the file
     * @param num_bytes: the number of bytes to read as a string with units
     * @return A smart pointer on the corresponding I/O activity
     */
     s4u::IoPtr File::write_async(const std::string& num_bytes) {
        return write_async(static_cast<sg_size_t>(xbt_parse_get_size("", 0, num_bytes, "")));
    }

    /**
     * @brief Asynchronously write data from the file
     * @param num_bytes: the number of bytes to read
     * @return A smart pointer on the corresponding I/O activity
     */
    s4u::IoPtr File::write_async(sg_size_t num_bytes) {
        int my_sequence_number = write_init_checks(num_bytes);
        s4u::IoPtr io = boost::dynamic_pointer_cast<s4u::Io>(partition_->get_storage()->write_async(num_bytes));
        io->on_this_completion_cb([this, my_sequence_number](s4u::Io const&) {
            // Update
            metadata_->set_access_date(s4u::Engine::get_clock());
            metadata_->set_modification_date(s4u::Engine::get_clock());
            metadata_->notify_write_end(my_sequence_number);
        });
        return io;
    }

    /**
     * @brief Write data to the file
     * @param num_bytes: the number of bytes to write as a string with units
     * @param simulate_it: if true simulate the I/O, if false the I/O takes zero time
     */
    sg_size_t File::write(const std::string& num_bytes, bool simulate_it) {
        return write(static_cast<sg_size_t>(xbt_parse_get_size("", 0, num_bytes, "")), simulate_it);
    }

    /**
     * @brief Write data to the file
     * @param num_bytes: the number of bytes to write
     * @param simulate_it: if true simulate the I/O, if false the I/O takes zero time
     */
    sg_size_t File::write(sg_size_t num_bytes, bool simulate_it) {
        if (num_bytes == 0) /* Nothing to write, return */
            return 0;
        int my_sequence_number = write_init_checks(num_bytes);

        // Do the I/O simulation if need be
        if (simulate_it) {
            try {
                partition_->get_storage()->write(num_bytes);
            } catch (StorageFailureException&) {
                throw xbt::UnimplementedError("Handling of hardware resource failures not implemented");
            }
        }

        // Update
        metadata_->set_access_date(s4u::Engine::get_clock());
        metadata_->set_modification_date(s4u::Engine::get_clock());
        metadata_->notify_write_end(my_sequence_number);

        return num_bytes;
    }

    /**
     * @brief Change the file pointer position
     * @param pos: the position as an offset from the first byte of the file
     */
    void File::update_current_position(sg_offset_t pos) {
        if (pos < 0) {
            throw InvalidSeekException(XBT_THROW_POINT, "Cannot seek before the first byte");
        }
        current_position_ = pos;
    }

    /**
     * @brief Seek to a position in the file from a given origin
     * @param pos: the position as an offset from the given origin in the file
     * @param origin: where to start adding the offset in the file
     */
    void File::seek(sg_offset_t pos, int origin) {
        switch (origin) {
            case SEEK_SET:
                update_current_position(pos);
                break;
            case SEEK_CUR:
                update_current_position(current_position_ + pos);
                break;
            case SEEK_END:
                update_current_position(metadata_->get_current_size() + pos);
                break;
            default:
                update_current_position(origin + pos);
                break;
        }
    }

    /**
     * @brief Obtain information about the file as a
     * @return A (
     */
    std::unique_ptr<FileStat> File::stat() const {
        auto stat_struct = std::make_unique<FileStat>();
        stat_struct->size_in_bytes = metadata_->get_current_size();
        stat_struct->last_access_date = metadata_->get_access_date();
        stat_struct->last_modification_date = metadata_->get_modification_date();
        stat_struct->refcount = metadata_->get_file_refcount();
        return stat_struct;
    }

    void File::close() const {
        metadata_->decrease_file_refcount();
        partition_->file_system_->num_open_files_--;
    }

}
