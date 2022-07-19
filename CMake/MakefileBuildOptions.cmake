# =============================================================================
# Copyright (C) 2016-2022 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Common CXX and ISPC flags
# =============================================================================

# ISPC should compile with --pic by default
set(CMAKE_ISPC_FLAGS "${CMAKE_ISPC_FLAGS} --pic")

# =============================================================================
# NMODL CLI options : common and backend specific
# =============================================================================
# ~~~
# if user pass arguments then use those as common arguments
# note that inlining is done by default
# ~~~
set(NMODL_COMMON_ARGS "passes --inline")

if(NOT "${CORENRN_NMODL_FLAGS}" STREQUAL "")
  string(APPEND NMODL_COMMON_ARGS " ${CORENRN_NMODL_FLAGS}")
endif()

set(NMODL_CPU_BACKEND_ARGS "host --c")
set(NMODL_ISPC_BACKEND_ARGS "host --ispc")
set(NMODL_ACC_BACKEND_ARGS "host --c acc --oacc")

# =============================================================================
# Construct the linker arguments that are used inside nrnivmodl-core (to build
# libcoreneuron from libcoreneuron-core, libcoreneuron-cuda and mechanism object
# files) and inside nrnivmodl (to link NEURON's special against CoreNEURON's
# libcoreneuron).
# =============================================================================
# Essentially we "just" want to unpack the CMake dependencies of the
# `coreneuron-core` target into a plain string that we can bake into the
# Makefiles in both NEURON and CoreNEURON.
function(coreneuron_process_target target)
  if(TARGET ${target})
    if(NOT target STREQUAL "coreneuron-core")
      # This is a special case: libcoreneuron-core.a is manually unpacked into .o
      # files by the nrnivmodl-core Makefile, so we do not want to also emit an
      # -lcoreneuron-core argument.
      # TODO: probably need to extract an -L and RPATH path and include that here?
      set_property(GLOBAL APPEND_STRING PROPERTY CORENEURON_LIB_LINK_FLAGS " -l${target}")
    endif()
    get_target_property(target_libraries ${target} LINK_LIBRARIES)
    if(target_libraries)
      foreach(child_target ${target_libraries})
        coreneuron_process_target(${child_target})
      endforeach()  
    endif()
    return()
  endif()
  get_filename_component(target_dir "${target}" DIRECTORY)
  message(STATUS "target=${target} target_dir=${target_dir}")
  if(NOT target_dir)
    # In case target is not a target but is just the name of a library, e.g. "dl"
    set_property(GLOBAL APPEND_STRING PROPERTY CORENEURON_LIB_LINK_FLAGS " -l${target}")
  elseif("${target_dir}" MATCHES "^(/lib|/lib64|/usr/lib|/usr/lib64)$")
    # e.g. /usr/lib64/libpthread.so -> -lpthread
    get_filename_component(libname ${target} NAME_WE)
    string(REGEX REPLACE "^lib" "" libname ${libname})
    set_property(GLOBAL APPEND_STRING PROPERTY CORENEURON_LIB_LINK_FLAGS " -l${libname}")
  else()
    # It's a full path, include that on the line
    set_property(GLOBAL APPEND_STRING PROPERTY CORENEURON_LIB_LINK_FLAGS " ${target}")
  endif()
endfunction()
coreneuron_process_target(coreneuron-core)
get_property(CORENEURON_LIB_LINK_FLAGS GLOBAL PROPERTY CORENEURON_LIB_LINK_FLAGS)
message(STATUS "CORENEURON_LIB_LINK_FLAGS=${CORENEURON_LIB_LINK_FLAGS}")

# Things that used to be in CORENEURON_LIB_LINK_FLAGS: -rdynamic -lrt
# -Wl,--whole-archive -L${CMAKE_HOST_SYSTEM_PROCESSOR} -Wl,--no-whole-archive
# -L${caliper_LIB_DIR} -l${CALIPER_LIB}

# =============================================================================
# Turn CORENRN_COMPILE_DEFS into a list of -DFOO[=BAR] options.
# =============================================================================
list(TRANSFORM CORENRN_COMPILE_DEFS PREPEND -D OUTPUT_VARIABLE CORENRN_COMPILE_DEF_FLAGS)

# =============================================================================
# Extra link flags that we need to include when linking libcoreneuron.{a,so} in
# CoreNEURON but that do not need to be passed to NEURON to use when linking
# nrniv/special (why?)
# =============================================================================
string(JOIN " " CORENRN_COMMON_LDFLAGS ${CORENEURON_LIB_LINK_FLAGS} ${CORENRN_EXTRA_LINK_FLAGS})
if(CORENRN_SANITIZER_LIBRARY_DIR)
  string(APPEND CORENRN_COMMON_LDFLAGS " -Wl,-rpath,${CORENRN_SANITIZER_LIBRARY_DIR}")
endif()
string(JOIN " " CORENRN_SANITIZER_ENABLE_ENVIRONMENT_STRING ${CORENRN_SANITIZER_ENABLE_ENVIRONMENT})

# =============================================================================
# compile flags : common to all backend
# =============================================================================
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE)
string(
  JOIN
  " "
  CORENRN_CXX_FLAGS
  ${CMAKE_CXX_FLAGS}
  ${CMAKE_CXX_FLAGS_${_BUILD_TYPE}}
  ${CMAKE_CXX17_STANDARD_COMPILE_OPTION}
  ${NVHPC_ACC_COMP_FLAGS}
  ${NVHPC_CXX_INLINE_FLAGS}
  ${CORENRN_COMPILE_DEF_FLAGS}
  ${CORENRN_EXTRA_MECH_CXX_FLAGS})

# =============================================================================
# nmodl/mod2c related options : TODO
# =============================================================================
# name of nmodl/mod2c binary
get_filename_component(nmodl_name ${CORENRN_MOD2CPP_BINARY} NAME)
set(nmodl_binary_name ${nmodl_name})
