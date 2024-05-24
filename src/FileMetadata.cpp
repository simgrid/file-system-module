#include <simgrid/s4u/Engine.hpp>
#include <utility>

#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"
#include "fsmod/FileSystemException.hpp"

namespace simgrid::fsmod {

   FileMetadata::FileMetadata(sg_size_t initial_size, Partition *partition, std::string dir_path, std::string file_name)
        : current_size_(initial_size),
          future_size_(initial_size),
          partition_(partition),
          dir_path_(std::move(dir_path)),
          file_name_(std::move(file_name)) {
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
} // namespace simgrid::fsmod