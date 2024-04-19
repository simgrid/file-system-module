#ifndef FSMOD_JBODSTORAGE_HPP
#define FSMOD_JBODSTORAGE_HPP

#include <simgrid/s4u/MessageQueue.hpp>
#include "Storage.hpp"


namespace simgrid::module::fs {

    class XBT_PUBLIC JBODStorage : public Storage {
    public:
        enum class RAID {RAID0 = 0, RAID1 = 1, RAID4 = 4 , RAID5 = 5, RAID6 = 6};

        static std::shared_ptr<JBODStorage> create(const std::string& name, const std::vector<simgrid::s4u::Disk*>& disks);
        void set_raid_level(RAID raid_level) { raid_level_ = raid_level; }
        s4u::MessageQueue* mqueue() { return mq_; }

        s4u::ActivityPtr read_init(sg_size_t size) override;
        s4u::ActivityPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::ActivityPtr write_init(sg_size_t size) override;
        s4u::ActivityPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;

    protected:
        JBODStorage(const std::string& name, const std::vector<simgrid::s4u::Disk*>& disks);

        s4u::MessageQueue* mq_;

    protected:
        unsigned int num_disks_;
        RAID raid_level_;
    };

}

#endif //FSMOD_JBODSTORAGE_HPP
