#include "OneDiskStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::module::fs {

    /**
     * @brief Method to create an instance of a one-disk storage
     * @param name: the storage's name
     * @param disk: the storage's disk
     * @return
     */
    std::shared_ptr<OneDiskStorage> OneDiskStorage::create(const std::string& name, s4u::Disk* disk) {
        return  std::shared_ptr<OneDiskStorage>(new OneDiskStorage(name, disk));
    }

    OneDiskStorage::OneDiskStorage(const std::string& name, s4u::Disk* disk) : Storage(name) {
        disks_.push_back(disk);
        controller_host_ = disk->get_host();
        // Create a no-op controller
        mq_ = s4u::MessageQueue::by_name(name+"_controller_mq");
        controller_ = s4u::Actor::create(name+"_controller", controller_host_, [this](){
            mq_->get<void*>();
        });
        controller_->daemonize();
    }

    s4u::IoPtr OneDiskStorage::read_async(sg_size_t size) {
        return disks_.front()->read_async(size);
    }

    void OneDiskStorage::read(sg_size_t size) {
        disks_.front()->read(size);
    }

    s4u::IoPtr OneDiskStorage::write_async(sg_size_t size) {
        return disks_.front()->write_async(size);
    }

    void OneDiskStorage::write(sg_size_t size) {
        disks_.front()->write(size);
    }

}
