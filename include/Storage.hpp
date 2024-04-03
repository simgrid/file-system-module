#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

namespace simgrid {
namespace module {
namespace fs {

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

} // namespace fs
} // namespace module
} // namespace simgrid

#endif