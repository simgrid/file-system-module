/* Copyright (c) 2025-2026. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <fsmod/File.hpp>
#include <fsmod/FileMetadata.hpp>
#include <fsmod/FileStat.hpp>
#include <fsmod/FileSystem.hpp>
#include <fsmod/FileSystemException.hpp>
#include <fsmod/JBODStorage.hpp>
#include <fsmod/OneDiskStorage.hpp>
#include <fsmod/OneRemoteDiskStorage.hpp>
#include <fsmod/Partition.hpp>
#include <fsmod/PartitionFIFOCaching.hpp>
#include <fsmod/PartitionLRUCaching.hpp>
#include <fsmod/PathUtil.hpp>
#include <fsmod/Storage.hpp>
#include <fsmod/version.hpp>

#include <xbt/log.h>

namespace py = pybind11;
using simgrid::fsmod::File;
using simgrid::fsmod::FileMetadata;
using simgrid::fsmod::FileStat;
using simgrid::fsmod::FileSystem;
using simgrid::fsmod::JBODStorage;
using simgrid::fsmod::OneDiskStorage;
using simgrid::fsmod::OneRemoteDiskStorage;
using simgrid::fsmod::Partition;
using simgrid::fsmod::PartitionFIFOCaching;
using simgrid::fsmod::PartitionLRUCaching;
using simgrid::fsmod::PathUtil;
using simgrid::fsmod::Storage;

XBT_LOG_NEW_DEFAULT_CATEGORY(python, "python");

namespace {
std::string get_fsmod_version()
{
  int major;
  int minor;
  int patch;
  fsmod_version_get(&major, &minor, &patch);
  return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
}
}

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

PYBIND11_MODULE(fsmod, m)
{
  m.doc() = "FSMOD userspace API";

  m.attr("fsmod_version") = get_fsmod_version();

  py::register_exception<simgrid::fsmod::NotEnoughSpaceException>(m, "NotEnoughSpaceException");
  py::register_exception<simgrid::fsmod::FileIsOpenException>(m, "FileIsOpenException");
  py::register_exception<simgrid::fsmod::FileAlreadyExistsException>(m, "FileAlreadyExistsException");
  py::register_exception<simgrid::fsmod::DirectoryAlreadyExistsException>(m, "DirectoryAlreadyExistsException");
  py::register_exception<simgrid::fsmod::DirectoryDoesNotExistException>(m, "DirectoryDoesNotExistException");
  py::register_exception<simgrid::fsmod::TooManyOpenFilesException>(m, "TooManyOpenFilesException");
  py::register_exception<simgrid::fsmod::FileNotFoundException>(m, "FileNotFoundException");
  py::register_exception<simgrid::fsmod::InvalidSeekException>(m, "InvalidSeekException");
  py::register_exception<simgrid::fsmod::InvalidMoveException>(m, "InvalidMoveException");
  py::register_exception<simgrid::fsmod::InvalidTruncateException>(m, "InvalidTruncateException");
  py::register_exception<simgrid::fsmod::InvalidPathException>(m, "InvalidPathException");

  /* Class File */
  py::class_<File, std::shared_ptr<File>>(m, "File", "A File represents an open file in a file system")
      .def_property_readonly("access_mode", &File::get_access_mode, "The access mode of the File (read-only)")
      .def_property_readonly("path", &File::get_path, "The path of the File (read-only)")
      .def_static("num_bytes_read", &File::get_num_bytes_read, py::arg("read"),
                  "The total number of bytes read from the File")
      .def_static("num_bytes_written", &File::get_num_bytes_written, py::arg("write"),
                  "The total number of bytes written to the File")
      .def_property_readonly("file_system", &File::get_file_system, "The FileSystem this File belongs to (read-only)")
      .def_property_readonly("tell", &File::tell, "The current position of the File (read-only)")
      .def("read_async", py::overload_cast<const std::string&>(&File::read_async), py::arg("num_bytes"),
           "Asynchronously read data from the File")
      .def("read_async", py::overload_cast<sg_size_t>(&File::read_async), py::arg("num_bytes"),
           "Asynchronously read data from the File")
      .def("read", py::overload_cast<const std::string&, bool>(&File::read), py::arg("num_bytes"),
           py::arg("simulate_it") = true, "Read data from the File")
      .def("read", py::overload_cast<sg_size_t, bool>(&File::read), py::arg("num_bytes"), py::arg("simulate_it") = true,
           "Read data from the File")
      .def("write_async", py::overload_cast<const std::string&, bool>(&File::write_async), py::arg("num_bytes"),
           py::arg("detached") = false, "Asynchronously write data to the File")
      .def("write_async", py::overload_cast<sg_size_t, bool>(&File::write_async), py::arg("num_bytes"),
           py::arg("detached") = false, "Asynchronously write data to the File")
      .def("write", py::overload_cast<const std::string&, bool>(&File::write), py::arg("num_bytes"),
           py::arg("simulate_it") = true, "Write data to the File")
      .def("write", py::overload_cast<sg_size_t, bool>(&File::write), py::arg("num_bytes"),
           py::arg("simulate_it") = true, "Write data to the File")
      .def("close", &File::close, "Close the File")
      .def("seek", &File::seek, py::arg("pos"), py::arg("origin") = SEEK_SET, "Set the current position of the File")
      .def("stat", &File::stat, "Get the FileStat of the File");

  /* Class Stat*/
  py::class_<FileStat>(m, "FileStat", "Statistics about a file")
      .def(py::init<>())
      .def_readwrite("size_in_bytes", &FileStat::size_in_bytes, "The file's size in bytes")
      .def_readwrite("last_access_date", &FileStat::last_access_date, "The file's last access date")
      .def_readwrite("last_modification_date", &FileStat::last_modification_date, "The file's last modification date")
      .def_readwrite("refcount", &FileStat::refcount, "The number of times the file is currently opened");

  /* Class Partition */
  py::class_<Partition, std::shared_ptr<Partition>> partition(
      m, "Partition", "A Partition represents a partition mounted on a FileSystem");
  partition.def_property_readonly("name", &Partition::get_name, "The name of the Partition (read-only)")
      .def_property_readonly("size", &Partition::get_size, "The size of the Partition (read-only)")
      .def_property_readonly("free_space", &Partition::get_free_space,
                             "The free space available on the Partition (read-only)")
      .def_property_readonly("num_files", &Partition::get_num_files,
                             "The number of files stored on the Partition (read-only)");
  py::enum_<Partition::CachingScheme>(partition, "CachingScheme",
                                      "An enum that defines the possible caching schemes for a Partition")
      .value("NONE", Partition::CachingScheme::NONE, "No caching")
      .value("FIFO", Partition::CachingScheme::FIFO, "FIFO caching behavior")
      .value("LRU", Partition::CachingScheme::LRU, "LRU caching behavior");

  /* Class Storage */
  py::class_<Storage, std::shared_ptr<Storage>> storage(m, "Storage", "A Storage represents a storage abstraction");
  storage.def_property_readonly("name", &Storage::get_name, "The name of the Storage (read-only)")
      .def_property_readonly("controller_host", &Storage::get_controller_host,
                             "The Host that controls the Storage (read-only)")
      .def_property_readonly("controller", &Storage::get_controller, "The Actor that controls the Storage (read-only)")
      .def_property_readonly("disks", &Storage::get_disks, "The disks in the Storage (read-only)")
      .def_property_readonly("num_disks", &Storage::get_num_disks, "The number of disks in the Storage (read-only)")
      .def_property_readonly("first_disk", &Storage::get_first_disk, "The first disk in the Storage (read-only)")
      .def("disk_at", &Storage::get_disk_at, py::arg("position"), "Get the disk at the given position in the Storage")
      .def("start_controller", &Storage::start_controller, py::call_guard<py::gil_scoped_release>(), py::arg("host"), 
           py::arg("func"), "Start the controller Actor for the Storage on the given Host");

  /* Class OneDiskStorage */
  py::class_<OneDiskStorage, Storage, std::shared_ptr<OneDiskStorage>>(
      m, "OneDiskStorage", "A OneDiskStorage represents a storage with a single disk")
      .def_static("create", &OneDiskStorage::create, py::arg("name"), py::arg("disk"), "Create a new OneDiskStorage");

  /* Class OneRemoteDiskStorage */
  py::class_<OneRemoteDiskStorage, Storage, std::shared_ptr<OneRemoteDiskStorage>>(
      m, "OneRemoteDiskStorage", "A OneRemoteDiskStorage represents a storage with a single remote disk")
      .def_static("create", &OneRemoteDiskStorage::create, py::arg("name"), py::arg("disk"),
                  "Create a new OneRemoteDiskStorage");
  /* Class JBODStorage */
  py::class_<JBODStorage, Storage, std::shared_ptr<JBODStorage>> jbod(
      m, "JBODStorage", "A JBODStorage represents a storage with multiple disks");
  py::enum_<JBODStorage::RAID>(jbod, "RAID",
                               "An enum that defines the possible RAID levels that can be used by a JBODStorage")
      .value("RAID0", JBODStorage::RAID::RAID0, "RAID level 0")
      .value("RAID1", JBODStorage::RAID::RAID1, "RAID level 1")
      .value("RAID2", JBODStorage::RAID::RAID2, "RAID level 2 (unsupported)")
      .value("RAID3", JBODStorage::RAID::RAID3, "RAID level 3 (unsupported)")
      .value("RAID4", JBODStorage::RAID::RAID4, "RAID level 4")
      .value("RAID5", JBODStorage::RAID::RAID5, "RAID level 5")
      .value("RAID6", JBODStorage::RAID::RAID6, "RAID level 6");
  jbod.def_static("create", &JBODStorage::create, py::arg("name"), py::arg("disks"),
                  py::arg("raid_level") = JBODStorage::RAID::RAID0, "Create a new JBODStorage")
      .def_property_readonly("raid_level", &JBODStorage::get_raid_level,
                             "The RAID level of the JBODStorage (read-only)")
      .def("set_raid_level", &JBODStorage::set_raid_level, py::arg("raid_level"),
           "Set the RAID level of the JBODStorage");

           /* class PathUtil */
  py::class_<PathUtil>(m, "PathUtil", "Path management helper functions")
      .def_static("simplify_path_string", &PathUtil::simplify_path_string, py::arg("path"),
                  "Simplify a path string by removing redundant components")
      .def_static("remove_trailing_slashes", &PathUtil::remove_trailing_slashes, py::arg("path"),
                  "Remove trailing slashes from a path string (in-place)")
      .def_static("split_path", &PathUtil::split_path, py::arg("path"), "Split a path into directory and filename")
      .def_static("is_at_mount_point", &PathUtil::is_at_mount_point, py::arg("simplified_absolute_path"),
                  py::arg("mount_point"), "Check if a path is at a mount point")
      .def_static("path_at_mount_point", &PathUtil::path_at_mount_point, py::arg("simplified_absolute_path"),
                  py::arg("mount_point"), "Get the relative path at a mount point");

