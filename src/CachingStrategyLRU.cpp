#include <iostream>

#include "fsmod/CachingStrategy.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_caching_strategy_lru, "File System module: LRU caching strategy");

namespace simgrid::module::fs {

    CachingStrategyLRU::CachingStrategyLRU(Partition *partition) : CachingStrategy(partition) {

    }

    void CachingStrategyLRU::create_space(sg_size_t num_bytes) {
        throw std::runtime_error("CachingStrategyLRU::create_space(): Not implemented");

    }

    void CachingStrategyLRU::file_access(FileMetadata *file_metadata) {
        throw std::runtime_error("CachingStrategyLRU::create_space(): Not implemented");
    }

}