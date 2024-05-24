[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL_v2.1-blue.svg)](https://www.gnu.org/licenses/lgpl-2.1)
[![Build-Linux](https://github.com/simgrid/file-system-module/actions/workflows/build.yml/badge.svg)](https://github.com/simgrid/file-system-module/actions/workflows/build.yml)
[![Doc](https://readthedocs.org/projects/pip/badge/?version=stable)](https://simgrid.github.io/file-system-module/)
[![SonarCloud Status](https://sonarcloud.io/api/project_badges/measure?project=simgrid_file-system-module&metric=alert_status)](https://sonarcloud.io/dashboard/?id=simgrid_file-system-module)
[![codecov](https://codecov.io/gh/simgrid/file-system-module/graph/badge.svg?token=6x9KmpEvpS)](https://codecov.io/gh/simgrid/file-system-module)
[![CodeFactor](https://www.codefactor.io/repository/github/simgrid/file-system-module/badge)](https://www.codefactor.io/repository/github/simgrid/file-system-module)

# FSMod: File System Module for SimGrid

## Overview

This project implements a simulated file system "module" on top of [SimGrid](https://simgrid.frama.io/simgrid/), to
be used in any SimGrid-based simulator.
It supports the notion of partitions that store directories that store files, with the
expected operations (e.g., create files, move files, unlink files, unlink directories, check for existence),
and the notion of a file descriptor with POSIX-like operations (open, seek, read, write, close).

The simulated file system implementation in FSMod is not full-featured (e.g., no notion of ownership, no notion of permission).
The biggest limitation is that a directory cannot contain another directory. That is,
a directory `/dev/a/tmp` could contain a file `foo.txt`, and a directory `/dev/a/tmp/other/c` could
contain a file `bar.txt`. But, directory `/dev/a/tmp` does **not** contain directory `other`.
`/dev/a/tmp` and `/dev/a/tmp/other/c` are two completely unrelated directory (there is no directory `/dev/a/tmp/other`).
In other words, each directory simply has a path-like unique name and can only contain files.
The rationale is that the intended use of this module is purely for simulation. As
a result, full-fledged file system functionality is not (or rarely) needed (and would add
overhead to the simulation). We expect that in most cases users will need/use a single directory.

## Dependencies and installation

The only required dependency is [SimGrid](https://simgrid.frama.io/simgrid/). An optional dependency
is [Google Test](https://github.com/google/googletest) for compiling the unit tests.

Here is the typical Ubuntu installation procedure:

```bash
cd file-system-module
mkdir build
cd build
cmake ..
make -j4
sudo make install
```
after which the FSMod library and header files will be installed in `/usr/local/`.

To compile and run the unit tests, just run the following command in the `build` directory:

```bash
make unit_tests
./unit_tests
```
## Examples

Example SimGrid simulators that use FSMod are provided in the `examples` directory (see `README.md` file therein).

## API Reference

The FSMod API is documented on [this page](https://simgrid.github.io/file-system-module/).

---
