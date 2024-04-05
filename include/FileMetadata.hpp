#ifndef SIMGRID_MODULE_FS_FILEMETADATA_H_
#define SIMGRID_MODULE_FS_FILEMETADATA_H_

#include <simgrid/forward.h>

namespace simgrid::module::fs {

    class XBT_PUBLIC FileMetadata {
        sg_size_t current_size_;
        sg_size_t future_size_;
        double modification_date_ = 0.0;
        double access_date_ = 0.0;
        unsigned int num_current_writes = 0;
    public:

    };

} // namespace

#endif