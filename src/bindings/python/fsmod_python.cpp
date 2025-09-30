/* Copyright (c) 2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <fsmod/FileSystem.hpp>
#include <fsmod/File.hpp>
#include <fsmod/FileMetadata.hpp>
#include <fsmod/FileStat.hpp>
#include <fsmod/FileSystemException.hpp>
#include <fsmod/Partition.hpp>
#include <fsmod/PartitionFIFOCaching.hpp>
#include <fsmod/PartitionLRUCaching.hpp>
#include <fsmod/Storage.hpp>
#include <fsmod/JBODStorage.hpp>
#include <fsmod/OneDiskStorage.hpp>
#include <fsmod/OneRemoteDiskStorage.hpp>

#include <xbt/log.h>

namespace py = pybind11;
using simgrid::fsmod::File;
using simgrid::fsmod::FileMetadata;
using simgrid::fsmod::FileStat;
using simgrid::fsmod::Partition;
using simgrid::fsmod::PartitionFIFOCaching;
using simgrid::fsmod::PartitionLRUCaching;
using simgrid::fsmod::Storage;
using simgrid::fsmod::JBODStorage;
using simgrid::fsmod::OneDiskStorage;
using simgrid::fsmod::OneRemoteDiskStorage;

XBT_LOG_NEW_DEFAULT_CATEGORY(python, "python");

namespace {
// std::string get_fsmod_version()
// {
//   int major;
//   int minor;
//   int patch;
//   fsmod_version_get(&major, &minor, &patch);
//   return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
// }

PYBIND11_MODULE(fsmod, m)
{
    m.doc() = "FSMOD userspace API";
    
    // m.attr("fsmod_version") = get_fsmod_version();
    
    py::register_exception<simgrid::fsmod::NotEnoughSpaceException>(m, "NotEnoughSpaceException");
    py::register_exception<simgrid::fsmod::FileIsOpenException>(m, "FileIsOpenException");
    py::register_exception<simgrid::fsmod::DirectoryAlreadyExistsException>(m, "DirectoryAlreadyExistsException");
    py::register_exception<simgrid::fsmod::DirectoryDoesNotExistException>(m, "DirectoryDoesNotExistException");
    py::register_exception<simgrid::fsmod::TooManyOpenFilesException>(m, "TooManyOpenFilesException");
    py::register_exception<simgrid::fsmod::FileNotFoundException>(m, "FileNotFoundException");
    py::register_exception<simgrid::fsmod::InvalidSeekException>(m, "InvalidSeekException");
    py::register_exception<simgrid::fsmod::InvalidMoveException>(m, "InvalidPathException");
    py::register_exception<simgrid::fsmod::InvalidTruncateException>(m, "InvalidTruncateException");
    py::register_exception<simgrid::fsmod::InvalidPathException>(m, "InvalidPathException"); 
    
    /* Class File */
    py::class_<File, std::shared_ptr<File>>(m, "File", "A File represents an open file in a file system")
        .def_property_readonly("access_mode", &File::get_access_mode, "The access mode of the File (read-only)")
        .def_property_readonly("path", &File::get_path, "The path of the File (read-only)")
        .def_property_readonly("num_bytes_read", &File::get_num_bytes_read, "The total number of bytes read from the File (read-only)")
        .def_property_readonly("num_bytes_written", &File::get_num_bytes_written, "The total number of bytes written to the File (read-only)") 
        .def_property_readonly("file_system", &File::get_file_system, "The FileSystem this File belongs to (read-only)")
        .def_property_readonly("tell", &File::tell, "The current position of the File (read-only)")
        .def("read_async", py::overload_cast<const std::string&>(&File::read_async), py::arg("num_bytes"), "Asynchronously read data from the File")
        .def("read_async", py::overload_cast<sg_size_t>(&File::read_async), py::arg("num_bytes"), "Asynchronously read data from the File")
        .def("read", py::overload_cast<const std::string&, bool>(&File::read), py::arg("num_bytes"), py::arg("simulate_it") = true,
             "Read data from the File")
        .def("read", py::overload_cast<sg_size_t, bool>(&File::read), py::arg("num_bytes"), py::arg("simulate_it") = true,
             "Read data from the File")
        .def("write_async", py::overload_cast<const std::string&>(&File::write_async), py::arg("num_bytes"), "Asynchronously write data to the File")
        .def("write_async", py::overload_cast<sg_size_t>(&File::write_async), py::arg("num_bytes"), "Asynchronously write data to the File")
        .def("write", py::overload_cast<const std::string&, bool>(&File::write), py::arg("num_bytes"), py::arg("simulate_it") = true,
             "Write data to the File")
        .def("write", py::overload_cast<sg_size_t, bool>(&File::write), py::arg("num_bytes"), py::arg("simulate_it") = true,
             "Write data to the File")
        .def("close", &File::close, "Close the File")
        .def("seek", &File::seek, py::arg("pos"), py::  arg("origin") = SEEK_SET, "Set the current position of the File")
        .def("stat", &File::stat, "Get the FileStat of the File");
}
} // namespace
