#include <iostream>
#include <simgrid/Exception.hpp>

#include "fsmod/CachingStrategy.hpp"
#include "fsmod/FileSystemException.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(fsmod_caching_strategy_none, "File System module: None caching strategy");

namespace simgrid::module::fs {

    void CachingStrategyNone::create_space(sg_size_t num_bytes) {
        throw FileSystemException(XBT_THROW_POINT, "Not enough space");
    }

}