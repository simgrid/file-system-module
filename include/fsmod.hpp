#ifndef SIMGRID_MODULE_FILE_SYSTEM_H_
#define SIMGRID_MODULE_FILE_SYSTEM_H_

#include <simgrid/forward.h> // sg_size_t
#include <xbt.h>
#include <xbt/config.h>

#include <memory>
#include <vector>

#include "Storage.hpp"

namespace simgrid {
namespace module {
namespace fs {

class XBT_PUBLIC FileSystem {
  std::vector<Partition*> partitions_;
  static int max_file_descriptors_;
  // Created lazily on need
  std::unique_ptr<std::vector<int>> file_descriptor_table = nullptr;
  StoragePtr storage_;

};

class XBT_PRIVATE Path {
public:
    static std::string simplify_path_string(const std::string &path);
    static bool goes_up(const std::string &simplified_path);
    static std::vector<std::string>::const_iterator find_mount_point(const std::string &simplified_absolute_path,
                                     const std::vector<std::string> &mount_points);
    static bool is_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point);
    static std::string path_at_mount_point(const std::string &simplified_absolute_path, const std::string &mount_point);

};

/// Cruft

#if 0
class XBT_PUBLIC File : public xbt::Extendable<File> {
  sg_size_t size_ = 0;
  std::string path_;
  std::string fullpath_;
  const Disk* local_disk_     = nullptr;
  std::string mount_point_;

  const Disk* find_local_disk_on(const Host* host);

  /** Retrieves the path to the file */
  const char* get_path() const { return fullpath_.c_str(); }

  void update_position(sg_offset_t);      /** set new position in file, grow it if necessary, and increased usage */

  /** Rename a file. WARNING: It is forbidden to move the file to another mount point */
  void move(const std::string& fullpath) const;
  int remote_copy(sg_host_t host, const std::string& fullpath);
  int remote_move(sg_host_t host, const std::string& fullpath);

  int unlink() const; /** Remove a file from the contents of a disk */
  void dump() const;
};

class XBT_PUBLIC FileSystemDiskExt {
  std::unique_ptr<std::map<std::string, sg_size_t, std::less<>>> content_;
  std::map<Host*, std::string> remote_mount_points_;
  std::string mount_point_;
  sg_size_t used_size_ = 0;
  sg_size_t size_      = static_cast<sg_size_t>(500 * 1024) * 1024 * 1024;

public:
  static simgrid::xbt::Extension<Disk, FileSystemDiskExt> EXTENSION_ID;
  explicit FileSystemDiskExt(const Disk* ptr);
  FileSystemDiskExt(const FileSystemDiskExt&) = delete;
  FileSystemDiskExt& operator=(const FileSystemDiskExt&) = delete;
  std::map<std::string, sg_size_t, std::less<>>* parse_content(const std::string& filename);
  std::map<std::string, sg_size_t, std::less<>>* get_content() const { return content_.get(); }
  const char* get_mount_point() const { return mount_point_.c_str(); }
  const char* get_mount_point(s4u::Host* remote_host) { return remote_mount_points_[remote_host].c_str(); }
  void add_remote_mount(Host* host, const std::string& mount_point);
  sg_size_t get_size() const { return size_; }
  sg_size_t get_used_size() const { return used_size_; }
  void decr_used_size(sg_size_t size);
  void incr_used_size(sg_size_t size);
};
#endif

} // namespace fs
} // namespace module
} // namespace simgrid

#endif