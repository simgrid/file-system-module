#include <iostream>
#include "../include/FileSystem.hpp"

#define KB 1000

int main() {

    std::cerr << "Creating a file system...\n";
    auto fs = new simgrid::module::fs::FileSystem(1024);
    std::cerr << "Mounting a 100KB partition...\n";
    fs->mount_partition("/dev/bogus/", 100*KB);
    std::cerr << "Opening a file...\n";
    auto file = fs->open("/dev/bogus/file.txt");
    std::cerr << "Writing 1GB to it...\n";
    file->write(10*KB, false);
    std::cerr << "Closing file...\n";
    file->close();

}
