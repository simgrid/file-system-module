#include <simgrid/s4u/Engine.hpp>

#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"
#include "fsmod/FileSystemException.hpp"

namespace simgrid::module::fs {

   FileMetadata::FileMetadata(sg_size_t initial_size, simgrid::module::fs::Partition *partition) :
           current_size_(initial_size), future_size_(initial_size), partition_(partition) {

       creation_date_ = s4u::Engine::get_clock();
       partition_->new_file_creation_event(this);

       access_date_ = s4u::Engine::get_clock();
       partition_->new_file_access_event(this);

       modification_date_ = s4u::Engine::get_clock();
   }

    void FileMetadata::set_access_date(double date) {
       access_date_ = date;
       partition_->new_file_access_event(this);
   }

}