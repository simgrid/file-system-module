#ifndef FSMOD_FILESYSTEMEXCEPTION_HPP
#define FSMOD_FILESYSTEMEXCEPTION_HPP

#include <simgrid/Exception.hpp>

// Macro copied over from SimGrid source code. Is it a good idea?
//#define DECLARE_SIMGRID_EXCEPTION(AnyException, ...)                                                                   \
//  class AnyException : public simgrid::Exception {                                                                     \
//  public:                                                                                                              \
//    using simgrid::Exception::Exception;                                                                               \
//    __VA_ARGS__                                                                                                        \
//    ~AnyException() override;                                                                                          \
//    XBT_ATTRIB_NORETURN void rethrow_nested(simgrid::xbt::ThrowPoint&& throwpoint,                                     \
//                                            const std::string& message) const override                                 \
//    {                                                                                                                  \
//      std::throw_with_nested(AnyException(std::move(throwpoint), message));                                            \
//    }                                                                                                                  \
//  }

namespace simgrid::module::fs {

class FileSystemException : public std::exception {
    std::string msg_;
public:
    FileSystemException(simgrid::xbt::ThrowPoint&& throwpoint, const std::string &msg) {

    }

    virtual const char *what() const throw() {
        // Without the strdup() below, we get some valgrind warnings...
        return strdup(msg_.c_str());
    }
    ~FileSystemException() = default;
};
}

#endif //FSMOD_FILESYSTEMEXCEPTION_HPP
