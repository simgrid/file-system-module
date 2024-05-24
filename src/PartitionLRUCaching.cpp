#include "fsmod/PartitionLRUCaching.hpp"

namespace simgrid::fsmod {

    void PartitionLRUCaching::new_file_access_event(FileMetadata *file_metadata) {
        priority_list_.erase(file_metadata->sequence_number_);
        file_metadata->sequence_number_ = sequence_number_++;
        priority_list_[file_metadata->sequence_number_] = file_metadata;
    }

}