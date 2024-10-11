/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>
#include <utility>

#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"

namespace simgrid::fsmod {

   FileMetadata::FileMetadata(sg_size_t initial_size, Partition *partition, std::string dir_path, std::string file_name)
        : partition_(partition),
          dir_path_(std::move(dir_path)),
          file_name_(std::move(file_name)),
          current_size_(initial_size),
          future_size_(initial_size) {
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