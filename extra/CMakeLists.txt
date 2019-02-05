set(nmodl_makefile "nrnivmodl_core_makefile")
set(nmodl_compile_scipt "nrnivmodl-core")

get_directory_property(CORENRN_COMPILE_FLAGS COMPILE_DEFINITIONS)

if (CORENRN_COMPILE_FLAGS)
    set(CORENRN_COMPILE_DEFS "-D${CORENRN_COMPILE_FLAGS}")
    string(REPLACE ";" " -D" CORENRN_COMPILE_DEFS "${CORENRN_COMPILE_DEFS}")
endif()

# Link libraries
string(REPLACE ";" " " CORENRN_LINK_DEFS "${CORENRN_LINK_LIBS}")
message("LINK LIBS = ${CORENRN_LINK_DEFS}")

# Compiler flags depending on BUILD_TYPE shared as BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
message("CXX Compile Flags from BUILD_TYPE: ${BUILD_TYPE_CXX_FLAGS}")


configure_file(${nmodl_makefile}.in ${nmodl_makefile} @ONLY)

# Makefile
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${nmodl_makefile}
        DESTINATION ${CMAKE_INSTALL_DATADIR})

# Launcher
install(PROGRAMS ${nmodl_compile_scipt}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
