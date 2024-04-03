#ifndef SIMGRID_MODULE_FILE_SYSTEM_H_
#define SIMGRID_MODULE_FILE_SYSTEM_H_

#include <simgrid/forward.h> // sg_size_t

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::File>;

namespace module {
namespace fs {


class XBT_PUBLIC Partition {
  std::string name_;
  std::string mount_point_;
  sg_size_t size_ = 0;

  public:
  explicit Partition(const std::string& name, const std::string& mount_point, sg_size_t size)
    : name_(name), mount_point_(mount_point), size_(size){}

};

class XBT_PUBLIC Storage {
  std::vector<Partition*> partitions_;
  std::vector<s4u::Disk*> disks_;
  s4u::Host* controller_host_;
  s4u::ActorPtr controller_;
  static int max_file_descriptors_;
  // Created lazily on need
  std::unique_ptr<std::vector<int>> file_descriptor_table = nullptr;

  public:
};

class XBT_PUBLIC File {
  std::string name_;
  std::string path_;
  sg_size_t size_ = 0;
  int desc_id     = 0;

}
/// Cruft

class XBT_PUBLIC File : public xbt::Extendable<File> {
  sg_size_t size_ = 0;
  std::string path_;
  std::string fullpath_;
  sg_size_t current_position_ = SEEK_SET;
  const Disk* local_disk_     = nullptr;
  std::string mount_point_;

  const Disk* find_local_disk_on(const Host* host);

protected:
  File(const std::string& fullpath, void* userdata);
  File(const std::string& fullpath, const_sg_host_t host, void* userdata);
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  ~File();

public:
  static File* open(const std::string& fullpath, void* userdata);
  static File* open(const std::string& fullpath, const_sg_host_t host, void* userdata);
  void close();

  /** Retrieves the path to the file */
  const char* get_path() const { return fullpath_.c_str(); }

  /** Simulates a local read action. Returns the size of data actually read */
  sg_size_t read(sg_size_t size);

  /** Simulates a write action. Returns the size of data actually written. */
  sg_size_t write(sg_size_t size, bool write_inside = false);

  sg_size_t size() const;
  void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
  void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
  sg_size_t tell() const;                 /** Retrieves the current file position */
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

class XBT_PUBLIC FileDescriptorHostExt {
public:
  static simgrid::xbt::Extension<Host, FileDescriptorHostExt> EXTENSION_ID;
};

} // namespace fs
} // namespace module
} // namespace simgrid

#endif