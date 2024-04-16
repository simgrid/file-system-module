#include "FileSystem.hpp"
#include "File.hpp"
#include "PathUtil.hpp"
#include "Partition.hpp"
#include <algorithm>
#include <memory>

namespace simgrid::module::fs {
    /**
     * @brief Private method to find the partition and path at mount point for an absolute path
     * @param fullpath: an absolute simplified file path
     * @return
     */
    std::pair<std::shared_ptr<Partition>, std::string> FileSystem::find_path_at_mount_point(const std::string &simplified_path) const {

        // Identify the mount point and path at mount point partition
        auto it = std::find_if(this->partitions_.begin(),
                               this->partitions_.end(),
                               [simplified_path](const std::pair<std::string, std::shared_ptr<Partition>>& element) {
                                   return PathUtil::is_at_mount_point(simplified_path, element.first);
                               });
        if (it == this->partitions_.end()) {
            throw std::runtime_error("EXCEPTION: WRONG MOUNT POINT"); // TODO
        }
        auto path_at_mount_point = PathUtil::path_at_mount_point(simplified_path, it->first);
        auto partition = it->second;
        return std::make_pair(partition, path_at_mount_point);
    }

    /*********************** PUBLIC INTERFACE *****************************/

    std::shared_ptr<FileSystem> FileSystem::create(const std::string &name, int max_num_open_files) {
        return std::shared_ptr<FileSystem>(new FileSystem(name, max_num_open_files));
    }

    /**
     * @brief A method to add a partition to the file system
     * @param mount_point: the partition's mount point
     * @param partition: the partition
     */
    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, const std::string& size) {
      mount_partition(mount_point, storage, static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")));
    }
    void FileSystem::mount_partition(const std::string &mount_point, std::shared_ptr<Storage> storage, sg_size_t size) {

        if (PathUtil::simplify_path_string(mount_point) != mount_point or not PathUtil::is_absolute(mount_point)) {
            throw std::invalid_argument("simgrid::module::fs::FileSystem::add_partition(): Mount point should be a simple absolute path");
        }
        for (auto const &mp : this->partitions_) {
            if ((mp.first.rfind(mount_point, 0) == 0) or (mount_point.rfind(mp.first, 0) == 0)) {
                throw std::invalid_argument("simgrid::module::fs::FileSystem::add_partition(): New mount point '" + mount_point +
                                            "' would conflict with existing mount point '" + mp.first +
                                            "') - mount points cannot be prefixes of each other.");
            }
        }
        this->partitions_[mount_point] = std::shared_ptr<Partition>(new Partition(mount_point, storage, size));
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name(const std::string& name) const {
        auto partition = partition_by_name_or_null(name);
        if (not partition)
            throw std::invalid_argument("Partition not found: '" + name + "'");
        return partition;
    }

    std::shared_ptr<Partition> FileSystem::partition_by_name_or_null(const std::string& name) const {

    auto partition = partitions_.find(name);
    if (partition != partitions_.end())
        return partition->second;
    else
        return nullptr;
    }
    /**
     * @brief Create a new file on the file system in zero time
     * @param fullpath: the absolute path to the file
     * @param size: the file size
     */
    void FileSystem::create_file(const std::string& fullpath, const std::string& size) {
        create_file(fullpath, static_cast<sg_size_t>(xbt_parse_get_size("", 0, size, "")));
    }

    void FileSystem::create_file(const std::string& fullpath, sg_size_t size) {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that the file doesn't already exist
        if (partition->get_content().find(path_at_mount_point) != partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE ALREADY EXISTS"); // TODO
        }

        // Create FileMetaData
        auto metadata = std::make_unique<FileMetadata>(size);

        // Add the file to the content
        partition->get_content()[path_at_mount_point] = std::move(metadata);
    }

   /**
     * @brief Open a file
     * @param fullpath: an absolute file path
     * @return
     */
    std::shared_ptr<File> FileSystem::open(const std::string &fullpath) {
        // Get a file descriptor
        if (this->num_open_files_ >= this->max_num_open_files_) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }

        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that file is there
        if (partition->get_content().find(path_at_mount_point) == partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE NOT FOUND"); // TODO
        }

        // Get the FileMetadata raw pointer
        auto metadata = partition->get_content().at(path_at_mount_point).get();

        // Create the file object
        auto file =  std::shared_ptr<File>(new File(simplified_path, metadata, partition.get()));

        this->num_open_files_++;
        return file;
    }

    /**
     * @brief
     * @param fullpath: an absolute file path
     * @return the file size in bytes
     */
    sg_size_t FileSystem::file_size(const std::string& fullpath) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        // Check that the file exist
        if (partition->get_content().find(path_at_mount_point) == partition->get_content().end()) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }

        return partition->get_content().at(path_at_mount_point)->get_current_size();
    }

    void FileSystem::unlink_file(const std::string& fullpath) const {
        // Get the partition and path
        std::string simplified_path = PathUtil::simplify_path_string(fullpath);
        auto [partition, path_at_mount_point] = this->find_path_at_mount_point(simplified_path);

        sg_size_t file_size = 0;
        try {
            file_size = partition->get_content().at(path_at_mount_point)->get_current_size();
        } catch (std::out_of_range& e) {
            throw std::runtime_error("EXCEPTION: FILE DOES NOT EXISTS"); // TODO
        }
        // "Liberate" space on partition and remove from content
        partition->increase_free_space(file_size);
        partition->get_content().erase(path_at_mount_point);
    }
}