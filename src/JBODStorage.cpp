#include "JBODStorage.hpp"
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmode_jbod, "File System module: JBOD Storage related logs");

namespace simgrid::module::fs {

    /**
     * @brief Method to create an instance of a JBOD (Just a Bunch of Disks) storage
     * @param name: the storage's name
     * @param disks: the storage's disks
     * @return
     */
    std::shared_ptr<JBODStorage> JBODStorage::create(const std::string& name, const std::vector<s4u::Disk*>& disks) {
        return std::shared_ptr<JBODStorage>(new JBODStorage(name, disks));
    }

    JBODStorage::JBODStorage(const std::string& name, const std::vector<s4u::Disk*>& disks) : Storage(name) {
        disks_ = disks;
        num_disks_ = disks.size();
        parity_disk_idx_ = num_disks_ - 1;
        controller_host_ = disks_.front()->get_host();
        // Create a no-op controller
        mq_ = s4u::MessageQueue::by_name(name+"_controller_mq");
        controller_ = s4u::Actor::create(name+"_controller", controller_host_, [this](){
            //mq_->get<void*>();
        });
        controller_->daemonize();
    }

    s4u::ActivityPtr JBODStorage::read_async(sg_size_t size) {
        throw std::runtime_error("JBODStorage::read_async(): NOT IMPLEMENTED YET");
    }

    void JBODStorage::read(sg_size_t size) {
        throw std::runtime_error("JBODStorage::read(): NOT IMPLEMENTED YET");

    }

    s4u::ActivityPtr JBODStorage::write_async(sg_size_t size) {
        // Transfer data from the host that requested a write to the controller host of the JBOD
        auto comm = s4u::Comm::sendto_init(s4u::Host::current(), controller_host_);
        // Determine what size to write on individual disks according to RAID level, and update information on
        // parity disk index
        sg_size_t write_size = 0;
        switch(raid_level_) {
            case RAID::RAID0:
                write_size = size / num_disks_;
                break;
            case RAID::RAID1:
                write_size = size;
                break;
            case RAID::RAID4:
                write_size = size / (num_disks_ - 1);
                break;
            case RAID::RAID5:
                update_parity_disk_idx();
                write_size = size / (num_disks_ - 1);
                break;
            case RAID::RAID6:
                update_parity_disk_idx();
                update_parity_disk_idx();
                write_size = size / (num_disks_ - 2);
                break;
            default:
                throw std::runtime_error("Unsupported RAID level. Supported level are: 0, 1, 4, 5, and 6");
        }
        // Compute the parity block (if any)
        s4u::ExecPtr parity_block_comp = nullptr;
        if (raid_level_ == RAID::RAID4 || raid_level_ == RAID::RAID5 || raid_level_ == RAID::RAID6) {
            // Assume 1 flop per byte to write per parity block and two for RAID6.
            // Do not assign the Exec yet, will be done after the completion of the CommPtr
            if (raid_level_ == RAID::RAID6)
                parity_block_comp = s4u::Exec::init()->set_flops_amount(200 * write_size);
            else
                parity_block_comp = s4u::Exec::init()->set_flops_amount(write_size);
        } else // Create a no-op activity
           parity_block_comp = s4u::Exec::init()->set_flops_amount(0);

        // Do not start computing the parity block before the completion of the comm to the controller
        comm->add_successor(parity_block_comp);
        // Start the comm by setting its payload
        comm->set_payload_size(size);
        // Parity Block Computation is now blocked by Comm, start it by assigning it the controller host
        parity_block_comp->set_host(controller_host_);

        // Create a no-op Activity that depends on the completion of all I/Os. This is the one ActivityPtr returned
        // to the caller
        s4u::ExecPtr completion_activity = s4u::Exec::init()->set_flops_amount(0);

        // Create the I/O activities on individual disks
        std::vector<s4u::IoPtr> ios;
        for (const auto* disk : disks_) {
            auto io = s4u::IoPtr(disk->io_init(write_size, s4u::Io::OpType::WRITE));
            io->set_name(disk->get_name());
            ios.push_back(io);
            // Do no start the I/Os before the completion of the computation of the parity block
            parity_block_comp->add_successor(io);
            // Have the completion activity depend on every I/O
            io->add_successor(completion_activity);
            io->start();
        }

        // Completion Actitity is now blocked by I/Os, start it
        completion_activity->start();

        return completion_activity;
    }

    void JBODStorage::write(sg_size_t size) {
       write_async(size)->wait();
    }
}
