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
    disk = host.add_disk("disk", "1kBps", "2kBps")
    zone.seal()

    # Creating a one-disk storage on the host's disk..."
    ods = OneDiskStorage.create("my_storage", disk)
    # Creating a file system
    fs = FileSystem.create("my_fs")
    # Mounting a 100MB partition
    fs.mount_partition("/dev/a/", ods, "1MB")

    return e, host, fs

def run_test_stat():
    e, host, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 100kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "100kB")
        this_actor.info("Open File '/dev/a/foo.txt'")
        file = fs.open("/dev/a/foo.txt", "r")
        this_actor.info("Calling stat()")
        stat_struct = file.stat()
        this_actor.info("Checking sanity")
        assert stat_struct.size_in_bytes == 100*1000
        assert stat_struct.last_modification_date == Engine.clock
        assert stat_struct.last_access_date == Engine.clock
        assert stat_struct.refcount == 1
        this_actor.info("Modifying file state")
        file2 = fs.open("/dev/a/foo.txt", "w")
        file2.seek(100*1000)
        file2.write(12*1000, False)
        file.close()
        this_actor.sleep_for(10)
        file2 = fs.open("/dev/a/foo.txt", "r")
        file.seek(0)
        file.read(1000, False)
        this_actor.info("Calling stat() again")
        stat_struct = file.stat()
        this_actor.info("Checking sanity")
        assert stat_struct.size_in_bytes == 112*1000
        assert stat_struct.last_modification_date== Engine.clock - 10
        assert stat_struct.last_access_date== Engine.clock
        assert stat_struct.refcount == 2
        this_actor.info("Modifying file state")
        file2.close()
        this_actor.info("Calling stat() again")
        stat_struct = file.stat()
        this_actor.info("Checking sanity")
        assert stat_struct.refcount == 1
        file.close()

    host.add_actor("TestActor", test_actor)
    e.run()
  
if __name__ == '__main__':
    tests = [
      run_test_stat
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
