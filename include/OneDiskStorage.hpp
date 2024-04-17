#ifndef FSMOD_ONEDISKSTORAGE_HPP
#define FSMOD_ONEDISKSTORAGE_HPP

#include <simgrid/s4u/MessageQueue.hpp>
#include "Storage.hpp"


namespace simgrid::module::fs {

    class XBT_PUBLIC OneDiskStorage : public Storage {

    public:
        static std::shared_ptr<OneDiskStorage> create(const std::string &name, simgrid::s4u::Disk *disk);

        s4u::ActivityPtr read_init(sg_size_t size) override;
        s4u::ActivityPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::ActivityPtr write_init(sg_size_t size) override;
        s4u::ActivityPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;

    protected:
        OneDiskStorage(const std::string &name, simgrid::s4u::Disk *disk);
        s4u::MessageQueue mq_;

    };

}

#endif //FSMOD_ONEDISKSTORAGE_HPP
