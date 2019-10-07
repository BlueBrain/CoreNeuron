# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(coreneuron)

# =============================================================================
# CMake common project settings
# =============================================================================
set(VERSION_MAJOR "0")
set(VERSION_MINOR "16")
set(VERSION_PATCH "0")
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Settings to enable project as submodule
# =============================================================================
set(CORENEURON_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CORENEURON_AS_SUBPROJECT OFF)
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CORENEURON_AS_SUBPROJECT ON)
endif()

# =============================================================================
# Include cmake modules
# =============================================================================
list(APPEND CMAKE_MODULE_PATH
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/packages
            ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/config)

include(CheckIncludeFiles)
include(ReleaseDebugAutoFlags)
include(CrayPortability)
include(SetRpath)
include(CTest)
include(GNUInstallDirs)

# global options
option(CORENRN_ENABLE_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(CORENRN_ENABLE_TIMEOUT "Disable nrn_timeout implementation" ON)
option(CORENRN_ENABLE_REPORTINGLIB "Enable use of ReportingLib for soma reports" OFF)
option(CORENRN_ENABLE_MPI "Enable MPI-based execution" ON)
option(CORENRN_ENABLE_SOA "Enable SoA Memory Layout" ON)
option(CORENRN_ENABLE_HOC_EXP "Disable wrapping exp with hoc_exp()" OFF)
option(CORENRN_ENABLE_SPLAYTREE_QUEU "Enable use of Splay tree for spike queuing" ON)
option(CORENRN_ENABLE_NET_RECEIVE_BUFFER "Enable event buffering in net_receive function" ON)
option(CORENRN_ENABLE_ISPC "Enable ispc interoperability structs and data" OFF)
option(CORENRN_ENABLE_NMODL "Enable external nmodl source-to-source compiler" OFF)
option(CORENRN_ENABLE_CALIPER_PROFILING "Enable Caliper instrumentation" OFF)
option(CORENRN_ENABLE_LIKWID_PROFILING "Enable LIKWID instrumentation" OFF)
option(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY "Enable Unified Memory implementation of GPU" OFF)
option(CORENRN_ENABLE_UNIT_TESTS "Enable unit tests compilation and execution" ON)
option(CORENRN_ENABLE_FUNCTIONAL_TESTS "Enable functional tests compilation and execution" ON)

set(CORENRN_FRONTEND_C_COMPILER gcc
    CACHE FILEPATH "C compiler for building mod2c [frontend executable]")
set(CORENRN_FRONTEND_CXX_COMPILER g++
    CACHE FILEPATH "C++ compiler for building mod2c [frontend executable]")

set(CORENRN_NMODL_ROOT "" CACHE PATH "Path nmodl source-to-source compiler root")
set(CORENRN_MOD2C "" CACHE FILEPATH "Path of CORENRN_MOD2C binary")

# ispc support
if(CORENRN_ENABLE_ISPC)
  enable_language(ISPC)
  add_definitions("-DISPC_INTEROP=1")
  set(CORENRN_ENABLE_NMODL ON)
endif()

if(CORENRN_ENABLE_NMODL)
  find_package(nmodl REQUIRED)
  set(CORENRN_MOD2C "${nmodl_BINARY}")
  configure_file(${nmodl_BINARY} ${CMAKE_BINARY_DIR}/bin/nmodl COPYONLY)
  set(NMODL_EXTRA_FLAGS "" CACHE STRING "Extra NMODL options such as passes")
  separate_arguments(NMODL_EXTRA_FLAGS_LIST UNIX_COMMAND "${NMODL_EXTRA_FLAGS}")
  include_directories(${nmodl_INCLUDE})
  if("$ENV{PYTHONPATH}" STREQUAL "")
    set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}")
  else()
    set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
  endif()
else()
  include(AddMod2cSubmodule)
endif()

