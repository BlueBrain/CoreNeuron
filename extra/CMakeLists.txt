# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

set(nrnivmodl_core_makefile "nrnivmodl_core_makefile")
set(nrnivmodl_core "nrnivmodl-core")

get_directory_property(CORENRN_COMPILE_FLAGS COMPILE_DEFINITIONS)

if(CORENRN_COMPILE_FLAGS)
  set(CORENRN_COMPILE_DEFS "-D${CORENRN_COMPILE_FLAGS}")
  string(REPLACE ";"
                 " -D"
                 CORENRN_COMPILE_DEFS
                 "${CORENRN_COMPILE_DEFS}")
endif()

# Link libraries CMake does some magic to transform sys libs to -l<libname>. We replicate it
foreach(link_lib ${CORENRN_LINK_LIBS})
  get_filename_component(pth ${link_lib} DIRECTORY)
  if(NOT pth)
    string(APPEND CORENRN_LINK_DEFS " -l${link_lib}")
  elseif("${pth}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    get_filename_component(libname ${link_lib} NAME_WE)
    string(REGEX
           REPLACE "^lib"
                   ""
                   libname
                   ${libname})
    string(APPEND CORENRN_LINK_DEFS " -l${libname}")
  else()
    string(APPEND CORENRN_LINK_DEFS " ${link_lib}")
  endif()
endforeach()

# PGI add --c++11;-A option for c++11 flag
string(REPLACE ";"
               " "
               CXX11_STANDARD_COMPILE_OPTION
               "${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")

# MPI path might be multiple directories
if(MPI_C_INCLUDE_PATH)
  string(REPLACE ";"
                 " -I"
                 CORENRN_MPI_INCLUDES
                 "${MPI_C_INCLUDE_PATH}")
endif()

# Compiler flags depending on BUILD_TYPE shared as BUILD_TYPE_<LANG>_FLAGS
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
set(BUILD_TYPE_C_FLAGS "${CMAKE_C_FLAGS_${_BUILD_TYPE}}")
set(BUILD_TYPE_CXX_FLAGS "${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}")
message(STATUS "CXX Compile flags from BUILD_TYPE: ${BUILD_TYPE_CXX_FLAGS}")

if(CORENRN_ENABLE_NMODL)
  set(nmodl_arguments_c "host --c ${CORENRN_NMODL_FLAGS}")
  set(nmodl_arguments_ispc "host --ispc ${CORENRN_NMODL_FLAGS}")
  get_filename_component(nmodl_name ${nmodl_BINARY} NAME)
  set(mod2c_bin_name ${nmodl_name})
else()
  set(nmodl_arguments_c "")
  set(nmodl_arguments_ispc "")
  set(mod2c_bin_name "mod2c_core")
endif()

# copy into build directory
configure_file(${nrnivmodl_core_makefile}.in ${CMAKE_BINARY_DIR}/bin/${nrnivmodl_core_makefile}
               @ONLY)
configure_file(${nrnivmodl_core} ${CMAKE_BINARY_DIR}/bin/${nrnivmodl_core} @ONLY)

# install
install(FILES ${CMAKE_BINARY_DIR}/bin/${nrnivmodl_core_makefile}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/${nrnivmodl_core} DESTINATION ${CMAKE_INSTALL_BINDIR})
