set(nmodl_makefile "corenrnmodl_makefile")
set(nmodl_compile_scipt "corenrnmodl")

configure_file(${nmodl_makefile}.in ${nmodl_makefile} @ONLY)

# Makefile
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${nmodl_makefile}
        DESTINATION ${CMAKE_INSTALL_DATADIR})

# Launcher
install(PROGRAMS ${nmodl_compile_scipt}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
