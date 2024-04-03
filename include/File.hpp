#ifndef SIMGRID_MODULE_FS_FILE_H_
#define SIMGRID_MODULE_FS_FILE_H_

namespace simgrid {
namespace module {
namespace fs {

class XBT_PUBLIC File {
  std::string name_;
  std::string path_;
  sg_size_t size_ = 0;
  sg_size_t current_position_ = SEEK_SET;
  int desc_id     = 0;

protected:
  File(const std::string& name, const std::string& fullpath);
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  ~File();

public:
  static File* open(const std::string& name, const std::string& fullpath);
  static File* create(const std::string& name, const std::string& fullpath, sg_size_t size);

  void read(sg_size_t size);
  void write(sg_size_t size);

  void append(sg_size_t size);
  void truncate(sg_size_t to_size);

  void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
  void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
  sg_size_t tell() const;                 /** Retrieves the current file position */

  void move(const std::string& fullpath) const;
  void copy(const std::string& fullpath) const;

  void close();
  int unlink() const; /** Remove a file from the contents of a disk */

  sg_size_t size() const;
  void dump() const;
};

} // namespace fs
} // namespace module
} // namespace simgrid

#endif