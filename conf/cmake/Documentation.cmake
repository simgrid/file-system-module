find_package(Doxygen QUIET)
if (NOT DOXYGEN_FOUND) 
    message("-- Doxygen: No (warning: Doxygen is needed in case you want to generate FSMOD documentation)")
endif()


if (DOXYGEN_FOUND)

    message("-- Found Doxygen")

    # FSMOD APIs documentation
    foreach (SECTION USER)
        string(TOLOWER ${SECTION} SECTION_LOWER)
        set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/logs/Doxyfile_${SECTION})
        set(FSMOD_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")

        if (${SECTION} STREQUAL "USER")
            set(FSMOD_SECTIONS "USER")
        else ()
            set(FSMOD_SECTIONS "NODICE")
        endif ()

        set(FSMOD_SECTIONS_OUTPUT ${SECTION_LOWER})
        configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doc-${SECTION_LOWER}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating FSMOD ${SECTION} documentation" VERBATIM)
        add_custom_command(TARGET doc-${SECTION_LOWER}
                COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/${FSMOD_RELEASE_VERSION}/${SECTION_LOWER}
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT})

        LIST(APPEND FSMOD_SECTIONS_LIST doc-${SECTION_LOWER})
    endforeach ()

    # FSMOD documentation pages
    set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/logs/Doxyfile_pages)
    set(FSMOD_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/doc ${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")
    set(FSMOD_SECTIONS "USER")
    set(FSMOD_SECTIONS_OUTPUT "pages")

    configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

    get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_HOME_DIRECTORY}/docs")

    add_custom_target(doc DEPENDS fsmod ${FSMOD_SECTIONS_LIST})

    add_custom_command(TARGET doc
            COMMAND rm -rf ${CMAKE_HOME_DIRECTORY}/docs/source
            COMMAND cp -r ${CMAKE_HOME_DIRECTORY}/doc/source ${CMAKE_HOME_DIRECTORY}/docs)
    add_custom_command(TARGET doc COMMAND python3
                        ${CMAKE_HOME_DIRECTORY}/doc/scripts/generate_rst.py 
                        ${CMAKE_HOME_DIRECTORY}/docs/${FSMOD_RELEASE_VERSION}
                        ${CMAKE_HOME_DIRECTORY}/docs/source
                        ${FSMOD_RELEASE_VERSION})
    add_custom_command(TARGET doc COMMAND sphinx-build 
                        ${CMAKE_HOME_DIRECTORY}/docs/source
                        ${CMAKE_HOME_DIRECTORY}/docs/build/${FSMOD_RELEASE_VERSION})
    add_custom_command(TARGET doc COMMAND cp -R
                        ${CMAKE_HOME_DIRECTORY}/docs/build/${FSMOD_RELEASE_VERSION}
                        ${CMAKE_HOME_DIRECTORY}/docs/build/latest)

endif()
