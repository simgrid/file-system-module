# Copyright (c) 2025. The FSMod Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
import multiprocessing
from fsmod import PathUtil

def run_test_path_simplification():
    input_output = [
            ("////", "/"),
            ("////foo", "/foo"),
            ("/////foo", "/foo"),
            ("../../../././././/////foo", "/foo"),
            ("../../", "/"),
            ("/../../","/"),
            ("foo/bar/../", "/foo"),
            ("/////foo/////././././bar/../bar/..///../../", "/"),
            ("./foo/.../........", "/foo/.../........"),
            ("./foo/", "/foo"),
            ("./foo", "/foo")
    ]
    for (input, ouput) in input_output:
        assert PathUtil.simplify_path_string(input) == ouput, input

def run_test_split_path():
    input_output = [
        ("/a/b/c/d",                     ("/a/b/c", "d")),
        ("/a/b////c/d",                  ("/a/b/c", "d")),
        ("/",                            ("/", "")),
        ("a",                            ("/", "a"))
    ]
    for (input, output) in input_output:
        simplified_path = PathUtil.simplify_path_string(input)
        dir, file = PathUtil.split_path(simplified_path)
        assert dir == output[0], f"{input} (wrong directory)"
        assert file == output[1], f"{input} (wrong file)"
    path = "a"
    dir, file = PathUtil.split_path(path)
    assert dir == "", "a (wrong directory)"
    assert file == "a", "a (wrong file)"

def run_test_path_at_mount_point():
    input_output = [
            ("////../",                 "/dev/a",    "Exception"),
            ("/dev/a/foo/bar/",         "/dev/a",    "/foo/bar"),
            ("/dev/a/foo/../bar/",      "/dev/a/",   "/bar"),
            ("/dev/a/foo/../bar////",   "/dev/a///", "/bar"),
            ("/dev/a////foo/../../",    "dev/a",     "Exception"),
            ("/dev/b/foo/../bar/bar2/", "/bar/bar2", "Exception"),
            ("/dev/b/foo/../bar/bar2",  "/dev/a",    "Exception"),
            ("/foo/dev/b/foo/bar/../",  "/dev/a",    "Exception"),
            ("/dev/a/../a/fioo",        "/dev/a",    "/fioo")
    ]
    for (path, mp, output) in input_output:
        simplified_path = PathUtil.simplify_path_string(path)
        try :
            path_at_mp = PathUtil.path_at_mount_point(simplified_path, "/dev/a")
            assert output == path_at_mp,  f"({path},{mp})"
        except ValueError:
            assert output == "Exception", f"({path},{mp})"

if __name__ == '__main__':
    tests = [
      run_test_path_simplification,
      run_test_split_path,
      run_test_path_at_mount_point
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
