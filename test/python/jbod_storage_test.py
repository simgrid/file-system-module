# Copyright (c) 2025-2026. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import math
import os
import sys
import tempfile

from simgrid import Engine, this_actor
from fsmod import JBODStorage, FileSystem, NotEnoughSpaceException

def capture_cpp_stderr(func, *args, **kwargs):
    # Save original stderr file descriptor
    original_stderr_fd = sys.stderr.fileno()
    saved_stderr_fd = os.dup(original_stderr_fd)

    with tempfile.TemporaryFile(mode='w+') as tmp:
        os.dup2(tmp.fileno(), original_stderr_fd)  # Redirect C-level stderr to tmp

        try:
            func(*args, **kwargs)  # Run the function that logs to stderr
        finally:
            os.dup2(saved_stderr_fd, original_stderr_fd)  # Restore original stderr
            os.close(saved_stderr_fd)

        tmp.seek(0)
        return tmp.read()

def setup_platform():
    e = Engine(sys.argv)
    e.set_log_control("no_loc")
    e.set_log_control("root.thresh:critical")
    e.set_config("network/crosstraffic:0")

    # Creating a platform with two hosts and a 4-disk JBOD...
    zone = e.netzone_root.add_netzone_full("zone")
    client = zone.add_host("fs_client", "100Mf")
    server = zone.add_host("fs_server", "200Mf")

    disks = [server.add_disk(f"jds_disk{i}", "2MBps", "1MBps") for i in range(4)]
    link = zone.add_link("link", 120e6 / 0.97).set_latency(0)
    zone.add_route(client, server, [link])
    zone.seal()

    # Creating a JBOD storage on fs_server...
    jds = JBODStorage.create("my_storage", disks)
    jds.set_raid_level(JBODStorage.RAID.RAID5)
    assert jds.raid_level == JBODStorage.RAID.RAID5

    # Creating a file system...
    fs = FileSystem.create("my_fs")
    # Mounting a 100MB partition...
    fs.mount_partition("/dev/a/", jds, "100MB")

    return e, client, server, fs, jds

def run_test_not_enough_disks():
    e, client, server, fs, jds = setup_platform()
    #TODO 
    e.run()

def run_test_single_read():
    e, client, server, fs, jds = setup_platform()

    def actor():
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

    client.add_actor("TestActor", actor)
    e.run()

def run_test_single_async_read():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Create a 60MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "60MB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "r")
        this_actor.info("Asynchronously read 12MB at /dev/a/foo.txt")
        my_read = file.read_async("12MB")
        this_actor.info("Sleep for 1 second")
        this_actor.sleep_for(1)
        this_actor.info("Sleep complete. Clock should be at 1s")
        assert Engine.clock == 1
        this_actor.info("Wait for read completion")
        my_read.wait()
        this_actor.info("Read complete. Clock should be at 2.1s (2s to read, 0.1 to transfer)")
        assert math.isclose(Engine.clock, 2.1)

    client.add_actor("TestActor", actor)
    e.run()

def run_test_single_write():
    e, client, server, fs, jds = setup_platform()

    def actor():
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

    client.add_actor("TestActor", actor)
    e.run()

def run_test_single_async_write():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode ('a')")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Asynchronously write 12MB at /dev/a/foo.txt")
        my_write = file.write_async("12MB")
        this_actor.info("Sleep for 1 second")
        this_actor.sleep_for(1)
        this_actor.info("Sleep complete. Clock should be at 1s")
        assert Engine.clock == 1
        this_actor.info("Wait for write completion")
        my_write.wait()
        this_actor.info("Write complete. Clock  is at 4.12s (.1s to transfer, 0.02 to compute parity, 4s to write)")
        assert math.isclose(Engine.clock, 4.12)
        this_actor.info("Close the file")
        file.close()

    client.add_actor("TestActor", actor)
    e.run()

def run_test_single_detached_write():
    e, client, server, fs, jds = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Open File '/dev/a/foo.txt' in append mode ('a')")
        file = fs.open("/dev/a/foo.txt", "a")
        this_actor.info("Asynchronously write 12MB at /dev/a/foo.txt")
        my_write = file.write_async("12MB", True)
        this_actor.info("Sleep for 4 seconds")
        this_actor.sleep_for(4)
        this_actor.info("Sleep complete. Clock should be at 4s and nothing should be written yet")
        assert Engine.clock == 4
        assert file.num_bytes_written(my_write) == 0
        this_actor.info("Sleep for 0.12 more second")
        this_actor.sleep_for(0.12)
        this_actor.info("Clock should be at 4.12s (.1s to transfer, 0.02 to compute parity, 4s to write), and write be complete")
        assert math.isclose(Engine.clock, 4.12)
        # FIXME On a JBOD we can't retrieve the number of bytes written (or read)
        # assert file.num_bytes_written(my_write) == 2000000
        this_actor.info("Close the file")
        file.close()

    client.add_actor("TestActor", test_actor)
    e.run()

