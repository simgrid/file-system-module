# Copyright (c) 2025. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import io
import sys
import multiprocessing
from simgrid import Engine, this_actor, Actor
from fsmod import FileSystem, OneDiskStorage

DEBUG = False # for run_test_no_actor which can use this_actor.info

def setup_platform():
    e = Engine(sys.argv)
    e.set_log_control("no_loc")
    e.set_log_control("root.thresh:critical")

    # Creating a platform with 3 netzones with one host and one disk in each...
    root = e.netzone_root.add_netzone_full("root")
    for i in range(3):
        # Creating Zone
        zone = root.add_netzone_full(f"zone_{i}")
        host = zone.add_host(f"host_{i}", "100Gf")
        disk = host.add_disk(f"disk_{i}", "1kBps", "2kBps")
        zone.seal()

        # Creating a one-disk storage on the host's disk..."
        ods = OneDiskStorage.create(f"my_storage{i}", disk)
        # Creating a file system
        fs = FileSystem.create(f"my_fs_{i}")
        # Mounting a 1MB partition
        fs.mount_partition("/dev/a/", ods, "1MB")
        # Register the file system in the NetZone...
        FileSystem.register_file_system(zone, fs)
        if i == 0:
            # Creating a second file system in that zone...
            extra_fs = FileSystem.create("my_extra_fs")
            # Register the file system in the NetZone...
            FileSystem.register_file_system(zone, extra_fs)
            # Mounting a 10MB partition...
            extra_fs.mount_partition("/tmp/", ods, "10MB")
    root.seal()

    return e

def run_test_retrieve_by_actor():
    e = setup_platform()
    hosts = e.all_hosts
    def test_actor(id):
        this_actor.info("Retrieve the file systems this Actor can access")
        accessible_file_systems = FileSystem.file_systems_by_actor(Actor.self())
        if (id == 0):
            this_actor.info(f"There should be only two (len(accessible_file_systems))")
            assert len(accessible_file_systems) == 2
            fs = accessible_file_systems["my_fs_0"]
            assert fs.name == "my_fs_0"
            assert fs.name == "my_fs_0"
            fs = accessible_file_systems["my_extra_fs"]
            assert fs.name == "my_extra_fs"
        else:
            this_actor.info("There should be only one,")
            assert len(accessible_file_systems) == 1
            fs = accessible_file_systems[f"my_fs_{id}"]
            assert fs.name == f"my_fs_{id}"

    id = 0
    for h in hosts:
        h.add_actor(f"TestActor-{id}", test_actor, id)
        id = id + 1
    e.run()

def run_test_retrieve_by_zone():
    e = setup_platform()
    netzones = e.all_netzones

    index = -1
    for nz in netzones:
        this_actor.info(f"Looking for file systems in NetZone {nz.name}")
        accessible_file_systems = FileSystem.file_systems_by_netzone(nz)
        if nz.name == "_world_" or nz.name == "root":
            assert len(accessible_file_systems) == 0
        else:
            if index == 0:
                assert len(accessible_file_systems) == 2
            else:
                assert len(accessible_file_systems) == 1
                name, fs = next(iter(accessible_file_systems.items()))
                assert name == f"my_fs_{index}"
                assert fs.name == f"my_fs_{index}"
        index = index + 1
    e.run()


def debug_print(msg):
    if DEBUG:
        print(msg)

def run_test_no_actor():
    debug_print("Creating a very small platform")
    e = Engine.instance
    root = e.netzone_root
    host = root.add_host("my_host", "100Gf")
    disk = host.add_disk("disk", "1MBps", "2MBps")
    ods = OneDiskStorage.create("my_storage", disk)
    root.seal()

    debug_print("Test we can has access to some file systems with no actor (we can't)")
    assert len(FileSystem.file_systems_by_actor(None)) == 0

    debug_print("Create and register a file system in the Root NetZone...")
    FileSystem.register_file_system(root, FileSystem.create("root_fs"))

    debug_print("Test if we can access to some file systems (we now should)")
    accessible_file_systems = FileSystem.file_systems_by_actor(None)
    assert len(accessible_file_systems) == 1

    debug_print("Retrieve the 'root_fs' file system")
    root_fs = accessible_file_systems["root_fs"]
    debug_print("Mounting a 10MB partition...")
    root_fs.mount_partition("/root/", ods, "10MB")
    debug_print("Open File '/root/foo.txt'")
    file = root_fs.open("/root/foo.txt", "w")
    debug_print("Write 1MB at /root/foo.txt")
    my_write = file.write_async("1MB")

    e.run()
    debug_print(f"Simulation time: {Engine.clock}")
    file.close()
    e.run()

if __name__ == '__main__':
    tests = [
      run_test_retrieve_by_actor,
      run_test_retrieve_by_zone,
      run_test_no_actor
    ]

    for test in tests:
        debug_print(f"\n🔧 Run {test.__name__} ...")
        p = multiprocessing.Process(target=test)
        p.start()
        p.join()

        if p.exitcode != 0:
            print(f"❌ {test.__name__} failed with exit code {p.exitcode}")
        else:
            print(f"✅ {test.__name__} passed")

