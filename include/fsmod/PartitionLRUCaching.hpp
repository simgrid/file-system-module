#ifndef SIMGRID_MODULE_FS_PARTITION_LRU_CACHING_H_
#define SIMGRID_MODULE_FS_PARTITION_LRU_CACHING_H_

#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include "fsmod/PartitionFIFOCaching.hpp"
#include "fsmod/FileMetadata.hpp"

namespace simgrid::module::fs {

    /** \cond EXCLUDE_FROM_DOCUMENTATION    */

    class Storage;

    class XBT_PUBLIC PartitionLRUCaching : public PartitionFIFOCaching {

    public:
        PartitionLRUCaching(std::string name, std::shared_ptr<Storage> storage, sg_size_t size) :
                PartitionFIFOCaching(std::move(name), std::move(storage), size) {}

    protected:
        // Methods to perform caching
        void new_file_access_event(FileMetadata *file_metadata) override;
    };

    /** \endcode     */

} // namespace simgrid::module::fs

#endif