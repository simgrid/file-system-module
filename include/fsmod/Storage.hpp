#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

#include <atomic>
#include <boost/intrusive_ptr.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Host.hpp>
#include <utility>

#include "Partition.hpp"

namespace simgrid::fsmod {

    /**
     * @brief A class that iplements a storage abstraction
     */
    class XBT_PUBLIC Storage {
        std::string name_;
        std::vector<s4u::Disk*> disks_;
        s4u::ActorPtr controller_ = nullptr;
        s4u::Host* controller_host_ = nullptr;

    protected:
        explicit Storage(std::string name): name_(std::move(name)){}
        virtual ~Storage() = default;

        void set_disk(s4u::Disk* disk) { disks_.push_back(disk); }
        void set_disks(const std::vector<s4u::Disk*>& disks) { disks_ = disks; }
        void set_controller_host(s4u::Host* host) { controller_host_ = host; }
        void set_controller(s4u::ActorPtr actor) { controller_ = actor; }

    public:
        [[nodiscard]] const std::string& get_name() const { return name_; }
        [[nodiscard]] const char* get_cname() const { return name_.c_str(); }
        [[nodiscard]] s4u::Host* get_controller_host() const { return controller_host_; }
        [[nodiscard]] s4u::ActorPtr get_controller() const { return controller_; }
        [[nodiscard]] std::vector<s4u::Disk*> get_disks() const { return disks_; };
        [[nodiscard]] size_t get_num_disks() const { return disks_.size(); };
        [[nodiscard]] s4u::Disk* get_first_disk() const { return disks_.front(); };
        [[nodiscard]] s4u::Disk* get_disk_at(unsigned long position) const { return disks_.at(position); };

        virtual s4u::IoPtr read_async(sg_size_t size) = 0;
        virtual void read(sg_size_t size) = 0;

        virtual s4u::IoPtr write_async(sg_size_t size) = 0;
        virtual void write(sg_size_t size) = 0;
    };

} // namespace simgrid::fsmod

#endif
