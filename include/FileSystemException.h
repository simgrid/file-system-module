#ifndef FSMOD_FILESYSTEMEXCEPTION_H
#define FSMOD_FILESYSTEMEXCEPTION_H

#include <simgrid/Exception.hpp>

// Macro copied over from SimGrid source code. Is it a good idea?
#define DECLARE_SIMGRID_EXCEPTION(AnyException, ...)                                                                   \
  class AnyException : public simgrid::Exception {                                                                     \
  public:                                                                                                              \
    using simgrid::Exception::Exception;                                                                               \
    __VA_ARGS__                                                                                                        \
    ~AnyException() override;                                                                                          \
    XBT_ATTRIB_NORETURN void rethrow_nested(simgrid::xbt::ThrowPoint&& throwpoint,                                     \
                                            const std::string& message) const override                                 \
    {                                                                                                                  \
      std::throw_with_nested(AnyException(std::move(throwpoint), message));                                            \
    }                                                                                                                  \
  }


namespace simgrid::module::fs {
    DECLARE_SIMGRID_EXCEPTION(FileSystemException);
    FileSystemException::~FileSystemException() = default;
}

#endif //FSMOD_FILESYSTEMEXCEPTION_H