def run_test_read_write_raid0():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Set RAID level to RAID0")
        jds.set_raid_level(JBODStorage.RAID.RAID0)
        this_actor.info("Create a 10MB file at '/dev/a/foo.txt'")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Opened File '/dev/a/foo.txt' in write mode")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Write 12MB at '/dev/a/foo.txt'")
        assert file.write("12MB") == 12_000_000
        this_actor.info("Write complete. Clock is at 3.1s (.1s to transfer, 3s to write)")
        assert math.isclose(Engine.clock, 3.1)
        this_actor.info("Close the file")
        file.close()

        this_actor.info("Opened File /dev/a/foo.txt in read mode")
        file = fs.open("/dev/a/foo.txt", "r")
        assert file.read("12MB") == 12_000_000
        this_actor.info("Read complete. Clock is at 4.7s (1.5s to read, .1s to transfer)")
        assert math.isclose(Engine.clock, 4.7)
        this_actor.info("Close the file")
        file.close()

    client.add_actor("TestActor", actor)
    e.run()

def run_test_read_write_raid1():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Set RAID level to RAID1")
        jds.set_raid_level(JBODStorage.RAID.RAID1)
        this_actor.info("Create a 10MB file at '/dev/a/foo.txt'")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Opened File '/dev/a/foo.txt' in write mode")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Write 6MB at '/dev/a/foo.txt'")
        assert file.write("6MB") == 6_000_000
        this_actor.info("Write complete. Clock is at 6.05s (.5s to transfer, 6s to write)")
        assert math.isclose(Engine.clock, 6.05)
        this_actor.info("Close the file")
        file.close()

        this_actor.info("Opened File /dev/a/foo.txt in read mode")
        file = fs.open("/dev/a/foo.txt", "r")
        assert file.read("6MB") == 6_000_000
        this_actor.info("Read complete. Clock is at 9.1s (3s to read, .05s to transfer)")
        assert math.isclose(Engine.clock, 9.1)
        this_actor.info("Close the file")
        file.close()

    client.add_actor("TestActor", actor)
    e.run()

def run_test_read_write_raid4():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Set RAID level to RAID4")
        jds.set_raid_level(JBODStorage.RAID.RAID4)
        this_actor.info("Create a 10MB file at '/dev/a/foo.txt'")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Opened File '/dev/a/foo.txt' in write mode")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Write 6MB at '/dev/a/foo.txt'")
        assert file.write("6MB") == 6_000_000
        this_actor.info("Write complete. Clock is at 2.06s (.5s to transfer, 0.01 to compute parity, 2s to write)")
        assert math.isclose(Engine.clock, 2.06)
        this_actor.info("Close the file")
        file.close()

        this_actor.info("Opened File /dev/a/foo.txt in read mode")
        file = fs.open("/dev/a/foo.txt", "r")
        assert file.read("6MB") == 6_000_000
        this_actor.info("Read complete. Clock is at 3.11s (1s to read, .05s to transfer)")
        assert math.isclose(Engine.clock, 3.11)
        this_actor.info("Close the file")
        file.close()

    client.add_actor("TestActor", actor)
    e.run()

def run_test_read_write_raid6():
    e, client, server, fs, jds = setup_platform()

    def actor():
        this_actor.info("Set RAID level to RAID6")
        jds.set_raid_level(JBODStorage.RAID.RAID6)
        this_actor.info("Create a 10MB file at '/dev/a/foo.txt'")
        fs.create_file("/dev/a/foo.txt", "10MB")
        this_actor.info("Opened File '/dev/a/foo.txt' in write mode")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Write 6MB at '/dev/a/foo.txt'")
        assert file.write("6MB") == 6_000_000
        this_actor.info("Write complete. Clock is at 2.06s (.05s to transfer, 0.02 to compute parity, 3s to write)")
        assert math.isclose(Engine.clock, 3.08)
        this_actor.info("Close the file")
        file.close()

        this_actor.info("Opened File /dev/a/foo.txt in read mode")
        file = fs.open("/dev/a/foo.txt", "r")
        assert file.read("6MB") == 6_000_000
        this_actor.info("Read complete. Clock is at 4.63s (1.5s to read, .05s to transfer)")
        assert math.isclose(Engine.clock, 4.63)
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Test the evolution of parity disks")
        expected_debug_outputs =[
            "Parity disks are #1 and #2. Reading From: jds_disk0 jds_disk3 ",
            "Parity disks are #0 and #1. Reading From: jds_disk2 jds_disk3 ",
            "Parity disks are #3 and #0. Reading From: jds_disk1 jds_disk2 "
        ]
        e.set_log_control("fsmod_jbod.thresh:debug")
        e.set_log_control("fsmod_jbod.fmt:'%m'")
        e.set_log_control("no_loc")
        for i in range(3):
            file = fs.open("/dev/a/foo.txt", "w")
            file.write("6MB")
            file.close()
            file = fs.open("/dev/a/foo.txt", "r")
            
            def read_file():
                file.read("6MB")

            captured_debug_output = capture_cpp_stderr(read_file)
            assert expected_debug_outputs[i] == captured_debug_output

        this_actor.info("Close the file")
        file.close()
        
    client.add_actor("TestActor", actor)
    e.run()

if __name__ == "__main__":
    import multiprocessing

    tests = [
        run_test_single_read,
        run_test_single_async_read,
        run_test_single_write,
        run_test_single_async_write,
        run_test_single_detached_write,
        run_test_read_write_raid0,
        run_test_read_write_raid1,
        run_test_read_write_raid4,
        run_test_read_write_raid6,
    ]

    for test in tests:
        print(f"\n🔧 Running {test.__name__} ...")
        p = multiprocessing.Process(target=test)
        p.start()
        p.join()
        if p.exitcode != 0:
            print(f"❌ {test.__name__} failed with exit code {p.exitcode}")
        else:
            print(f"✅ {test.__name__} passed")
