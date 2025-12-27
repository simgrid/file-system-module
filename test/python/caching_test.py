# Copyright (c) 2025-2026. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import sys
import multiprocessing
from simgrid import Engine, this_actor
from fsmod import FileSystem, OneDiskStorage, Partition, FileNotFoundException, NotEnoughSpaceException

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
    # Mounting a 100MB FIFO partition
    fs.mount_partition("/dev/fifo/", ods, "100MB", Partition.CachingScheme.FIFO)
    # Mounting a 100MB LRU partition
    fs.mount_partition("/dev/lru/", ods, "100MB", Partition.CachingScheme.LRU)
    
    return e, host, disk, fs

def run_test_logic_test():
    e, host, disk,fs = setup_platform()
    def test_actor():
        ods = OneDiskStorage.create("my_storage", disk)
        this_actor.info("Try to mount partition with empty name")
        try:
            fs.mount_partition("", ods, "100MB")
        except ValueError:
            pass
        try:
            fs.mount_partition("", ods, "100MB", Partition.CachingScheme.FIFO)
        except ValueError:
            pass
        try:
            fs.mount_partition("", ods, "100MB", Partition.CachingScheme.LRU)
        except ValueError:
            pass
        this_actor.sleep_for(1)

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_FIFO_basics():
    e, host, disk,fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 20MB file at /dev/fifo/20mb.txt")
        fs.create_file("/dev/fifo/20mb.txt", "20MB")
        this_actor.info("Create a 60MB file at /dev/fifo/60mb.txt")
        fs.create_file("/dev/fifo/60mb.txt", "60MB")
        this_actor.info("Create a 30MB file at /dev/fifo/30mb.txt")
        fs.create_file("/dev/fifo/30mb.txt", "30MB")
        this_actor.info("Check that files are as they should be")
        assert False == fs.file_exists("/dev/fifo/20mb.txt")
        assert True == fs.file_exists("/dev/fifo/60mb.txt")
        assert True == fs.file_exists("/dev/fifo/30mb.txt")

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_FIFO_basics_with_unevictable_files():
    e, host, disk,fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 20MB file at /dev/fifo/20mb.txt")
        fs.create_file("/dev/fifo/20mb.txt", "20MB")
        this_actor.info("Making the 20MB file at /dev/fifo/20mb.txt unevictable")
        try:
            fs.make_file_evictable("/dev/fifo/bogus.txt", False)
        except FileNotFoundException:
            pass
        fs.make_file_evictable("/dev/fifo/20mb.txt", False)
        this_actor.info("Create a 60MB file at /dev/fifo/60mb.txt")
        fs.create_file("/dev/fifo/60mb.txt", "60MB")
        this_actor.info("Create a 30MB file at /dev/fifo/30mb.txt")
        fs.create_file("/dev/fifo/30mb.txt", "30MB")
        this_actor.info("Check that files are as they should be")
        assert True== fs.file_exists("/dev/fifo/20mb.txt")
        assert False == fs.file_exists("/dev/fifo/60mb.txt")
        assert True == fs.file_exists("/dev/fifo/30mb.txt")
        fs.make_file_evictable("/dev/fifo/20mb.txt", True)
        fs.create_file("/dev/fifo/60mb.txt", "60MB")
        assert False == fs.file_exists("/dev/fifo/20mb.txt")
        assert True == fs.file_exists("/dev/fifo/60mb.txt")
        assert True == fs.file_exists("/dev/fifo/30mb.txt")

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_FIFO_do_not_evict_open_files():
    e, host, disk,fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 20MB file at /dev/fifo/20mb.txt")
        fs.create_file("/dev/fifo/20mb.txt", "20MB")
        this_actor.info("Create a 60MB file at /dev/fifo/60mb.txt")
        fs.create_file("/dev/fifo/60mb.txt", "60MB")
        this_actor.info("Opening the 20MB file")
        file = fs.open("/dev/fifo/20mb.txt", "r")
        this_actor.info("Create a 30MB file at /dev/fifo/30mb.txt")
        fs.create_file("/dev/fifo/30mb.txt", "30MB")
        this_actor.info("Check that files are as they should be")
        assert True== fs.file_exists("/dev/fifo/20mb.txt")
        assert False == fs.file_exists("/dev/fifo/60mb.txt")
        assert True == fs.file_exists("/dev/fifo/30mb.txt")
        this_actor.info("Opening the 30MB file")
        file = fs.open("/dev/fifo/30mb.txt", "r")
        this_actor.info("Create a 60MB file at /dev/fifo/60mb.txt")
        try:
            fs.create_file("/dev/fifo/60mb.txt", "60MB")
        except NotEnoughSpaceException:
            pass

    host.add_actor("TestActor", test_actor)
    e.run()
