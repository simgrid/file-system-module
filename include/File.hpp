#ifndef SIMGRID_MODULE_FS_FILE_H_
#define SIMGRID_MODULE_FS_FILE_H_

#include <simgrid/forward.h>
#include <xbt/parse_units.hpp>

#include "FileMetadata.hpp"
#include "Partition.hpp"

namespace simgrid::module::fs {

    class XBT_PUBLIC File {
        friend class FileSystem;

        std::string path_;
        sg_size_t current_position_ = SEEK_SET;
        int desc_id     = 0;
        FileMetadata* metadata_;
        Partition* partition_;

        void update_current_position(sg_offset_t pos);
    public:
        virtual ~File() = default;

    protected:
        File(const std::string& fullpath, FileMetadata *metadata, Partition *partition) :
                path_(fullpath), metadata_(metadata), partition_(partition) {};
        File(const File&) = delete;
        File& operator=(const File&) = delete;

    public:

        //void read(const std::string& num_bytes);
        void read(sg_size_t num_bytes);
        void write(const std::string& num_bytes, bool simulate_it=true);
        void write(sg_size_t num_bytes, bool simulate_it=true);

        void append(sg_size_t num_bytes);
        void truncate(sg_size_t num_bytes);

        void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
        void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
        /** Retrieves the current file position */
        sg_size_t tell() const {return current_position_; }

        void close();

        void dump() const;
    };

} // namespace simgrid::module::fs


#endif