# Find all dependencies
if(CORENRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  add_definitions("-DNRNMPI=1")
else()
  message(STATUS "Use of MPI disabled by user-provided option")
  add_definitions("-DNRNMPI=0")
  add_definitions("-DNRN_MULTISEND=0")
endif()

# Memory layout transformation: 0 for SoA and 1 for AoS
if(CORENRN_ENABLE_SOA)
  add_definitions("-DLAYOUT=0")
else()
  add_definitions("-DLAYOUT=1")
endif()

# Disable hoc_exp wrapper for exp
if(NOT CORENRN_ENABLE_HOC_EXP)
  add_definitions("-DDISABLE_HOC_EXP")
endif()

# Enable use of splay tree for queuing (for models using net_move)
if(CORENRN_ENABLE_SPLAYTREE_QUEU)
  add_definitions("-DENABLE_SPLAYTREE_QUEUING")
endif()

# Turn off event buffering if asked
if(NOT CORENRN_ENABLE_NET_RECEIVE_BUFFER)
  add_definitions("-DNET_RECEIVE_BUFFERING=0")
endif()

set(Boost_NO_BOOST_CMAKE TRUE)
find_package(Boost 1.41.0 QUIET
             COMPONENTS filesystem
                        system
                        atomic
                        unit_test_framework)

# Threading
if(CORENRN_ENABLE_OPENMP)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
  endif(OPENMP_FOUND)
endif(CORENRN_ENABLE_OPENMP)

# ReportingLib
if(CORENRN_ENABLE_REPORTINGLIB)
  find_package(reportinglib REQUIRED)
  if(reportinglib_FOUND)
    include_directories(${reportinglib_INCLUDE_DIR})
    add_definitions("-DENABLE_REPORTING")
  else(reportinglib_FOUND)
    message(FATAL_ERROR "Enabled use of ReportingLib but ReportingLib installion not found!")
  endif(reportinglib_FOUND)
endif(CORENRN_ENABLE_REPORTINGLIB)

# Code instrumentation
if(CORENRN_ENABLE_CALIPER_PROFILING)
  find_package(caliper REQUIRED)
  include_directories(${caliper_INCLUDE_DIR})
  add_definitions("-DCORENEURON_CALIPER")
  set(CALIPER_LIB "caliper")
  if(CORENRN_ENABLE_MPI)
    set(CALIPER_MPI_LIB "caliper-mpi")
  endif()
endif(CORENRN_ENABLE_CALIPER_PROFILING)

# Code instrumentation
set(LIKWID_ROOT "" CACHE PATH "Path likwid performance monitoring and benchmarking suite")
if(CORENRN_ENABLE_LIKWID_PROFILING)
  find_package(likwid REQUIRED)
  include_directories(${likwid_INCLUDE_DIRS})
  add_definitions("-DLIKWID_PERFMON")
endif(CORENRN_ENABLE_LIKWID_PROFILING)

check_include_files(malloc.h have_malloc_h)
if(have_malloc_h)
  add_definitions("-DHAVE_MALLOC_H")
endif()

# Some mechanisms use NEURON specific code which is skipped using this macro for CoreNeuron build
add_definitions(-DCORENEURON_BUILD)

option(ENABLE_SELECTIVE_GPU_PROFILING "Enable GPU profiling only for Solver" ON)
option(ENABLE_OPENACC "Enable use of OpenACC" OFF)

set(CLANG_FORMAT_MIN_VERSION "4.0")
set(CLANG_FORMAT_MAX_VERSION "4.9")
find_package(ClangFormat)
if(CLANG_FORMAT_FOUND)
  message("clang-format : ${CLANG_FORMAT_EXECUTABLE} : ${CLANG_FORMAT_VERSION}")
endif()

if(CLANG_FORMAT_FOUND)
  file(COPY ${CORENEURON_PROJECT_SOURCE_DIR}/.clang-format DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  # to indent files using clang-format
  file(GLOB_RECURSE SRC_FILES_FOR_CLANG_FORMAT
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.c
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.cpp
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.h*
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/*.ipp*)

  # exclude ezoption header file
  list(REMOVE_ITEM SRC_FILES_FOR_CLANG_FORMAT
                   ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/ezoption/ezOptionParser.hpp
                   ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/endianness.h
                   ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/swap_endian.h)

  # exclude random123 files
  file(GLOB_RECURSE RANDOM123_FILES
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/randoms/Random123/*.cpp
                    ${CORENEURON_PROJECT_SOURCE_DIR}/coreneuron/utils/randoms/Random123/*.h)
  foreach(R123_PATH ${RANDOM123_FILES})
    list(REMOVE_ITEM SRC_FILES_FOR_CLANG_FORMAT ${R123_PATH})
  endforeach(R123_PATH)

  add_custom_target(formatsource
                    COMMAND ${CMAKE_COMMAND} -DSOURCE_FILES:STRING="${SRC_FILES_FOR_CLANG_FORMAT}"
                            -DCLANG_FORMAT_EXECUTABLE=${CLANG_FORMAT_EXECUTABLE} -P
                            "${CORENEURON_PROJECT_SOURCE_DIR}/CMake/ClangFormatUtils.cmake")
endif()

# Turn off inline assembly for PGI
if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
  add_definitions(-DSWAP_ENDIAN_DISABLE_ASM)
endif()

if(ENABLE_OPENACC)
  set(COMPILE_LIBRARY_TYPE "STATIC")
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    add_definitions(-DUNIFIED_MEMORY)
  endif()
  if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
    add_definitions(-DPG_ACC_BUGS)
    set(ACC_FLAGS "-acc -Minline=size:200,levels:10")
    set(CMAKE_C_FLAGS "${ACC_FLAGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${ACC_FLAGS} ${CMAKE_CXX_FLAGS}")
  else()
    message(WARNING "OpenACC implementation is only supported and tested using only PGI")
    message(WARNING "Add required compiler flags to enabled OpenACC")
  endif()

  find_package(CUDA 5.0 REQUIRED)
  set(CUDA_SEPARABLE_COMPILATION ON)
  if(CUDA_FOUND)
    if(ENABLE_SELECTIVE_GPU_PROFILING)
      add_definitions(-DCUDA_PROFILING)
    endif(ENABLE_SELECTIVE_GPU_PROFILING)
  endif(CUDA_FOUND)
else(ENABLE_OPENACC)
  set(ENABLE_SELECTIVE_GPU_PROFILING OFF)
  # OpenACC pragmas are not guarded, disable all unknown pragm warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif(ENABLE_OPENACC)

if(NOT CORENRN_ENABLE_TIMEOUT)
  add_definitions("-DDISABLE_TIMEOUT")
endif()

# Core library and main exe
add_subdirectory(coreneuron)
add_subdirectory(apps)
add_subdirectory(tests)
add_subdirectory(extra)

install(FILES CMake/coreneuron-config.cmake DESTINATION share/cmake)
install(EXPORT coreneuron DESTINATION share/cmake)
