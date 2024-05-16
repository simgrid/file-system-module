#include <memory>

#include "fsmod/PartitionLRUCaching.hpp"

namespace simgrid::module::fs {

    void PartitionLRUCaching::create_space(sg_size_t num_bytes) {
        throw std::runtime_error("NOT IMPLEMENTED YET");
    }

    void PartitionLRUCaching::new_file_creation_event(FileMetadata *file_metadata) {
        throw std::runtime_error("NOT IMPLEMENTED YET");
    }

    void PartitionLRUCaching::new_file_access_event(FileMetadata *file_metadata) {
        throw std::runtime_error("NOT IMPLEMENTED YET");
    }

    void PartitionLRUCaching::new_file_deletion_event(FileMetadata *file_metadata) {
        throw std::runtime_error("NOT IMPLEMENTED YET");
    }


}