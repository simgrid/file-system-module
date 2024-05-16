#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"
#include "fsmod/FileSystemException.hpp"
#include "fsmod/CachingStrategy.hpp"

namespace simgrid::module::fs {

    /**
     * @brief Retrieve the metadata for a file
     * @param dir_path: the path to the directory in which the file is located
     * @param file_name: the file name
     * @return A pointer to MetaData, or nullptr if the directory or file is not found
     */
    FileMetadata* Partition::get_file_metadata(const std::string& dir_path, const std::string& file_name) {
        try {
            return content_.at(dir_path).at(file_name).get();
        } catch (std::out_of_range &e) {
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
//            throw FileSystemException(XBT_THROW_POINT, "Not enough space");
        }

        // Check that the file doesn't already exit
        if (this->get_file_metadata(dir_path, file_name)) {
            throw FileSystemException(XBT_THROW_POINT, "File already exists");
        }

        content_[dir_path][file_name] = std::make_unique<FileMetadata>(size);
        free_space_ -= size;
    }

    /**
     * @brief Delete a file (and increase the free space). Will silently do nothing
     *        if the directory or file does not exist
     * @param dir_path: the path to the directory in which the file is located
     * @param file_name: the file name
     */
    void Partition::delete_file(const std::string &dir_path, const std::string &file_name) {
        FileMetadata *metadata_ptr = this->get_file_metadata(dir_path, file_name);
        if (not metadata_ptr) {
            throw FileSystemException(XBT_THROW_POINT, "File not found);");
        }

        if (metadata_ptr->get_file_refcount() > 0) {
            throw FileSystemException(XBT_THROW_POINT, "Cannot unlink a file that is opened");
        }

        auto meta_data = this->get_file_metadata(dir_path, file_name);
        if (meta_data) {
            free_space_ += meta_data->get_current_size();
            content_.at(dir_path).erase(file_name);
        }
    }

    /**
     * @brief Move a file
     * @param src_dir_path: source directory path
     * @param src_file_name: source file name
     * @param dst_dir_path: destination directory path
     * @param dst_file_name: destination file name
     */
    void Partition::move_file(const std::string& src_dir_path, const std::string& src_file_name,
                              const std::string& dst_dir_path, const std::string& dst_file_name) {

        // Get the src metadata, which must exist
        FileMetadata *src_metadata = this->get_file_metadata(src_dir_path, src_file_name);
        if (not src_metadata) {
            throw FileSystemException(XBT_THROW_POINT, "File not found");
        }

        // Get the dst metadata, if any
        FileMetadata *dst_metadata = this->get_file_metadata(dst_dir_path, dst_file_name);

        // No-op mv?
        if (src_metadata == dst_metadata) {
            return; // Just like in the real world
        }

        // Sanity checks
        if (src_metadata->get_file_refcount() > 0) {
            throw FileSystemException(XBT_THROW_POINT, "Cannot move a file that is open");
        }
        if (dst_metadata and dst_metadata->get_file_refcount()) {
            throw FileSystemException(XBT_THROW_POINT, "Cannot move to a destination file that is open");
        }

        // Update free space if needed
        if (dst_metadata) {
            auto src_size = src_metadata->get_current_size();
            auto dst_size = dst_metadata->get_current_size();
            if (dst_size < src_size) {
                if (src_size - dst_size > this->get_free_space()) {
                    this->create_space(src_size - dst_size - this->get_free_space());
//                    throw FileSystemException(XBT_THROW_POINT, "Not enough space");
                }
            }
            free_space_ += dst_size;
        }

        // Do the move (reusing the original unique ptr, just in case)
        std::unique_ptr<FileMetadata> uniq_ptr = std::move(content_.at(src_dir_path).at(src_file_name));
        content_.at(src_dir_path).erase(src_file_name);
        content_[dst_dir_path][dst_file_name] = std::move(uniq_ptr);
    }

    std::set<std::string> Partition::list_files_in_directory(const std::string &dir_path) {
        if (content_.find(dir_path) == content_.end()) {
            throw FileSystemException(XBT_THROW_POINT, "Directory does not exist");
        }
        std::set<std::string> keys;
        for (auto const &key: content_[dir_path]) {
            keys.insert(key.first);
        }
        return keys;
    }

    void Partition::delete_directory(const std::string &dir_path) {
        if (content_.find(dir_path) == content_.end()) {
            throw FileSystemException(XBT_THROW_POINT, "Directory does not exist");
        }
        // Check that no file is open
        for (const auto &item : content_.at(dir_path)) {
            if (item.second->get_file_refcount() != 0) {
                throw FileSystemException(XBT_THROW_POINT,
                                          "Cannot delete a file that is open - no content deleted in directory");
            }
        }
        // Wipe everything out
        content_.erase(dir_path);
    }

    void Partition::set_caching_scheme(Partition::CachingScheme caching_scheme) {
        switch (caching_scheme) {
            case CachingScheme::NONE:
                caching_strategy_ = nullptr;
                break;
            case CachingScheme::FIFO:
                caching_strategy_ = std::shared_ptr<CachingStrategyFIFO>(new CachingStrategyFIFO(this));
                break;
            case CachingScheme::LRU:
                caching_strategy_ = std::shared_ptr<CachingStrategyLRU>(new CachingStrategyLRU(this));
                break;
            default:
                throw std::invalid_argument("Unknown/invalid caching scheme");
        }
    }

    Partition::CachingScheme Partition::get_caching_scheme() const {
        if (caching_strategy_ == nullptr) return CachingScheme::NONE;
        if (std::dynamic_pointer_cast<CachingStrategyFIFO>(caching_strategy_)) return CachingScheme::FIFO;
        if (std::dynamic_pointer_cast<CachingStrategyLRU>(caching_strategy_)) return CachingScheme::LRU;
        throw std::runtime_error("Internal error/bug");
    }


    void Partition::create_space(sg_size_t num_bytes) {
        if (not this->caching_strategy_) {
            throw FileSystemException(XBT_THROW_POINT, "Not enough space");
        } else {
            caching_strategy_->create_space(num_bytes);
        }
    }
}