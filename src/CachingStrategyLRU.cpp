#include <iostream>

#include "fsmod/CachingStrategy.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_caching_strategy_lru, "File System module: LRU caching strategy");

namespace simgrid::module::fs {

    CachingStrategyLRU::CachingStrategyLRU(Partition *partition) {

    }

    void CachingStrategyLRU::create_space(sg_size_t num_bytes) {

    }

}