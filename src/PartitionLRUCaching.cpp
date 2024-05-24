#include "fsmod/PartitionLRUCaching.hpp"

namespace simgrid::fsmod {

    void PartitionLRUCaching::new_file_access_event(FileMetadata *file_metadata) {
        rm_from_priority_list(file_metadata);
        file_metadata->sequence_number_ = get_next_sequence_number();
        add_to_priority_list(file_metadata);
    }

}