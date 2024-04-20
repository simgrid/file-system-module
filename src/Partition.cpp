#include "Partition.hpp"
#include "FileMetadata.hpp"
#include "FileSystemException.hpp"

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
            throw FileSystemException(XBT_THROW_POINT, "create_new_file(): Not enough space");
        }

        // Check that the file doesn't already exit
        if (this->get_file_metadata(dir_path, file_name)) {
            throw FileSystemException(XBT_THROW_POINT, "create_new_file(): file already exists");
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
            throw FileSystemException(XBT_THROW_POINT, "unlink_file(): File not found);");
        }

        if (metadata_ptr->get_file_refcount() > 0) {
            throw FileSystemException(XBT_THROW_POINT, "unlink_file(): Cannot unlink a file that is opened");
        }

        auto meta_data = this->get_file_metadata(dir_path, file_name);
        if (meta_data) {
            free_space_ += meta_data->get_current_size();
            content_.at(dir_path).erase(file_name);
        }
    }

    void Partition::move_file(const std::string& src_dir_path, const std::string& src_file_name,
                              const std::string& dst_dir_path, const std::string& dst_file_name) {

        // Get the src metadata, which must exist
        FileMetadata *src_metadata = this->get_file_metadata(src_dir_path, src_file_name);
        if (not src_metadata) {
            throw FileSystemException(XBT_THROW_POINT, "move_file(): file not found");
        }

        // Get the dst metadata, if any
        FileMetadata *dst_metadata = this->get_file_metadata(dst_dir_path, dst_file_name);

        // No-op mv?
        if (src_metadata == dst_metadata) {
            return; // Just like in the real world
        }

        // Sanity checks
        if (src_metadata->get_file_refcount() > 0) {
            throw FileSystemException(XBT_THROW_POINT, "move_file(): Cannot move a file that is open");
        }
        if (dst_metadata and dst_metadata->get_file_refcount()) {
            throw FileSystemException(XBT_THROW_POINT, "move_file(): Cannot move to a destination file that is open");
        }

        // Update free space if needed
        if (dst_metadata) {
            auto src_size = src_metadata->get_current_size();
            auto dst_size = dst_metadata->get_current_size();
            if (dst_size > src_size) {
                free_space_ += (dst_size - src_size);
            } else {
                if (src_size - dst_size <= this->get_free_space()) {
                    free_space_ -= (src_size - dst_size);
                } else {
                    throw FileSystemException(XBT_THROW_POINT, "move_file(): Not enough space");
                }
            }
        }

        // Do the move (reusing the original unique ptr, just in case)
        std::unique_ptr<FileMetadata> uniq_ptr = std::move(content_.at(src_dir_path).at(src_file_name));
        content_.at(src_dir_path).erase(src_file_name);
        content_[dst_dir_path][dst_file_name] = std::move(uniq_ptr);
    }

}