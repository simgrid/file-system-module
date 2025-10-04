# Copyright (c) 2025. The FSMod Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL-2.1-only) which comes with this package.

# python3 setup.py sdist # Build a source distrib (building binary distribs is complex on linux)

# twine upload --repository-url https://test.pypi.org/legacy/ dist/fsmod-*.tar.gz # Upload to test
# pip3 install --user --index-url https://test.pypi.org/simple  fsmod

# Once it works, upload to the real infra.  /!\ you cannot modify a file once uploaded
# twine upload dist/fsmod-*.tar.gz

import os
import platform
import re
import subprocess
import sys
from distutils.version import LooseVersion

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build python bindings of FSMod")

        if not os.path.exists("MANIFEST.in"):
            raise RuntimeError(
                "Please generate a MANIFEST.in file at the root of the source directory")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        from pybind11 import get_cmake_dir
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-Denable_python=ON',
                      '-Dpybind11_DIR=' + get_cmake_dir()
                      ]

        build_args = []

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        env['LDFLAGS'] = '{} -L{}'.format(env.get('LDFLAGS', ''), extdir)
        env['MAKEFLAGS'] = '-j'+str(os.cpu_count())
        # env['VERBOSE'] = "1" # Please, make, be verbose about the commands you run

        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] +
                              cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] +
                              build_args, cwd=self.build_temp, env=env)


setup(
    name='fsmod',
    version='0.4.0',
    author='The FSMod Team',
    author_email='simgrid-community@inria.fr',
    description='File System Module for SimGrid',
    long_description=("This project implements a simulated file system "module" on top of SimGrid, to be "
                      "used in any SimGrid-based simulator. It supports the notion of partitions that store "
                      "directories that store files, with the expected operations (e.g., create files, move "
                      "files, unlink files, unlink directories, check for existence), and the notion of a file "
                      "descriptor with POSIX-like operations (open, seek, read, write, close).\n\n"
                      "This package contains a native library. Please install cmake, boost, pybind11 and a "
                      "C++ compiler before using pip3. On Debian/Ubuntu, this is as easy as\n"
                      "sudo apt install cmake libboost-dev pybind11-dev g++ gcc"),
    ext_modules=[CMakeExtension('fsmod')],
    cmdclass=dict(build_ext=CMakeBuild),
    install_requires=['pybind11>=2.4'],
    setup_requires=['pybind11>=2.4'],
    zip_safe=False,
    classifiers=[
        "Development Status :: 4 - Beta",
        "Environment :: Console",
        "Intended Audience :: Education",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: GNU Lesser General Public License v2 (LGPLv2)",
        "Operating System :: POSIX",
        "Operating System :: MacOS",
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "Topic :: System :: Distributed Computing",
        "Topic :: System :: Systems Administration",
    ],
    project_urls={
        'Source':  'https://github.com/simgrid/file-system-module',
    },
)
