find_package(Doxygen QUIET)
if (NOT DOXYGEN_FOUND) 
    message("-- Doxygen: No (warning: Doxygen is needed in case you want to generate FSMOD documentation)")
endif()


if (DOXYGEN_FOUND)

    message("-- Found Doxygen")

    # FSMOD APIs documentation
    foreach (SECTION USER)
        string(TOLOWER ${SECTION} SECTION_LOWER)
	set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs/logs/Doxyfile_${SECTION})
        set(FSMOD_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")

        if (${SECTION} STREQUAL "USER")
            set(FSMOD_SECTIONS "USER")
        else ()
            set(FSMOD_SECTIONS "NODICE")
        endif ()

        set(FSMOD_SECTIONS_OUTPUT ${SECTION_LOWER})
        configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doc-${SECTION_LOWER}
                WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
                COMMENT "Generating FSMOD ${SECTION} documentation" VERBATIM)
        add_custom_command(TARGET doc-${SECTION_LOWER} POST_BUILD
		COMMAND echo "HERE"
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/docs/${FSMOD_RELEASE_VERSION}/${SECTION_LOWER}
		COMMAND echo "CALLING DOXYGEN: ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}"
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		COMMAND echo "CALLING DOX")

        LIST(APPEND FSMOD_SECTIONS_LIST doc-${SECTION_LOWER})
    endforeach ()

    # FSMOD documentation pages
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs/logs/Doxyfile_pages)
    set(FSMOD_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/doc ${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")
    set(FSMOD_SECTIONS "USER")
    set(FSMOD_SECTIONS_OUTPUT "pages")

    configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

    get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_CURRENT_BINARY_DIR}/docs")

    add_custom_target(doc DEPENDS fsmod ${FSMOD_SECTIONS_LIST})

    add_custom_command(TARGET doc POST_BUILD
	    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/source
	    COMMAND cp -r ${CMAKE_HOME_DIRECTORY}/doc/source ${CMAKE_CURRENT_BINARY_DIR}/docs)
    add_custom_command(TARGET doc POST_BUILD
	    COMMAND python3 ${CMAKE_HOME_DIRECTORY}/doc/scripts/generate_rst.py ${CMAKE_CURRENT_BINARY_DIR}/docs/${FSMOD_RELEASE_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/docs/source ${FSMOD_RELEASE_VERSION})
    add_custom_command(TARGET doc POST_BUILD
	    COMMAND sphinx-build ${CMAKE_CURRENT_BINARY_DIR}/docs/source ${CMAKE_CURRENT_BINARY_DIR}/docs/build/${FSMOD_RELEASE_VERSION})
    add_custom_command(TARGET doc POST_BUILD
	    COMMAND cp -R ${CMAKE_CURRENT_BINARY_DIR}/docs/build/${FSMOD_RELEASE_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/docs/build/latest)
    add_custom_command(TARGET doc POST_BUILD
	    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/${FSMOD_RELEASE_VERSION}
	    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/latest
	    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/logs
	    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/docs/source
	    COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/docs/build/* ${CMAKE_CURRENT_BINARY_DIR}/docs
	    COMMAND rmdir ${CMAKE_CURRENT_BINARY_DIR}/docs/build)


endif()
