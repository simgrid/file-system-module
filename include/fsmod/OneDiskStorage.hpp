#ifndef FSMOD_ONEDISKSTORAGE_HPP
#define FSMOD_ONEDISKSTORAGE_HPP

#include <simgrid/s4u/MessageQueue.hpp>
#include "Storage.hpp"

namespace simgrid::module::fs {

    /**
     * @brief A class that implements a one-disk storage
     */
    class XBT_PUBLIC OneDiskStorage : public Storage {
    public:
        static std::shared_ptr<OneDiskStorage> create(const std::string &name, simgrid::s4u::Disk *disk);

        s4u::IoPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::IoPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;

    protected:
        OneDiskStorage(const std::string &name, simgrid::s4u::Disk *disk);
        s4u::MessageQueue *mq_;
    };
} // namespace simgrid::module::fs

#endif //FSMOD_ONEDISKSTORAGE_HPP
