cmake_minimum_required(VERSION 3.12)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/conf/cmake/")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

project(fsmod VERSION 0.4 DESCRIPTION "SimGrid-based File System simulation module")

include(GNUInstallDirs)
find_package(SimGrid 4.0.1 REQUIRED)

include_directories(
		${CMAKE_SOURCE_DIR}/include
		${SimGrid_INCLUDE_DIR}
		${CMAKE_BINARY_DIR}/include
		/opt/local/include
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -funroll-loops -fno-strict-aliasing -flto=auto")

# build the version number
set(FSMOD_VERSION_MAJOR "0")
set(FSMOD_VERSION_MINOR "4")
set(FSMOD_VERSION_PATCH "0")
set(FSMOD_VERSION_EXTRA "dev")

## GIT version check
####################

if(EXISTS ${CMAKE_HOME_DIRECTORY}/.git/)
  execute_process(COMMAND git rev-parse --verify --short HEAD
     WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
     OUTPUT_VARIABLE GIT_VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE)
  # Check for uncommitted changes
  execute_process(COMMAND git diff --name-only HEAD
    WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    OUTPUT_VARIABLE files_changed)
  if(files_changed)
    set(GIT_VERSION "${GIT_VERSION}-dirty")
  endif()
elseif(EXISTS ${CMAKE_HOME_DIRECTORY}/.gitversion)
  FILE(STRINGS ${CMAKE_HOME_DIRECTORY}/.gitversion GIT_VERSION)
else()
  set(GIT_VERSION "none, release version")
endif()
message(STATUS "Git version: ${GIT_VERSION}")

if (${FSMOD_VERSION_PATCH} EQUAL "0")
    set(FSMOD_RELEASE_VERSION "${FSMOD_VERSION_MAJOR}.${FSMOD_VERSION_MINOR}")
else ()
    set(FSMOD_RELEASE_VERSION "${FSMOD_VERSION_MAJOR}.${FSMOD_VERSION_MINOR}.${FSMOD_VERSION_PATCH}")
endif ()

### Generate the required headers
#################################

# Avoid triggering a (full) rebuild by touching the files if they did not really change
configure_file("${CMAKE_HOME_DIRECTORY}/include/fsmod/version.hpp.in" "${CMAKE_BINARY_DIR}/include/fsmod/version.hpp.generated" @ONLY IMMEDIATE)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/include/fsmod/version.hpp.generated ${CMAKE_BINARY_DIR}/include/fsmod/version.hpp)
file(REMOVE ${CMAKE_BINARY_DIR}/include/fsmod/version.hpp.generated)

