#include <iostream>
#include "../include/FileSystem.hpp"

#define KB 1000

int main() {

    std::cerr << "Creating a file system...\n";
    auto fs = new simgrid::module::fs::FileSystem(1024);
    std::cerr << "Mounting a 100KB partition...\n";
    fs->mount_partition("/dev/bogus/", 100*KB);
    std::cerr << "Creating a 10KB file...\n";
    fs->create("/dev/bogus/file.txt", 10*KB);
    std::cerr << "The file size it: " << fs->size("/dev/bogus/../bogus/file.txt") << "\n";
    std::cerr << "Opening the file...\n";
    auto file = fs->open("/dev/bogus/file.txt");
    std::cerr << "Seek to offset 5KB...\n";
    file->seek(5*KB);
    std::cerr << "Writing 6GB to it at offset 0...\n";
    file->write(6*KB, false);
    std::cerr << "Closing file...\n";
    file->close();
    std::cerr << "The file size it: " << fs->size("/dev/bogus/../bogus/file.txt") << "\n";


}
