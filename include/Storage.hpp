#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

#include <atomic>
#include <boost/intrusive_ptr.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Host.hpp>
#include <utility>

#include "Partition.hpp"

namespace simgrid::module::fs {

//class Storage;
//using StoragePtr = boost::intrusive_ptr<Storage>;

    class XBT_PUBLIC Storage {

    protected:
        std::string name_;
        std::vector<s4u::Disk*> disks_;
        s4u::Host* controller_host_ = nullptr;
        s4u::ActorPtr controller_ = nullptr;
        std::atomic_int_fast32_t refcount_{1};
        explicit Storage(std::string name): name_(std::move(name)){}

    public:
        virtual ~Storage() = default;

        const std::string& get_name() const { return name_; }
        const char* get_cname() const { return name_.c_str(); }
        s4u::Host* get_controller_host() const { return controller_host_; }
        s4u::ActorPtr get_controller() const { return controller_; }
        std::vector<s4u::Disk*> get_disks() const { return disks_; };

        virtual s4u::ActivityPtr read_init(sg_size_t size) = 0;
        virtual s4u::ActivityPtr read_async(sg_size_t size) = 0;
        virtual void read(sg_size_t size) = 0;

        virtual s4u::ActivityPtr write_init(sg_size_t size) = 0;
        virtual s4u::ActivityPtr write_async(sg_size_t size) = 0;
        virtual void write(sg_size_t size) = 0;

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