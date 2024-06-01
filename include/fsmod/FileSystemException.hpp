#ifndef FSMOD_FILESYSTEMEXCEPTION_HPP
#define FSMOD_FILESYSTEMEXCEPTION_HPP

/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>

#define DECLARE_FSMOD_EXCEPTION(AnyException, msg_prefix, ...)                                                         \
  class AnyException : public simgrid::Exception {                                                                     \
  public:                                                                                                              \
    using simgrid::Exception::Exception;                                                                               \
    __VA_ARGS__                                                                                                        \
    AnyException(simgrid::xbt::ThrowPoint&& throwpoint, const std::string& message) :                                  \
        simgrid::Exception(std::move(throwpoint), std::string(msg_prefix) + message) {}                                \
    ~AnyException() override = default;                                                                                \
    XBT_ATTRIB_NORETURN void rethrow_nested(simgrid::xbt::ThrowPoint&& throwpoint,                                     \
                                            const std::string& message) const override                                 \
    {                                                                                                                  \
      std::string augmented_message = std::string("CRAP: ") + message;                                                 \
      std::throw_with_nested(AnyException(std::move(throwpoint), augmented_message));                                  \
    }                                                                                                                  \
  }


namespace simgrid::fsmod {

    DECLARE_FSMOD_EXCEPTION(Exception, "FSMod Exception: "); // TODO: REMOVE EVENTUALLY
    DECLARE_FSMOD_EXCEPTION(FileNotFoundException, "File not found: ");
}

#endif //FSMOD_FILESYSTEMEXCEPTION_HPP
