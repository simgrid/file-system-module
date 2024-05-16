#ifndef SIMGRID_MODULE_FS_PARTITION_H_
#define SIMGRID_MODULE_FS_PARTITION_H_

#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include "fsmod/FileMetadata.hpp"

namespace simgrid::module::fs {

    class Storage;


    class XBT_PUBLIC Partition {
        friend class FileSystem;

        std::string name_;
        sg_size_t size_ = 0;
        sg_size_t free_space_ = 0;
        std::shared_ptr<Storage> storage_;
        std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<FileMetadata>>> content_;

    public:
        enum class CachingScheme {NONE = 0, FIFO = 1, LRU = 2};

    protected:
        Partition(std::string name, std::shared_ptr<Storage> storage, sg_size_t size);

    public:
        ~Partition() = default;
        [[nodiscard]] const std::string& get_name() const { return name_; }
        [[nodiscard]] const char* get_cname() const { return name_.c_str(); }
        [[nodiscard]] sg_size_t get_size() const { return size_; }

        [[nodiscard]] sg_size_t get_free_space() const { return free_space_; }

    protected:
        // Methods to perform caching
        virtual void create_space(sg_size_t num_bytes);
        virtual void new_file_creation_event(FileMetadata *file_metadata);
        virtual void new_file_access_event(FileMetadata *file_metadata);
        virtual void new_file_deletion_event(FileMetadata *file_metadata);

    private:
//        friend class FileSystem;
        friend class File;
        friend class FileMetadata;

        void decrease_free_space(sg_size_t num_bytes) { free_space_ -= num_bytes; }
        void increase_free_space(sg_size_t num_bytes) { free_space_ += num_bytes; }

        [[nodiscard]] std::shared_ptr<Storage> get_storage() const { return storage_; }

        [[nodiscard]] bool directory_exists(const std::string& dir_path) { return content_.find(dir_path) != content_.end(); }
        std::set<std::string> list_files_in_directory(const std::string &dir_path);
        void delete_directory(const std::string &dir_path);

        [[nodiscard]] FileMetadata* get_file_metadata(const std::string& dir_path, const std::string& file_name);
        void create_new_file(const std::string& dir_path,
                             const std::string& file_name,
                             sg_size_t size);
        void move_file(const std::string& src_dir_path, const std::string& src_file_name,
                       const std::string& dst_dir_path, const std::string& dst_file_name);
    protected:
        void delete_file(const std::string& dir_path, const std::string& file_name);
    };



} // namespace simgrid::module::fs

#endif