/* Class FileSystem */
  py::class_<FileSystem, std::shared_ptr<::FileSystem>>(m, "FileSystem",
                                                        "A FileSystem represents a file system abstraction")
    .def_static("create", &FileSystem::create, py::arg("name"), py::arg("max_num_open_files") = 1024,
                "Create a new FileSystem")
    .def_static("file_systems_by_actor", &FileSystem::get_file_systems_by_actor, py::arg("actor"),
                "Get all FileSystems available to the given Actor")
    .def_static("file_systems_by_netzone", &FileSystem::get_file_systems_by_netzone, py::arg("netzone"),
                "Get all FileSystems available to the given NetZone")
    .def_static("register_file_system", &FileSystem::register_file_system, py::arg("netzone"), py::arg("fs"),
                "Register a FileSystem to a NetZone")
    .def_property_readonly("name", &FileSystem::get_name, "The name of the FileSystem (read-only)")
    .def("mount_partition",
         py::overload_cast<const std::string&, std::shared_ptr<Storage>, sg_size_t, Partition::CachingScheme>(
             &FileSystem::mount_partition),
         py::arg("mount_point"), py::arg("storage"), py::arg("size"),
         py::arg("caching_scheme") = Partition::CachingScheme::NONE, "Mount a Partition on the FileSystem")
    .def("mount_partition",
         py::overload_cast<const std::string&, std::shared_ptr<Storage>, const std::string&, Partition::CachingScheme>(
             &FileSystem::mount_partition),
         py::arg("mount_point"), py::arg("storage"), py::arg("size"),
         py::arg("caching_scheme") = Partition::CachingScheme::NONE, "Mount a Partition on the FileSystem")

    .def("create_file", py::overload_cast<const std::string&, sg_size_t>(&FileSystem::create_file, py::const_),
         py::arg("full_path"), py::arg("size"), "Create a file on the FileSystem")
    .def("create_file", py::overload_cast<const std::string&, const std::string&>(&FileSystem::create_file, py::const_),
         py::arg("full_path"), py::arg("size"), "Create a file on the FileSystem")
    .def("truncate_file", &FileSystem::truncate_file, py::arg("full_path"), py::arg("size"),
         "Truncate a file on the FileSystem")
    .def("make_file_evictable", &FileSystem::make_file_evictable, py::arg("full_path"), py::arg("evictable"),
         "Make a file evictable or not")
    .def("file_exists", &FileSystem::file_exists, py::arg("full_path"), "Check whether a file exists on the FileSystem")
    .def("move_file", &FileSystem::move_file, py::arg("src_full_path"), py::arg("dst_full_path"),
         "Move a file on the FileSystem")
    .def("unlink_file", &FileSystem::unlink_file, py::arg("full_path"), "Unlink (delete) a file on the FileSystem")
    .def("create_directory", &FileSystem::create_directory, py::arg("full_dir_path"),
         "Create a directory on the FileSystem")
    .def("directory_exists", &FileSystem::directory_exists, py::arg("full_dir_path"),
         "Check whether a directory exists on the FileSystem")
    .def("unlink_directory", &FileSystem::unlink_directory, py::arg("full_dir_path"),
         "Unlink (delete) a directory on the FileSystem")
    .def("files_in_directory", &FileSystem::list_files_in_directory, py::arg("full_dir_path"),
         "List files in a directory on the FileSystem")
    .def("file_size", &FileSystem::file_size, py::arg("full_path"), "Get the size of a file on the FileSystem")
    .def("open", &FileSystem::open, py::arg("full_path"), py::arg("access_mode"), "Open a file on the FileSystem")
    .def_property_readonly("partitions", &FileSystem::get_partitions, "Get all partitions mounted on the FileSystem")
    .def("partition_by_name", &FileSystem::partition_by_name, py::arg("name"),
         "Get a Partition by its name (throws if not found)")
    .def("partition_by_name_or_null", &FileSystem::partition_by_name_or_null, py::arg("name"),
         "Get a Partition by its name (returns None if not found)")
    .def("partition_for_path_or_null", &FileSystem::get_partition_for_path_or_null, py::arg("full_path"),
         "Get the Partition that contains the given path (returns None if not found)")
    .def("free_space_at_path", &FileSystem::get_free_space_at_path, py::arg("full_path"),
         "Get the free space available at the given path");
}