# Python binding (with pybind11)
################
if((NOT DEFINED enable_python) OR enable_python)
  find_package(Python3 COMPONENTS Interpreter)
  if(NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Please install Python (version 3 or higher) to compile FSMod Python bindings.")
  endif() 
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

  find_package(pybind11 CONFIG)
  message(STATUS "Pybind11 version: ${pybind11_VERSION}")
  if (pybind11_VERSION VERSION_LESS 2.4)
    message(STATUS "FSMod needs at least v2.4 of pybind11. Disabling the Python bindings.")
    set(pybind11_FOUND OFF)
  endif()
endif()

find_package(Python3 COMPONENTS Development)
if(NOT Python3_Development_FOUND OR NOT pybind11_FOUND)
  message(STATUS "FSMod Python bindings cannot be built on this system.")
  set(default_enable_python OFF)
else()
  set(default_enable_python ON)
endif()

option(enable_python "Whether the Python bindings are activated." ${default_enable_python}) # ON by default if dependencies are met

if(enable_python)
  if(NOT Python3_Development_FOUND)
    message(FATAL_ERROR "Please install the development components of Python (python3-dev on Debian) to build the Python bindings (or disable that option).")
  endif()
  if(pybind11_FOUND)
    message(STATUS "Python bindings will be built.")
    set(python_files ${CMAKE_SOURCE_DIR}/src/bindings/python/fsmod_python.cpp)

    message(STATUS "Files to compile: ${python_files}")
    pybind11_add_module(python-bindings
                        ${python_files}
                        ${pybind11_options})

    target_compile_features(python-bindings PRIVATE cxx_std_14)
    target_link_libraries(python-bindings PUBLIC fsmod)
    set_target_properties(python-bindings PROPERTIES
                          LIBRARY_OUTPUT_NAME fsmod
                          CXX_VISIBILITY_PRESET "default"
                          INTERPROCEDURAL_OPTIMIZATION FALSE)
   
    set_property(TARGET python-bindings
                 APPEND PROPERTY INCLUDE_DIRECTORIES "${INTERNAL_INCLUDES}")

    if("${FSMOD_PYTHON_LIBDIR}" STREQUAL "") # value not manually set
      if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
        set(FSMOD_PYTHON_LIBDIR ${Python3_SITEARCH})
      else("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
        string(REGEX REPLACE "^/usr/local/" "${CMAKE_INSTALL_PREFIX}/" FSMOD_PYTHON_LIBDIR ${Python3_SITEARCH})
      endif("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
    endif()
    install(TARGETS python-bindings
      	    LIBRARY DESTINATION "${FSMOD_PYTHON_LIBDIR}")
  else()
    message(FATAL_ERROR "Please install pybind11-dev to build the Python bindings (or disable that option).")
  endif()
endif()
mark_as_advanced(PYBIND11_PYTHON_VERSION)
mark_as_advanced(pybind11_DIR)

if(pybind11_FOUND)
  message("        Compile Python bindings .....: ${enable_python}")
  message("          module ....................: ${PYTHON_MODULE_PREFIX}fsmod${PYTHON_MODULE_EXTENSION}")
  message("          install path ..............: ${FSMOD_PYTHON_LIBDIR} (force another value with -DFSMOD_PYTHON_LIBDIR)")
else()
  message("        Compile Python bindings .....: OFF (disabled, or pybind11 not found)")
endif()
set(SOURCE_FILES
		src/PathUtil.cpp
		src/FileSystem.cpp
		src/File.cpp
		src/FileMetadata.cpp
		src/Partition.cpp
		src/PartitionFIFOCaching.cpp
		src/PartitionLRUCaching.cpp
		src/Storage.cpp
    src/JBODStorage.cpp
		src/OneDiskStorage.cpp
		src/OneRemoteDiskStorage.cpp
    src/fsmod_version.cpp
		)

set(HEADER_FILES
		include/fsmod/File.hpp
		include/fsmod/FileStat.hpp
		include/fsmod/FileSystemException.hpp
		include/fsmod/Partition.hpp
		include/fsmod/PartitionFIFOCaching.hpp
		include/fsmod/PartitionLRUCaching.hpp
		include/fsmod/FileMetadata.hpp
		include/fsmod/JBODStorage.hpp
		include/fsmod/PathUtil.hpp
		include/fsmod/FileSystem.hpp
		include/fsmod/OneDiskStorage.hpp
		include/fsmod/OneRemoteDiskStorage.hpp
		include/fsmod/Storage.hpp
    include/fsmod/version.hpp.in
	)

add_library(fsmod SHARED ${SOURCE_FILES})

set_target_properties(fsmod PROPERTIES
		SOVERSION 0.1
		LINKER_LANGUAGE CXX
		PUBLIC_HEADER "${HEADER_FILES}")

target_include_directories(fsmod PRIVATE include)
target_link_libraries(fsmod PUBLIC ${SimGrid_LIBRARY})

install(TARGETS fsmod
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fsmod)
install(FILES include/fsmod.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_custom_target(examples    COMMENT "Recompiling the examples")

# Google test
find_library(GTEST_LIBRARY NAMES gtest)
if(GTEST_LIBRARY)
	set(TEST_FILES
			test/jbod_storage_test.cpp
			test/one_disk_storage_test.cpp
			test/one_remote_disk_storage_test.cpp
			test/path_util_test.cpp
			test/file_system_test.cpp
			test/seek_test.cpp
			test/truncate_test.cpp
			test/caching_test.cpp
			test/register_test.cpp
			test/stat_test.cpp
			test/main.cpp
			test/test_util.hpp
			include/fsmod.hpp src/Storage.cpp)

	add_definitions(-DGTEST_USED)
	add_executable(unit_tests EXCLUDE_FROM_ALL ${SOURCE_FILES} ${HEADER_FILES} ${TEST_FILES})
	target_include_directories(unit_tests PRIVATE include)
	target_link_libraries(unit_tests ${GTEST_LIBRARY} ${SIMGRID_LIBRARY} fsmod -lpthread -lm)
	set_target_properties(unit_tests PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
	set_target_properties(unit_tests PROPERTIES LINK_FLAGS "--coverage")
	#add_custom_command(TARGET unit_tests COMMAND find . -name *.gcda -delete)
else()
	add_custom_target(unit_tests echo "ERROR: Cannot build unit_tests because Google Test (libgtest) was not found by cmake." COMMAND echo "       If you have installed Google Test, re-run cmake." VERBATIM)
endif()

# Python test
add_custom_target(copy_python_tests ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/test/python
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/test/python
            ${CMAKE_BINARY_DIR}/test/python
    COMMENT "Copying Python test scripts to build directory"
)

# Documentation
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Documentation.cmake)

# Examples
include(${CMAKE_HOME_DIRECTORY}/examples/Examples.cmake)
