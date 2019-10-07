# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include_directories(${CORENEURON_PROJECT_SOURCE_DIR}
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${PROJECT_BINARY_DIR}/coreneuron)

add_executable(coreneuron_exec "${CMAKE_CURRENT_SOURCE_DIR}/coreneuron.cpp")

configure_file(coreneuron.cpp ${CMAKE_BINARY_DIR}/share/coreneuron/coreneuron.cpp)

target_link_libraries(coreneuron_exec
                      corenrnmech
                      ${MPI_C_LIBRARIES}
                      ${MPI_CXX_LIBRARIES}
                      ${reportinglib_LIBRARY})

set_target_properties(coreneuron_exec PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS coreneuron_exec DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Install main to recompile for special-core
install(FILES coreneuron.cpp DESTINATION ${CMAKE_INSTALL_DATADIR}/coreneuron)
