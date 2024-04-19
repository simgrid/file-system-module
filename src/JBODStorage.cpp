#include "JBODStorage.hpp"
#include <simgrid/s4u/Actor.hpp>

namespace simgrid::module::fs {

    static void jbod_controller(JBODStorage* js) {
        js->mqueue()->get<void*>();
    }
    /**
     * @brief Method to create an instance of a JBOD (Just a Bunch of Disks) storage
     * @param disks: the disks
     * @return
     */
    std::shared_ptr<JBODStorage> JBODStorage::create(const std::string& name, const std::vector<s4u::Disk*>& disks) {
        return std::shared_ptr<JBODStorage>(new JBODStorage(name, disks));
    }

    JBODStorage::JBODStorage(const std::string& name, const std::vector<s4u::Disk*>& disks) : Storage(name) {
        disks_ = disks;
        num_disks_ = disks.size();
        controller_host_ = disks_.front()->get_host();
        // Create a no-op controller
        mq_ = s4u::MessageQueue::by_name(name+"_controller_mq");
        controller_ = s4u::Actor::create(name+"_controller", controller_host_, jbod_controller, this);
        controller_->daemonize();
    }

    s4u::ActivityPtr JBODStorage::read_init(sg_size_t size) {
        throw std::runtime_error("JBODStorage::read_init(): NOT IMPLEMENTED YET");
    }

    s4u::ActivityPtr JBODStorage::read_async(sg_size_t size) {
        throw std::runtime_error("JBODStorage::read_async(): NOT IMPLEMENTED YET");
    }

    void JBODStorage::read(sg_size_t size) {
        throw std::runtime_error("JBODStorage::read(): NOT IMPLEMENTED YET");

    }

    s4u::ActivityPtr JBODStorage::write_init(sg_size_t size) {
        throw std::runtime_error("JBODStorage::write_init(): NOT IMPLEMENTED YET");
    }

    s4u::ActivityPtr JBODStorage::write_async(sg_size_t size) {
        throw std::runtime_error("JBODStorage::write_async(): NOT IMPLEMENTED YET");
    }

    void JBODStorage::write(sg_size_t size) {
        throw std::runtime_error("JBODStorage::write(): NOT IMPLEMENTED YET");
    }
}
