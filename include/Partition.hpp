#ifndef SIMGRID_MODULE_FS_PARTITION_H_
#define SIMGRID_MODULE_FS_PARTITION_H_

#include "Storage.hpp"
#include "FileMetadata.hpp"

namespace simgrid::module::fs {

class XBT_PUBLIC Partition {
  std::string name_;
  sg_size_t size_ = 0;
  StoragePtr storage_;
  std::map<std::string, std::unique_ptr<FileMetadata>, std::less<>> content_;

public:
  explicit Partition(const std::string& name, sg_size_t size)
    : name_(name), size_(size){}
  ~Partition() = default;

  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  sg_size_t get_size() const { return size_; }
  const std::map<std::string, std::unique_ptr<FileMetadata>, std::less<>>& get_content() const { return content_; }
};

} // namespace simgrid::module::fs

#endif