def run_test_FIFO_extensive():
    e, host, disk,fs = setup_platform()
    def test_actor():
        experiments = [
            [["create", "f1", "20MB"], ["f1"], []],
            [["create", "f2", "50MB"], ["f1", "f2"], []],
            [["create", "f3", "30MB"], ["f1", "f2", "f3"], []],
            [["delete", "f1"], ["f2", "f3"], ["f1"]],
            [["create", "f4", "30MB"], ["f3", "f4"], ["f2"]],
            [["create", "f5", "50MB"], ["f4", "f5"], ["f3"]],
            [["create", "f6", "10MB"], ["f4", "f5", "f6"], []],
            [["create", "f7", "10MB"], ["f4", "f5", "f6", "f7"], []],
            [["create", "f8", "10MB"], ["f5", "f6", "f7", "f8"], ["f4"]]
        ]
        for action, expected_files_there, expected_files_not_there in experiments:
            # Perform action
            if action[0] == "create":
                file_name = action[1]
                file_size = action[2]
                fs.create_file(f"/dev/fifo/{file_name}", file_size)
            else:
                if action[0] == "delete":
                    file_name = action[1]
                    fs.unlink_file(f"/dev/fifo/{file_name}")
                else: 
                    if action[0] == "access":
                        file_name = action[1]
                        file = fs.open(f"/dev/fifo/{file_name}", "r")
                        file.read(1)
                        file.close()
            
            # Check state
            for f in expected_files_there:
                assert True == fs.file_exists(f"/dev/fifo/{f}")  
            for f in expected_files_not_there:
                assert False == fs.file_exists(f"/dev/fifo/{f}")  

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_LRU_basics():
    e, host, disk,fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 20MB file at /dev/lru/20mb.txt")
        fs.create_file("/dev/lru/20mb.txt", "20MB")
        this_actor.info("Create a 60MB file at /dev/lru/60mb.txt")
        fs.create_file("/dev/lru/60mb.txt", "60MB")
        this_actor.info("Open file 20mb.txt and access it")
        file = fs.open("/dev/lru/20mb.txt", "r")
        file.read(10)
        file.close()
        fs.create_file("/dev/lru/30mb.txt", "30MB")
        this_actor.info("Check that files are as they should be")
        assert True == fs.file_exists("/dev/lru/20mb.txt")
        assert False == fs.file_exists("/dev/lru/60mb.txt")
        assert True == fs.file_exists("/dev/lru/30mb.txt")
    
    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_LRU_extensive():
    e, host, disk,fs = setup_platform()
    def test_actor():
        experiments = [
            [["create", "f1", "20MB"], ["f1"], []],
            [["create", "f2", "50MB"], ["f1", "f2"], []],
            [["create", "f3", "30MB"], ["f1", "f2", "f3"], []],
            [["access", "f1"], ["f1", "f2", "f3"], []],
            [["create", "f4", "30MB"], ["f1", "f3", "f4"], ["f2"]],
            [["delete", "f1"], ["f3", "f4"], ["f1"]],
            [["create", "f5", "10MB"], ["f3", "f4", "f5"], []],
            [["create", "f6", "80MB"], ["f5", "f6"], ["f3", "f4"]],
            [["access", "f5"], ["f5", "f6"], []],
            [["create", "f7", "20MB"], ["f5", "f7"], ["f6"]]
        ]

        for action, expected_files_there, expected_files_not_there in experiments:
            # Perform action
            if action[0] == "create":
                file_name = action[1]
                file_size = action[2]
                fs.create_file(f"/dev/lru/{file_name}", file_size)
            else:
                if action[0] == "delete":
                    file_name = action[1]
                    fs.unlink_file(f"/dev/lru/{file_name}")
                else: 
                    if action[0] == "access":
                        file_name = action[1]
                        file = fs.open(f"/dev/lru/{file_name}", "r")
                        file.read(1)
                        file.close()
            
            # Check state
            for f in expected_files_there:
                assert True == fs.file_exists(f"/dev/lru/{f}")  
            for f in expected_files_not_there:
                assert False == fs.file_exists(f"/dev/lru/{f}")  

    host.add_actor("TestActor", test_actor)
    e.run()

if __name__ == '__main__':
    tests = [
      run_test_logic_test,
      run_test_FIFO_basics,
      run_test_FIFO_basics_with_unevictable_files,
      run_test_FIFO_do_not_evict_open_files,
      run_test_FIFO_extensive,
      run_test_LRU_basics,
      run_test_LRU_extensive
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
