#include <iostream>
#include "../include/FileSystem.hpp"

int main() {

    std::cerr << "Creating a file system...\n";
    auto fs = new simgrid::module::fs::FileSystem(1024);
    std::cerr << "Mounting a 100kB partition...\n";
    fs->mount_partition("/dev/bogus/", "100kB");
    std::cerr << "Creating a 10kB file...\n";
    fs->create("/dev/bogus/file.txt", "10kB");
    std::cerr << "The file size it: " << fs->size("/dev/bogus/../bogus/file.txt") << "\n";
    std::cerr << "Opening the file...\n";
    auto file = fs->open("/dev/bogus/file.txt");
    std::cerr << "Seek to offset 5kB...\n";
    file->seek(5000);
    std::cerr << "Writing 6kB to it at offset 0...\n";
    file->write("6kB", false);
    std::cerr << "Closing file...\n";
    file->close();
    std::cerr << "The file size it: " << fs->size("/dev/bogus/../bogus/file.txt") << "\n";


}
