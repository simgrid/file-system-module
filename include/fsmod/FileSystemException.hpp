#ifndef FSMOD_FILESYSTEMEXCEPTION_HPP
#define FSMOD_FILESYSTEMEXCEPTION_HPP

/* Copyright (c) 2024-2025. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>

#define DECLARE_FSMOD_EXCEPTION(AnyException, msg_prefix, ...)                                                         \
  class AnyException : public simgrid::Exception {                                                                     \
  public:                                                                                                              \
    using simgrid::Exception::Exception;                                                                               \
    __VA_ARGS__                                                                                                        \
    AnyException(simgrid::xbt::ThrowPoint&& throwpoint, const std::string& message) :                                  \
        simgrid::Exception(std::move(throwpoint),                                                                      \
            std::string(msg_prefix) + (message.empty() ? "" : std::string(": " + message))) {}                                \
    explicit AnyException(simgrid::xbt::ThrowPoint&& throwpoint) :  AnyException(std::move(throwpoint), "") {}                  \
    ~AnyException() override = default;                                                                                \
    XBT_ATTRIB_NORETURN void rethrow_nested(simgrid::xbt::ThrowPoint&& throwpoint,                                     \
                                            const std::string& message) const override                                 \
    {                                                                                                                  \
      std::string augmented_message = std::string(msg_prefix) + message;                                                 \
      std::throw_with_nested(AnyException(std::move(throwpoint), augmented_message));                                  \
    }                                                                                                                  \
  }


namespace simgrid::fsmod {

    // All these exceptions derive from simgrid::Exception (but are in the name space simgrid::fsmod namespace)
    DECLARE_FSMOD_EXCEPTION(FileNotFoundException, "File not found");
    DECLARE_FSMOD_EXCEPTION(NotEnoughSpaceException, "Not enough space");
    DECLARE_FSMOD_EXCEPTION(FileIsOpenException, "Operation not permitted on an opened file");
    DECLARE_FSMOD_EXCEPTION(DirectoryAlreadyExistsException, "Directory already exists");
    DECLARE_FSMOD_EXCEPTION(DirectoryDoesNotExistException, "Directory does not exist");
    DECLARE_FSMOD_EXCEPTION(TooManyOpenFilesException, "Too many open files");
    DECLARE_FSMOD_EXCEPTION(FileAlreadyExistsException, "File already exists");
    DECLARE_FSMOD_EXCEPTION(InvalidSeekException, "Invalid seek");
    DECLARE_FSMOD_EXCEPTION(InvalidMoveException, "Invalid move");
    DECLARE_FSMOD_EXCEPTION(InvalidTruncateException, "Invalid truncate");
    DECLARE_FSMOD_EXCEPTION(InvalidPathException, "Invalid path");
}

#endif //FSMOD_FILESYSTEMEXCEPTION_HPP
