#ifndef SIMGRID_MODULE_FS_FILE_H_
#define SIMGRID_MODULE_FS_FILE_H_

#include <simgrid/forward.h>

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

    public:
        virtual ~File() = default;

    protected:
        File(const std::string& fullpath, FileMetadata *metadata, Partition *partition) :
                path_(fullpath), metadata_(metadata), partition_(partition) {};
        File(const File&) = delete;
        File& operator=(const File&) = delete;

    public:

        void read(sg_size_t size);
        void write(sg_size_t size);

        void append(sg_size_t size);
        void truncate(sg_size_t to_size);

        void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
        void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
        sg_size_t tell() const;                 /** Retrieves the current file position */

        void close();

        void dump() const;
    };

} // namespace simgrid::module::fs


#endif