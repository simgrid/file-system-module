# Copyright (c) 2025. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import sys
import multiprocessing
from simgrid import Engine, this_actor, ActivitySet, StorageFailureException
from fsmod import FileSystem, OneDiskStorage, File, NotEnoughSpaceException

def setup_platform():
    e = Engine(sys.argv)
    e.set_log_control("no_loc")
    e.set_log_control("root.thresh:critical")

    # Creating a platform with one host and one disk...
    zone = e.netzone_root.add_netzone_full("zone")
    host = zone.add_host("my_host", "100Gf")
    disk = host.add_disk("disk", "2MBps", "1MBps")
    zone.seal()

    # Creating a one-disk storage on the host's disk..."
    ods = OneDiskStorage.create("my_storage", disk)
    # Creating a file system
    fs = FileSystem.create("my_fs")
    # Mounting a 100MB partition
    fs.mount_partition("/dev/a/", ods, "100MB")

    return e, host, disk, fs

def run_test_single_read():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10kB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "r")
        this_actor.info("read 0B at /dev/a/foo.txt, which should return 0")
        assert file.read(0) == 0
        this_actor.info("read 100kB at /dev/a/foo.txt, which should return only 10kB")
        assert file.read("100kB") == 10000
        this_actor.info("read 10kB at /dev/a/foo.txt, which should return O as we are at the end of the file")
        assert file.read("10kB") == 0
        this_actor.info("Seek back to SEEK_SET and read 9kB at /dev/a/foo.txt")
        file.seek(io.SEEK_SET)
        assert file.read("9kB") == 9000
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_single_async_read():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "r")
        this_actor.info("Asynchronously read 4MB at /dev/a/foo.txt")
        my_read = file.read_async("4MB")
        this_actor.info("Sleep for 1 second")
        this_actor.sleep_for(1)
        this_actor.info("Sleep complete. Clock should be at 1s")
        assert Engine.clock == 1
        this_actor.info("Wait for read completion")
        my_read.wait()
        this_actor.info("Read complete. Clock should be at 2s")
        assert Engine.clock == 2
        assert File.num_bytes_read(my_read) == 4000000
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_single_write():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 1MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "1MB")
        this_actor.info("Check remaining space")
        assert fs.partition_by_name("/dev/a").free_space == 99 * 1000 *1000
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Opening file in 'w' mode reset size to 0. Check remaining space")
        assert fs.partition_by_name("/dev/a").free_space, 100 * 1000 *1000
        this_actor.info("Write 0B at /dev/a/foo.txt, which should return 0")
        assert file.write(0) == 0
        this_actor.info("Write 200MB at /dev/a/foo.txt, which should not work")
        try:
            file.write("200MB")
            assert False, "Expected NotEnoughSpaceException was not raised"
        except NotEnoughSpaceException:
            pass
        this_actor.info("Write 2MB at /dev/a/foo.txt")
        assert file.write("2MB") == 2000000
        this_actor.info("Check remaining space")
        assert fs.partition_by_name("/dev/a").free_space == 98 * 1000 * 1000
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()    

def run_test_single_async_write():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode ('a')")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Asynchronously write 2MB at /dev/a/foo.txt")
        my_write = file.write_async("2MB")
        this_actor.info("Sleep for 1 second")
        this_actor.sleep_for(1)
        this_actor.info("Sleep complete. Clock should be at 1s")
        assert Engine.clock == 1
        this_actor.info("Wait for write completion")
        my_write.wait()
        this_actor.info("Write complete. Clock should be at 2s")
        assert Engine.clock == 2
        assert File.num_bytes_written(my_write) == 2000000
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_double_async_append():
    e, host, disk, fs = setup_platform()
    def test_actor():
        pending_writes = ActivitySet()
        this_actor.info("Create an empty file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "0B")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Asynchronously write 2MB at /dev/a/foo.txt")
        pending_writes.push(file.write_async("2MB"))
        this_actor.info("Sleep for .1 second")
        this_actor.sleep_for(0.1)
        this_actor.info("Asynchronously write another 2MB at /dev/a/foo.txt")
        pending_writes.push(file.write_async("2MB"))
        this_actor.info("Wait for completion of both write operations")
        pending_writes.wait_all()
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Check the file size, should be 4MB")
        assert fs.file_size("/dev/a/foo.txt") == 4*1000*1000

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_single_append_write():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode ('a')")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Write 2MB at /dev/a/foo.txt")
        file.write("2MB")
        this_actor.info("Check file size, it should still be 12MB")
        assert fs.file_size("/dev/a/foo.txt") == 12 * 1000 * 1000
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_disk_failure():
    e, host, disk, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode ('a')")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Write 2MB at /dev/a/foo.txt")
        my_write = file.write_async("2MB")
        this_actor.sleep_for(1)
        disk.turn_off()
        this_actor.sleep_for(1)
        disk.turn_on()
        try:
            my_write.wait()
            assert False, "Expected StorageFailureException was not raised"
        except StorageFailureException:
            pass
        this_actor.info("Close the file")
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()
        
if __name__ == '__main__':
    tests = [
      run_test_single_read,
      run_test_single_async_read,
      run_test_single_write,
      run_test_single_async_write,
      run_test_double_async_append,
      run_test_single_append_write,
      run_test_disk_failure
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

