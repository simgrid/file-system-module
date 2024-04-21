.. _install:

Installing FSMOD
*****************

.. _install-prerequisites:

Prerequisites
=============


Required Dependencies
---------------------


Optional Dependencies
---------------------

-  `Google Test <https://github.com/google/googletest>`__ – version 1.8
   or higher (only required for running tests)
-  `Doxygen <http://www.doxygen.org>`__ – version 1.8 or higher (only
   required for generating documentation)
-  `Sphinx <https://www.sphinx-doc.org/en/master/usage/installation.html>`__ - 
   version 4.5 or higher along with the following Python packages: 
   ``pip3 install sphinx-rtd-theme breathe recommonmark``  (only required 
   for generating documentation)

.. _install-source:

Source Install
==============

.. _install-source-build:

Building FSMOD
---------------

You can download the ``XXX-<version>.tar.gz`` archive from the `GitHub
releases <https://github.com/XXX-project/wrench/releases>`__ page.
Once you have installed dependencies (see above), you can install WRENCH
as follows:

.. code:: sh

      tar xf XXX-<version>.tar.gz
      cd XXX-<version>
      mkdir build
      cd build
      cmake ..
      make -j8
      make install # try "sudo make install" if you do not have write privileges

If you want to see actual compiler and linker invocations, add
``VERBOSE=1`` to the compilation command:

.. code:: sh

   make -j8 VERBOSE=1

To enable the use of Batsched (provided you have installed that package,
see above): 

.. code:: sh

   cmake -DENABLE_BATSCHED=on .

If you want to stay on the bleeding edge, you should get the latest git
version, and recompile it as you would do for an official archive:

.. code:: sh

   git clone https://github.com/XXX-project/wrench

.. _install-unit-tests:

Compiling and running unit tests
--------------------------------

Building and running the unit tests, which requires Google Test, is done
as:

.. code:: sh

   make -j8 unit_tests
   ./unit_tests

.. _install-examples:

Compiling and running examples
------------------------------

Building the examples is done as:

.. code:: sh

   make -j8 examples

All binaries for the examples are then created in subdirectories of
``build/examples/``

.. _install-troubleshooting:

Installation Troubleshooting
----------------------------

Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  This error on MacOS is because the ``pkg-config`` package is not
   installed
-  Solution: install this package

   -  MacPorts: ``sudo port install pkg-config``
   -  Brew: ``sudo brew install pkg-config``

Could not find libgfortran when building the SimGrid dependency
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  This is an error that sometimes occurs on MacOS
-  A quick fix is to disable the SMPI feature of SimGrid when
   configuring it: ``cmake -Denable_smpi=off .``

.. _install-docker:

Docker Containers
=================

WRENCH is also distributed in Docker containers. Please, visit the
`WRENCH Repository on Docker
Hub <https://hub.docker.com/r/XXXproject/wrench/>`__ to pull WRENCH’s
Docker images.

The ``latest`` tag provides a container with the latest `WRENCH
release <https://github.com/XXX-project/wrench/releases>`__:

.. code:: sh

   docker pull XXXproject/wrench 
   # or
   docker run --rm -it XXXproject/wrench /bin/bash

The ``unstable`` tag provides a container with the (almost) current code
in the GitHub’s ``master`` branch:

.. code:: sh

   docker pull XXXproject/wrench:unstable
   # or
   docker run --rm -it XXXproject/wrench:unstable /bin/bash

Additional tags are available for all WRENCH releases.
