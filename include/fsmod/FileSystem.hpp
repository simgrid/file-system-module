#ifndef SIMGRID_MODULE_FILESYSTEM_H_
#define SIMGRID_MODULE_FILESYSTEM_H_

#include <simgrid/forward.h> // sg_size_t
#include <xbt.h>
#include <xbt/config.h>
#include <utility>
#include <xbt/parse_units.hpp>

#include <memory>
#include <vector>

#include "Partition.hpp"
#include "File.hpp"

namespace simgrid::module::fs {

	/**
 	* @brief A class that implements a file system abstraction
 	*/
    class XBT_PUBLIC FileSystem {
        const std::string name_;
        int max_num_open_files_;

    public:
        static std::shared_ptr<FileSystem> create(const std::string &name, int max_num_open_files = 1024);

        void mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, sg_size_t size,
                             Partition::CachingScheme caching_scheme  = Partition::CachingScheme::NONE);
        void mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, const std::string& size,
                             Partition::CachingScheme caching_scheme  = Partition::CachingScheme::NONE);

        void create_file(const std::string& full_path, sg_size_t size);
        void create_file(const std::string& full_path, const std::string& size);

        [[nodiscard]] bool file_exists(const std::string& full_path);
        [[nodiscard]] bool directory_exists(const std::string& full_dir_path);
        std::set<std::string> list_files_in_directory(const std::string& full_dir_path);

        void move_file(const std::string& src_full_path, const std::string& dst_full_path) const;
        void unlink_file(const std::string& full_path) const;
        void unlink_directory(const std::string& full_dir_path);

        [[nodiscard]] sg_size_t file_size(const std::string& full_path) const;

        std::shared_ptr<File> open(const std::string& full_path);
        void close(std::shared_ptr<File> f);

        [[nodiscard]] std::shared_ptr<Partition> partition_by_name(const std::string& name) const;
        [[nodiscard]] std::shared_ptr<Partition> partition_by_name_or_null(const std::string& name) const;

    protected:
        explicit FileSystem(std::string name, int max_num_open_files) : name_(std::move(name)), max_num_open_files_(max_num_open_files) {};

    private:
        [[nodiscard]] std::pair<std::shared_ptr<Partition>, std::string> find_path_at_mount_point(const std::string &full_path) const;

        std::map<std::string, std::shared_ptr<Partition>> partitions_;

        int num_open_files_ = 0;
    };
} // namespace simgrid::module::fs

#endif
