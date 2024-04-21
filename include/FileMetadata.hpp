#ifndef SIMGRID_MODULE_FS_FILEMETADATA_H_
#define SIMGRID_MODULE_FS_FILEMETADATA_H_

#include <unordered_map>
#include <simgrid/forward.h>
#include <iostream>

namespace simgrid::module::fs {

    /** \cond EXCLUDE_FROM_DOCUMENTATION    */

    class XBT_PUBLIC FileMetadata {
        sg_size_t current_size_;
        sg_size_t future_size_;
        double modification_date_ = 0.0;
        double access_date_ = 0.0;
        std::unordered_map<int, sg_size_t> ongoing_writes_;
        unsigned file_refcount_ = 0;

    public:
        explicit FileMetadata(sg_size_t initial_size) : current_size_(initial_size), future_size_(initial_size) {}
        [[nodiscard]] sg_size_t get_current_size() const { return current_size_; }
        void set_current_size(sg_size_t num_bytes) { current_size_ = num_bytes; }
        [[nodiscard]] sg_size_t get_future_size() const { return future_size_; }
        [[nodiscard]] double get_modification_date() const { return modification_date_; }
        void set_modification_date(double date) { modification_date_ = date; }
        [[nodiscard]] double get_access_date() const { return access_date_; }
        void set_access_date(double date) { access_date_ = date; }
        unsigned get_file_refcount() { return file_refcount_; }
        void increase_file_refcount() { file_refcount_++; }
        void decrease_file_refcount() { file_refcount_--; }

        void notify_write_start(int write_id, sg_size_t new_size) {
            ongoing_writes_[write_id] = new_size;
            future_size_ = std::max(new_size, future_size_);
        }

        void notify_write_end(int write_id) {
            current_size_ = ongoing_writes_.at(write_id);
            ongoing_writes_.erase(write_id);
        }
    };

    /** \endcond **/


} // namespace

#endif