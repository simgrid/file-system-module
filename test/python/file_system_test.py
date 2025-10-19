import io
import sys
import multiprocessing
from simgrid import Engine, this_actor, Host
from fsmod import FileSystem, OneDiskStorage, InvalidPathException, NotEnoughSpaceException, FileAlreadyExistsException, DirectoryDoesNotExistException, DirectoryAlreadyExistsException, FileIsOpenException, FileNotFoundException, TooManyOpenFilesException, InvalidMoveException

def setup_platform():
    e = Engine(sys.argv)
    e.set_log_control("no_loc")
    e.set_log_control("root.thresh:critical")

    # Creating a platform with one host and two disks...
    zone = e.netzone_root.add_netzone_full("zone")
    host = zone.add_host("my_host", "100Gf")
    disk_one = host.add_disk("disk_one", "1kBps", "2kBps")
    disk_two = host.add_disk("disk_two", "1kBps", "2kBps")
    zone.seal()

    # Creating a one-disk storage on the host's first disk..."
    ods = OneDiskStorage.create("my_storage", disk_one)
    # Creating a file system
    fs = FileSystem.create("my_fs")
    # Mounting a 100kB partition
    fs.mount_partition("/dev/a/", ods, "100kB")

    return e, host, disk_one, disk_two, fs


def run_test_mount_partition():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Creating a one-disk storage on the host's second disk...")
        ods = OneDiskStorage.create("my_storage", disk_two)
        # Coverage
        assert "my_storage" == ods.name
        assert None == ods.controller_host
        assert None == ods.controller
        ods.start_controller(disk_one.host, lambda: None)

        this_actor.info("Mount a new partition with a name that's not a clean path, which shouldn't work")
        try:
            fs.mount_partition("/dev/../dev/a", ods, "100kB")
        except ValueError:
            pass
        this_actor.info("Mount a new partition with a name conflict, which shouldn't work")
        try:
            fs.mount_partition("/dev/a", ods, "100kB")
        except ValueError:
            pass
        this_actor.info("Mount a new partition incorrectly")
        try:
            fs.mount_partition("/dev/", ods, "100kB")
        except ValueError:
            pass
        this_actor.info("Mount a new partition correctly")
        fs.mount_partition("/dev/b", ods, "100kB")
        this_actor.info("Try to access a non-existing partition by name, which shouldn't work")
        try:
            fs.partition_by_name("/dev/")
        except ValueError:
            pass
        this_actor.info("Retrieving the file system's partitions")
        partitions = fs.partitions
        assert len(partitions) == 2

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_file_create():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10MB file at /foo/foo.txt, which should fail")
        try:
            fs.create_file("/foo/foo.txt", "10MB")
        except InvalidPathException:
            pass
        this_actor.info("Create a 10MB file at /dev/a/foo.txt, which should fail")
        try:
            fs.create_file("/dev/a/foo.txt", "10MB")
        except NotEnoughSpaceException:
            pass
        this_actor.info("Create a 10kB file at /dev/a/foo.txt, which should work")
        fs.create_file("/dev/a/foo.txt", "10kB")
        partition = fs.partition_by_name("/dev/a")
        assert fs.partition_for_path_or_null("/dev/a/foo.txt") == partition
        assert fs.partition_for_path_or_null("/dev/bogus/foo.txt") == None
        assert fs.partition_by_name("/dev/a").name == "/dev/a"
        assert partition.name == "/dev/a"
        assert 100*1000 == partition.size
        assert 1 == fs.partition_by_name("/dev/a").num_files
        this_actor.info("Create the same file again at /dev/a/foo.txt, which should fail")
        try:
            fs.create_file("/dev/a/foo.txt", "10kB")
        except FileAlreadyExistsException:
            pass
        this_actor.info("Check remaining space")
        assert fs.partition_by_name("/dev/a").free_space == 90 * 1000
        
    host.add_actor("TestActor", test_actor)
    e.run()
    assert fs.partition_by_name("/dev/a").free_space == 90 * 1000

