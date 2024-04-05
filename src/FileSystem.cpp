#include "FileSystem.hpp"
#include "File.hpp"

namespace simgrid::module::fs {

    /**
     * @brief Open a file
     * @param name
     * @param fullpath
     * @return
     */
    File *FileSystem::open(const std::string &fullpath) {
        // Get a file descriptor
        this->available_file_descriptors.pop_back();
        if (this->available_file_descriptors.empty()) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }
        auto filed = this->available_file_descriptors.back();
        this->available_file_descriptors.pop_back();

        // Create the file object



    }

};