#ifndef SIMGRID_MODULE_FS_STORAGE_H_
#define SIMGRID_MODULE_FS_STORAGE_H_

#include <atomic>
#include <boost/intrusive_ptr.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Host.hpp>
#include <utility>

#include "Partition.hpp"

namespace simgrid::module::fs {

    class XBT_PUBLIC Storage {

    protected:
        std::string name_;
        std::vector<s4u::Disk*> disks_;
        s4u::Host* controller_host_ = nullptr;
        s4u::ActorPtr controller_ = nullptr;
        explicit Storage(std::string name): name_(std::move(name)){}

    public:
        const std::string& get_name() const { return name_; }
        const char* get_cname() const { return name_.c_str(); }
        s4u::Host* get_controller_host() const { return controller_host_; }
        s4u::ActorPtr get_controller() const { return controller_; }
        std::vector<s4u::Disk*> get_disks() const { return disks_; };

        virtual s4u::IoPtr read_async(sg_size_t size) = 0;
        virtual void read(sg_size_t size) = 0;

        virtual s4u::IoPtr write_async(sg_size_t size) = 0;
        virtual void write(sg_size_t size) = 0;
    };

} // namespace simgrid::module::fs

#endif