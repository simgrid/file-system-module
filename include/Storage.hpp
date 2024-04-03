#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

namespace simgrid::module::fs {

class Storage;
using StoragePtr = boost::intrusive_ptr<Storage>;

class XBT_PUBLIC Storage {
  std::vector<s4u::Disk*> disks_;
  s4u::Host* controller_host_;
  s4u::ActorPtr controller_;
  std::atomic_int_fast32_t refcount_{1};

public:
#ifndef DOXYGEN
  friend void intrusive_ptr_release(Storage* storage)
  {
    if (storage->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete storage;
    }
  }
  friend void intrusive_ptr_add_ref(Storage* storage) { storage->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
};

} // namespace simgrid::module::fs

#endif