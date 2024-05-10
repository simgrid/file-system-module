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

        s4u::IoPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::IoPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;

    protected:
        JBODStorage(const std::string& name, const std::vector<simgrid::s4u::Disk*>& disks);
        void update_parity_disk_idx() { parity_disk_idx_ = (parity_disk_idx_- 1) % num_disks_; }

        int get_next_read_disk_idx() { return (++read_disk_idx_) % num_disks_; }

        s4u::MessageQueue* mq_;

    private:
        unsigned int num_disks_;
        RAID raid_level_;
        unsigned int parity_disk_idx_;
        int read_disk_idx_ = -1;
    };
}

#endif //FSMOD_JBODSTORAGE_HPP
