/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <memory>

#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"
#include "fsmod/FileSystemException.hpp"

namespace simgrid::fsmod {


    Partition::Partition(std::string name, std::shared_ptr<Storage> storage, sg_size_t size)
            : name_(std::move(name)), storage_(std::move(storage)), size_(size), free_space_(size) {
    }

    /**
     * @brief Retrieve the metadata for a file
     * @param dir_path: the path to the directory in which the file is located
     * @param file_name: the file name
     * @return A pointer to MetaData, or nullptr if the directory or file is not found
     */
    FileMetadata *Partition::get_file_metadata(const std::string &dir_path, const std::string &file_name) const {
        try {
            return content_.at(dir_path).at(file_name).get();
        } catch (std::out_of_range&) {
            return nullptr;
        }
    }

    /**
     * @brief Create a new file (and decrease the free space)
     * @param dir_path: the path to the directory in which the file is to be created
     * @param file_name: the file name
     * @param size: the file size in bytes
     */
    void Partition::create_new_file(const std::string &dir_path, const std::string &file_name, sg_size_t size) {
        // Check that there is enough space
        if (free_space_ < size) {
            this->create_space(size - free_space_);
        }

        // Check that the file doesn't already exit
        if (this->get_file_metadata(dir_path, file_name)) {
            throw FileAlreadyExistsException(XBT_THROW_POINT, dir_path + "/" + file_name);
        }

        content_[dir_path][file_name] = std::make_unique<FileMetadata>(size, this, dir_path, file_name);
        free_space_ -= size;
    }

    /**
     * @brief Delete a file (and increase the free space). Will silently do nothing
     *        if the directory or file does not exist
     * @param dir_path: the path to the directory in which the file is located
     * @param file_name: the file name
     */
    void Partition::delete_file(const std::string &dir_path, const std::string &file_name) {
        auto* metadata_ptr = this->get_file_metadata(dir_path, file_name);
        if (not metadata_ptr) {
            throw FileNotFoundException(XBT_THROW_POINT, dir_path + "/" + file_name);
        }

        if (metadata_ptr->get_file_refcount() > 0) {
            throw FileIsOpenException(XBT_THROW_POINT, dir_path + "/" + file_name);
        }

        this->new_file_deletion_event(metadata_ptr);
        free_space_ += metadata_ptr->get_current_size();
        content_.at(dir_path).erase(file_name);
    }

    /**
     * @brief Move a file
     * @param src_dir_path: source directory path
     * @param src_file_name: source file name
     * @param dst_dir_path: destination directory path
     * @param dst_file_name: destination file name
     */
    void Partition::move_file(const std::string &src_dir_path, const std::string &src_file_name,
                              const std::string &dst_dir_path, const std::string &dst_file_name) {
        // Get the src metadata, which must exist
        const FileMetadata* src_metadata = this->get_file_metadata(src_dir_path, src_file_name);
        if (not src_metadata) {
            throw FileNotFoundException(XBT_THROW_POINT, src_dir_path + "/" + src_file_name);
        }

        // Get the dst metadata, if any
        FileMetadata *dst_metadata = this->get_file_metadata(dst_dir_path, dst_file_name);

        // No-op mv?
        if (src_metadata == dst_metadata) {
            return; // Just like in the real world
        }

        // Sanity checks
        if (src_metadata->get_file_refcount() > 0) {
            throw FileIsOpenException(XBT_THROW_POINT, src_dir_path + "/" + src_file_name);
        }
        if (dst_metadata && dst_metadata->get_file_refcount()) {
            throw FileIsOpenException(XBT_THROW_POINT, dst_dir_path + "/" + dst_file_name);
        }

        // Update free space if needed
        if (dst_metadata) {
//            auto src_size = src_metadata->get_current_size();
            auto dst_size = dst_metadata->get_current_size();
//            if (dst_size < src_size) {
//                if (src_size - dst_size > this->get_free_space()) {
//                    this->create_space(src_size - dst_size - this->get_free_space());
//                }
//            }
            this->new_file_deletion_event(dst_metadata);
            free_space_ += dst_size;
        }

        // Do the move (reusing the original unique ptr, just in case)
        auto uniq_ptr = std::move(content_.at(src_dir_path).at(src_file_name));
        content_.at(src_dir_path).erase(src_file_name);
        content_[dst_dir_path][dst_file_name] = std::move(uniq_ptr);
    }

    std::set<std::string, std::less<>> Partition::list_files_in_directory(const std::string &dir_path) const {
        if (content_.find(dir_path) == content_.end()) {
            throw DirectoryDoesNotExistException(XBT_THROW_POINT, dir_path);
        }
        std::set<std::string, std::less<>> keys;
        for (auto const &[filename, metadata]: content_.at(dir_path)) {
            keys.insert(filename);
        }
        return keys;
    }


    void Partition::create_new_directory(const std::string &dir_path) {
        if (content_.find(dir_path) != content_.end()) {
            throw DirectoryAlreadyExistsException(XBT_THROW_POINT, dir_path);
        }
        this->content_[dir_path] = (std::unordered_map<std::string, std::unique_ptr<FileMetadata>>){};
    }

    void Partition::delete_directory(const std::string &dir_path) {
        if (content_.find(dir_path) == content_.end()) {
            throw DirectoryDoesNotExistException(XBT_THROW_POINT, dir_path);
        }
        // Check that no file is open
        for (const auto &[filename, metadata]: content_.at(dir_path)) {
            if (metadata->get_file_refcount() != 0) {
                throw FileIsOpenException(XBT_THROW_POINT, "No content deleted in directory before file " + filename + " is open");
            }
        }
        // Wipe everything out
        content_.erase(dir_path);
    }


    void Partition::create_space(sg_size_t num_bytes) {
        throw NotEnoughSpaceException(XBT_THROW_POINT);
    }

    void Partition::new_file_creation_event(FileMetadata *file_metadata) {
        // No-op
    }

    void Partition::new_file_access_event(FileMetadata *file_metadata) {
        // No-op
    }

    void Partition::new_file_deletion_event(FileMetadata *file_metadata) {
        // No-op
    }




}