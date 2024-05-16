cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/conf/cmake/")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

project(fsmod VERSION 0.1 DESCRIPTION "SimGrid-based File System simulation module")

include(GNUInstallDirs)
find_package(SimGrid 3.35.1 REQUIRED)

include_directories(
		${CMAKE_SOURCE_DIR}/include
		${SimGrid_INCLUDE_DIR}
		${CMAKE_BINARY_DIR}/include
		/opt/local/include
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# build the version number
set(FSMOD_VERSION_MAJOR "0")
set(FSMOD_VERSION_MINOR "1")
set(FSMOD_VERSION_PATCH "0")
set(FSMOD_VERSION_EXTRA "dev")

if (${FSMOD_VERSION_PATCH} EQUAL "0")
    set(FSMOD_RELEASE_VERSION "${FSMOD_VERSION_MAJOR}.${FSMOD_VERSION_MINOR}")
else ()
    set(FSMOD_RELEASE_VERSION "${FSMOD_VERSION_MAJOR}.${FSMOD_VERSION_MINOR}.${FSMOD_VERSION_PATCH}")
endif ()


set(CMAKEFILES_TXT
  		examples/CMakeLists.txt
)

set(SOURCE_FILES
		src/PathUtil.cpp
		src/FileSystem.cpp
		src/File.cpp
		src/Partition.cpp
		src/Storage.cpp
		src/JBODStorage.cpp
		src/OneDiskStorage.cpp
		)

set(HEADER_FILES
		include/fsmod/File.hpp
		include/fsmod/FileStat.hpp
		include/fsmod/FileSystemException.hpp
		include/fsmod/Partition.hpp
		include/fsmod/FileMetadata.hpp
		include/fsmod/JBODStorage.hpp
		include/fsmod/PathUtil.hpp
		include/fsmod/FileSystem.hpp
		include/fsmod/OneDiskStorage.hpp
		include/fsmod/Storage.hpp
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

### Define subdirectories
foreach(cmakefile ${CMAKEFILES_TXT})
  string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
  add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
endforeach()

# Google test
find_library(GTEST_LIBRARY NAMES gtest)
if(GTEST_LIBRARY)
	set(TEST_FILES
			test/jbod_storage_test.cpp
			test/one_disk_storage_test.cpp
			test/path_util_test.cpp
			test/file_system_test.cpp
			test/seek_test.cpp
			test/stat_test.cpp
			test/main.cpp
			test/test_util.hpp
			include/fsmod.hpp)

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

# Documentation
include(${CMAKE_HOME_DIRECTORY}/conf/cmake/Documentation.cmake)
