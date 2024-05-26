#include "fsmod/OneDiskStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::fsmod {

    /**
     * @brief Method to create an instance of a one-disk storage
     * @param name: the storage's name
     * @param disk: the storage's disk
     * @return
     */
    std::shared_ptr<OneDiskStorage> OneDiskStorage::create(const std::string& name, s4u::Disk* disk) {
        return std::make_shared<OneDiskStorage>(name, disk);
    }

    OneDiskStorage::OneDiskStorage(const std::string& name, s4u::Disk* disk) : Storage(name) {
        set_disk(disk);
        set_controller_host(disk->get_host());
        // Create a no-op controller
        mq_ = s4u::MessageQueue::by_name(name+"_controller_mq");
        set_controller(s4u::Actor::create(name+"_controller", get_controller_host(), [this](){ mq_->get<void*>(); })->daemonize());
    }

    s4u::IoPtr OneDiskStorage::read_async(sg_size_t size) {
        return get_first_disk()->read_async(size);
    }

    void OneDiskStorage::read(sg_size_t size) {
        get_first_disk()->read(size);
    }

    s4u::IoPtr OneDiskStorage::write_async(sg_size_t size) {
        return get_first_disk()->write_async(size);
    }

    void OneDiskStorage::write(sg_size_t size) {
        get_first_disk()->write(size);
    }

}
