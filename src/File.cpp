#include "File.hpp"
#include <simgrid/s4u/Engine.hpp>

namespace simgrid::module::fs {

    /**
     * @brief Write data to the file
     * @param num_bytes: the number of bytes to write
     */
    void File::write(sg_size_t num_bytes, bool simulate_it) {

        // Check whether there is enough space
        sg_size_t added_bytes = std::max<sg_size_t >(0, current_position_ + num_bytes - metadata_->get_current_size());
        if (added_bytes > partition_->get_free_space()) {
            throw std::runtime_error("EXCEPTION"); // TODO
        }
        sg_size_t new_file_size_if_i_succeed = metadata_->get_current_size() + added_bytes;

        // Do the I/O simulation if need be
        if (simulate_it) {
            partition_->get_storage()->write(num_bytes);
        }

        // Update the file size if needed
        if (new_file_size_if_i_succeed > metadata_->get_current_size()) {
            if (metadata_->get_current_size() < new_file_size_if_i_succeed) {
                metadata_->set_current_size(new_file_size_if_i_succeed);
            }
        }

        // Update
        metadata_->set_access_date(simgrid::s4u::Engine::get_clock());
        metadata_->set_modification_date(simgrid::s4u::Engine::get_clock());


    }
}