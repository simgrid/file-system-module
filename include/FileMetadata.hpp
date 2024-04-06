#ifndef SIMGRID_MODULE_FS_FILEMETADATA_H_
#define SIMGRID_MODULE_FS_FILEMETADATA_H_

#include <simgrid/forward.h>

namespace simgrid::module::fs {

    class XBT_PUBLIC FileMetadata {
        sg_size_t current_size_;
        sg_size_t future_size_;
        double modification_date_ = 0.0;
        double access_date_ = 0.0;
        unsigned int num_current_writes_ = 0;
    public:
        explicit FileMetadata(sg_size_t initial_size) : current_size_(initial_size), future_size_(initial_size) {}
        [[nodiscard]] sg_size_t get_current_size() const { return current_size_;}
        [[nodiscard]] sg_size_t get_future_size() const { return current_size_;}
        void set_future_size(sg_size_t num_bytes) { future_size_ = num_bytes;}
        void set_current_size(sg_size_t num_bytes) { current_size_ = num_bytes;}
        [[nodiscard]] double get_modification_date() const { return modification_date_; }
        void set_modification_date(double date) { modification_date_ = date; }
        [[nodiscard]] double get_access_date() const { return access_date_; }
        void set_access_date(double date) { access_date_ = date; }
        void increment_num_current_writes() { num_current_writes_ ++;}
        void decrement_num_current_writes() { num_current_writes_ --;}
    };

} // namespace

#endif