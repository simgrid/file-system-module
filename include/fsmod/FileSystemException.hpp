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

namespace simgrid::fsmod {

    /**
     * @brief The exception class for fsmod
     */
    class FileSystemException : public std::exception {
        std::string msg_;
    public:
        FileSystemException(simgrid::xbt::ThrowPoint&& throwpoint, const std::string &msg) {
            msg_ = msg;
        }

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
