# Copyright (c) 2025. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import sys
import multiprocessing
from simgrid import Engine, this_actor
from fsmod import FileSystem, OneDiskStorage, FileNotFoundException, InvalidSeekException

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
    # Mounting a 1MB partition
    fs.mount_partition("/dev/a/", ods, "1MB")

    return e, host, fs

def run_test_seek_and_tell():
    e, host, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 100kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "100kB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Check current position, should be 100k ('append' mode)")
        assert file.tell == 100*1000
        this_actor.info("Seek back to beginning of file")
        file.seek(io.SEEK_SET)
        this_actor.info("Check current position, should be 0")
        assert file.tell == 0
        this_actor.info("Seek 1kB forward")
        file.seek(1000)
        this_actor.info("Check current position, should be 1k")
        assert file.tell == 1000
        this_actor.info("Try to seek before the beginning of the file, should not work.")
        try:
            file.seek(-1000, io.SEEK_SET)
            assert False, "Expected InvalidSeekException was not raised"
        except InvalidSeekException:
            pass
        this_actor.info("Try to seek backwards from the end of the file, should work")
        file.seek(-1000, io.SEEK_END)
        this_actor.info("Check current position, should be 99k")
        assert file.tell == 99*1000
        this_actor.info("Seek backwards from the current position in file, should work")
        file.seek(-1000, io.SEEK_CUR)
        this_actor.info("Check current position, should be 98k")
        assert file.tell == 98*1000
        this_actor.info("Try to seek beyond the end of the file, should work.")
        file.seek(5000, io.SEEK_CUR)
        this_actor.info("Check current position, should be 103k")
        assert file.tell == 103*1000
        this_actor.info("Check file size, it should still be 100k")
        assert fs.file_size("/dev/a/foo.txt") == 100000
        this_actor.info("Write 1kB to /dev/a/foo.txt")
        file.write("1kB")
        this_actor.info("Check file size, it should now be 104k")
        assert fs.file_size("/dev/a/foo.txt") == 104000
        this_actor.info("Seek from an arbitrary position in file, should work")
        file.seek(1000, 2000)
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Checking file size of a non-existing file, which should fail")
        try:
            fs.file_size("/dev/a/foo_bogus.txt")
            assert False, "Expected FileNotFoundException was not raised"
        except FileNotFoundException:
            pass
    host.add_actor("TestActor", test_actor)
    e.run()


if __name__ == '__main__':
    tests = [
      run_test_seek_and_tell
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
