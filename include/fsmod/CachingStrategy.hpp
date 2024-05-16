#ifndef SIMGRID_MODULE_FS_CACHING_STRATEGY_H_
#define SIMGRID_MODULE_FS_CACHING_STRATEGY_H_

#include <simgrid/forward.h>
#include <simgrid/s4u/Io.hpp>
#include <utility>
#include <xbt/parse_units.hpp>

#include "FileMetadata.hpp"
#include "FileStat.hpp"

namespace simgrid::module::fs {

    /** \cond EXCLUDE_FROM_DOCUMENTATION */
    class Partition; // forward

    class XBT_PUBLIC CachingStrategy {
    public:
        virtual void create_space(sg_size_t num_bytes) = 0;
        virtual ~CachingStrategy() = default;
    };


    class XBT_PUBLIC CachingStrategyFIFO : public CachingStrategy {
    public:
        CachingStrategyFIFO(Partition *partition);
        void create_space(sg_size_t num_bytes) override;
    private:

    };

    class XBT_PUBLIC CachingStrategyLRU : public CachingStrategy {
    public:
        CachingStrategyLRU(Partition *partition);
        void create_space(sg_size_t num_bytes) override;
    private:

    };

    /** \endcond **/



} // namespace simgrid::module::fs


#endif