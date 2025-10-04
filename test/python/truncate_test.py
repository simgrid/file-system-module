# Copyright (c) 2025. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import sys
import multiprocessing
from simgrid import Engine, this_actor
from fsmod import FileSystem, OneDiskStorage, InvalidTruncateException

def setup_platform():
    e = Engine(sys.argv)
    e.set_log_control("no_loc")
    e.set_log_control("root.thresh:critical")

    # Creating a platform with one host and one disk...
    zone = e.netzone_root.add_netzone_full("zone")
    host = zone.add_host("my_host", "100Gf")
    disk = host.add_disk("disk", "1kBps", "2kBps")
    zone.seal()

    # Creating a one-disk storage on the host's disk..."
    ods = OneDiskStorage.create("my_storage", disk)
    # Creating a file system
    fs = FileSystem.create("my_fs")
    # Mounting a 100MB partition
    fs.mount_partition("/dev/a/", ods, "1MB")

    return e, host, fs

def run_test_truncate_and_tell():
    e, host, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 100kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "100kB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Check current position, should be 100k ('append' mode)")
        assert file.tell == 100*1000
        this_actor.info("Try to truncate an opened file, which should fail")
        try:
            fs.truncate_file("/dev/a/foo.txt", 50*1000)
            assert False, "Expected InvalidTruncateException was not raised"
        except InvalidTruncateException:
            pass
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Truncate the file to half its size")
        fs.truncate_file("/dev/a/foo.txt", 50*1000)
        this_actor.info("Check file size")
        fs.file_size("/dev/a/foo.txt") == 50*1000
        this_actor.info("Check free space")
        fs.partitions[0].free_space == 1000*1000 - 50*1000
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Write 1kB")
        file.write(1000, False)
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Check file size")
        fs.file_size("/dev/a/foo.txt") ==  51*1000
        this_actor.info("Check free space")
        fs.partitions[0].free_space == 1000*1000 - 51000
 
    host.add_actor("TestActor", test_actor)
    e.run()


if __name__ == '__main__':
    tests = [
      run_test_truncate_and_tell
    ]

    for test in tests:
        print(f"\n🔧 Run {test.__name__} ...")
        p = multiprocessing.Process(target=test)
        p.start()
        p.join()

        if p.exitcode != 0:
            print(f"❌ {test.__name__} failed with exit code {p.exitcode}")
        else:
            print(f"✅ {test.__name__} passed")