def run_test_directories():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Check that directory /dev/a/ exists")
        assert True == fs.directory_exists("/dev/a/")
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10kB")
        this_actor.info("Create a 10kB file at /dev/a/b/c/foo.txt")
        fs.create_file("/dev/a/b/c/foo.txt", "10kB")
        assert True == fs.file_exists("/dev/a/b/c/foo.txt")
        this_actor.info("Create a 10kB file at /dev/a/b/c, which should fail")
        try:
            fs.create_file("/dev/a/b/c/", "10kB")
        except InvalidPathException:
            pass
        this_actor.info("Check that what should exist does")
        assert False == fs.file_exists("/dev/a/b/c")
        this_actor.info("Check free space")
        assert fs.partition_by_name("/dev/a//////").free_space == 80*1000
        this_actor.info("Create a 10kB file at /dev/a/b/c/faa.txt")
        fs.create_file("/dev/a/b/c/faa.txt", "10kB")
        try:
            found_files = fs.files_in_directory("/dev/a/b/c_bogus")
        except DirectoryDoesNotExistException:
            pass
        found_files = fs.files_in_directory("/dev/a/b/c")
        assert "foo.txt" in found_files
        assert "faa.txt" in found_files
        this_actor.info("Try to unlink non-existing directory. This shouldn't work")
        try:
            fs.unlink_directory("/dev/a/b/d")
        except DirectoryDoesNotExistException:
            pass
        assert False == fs.directory_exists("/dev/a/b/d")

        try:
            fs.create_directory("/whatever/bogus")
        except InvalidPathException:
            pass
        try:
            fs.create_directory("/dev/a/foo.txt")
        except InvalidPathException:
            pass
        fs.create_directory("/dev/a/new_dir")
        try:
            fs.create_directory("/dev/a/new_dir")
        except DirectoryAlreadyExistsException:
            pass
        fs.unlink_directory("/dev/a/new_dir")

        this_actor.info("Try to unlink a directory in which one file is opened. This shouldn't work")
        file = fs.open("/dev/a/b/c/foo.txt", "r")
        assert file.path == "/dev/a/b/c/foo.txt"
        assert file.access_mode == "r"
        assert file.file_system == fs
        try:
            fs.unlink_directory("/dev/a/b/c")
        except FileIsOpenException:
            pass
        file.close()
        fs.unlink_directory("/dev/a/b/c")
        assert False == fs.directory_exists("/dev/a/b/c")

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_file_move():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Try to move a non-existing file. This shouldn't work")
        try:
            fs.move_file("/dev/a/foo.txt", "/dev/a/b/c/foo.txt")
        except FileNotFoundException:
            pass
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10kB")
        assert fs.partition_by_name("/dev/a/").free_space == 90*1000
        assert fs.free_space_at_path("/dev/a/") == 90*1000
        assert fs.free_space_at_path("/dev/a/foo.txt") == 90*1000
        this_actor.info("Try to move file /dev/a/foo.txt to the same path. This should work (no-op)")
        fs.move_file("/dev/a/foo.txt", "/dev/a/foo.txt")
        this_actor.info("Move file /dev/a/foo.txt to /dev/a/b/c/foo.txt")
        fs.move_file("/dev/a/foo.txt", "/dev/a/b/c/foo.txt")
        assert True == fs.file_exists("/dev/a/b/c/foo.txt")
        assert False == fs.file_exists("/dev/a/foo.txt")

        this_actor.info("Create a 20kB file at /dev/a/stuff.txt")
        fs.create_file("/dev/a/stuff.txt", "20kB")
        assert fs.partition_by_name("/dev/a/").free_space == 70*1000

        # Moving a smaller file to a bigger file
        this_actor.info("Move file /dev/a/b/c/foo.txt to /dev/a/stuff.txt")
        fs.move_file("/dev/a/b/c/foo.txt", "/dev/a/stuff.txt")
        assert False == fs.file_exists("/dev/a/b/c/foo.txt")
        assert True ==fs.file_exists("/dev/a/stuff.txt")
        assert fs.partition_by_name("/dev/a/").free_space == 90*1000

        # Moving a bigger file to a smaller file
        this_actor.info("Create a 20kB file at /dev/a/big.txt")
        fs.create_file("/dev/a/big.txt", "20kB")
        assert fs.partition_by_name("/dev/a/").free_space == 70*1000
        this_actor.info("Move file /dev/a/stuff.txt to /dev/a/big.txt")
        fs.move_file("/dev/a/stuff.txt", "/dev/a/big.txt")
        assert False == fs.file_exists("/dev/a/stuff.txt")
        assert True ==fs.file_exists("/dev/a/big.txt")
        assert fs.partition_by_name("/dev/a/").free_space == 90*1000

        ods = OneDiskStorage.create("my_storage", disk_two)
        this_actor.info("Mount a new partition")
        fs.mount_partition("/dev/b/", ods, "100kB")
        this_actor.info("Move file /dev/a/stuff.txt to /dev/b/stuff.txt which is forbidden")
        try:
            fs.move_file("/dev/a/stuff.txt", "/dev/b/stuff.txt")
        except InvalidMoveException:
            pass

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_file_open_close():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/stuff/foo.txt", "10kB")
        this_actor.info("Create a 10kB file at /dev/a/stuff/other.txt")
        fs.create_file("/dev/a/stuff/other.txt", "10kB")

        this_actor.info("Opening the file")
        file = fs.open("/dev/a/stuff/foo.txt", "r")

        this_actor.info("Trying to move the file")
        try:
            fs.move_file("/dev/a/stuff/foo.txt", "/dev/a/bar.txt")
        except FileIsOpenException:
            pass
        this_actor.info("Trying to unlink the file")
        try:
            fs.unlink_file("/dev/a/stuff/foo.txt")
        except FileIsOpenException:
            pass
        this_actor.info("Trying to overwrite the file")
        try:
            fs.move_file("/dev/a/stuff/other.txt", "/dev/a/stuff/foo.txt")
        except FileIsOpenException:
            pass

        this_actor.info("Close the file")
        file.close()
        this_actor.info("Trying to unlink the file")
        fs.unlink_file("/dev/a/stuff/foo.txt")
        assert False == fs.file_exists("/dev/a/stuff/foo.txt")
        this_actor.info("Trying to unlink the file again")
        try:
            fs.unlink_file("/dev/a/stuff/foo.txt")
        except FileNotFoundException:
            pass

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_too_many_files_opened():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Creating a one-disk storage on the host's second disk...")
        ods = OneDiskStorage.create("my_storage", disk_two)
        this_actor.info("Creating another file system ...on which you can only open 2 files at a time")
        limited_fs = FileSystem.create("my_limited_fs", 2)
        this_actor.info("Mounting a 100kB partition...")
        limited_fs.mount_partition("/dev/a/", ods, "100kB")

        this_actor.info("Opening a first file, should be fine")
        file = limited_fs.open("/dev/a/stuff/foo.txt", "w")
        this_actor.info("Opening a second file, should be fine")
        file2 = limited_fs.open("/dev/a/stuff/bar.txt", "w")
        this_actor.info("Opening a third file, should not work")
        try:
            limited_fs.open("/dev/a/stuff/baz.txt", "a")
        except TooManyOpenFilesException:
            pass
        this_actor.info("Close the first file")
        file.close()
        this_actor.info("Opening a third file should now work")
        file3 = limited_fs.open("/dev/a/stuff/baz.txt", "w")

    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_bad_access_mode():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10kB")
        this_actor.info("Open the file in read mode ('r')")
        file = fs.open("/dev/a/foo.txt", "r")
        this_actor.info("Try to write in a file opened in read mode, which should fail")
        try:
            file.write("5kB")
        except ValueError:
            pass
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Open the file in write mode ('w')")
        file = fs.open("/dev/a/foo.txt", "w")
        this_actor.info("Try to read from a file opened in write mode, which should fail")
        try:
            file.read("5kB")
        except ValueError:
            pass
            this_actor.info("Asynchronous read from a file opened in write mode should also fail")
        try:
             file.read_async("5kB")
        except ValueError:
            pass
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Open the file in unsupported mode ('w+'), which should fail")
        try:
            fs.open("/dev/a/foo.txt", "w+")
        except ValueError:
            pass
        this_actor.info("Open a non-existing file in read mode ('r'), which should fail")
        try:
            fs.open("/dev/a/bar.txt", "r")
        except FileNotFoundException:
            pass
   
    host.add_actor("TestActor", test_actor)
    e.run()

def run_test_read_plus_mode():
    e, host, disk_one, disk_two, fs = setup_platform()
    def test_actor():
        this_actor.info("Create a 10kB file at /dev/a/foo.txt")
        fs.create_file("/dev/a/foo.txt", "10kB")
        this_actor.info("Open the file in read+ mode ('r+')")
        file = fs.open("/dev/a/foo.txt", "r+")
        this_actor.info("Seek to the mid point of the file")
        file.seek(-5000, io.SEEK_END)
        this_actor.info("Write 10Kb to the file")
        file.write("10kB")
        this_actor.info("Close the file")
        file.close()
        this_actor.info("Check the file size")
        assert fs.file_size("/dev/a/foo.txt") == 15000


    host.add_actor("TestActor", test_actor)
    e.run()
  
if __name__ == '__main__':
    tests = [
      run_test_mount_partition,
      run_test_file_create,
      run_test_directories,
      run_test_file_move,
      run_test_file_open_close,
      run_test_too_many_files_opened,
      run_test_bad_access_mode,
      run_test_read_plus_mode
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
