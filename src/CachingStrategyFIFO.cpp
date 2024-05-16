#include <iostream>

#include "fsmod/CachingStrategy.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_caching_strategy_fifo, "File System module: FIFO caching strategy");

namespace simgrid::module::fs {

    CachingStrategyFIFO::CachingStrategyFIFO(Partition *partition) : CachingStrategy(partition) {

    }

    void CachingStrategyFIFO::create_space(sg_size_t num_bytes) {
        throw std::runtime_error("CachingStrategyFIFO::create_space(): Not implemented");
    }

    void CachingStrategyFIFO::file_creation(simgrid::module::fs::FileMetadata *file_metadata) {
        throw std::runtime_error("CachingStrategyFIFO::file_creation(): Not implemented");
    }

}