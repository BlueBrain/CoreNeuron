set(nmodl_makefile "corenrnmodl_makefile")
set(nmodl_compile_scipt "corenrnmodl")

get_directory_property(CORENRN_COMPILE_FLAGS COMPILE_DEFINITIONS)
if (CORENRN_COMPILE_FLAGS)
    set(CORENRN_COMPILE_DEFS "-D${CORENRN_COMPILE_FLAGS}")
    string(REPLACE ";" " -D" CORENRN_COMPILE_DEFS "${CORENRN_COMPILE_DEFS}")
endif()
# Link libraries
string(REPLACE ";" " " CORENRN_LINK_DEFS ${CORENRN_LINK_LIBS})
#if (CORENRN_LINK_LIBS)
#    set(CORENRN_LINK_DEFS "${CMAKE_LINK_LIBRARY_FLAG} ${CORENRN_LINK_LIBS}")
#    string(REPLACE ";" " ${CMAKE_LINK_LIBRARY_FLAG} " CORENRN_LINK_DEFS ${CORENRN_LINK_DEFS})
#endif()

configure_file(${nmodl_makefile}.in ${nmodl_makefile} @ONLY)
        
# Makefile
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${nmodl_makefile}
        DESTINATION ${CMAKE_INSTALL_DATADIR})

# Launcher
install(PROGRAMS ${nmodl_compile_scipt}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
