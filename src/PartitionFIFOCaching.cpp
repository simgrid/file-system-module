#include <simgrid/Exception.hpp>

#include "fsmod/PartitionFIFOCaching.hpp"
#include "fsmod/FileSystemException.hpp"

namespace simgrid::fsmod {

    void PartitionFIFOCaching::create_space(sg_size_t num_bytes) {
        sg_size_t space_that_can_be_created = 0.0;
        std::vector<unsigned long> files_to_remove_to_create_space;

        for (auto const &victim: priority_list_) {
            // Never evict an open file
            if (victim.second->file_refcount_ > 0) {
                continue;
            }
            // Found a victim
            files_to_remove_to_create_space.push_back(victim.first);
            space_that_can_be_created += victim.second->current_size_;
            if (space_that_can_be_created >= num_bytes) {
                break;
            }
        }
        if (space_that_can_be_created < num_bytes) {
            throw FileSystemException(XBT_THROW_POINT, "Unable to evict files to create enough space");
        }
        for (auto const &victim: files_to_remove_to_create_space) {
            this->delete_file(this->priority_list_[victim]->dir_path_, this->priority_list_[victim]->file_name_);
        }
    }

    void PartitionFIFOCaching::new_file_creation_event(FileMetadata *file_metadata) {
        file_metadata->sequence_number_ = sequence_number_++;
        priority_list_[file_metadata->sequence_number_] = file_metadata;
    }

    void PartitionFIFOCaching::new_file_access_event(FileMetadata *file_metadata) {
        // No-op
    }

    void PartitionFIFOCaching::new_file_deletion_event(FileMetadata *file_metadata) {
        priority_list_.erase(file_metadata->sequence_number_);
    }


}