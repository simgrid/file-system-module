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
        explicit CachingStrategy(Partition *partition) : partition_(partition) {}
        virtual ~CachingStrategy() {};
        virtual void create_space(sg_size_t num_bytes) {};
        virtual void file_creation(FileMetadata *file_metadata) {};
        virtual void file_access(FileMetadata *file_metadata) {};

    private:
        Partition *partition_;
    };

    class XBT_PUBLIC CachingStrategyNone : public CachingStrategy {
    public:
        explicit CachingStrategyNone(Partition *partition) : CachingStrategy(partition) {};
        void create_space(sg_size_t num_bytes) override;
        void file_creation(FileMetadata *file_metadata) override {};
        void file_access(FileMetadata *file_metadata) override {};
    private:
    };

    class XBT_PUBLIC CachingStrategyFIFO : public CachingStrategy {
    public:
        explicit CachingStrategyFIFO(Partition *partition);
        void create_space(sg_size_t num_bytes) override;
        void file_creation(FileMetadata *file_metadata) override ;
        void file_access(FileMetadata *file_metadata) override {};
    private:

    };

    class XBT_PUBLIC CachingStrategyLRU : public CachingStrategy {
    public:
        explicit CachingStrategyLRU(Partition *partition);
        void create_space(sg_size_t num_bytes) override;
        void file_creation(FileMetadata *file_metadata) override {};
        void file_access(FileMetadata *file_metadata) override;
    private:

    };

    /** \endcond **/



} // namespace simgrid::module::fs


#endif