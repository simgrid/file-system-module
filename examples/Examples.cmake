foreach (example jbod open_seek_write)
  if(NOT DEFINED _${example}_sources)
    set(_${example}_sources ${CMAKE_HOME_DIRECTORY}/examples/${example}.cpp)
  endif()

  add_executable       (${example} EXCLUDE_FROM_ALL ${_${example}_sources})
  add_dependencies     (examples ${example})
  target_link_libraries(${example} ${SimGrid_LIBRARY} fsmod)
  set_target_properties(${example} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples)
  set_target_properties(${example} PROPERTIES COMPILE_FLAGS "-g -O0 --coverage")
  set_target_properties(${example} PROPERTIES LINK_FLAGS "--coverage")

  foreach(file ${_${example}_sources})
    set(examples_src  ${examples_src} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
  endforeach()

endforeach()