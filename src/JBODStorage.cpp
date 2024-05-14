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

    s4u::IoPtr JBODStorage::read_async(sg_size_t size) {
        // Determine what to read from each disk
        sg_size_t read_size = 0;
        std::vector<s4u::Disk*> targets;
        switch(raid_level_) {
            case RAID::RAID0:
                read_size = size / num_disks_;
                targets = disks_;
                break;
            case RAID::RAID1:
                read_size = size;
                targets.push_back(disks_.at(get_next_read_disk_idx()));
                break;
            case RAID::RAID4:
                read_size = size / (num_disks_ - 1);
                targets = disks_;
                targets.pop_back();
                break;
            case RAID::RAID5:
                read_size = size / (num_disks_ - 1);
                targets = disks_;
                targets.erase(targets.begin() + ((get_parity_disk_idx() + 1) % num_disks_));
              break;
            case RAID::RAID6:
                read_size = size / (num_disks_ - 2);
                targets = disks_;
                if ( ((get_parity_disk_idx() + 2) % num_disks_) == 0 ) {
                    targets.pop_back();
                    targets.erase(targets.begin());
                } else if (get_parity_disk_idx() + 1 == static_cast<int>(num_disks_)) {
                    targets.pop_back();
                    targets.pop_back();
                } else {
                    targets.erase(targets.begin() + (get_parity_disk_idx() + 1) % num_disks_,
                                  targets.begin() + get_parity_disk_idx() + 3);
                }
                break;
            default:
                throw std::runtime_error("Unsupported RAID level. Supported level are: 0, 1, 4, 5, and 6");
        }

        // Create a Comm to transfer data to the host that requested a read to the controller host of the JBOD
        // Do not assign the destination of the Comm yet, will be done after the completion of the IOs
        auto comm = s4u::Comm::sendto_init()->set_source(controller_host_)->set_payload_size(size);
        comm->set_name("Transfer from JBod");

        // Create the I/O activities on individual disks
        std::vector<s4u::IoPtr> ios;
        for (const auto* disk : targets) {
            auto io = s4u::IoPtr(disk->io_init(read_size, s4u::Io::OpType::READ));
            io->set_name(disk->get_name());
            ios.push_back(io);
            // Have the completion activity depend on every I/O
            io->add_successor(comm);
            io->detach();
        }

        // Create a no-op Activity that depends on the completion of the Comm. This is the one ActivityPtr returned
        // to the caller
        s4u::IoPtr completion_activity = s4u::Io::init()->set_op_type(s4u::Io::OpType::READ)->set_size(0);
        completion_activity->set_name("JBOD Read Completion");
        comm->add_successor(completion_activity);

        // Start the comm by setting its destination
        comm->set_destination(s4u::Host::current());

        // Completion Actitity is now blocked by tje Comm, start it by assigning it to the controller host first disk
        completion_activity->set_disk(disks_.front());

        return completion_activity;
   }

    void JBODStorage::read(sg_size_t size) {
        read_async(size)->wait();
    }

    s4u::IoPtr JBODStorage::write_async(sg_size_t size) {
        // Transfer data from the host that requested a write to the controller host of the JBOD
        auto comm = s4u::Comm::sendto_init()->set_payload_size(size)->set_source(s4u::Host::current());
        comm->set_name("Transfer to JBod");

        // Determine what to write on each individual disk according to RAID level and which disk will store the
        // parity block
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
        parity_block_comp->set_name("Parity Block Computation");

        // Do not start computing the parity block before the completion of the comm to the controller
        comm->add_successor(parity_block_comp);
        // Start the comm by setting its destination
        comm->set_destination(controller_host_);

        // Parity Block Computation is now blocked by Comm, start it by assigning it to the controller host
        parity_block_comp->detach();
        parity_block_comp->set_host(controller_host_);

        // Create a no-op Activity that depends on the completion of all I/Os. This is the one ActivityPtr returned
        // to the caller
        s4u::IoPtr completion_activity = s4u::Io::init()->set_op_type(s4u::Io::OpType::WRITE)->set_size(0);
        completion_activity->set_name("JBOD Write Completion");

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
            io->detach();
        }

        // Completion Actitity is now blocked by I/Os, start it by assigning it to the controller host first disk
        completion_activity->set_disk(disks_.front());

        return completion_activity;
    }

    void JBODStorage::write(sg_size_t size) {
       write_async(size)->wait();
    }
}
