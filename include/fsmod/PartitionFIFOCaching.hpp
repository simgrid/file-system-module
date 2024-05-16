#ifndef SIMGRID_MODULE_FS_PARTITION_FIFO_CACHING_H_
#define SIMGRID_MODULE_FS_PARTITION_FIFO_CACHING_H_

#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>

#include "fsmod/Partition.hpp"
#include "fsmod/FileMetadata.hpp"

namespace simgrid::module::fs {


    /** \cond EXCLUDE_FROM_DOCUMENTATION    */

    class Storage;

    class XBT_PUBLIC PartitionFIFOCaching : public Partition {

    public:
        PartitionFIFOCaching(std::string name, std::shared_ptr<Storage> storage, sg_size_t size) :
                Partition(std::move(name), std::move(storage), size) {}

    protected:

        // Methods to perform caching
        void create_space(sg_size_t num_bytes) override;
        void new_file_creation_event(FileMetadata *file_metadata) override;
        void new_file_access_event(FileMetadata *file_metadata) override;
        void new_file_deletion_event(FileMetadata *file_metadata) override;

    private:
        unsigned long sequence_number_ = 0;
        std::map<unsigned long, FileMetadata*> fifo_list_;

    };

    /** \endcond    */


} // namespace simgrid::module::fs

#endif