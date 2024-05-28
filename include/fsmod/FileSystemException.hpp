#ifndef FSMOD_FILESYSTEMEXCEPTION_HPP
#define FSMOD_FILESYSTEMEXCEPTION_HPP

/* Copyright (c) 2024. The FSMOD Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>

namespace simgrid::fsmod {

    /**
     * @brief The exception class for fsmod
     */
    class FileSystemException : public std::exception {
        std::string msg_;
    public:
        FileSystemException(simgrid::xbt::ThrowPoint&& /*throwpoint*/, std::string msg) : msg_(std::move(msg)){}

        /**
         * @brief Retrieves the exception's human-readable message
         * @return a message as a string
         */
        [[nodiscard]] const char *what() const noexcept override {
            // Without the strdup() below, we get some valgrind warnings...
            return strdup(msg_.c_str());
        }
        ~FileSystemException() override = default;
    };
}

#endif //FSMOD_FILESYSTEMEXCEPTION_HPP
