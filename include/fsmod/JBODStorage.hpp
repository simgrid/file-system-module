#ifndef FSMOD_JBODSTORAGE_HPP
#define FSMOD_JBODSTORAGE_HPP

#include <simgrid/s4u/MessageQueue.hpp>
#include "Storage.hpp"


namespace simgrid::fsmod {

    /**
     * @brief A class that implements an abstraction of a "Just a Bunch Of Disks" storage
     */
    class XBT_PUBLIC JBODStorage : public Storage {
    public:
        /**
         * @brief An enum that defines the possible RAID levels that can be used by a JBODStorage
         */
        enum class RAID {
                /** @brief RAID level 0 */
                RAID0 = 0,
                /** @brief RAID level 1 */
                RAID1 = 1,
                /** @brief RAID level 2 (unsupported) */
                RAID2 = 2,
                /** @brief RAID level 3 (unsupported) */
                RAID3 = 3,
                /** @brief RAID level 4 */
                RAID4 = 4 ,
                /** @brief RAID level 5 */
                RAID5 = 5,
                /** @brief RAID level 6 */
                RAID6 = 6};

        JBODStorage(const std::string& name, const std::vector<simgrid::s4u::Disk*>& disks);
        static std::shared_ptr<JBODStorage> create(const std::string& name,
                                                   const std::vector<simgrid::s4u::Disk*>& disks,
                                                   JBODStorage::RAID raid_level = RAID::RAID0);
        ~JBODStorage() override = default;
        /**
         * @brief Retrieves the storage's RAID level
         * @return A RAID level
         */
        [[nodiscard]] RAID get_raid_level() const { return raid_level_; }

        void set_raid_level(RAID raid_level);


        s4u::IoPtr read_async(sg_size_t size) override;
        void read(sg_size_t size) override;
        s4u::IoPtr write_async(sg_size_t size) override;
        void write(sg_size_t size) override;

    protected:
        s4u::MessageQueue* mqueue() const { return mq_; }

        void update_parity_disk_idx() { parity_disk_idx_ = (parity_disk_idx_- 1) % num_disks_; }

        long get_next_read_disk_idx() { return (++read_disk_idx_) % num_disks_; }
        [[nodiscard]] unsigned long get_parity_disk_idx() const { return parity_disk_idx_; }

    private:
        unsigned long num_disks_;
        RAID raid_level_;
        unsigned long parity_disk_idx_;
        long read_disk_idx_ = -1;
        s4u::MessageQueue* mq_;
    };
}

#endif //FSMOD_JBODSTORAGE_HPP
