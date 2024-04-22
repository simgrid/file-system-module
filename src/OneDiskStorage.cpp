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

    s4u::ActivityPtr OneDiskStorage::read_init(sg_size_t size) {
        throw std::runtime_error("OneDiskStorage::read_init(): NOT IMPLEMENTED YET");
    }

    s4u::ActivityPtr OneDiskStorage::read_async(sg_size_t size) {
        throw std::runtime_error("OneDiskStorage::read_async(): NOT IMPLEMENTED YET");
    }

    void OneDiskStorage::read(sg_size_t size) {
        throw std::runtime_error("OneDiskStorage::read(): NOT IMPLEMENTED YET");

    }

    s4u::ActivityPtr OneDiskStorage::write_init(sg_size_t size) {
        throw std::runtime_error("OneDiskStorage::write_init(): NOT IMPLEMENTED YET");
    }

    s4u::ActivityPtr OneDiskStorage::write_async(sg_size_t size) {
        throw std::runtime_error("OneDiskStorage::write_async(): NOT IMPLEMENTED YET");
    }

    void OneDiskStorage::write(sg_size_t size) {
        this->disks_.front()->write(size);
    }

}
