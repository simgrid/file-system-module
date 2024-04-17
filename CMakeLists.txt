cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

project(fsmod VERSION 0.1 DESCRIPTION "SimGrid-based File System simulation module")

include(GNUInstallDirs)
find_package(SimGrid 3.35.1 REQUIRED)

#add_definitions("-DBOOST_ALLOW_DEPRECATED_HEADERS")


include_directories(
		${SimGrid_INCLUDE_DIR}
		${CMAKE_BINARY_DIR}/include
		/opt/local/include
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKEFILES_TXT
#  tests/s4u/CMakeLists.txt
#)

add_library(fsmod SHARED
		src/PathUtil.cpp
		src/FileSystem.cpp
		src/File.cpp
		src/Partition.cpp
		src/Storage.cpp
		src/OneDiskStorage.cpp
		)

add_executable(path_test examples/path_test.cpp)
target_link_libraries(path_test fsmod ${SimGrid_LIBRARY})

add_executable(open_seek_write_test examples/open_seek_write_test.cpp)
target_link_libraries(open_seek_write_test fsmod ${SimGrid_LIBRARY})

set_target_properties(fsmod PROPERTIES
		SOVERSION 0.1
		LINKER_LANGUAGE CXX
		PUBLIC_HEADER fsmod.hpp)

target_include_directories(fsmod PRIVATE include)
target_link_libraries(fsmod PUBLIC ${SimGrid_LIBRARY})

install(TARGETS fsmod
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#add_custom_target(tests    COMMENT "Recompiling the tests")
### Define subdirectories
#foreach(cmakefile ${CMAKEFILES_TXT})
#  string(REPLACE "/CMakeLists.txt" "" repository ${cmakefile})
#  add_subdirectory("${CMAKE_HOME_DIRECTORY}/${repository}")
#endforeach()
