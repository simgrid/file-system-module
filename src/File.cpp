#include "File.hpp"

namespace simgrid::module::fs {

    std::shared_ptr<File> File::createInstance(const std::string &fullpath,
                                               simgrid::module::fs::FileMetadata *metadata,
                                               simgrid::module::fs::Partition *partition) {
        return std::shared_ptr<File>(new File(fullpath, metadata, partition));
    }
}