# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
# CoreNEURON's version jumped from 1.0 to 8.2.0 with the introduction of the NRN_VERSION_* macros
# for use in VERBATIM blocks. Starting from this version, the NEURON and CoreNEURON versions are
# locked together. A version has to be hardcoded here to handle the case that CoreNEURON is built
# standalone.
project(
  coreneuron
  VERSION 9.0.0
  LANGUAGES CXX)

# ~~~
# It is a bad idea having floating point versions, since macros cant handle them
# We therefore, have version as an int, which is pretty much standard
# ~~~
math(EXPR CORENEURON_VERSION_COMBINED
     "${coreneuron_VERSION_MAJOR} * 100 + ${coreneuron_VERSION_MINOR}")

# =============================================================================
# CMake common project settings
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE
    RelWithDebInfo
    CACHE STRING "Empty or one of Debug, Release, RelWithDebInfo")

if(NOT "cxx_std_17" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  message(
    FATAL_ERROR
      "This compiler does not fully support C++17, choose a higher version or another compiler.")
endif()

# =============================================================================
# Settings to enable project as submodule
# =============================================================================
set(CORENEURON_PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CORENEURON_PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CORENEURON_AS_SUBPROJECT OFF)
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CORENEURON_AS_SUBPROJECT ON)
  # Make these visible to the parent project (NEURON) so it can do some sanity checking.
  set_property(GLOBAL PROPERTY CORENRN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set_property(GLOBAL PROPERTY CORENRN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set_property(GLOBAL PROPERTY CORENRN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
endif()
if(NOT DEFINED NRN_VERSION_MAJOR
   OR NOT DEFINED NRN_VERSION_MINOR
   OR NOT DEFINED NRN_VERSION_PATCH)
  if(CORENEURON_AS_SUBPROJECT)
    set(level WARNING)
  else()
    set(level STATUS)
  endif()
  # Typically in this case CoreNEURON is being built standalone. In this case NRN_VERSION_* macros
  # resolve to the CoreNEURON version, which is supposed to be moving in lockstep with the NEURON
  # version.
  set(NRN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(NRN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(NRN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  message(${level} "CoreNEURON could not determine the NEURON version, using the hardcoded "
          "${NRN_VERSION_MAJOR}.${NRN_VERSION_MINOR}.${NRN_VERSION_PATCH}")
endif()
# Regardless of whether we are being built as a submodule of NEURON, NRN_VERSION_{MAJOR,MINOR,PATCH}
# are now set to the version that we should claim compatibility with when compiling translated MOD
# files. Generate a header under a special `generated` prefix in the build directory, so that
# -I/path/to/src -I/path/to/build/generated is safe (headers from the source prefix are copied
# elsewhere under the build prefix, so there is scope for confusion)
configure_file(coreneuron/config/neuron_version.hpp.in
               generated/coreneuron/config/neuron_version.hpp)

# =============================================================================
# Include cmake modules path
# =============================================================================
list(APPEND CMAKE_MODULE_PATH ${CORENEURON_PROJECT_SOURCE_DIR}/CMake
     ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/packages ${CORENEURON_PROJECT_SOURCE_DIR}/CMake/config)

# =============================================================================
# HPC Coding Conventions
# =============================================================================
set(CODING_CONV_PREFIX "CORENRN")
set(CORENRN_3RDPARTY_DIR "external")
include(AddHpcCodingConvSubmodule)
add_subdirectory(CMake/hpc-coding-conventions/cpp)

# =============================================================================
# Enable sanitizer support if the CORENRN_SANITIZERS variable is set
# =============================================================================
include(CMake/hpc-coding-conventions/cpp/cmake/sanitizers.cmake)
set(CORENRN_EXTRA_CXX_FLAGS
    ""
    CACHE STRING "Add extra compile flags for CoreNEURON sources")
separate_arguments(CORENRN_EXTRA_CXX_FLAGS)
set(CORENRN_EXTRA_MECH_CXX_FLAGS
    ""
    CACHE STRING "Add extra compile flags for translated mechanisms")
separate_arguments(CORENRN_EXTRA_MECH_CXX_FLAGS)
list(APPEND CORENRN_EXTRA_CXX_FLAGS ${CORENRN_SANITIZER_COMPILER_FLAGS})
list(APPEND CORENRN_EXTRA_MECH_CXX_FLAGS ${CORENRN_SANITIZER_COMPILER_FLAGS})
list(APPEND CORENRN_EXTRA_LINK_FLAGS ${CORENRN_SANITIZER_COMPILER_FLAGS})

# =============================================================================
# Include common cmake modules
# =============================================================================
include(CheckIncludeFiles)
include(ReleaseDebugAutoFlags)
include(CrayPortability)
include(SetRpath)
include(CTest)
include(AddRandom123Submodule)
include(GitRevision)

set(CORENRN_3RDPARTY_DIR external)
include(CMake/hpc-coding-conventions/cpp/cmake/3rdparty.cmake)
cpp_cc_git_submodule(CLI11 BUILD PACKAGE CLI11 REQUIRED)

# =============================================================================
# Build options
# =============================================================================
option(CORENRN_ENABLE_OPENMP "Build the CORE NEURON with OpenMP implementation" ON)
option(CORENRN_ENABLE_OPENMP_OFFLOAD "Prefer OpenMP target offload to OpenACC" ON)
option(CORENRN_ENABLE_TIMEOUT "Enable nrn_timeout implementation" ON)
option(CORENRN_ENABLE_REPORTING "Enable use of ReportingLib for soma reports" OFF)
option(CORENRN_ENABLE_MPI "Enable MPI-based execution" ON)
option(CORENRN_ENABLE_MPI_DYNAMIC "Enable dynamic MPI support" OFF)
option(CORENRN_ENABLE_HOC_EXP "Enable wrapping exp with hoc_exp()" OFF)
option(CORENRN_ENABLE_SPLAYTREE_QUEUING "Enable use of Splay tree for spike queuing" ON)
option(CORENRN_ENABLE_NET_RECEIVE_BUFFER "Enable event buffering in net_receive function" ON)
option(CORENRN_ENABLE_NMODL "Enable external nmodl source-to-source compiler" OFF)
option(CORENRN_ENABLE_CALIPER_PROFILING "Enable Caliper instrumentation" OFF)
option(CORENRN_ENABLE_LIKWID_PROFILING "Enable LIKWID instrumentation" OFF)
option(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY "Enable CUDA unified memory support" OFF)
option(CORENRN_ENABLE_UNIT_TESTS "Enable unit tests execution" ON)
option(CORENRN_ENABLE_GPU "Enable GPU support using OpenACC or OpenMP" OFF)
option(CORENRN_ENABLE_SHARED "Enable shared library build" ON)
option(CORENRN_ENABLE_LEGACY_UNITS "Enable legacy FARADAY, R, etc" OFF)
option(CORENRN_ENABLE_PRCELLSTATE "Enable NRN_PRCELLSTATE debug feature" OFF)

set(CORENRN_NMODL_DIR
    ""
    CACHE PATH "Path to nmodl source-to-source compiler installation")
set(LIKWID_DIR
    ""
    CACHE PATH "Path to likwid performance analysis suite")

# Older CMake versions label NVHPC as PGI, newer ones label it as NVHPC.
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "PGI" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "NVHPC")
  set(CORENRN_HAVE_NVHPC_COMPILER ON)
else()
  set(CORENRN_HAVE_NVHPC_COMPILER OFF)
endif()

set(CORENRN_ACCELERATOR_OFFLOAD "Disabled")
if(CORENRN_ENABLE_GPU)
  # Older CMake versions than 3.15 have not been tested for GPU/CUDA/OpenACC support after
  # https://github.com/BlueBrain/CoreNeuron/pull/609.

  # Fail hard and early if we don't have the PGI/NVHPC compiler.
  if(NOT CORENRN_HAVE_NVHPC_COMPILER)
    message(
      FATAL_ERROR
        "GPU support is available via OpenACC using PGI/NVIDIA compilers."
        " Use NVIDIA HPC SDK with -DCMAKE_C_COMPILER=nvc -DCMAKE_CUDA_COMPILER=nvcc -DCMAKE_CXX_COMPILER=nvc++"
    )
  endif()

  # Set some sensible default CUDA architectures.
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 70 80)
    message(STATUS "Setting default CUDA architectures to ${CMAKE_CUDA_ARCHITECTURES}")
  endif()

  # See https://gitlab.kitware.com/cmake/cmake/-/issues/23081, this should not be needed according
  # to the CMake documentation, but it is not clear that any version behaves as documented.
  if(DEFINED CMAKE_CUDA_HOST_COMPILER)
    unset(ENV{CUDAHOSTCXX})
  endif()

  # Enable CUDA language support.
  enable_language(CUDA)

  # Prefer shared libcudart.so
  if(${CMAKE_VERSION} VERSION_LESS 3.17)
    # Ugly workaround from https://gitlab.kitware.com/cmake/cmake/-/issues/17559, remove when
    # possible
    if(CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES)
      list(REMOVE_ITEM CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudart_static")
      list(REMOVE_ITEM CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudadevrt")
      list(APPEND CMAKE_CUDA_HOST_IMPLICIT_LINK_LIBRARIES "cudart")
    endif()
    if(CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES)
      list(REMOVE_ITEM CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudart_static")
      list(REMOVE_ITEM CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudadevrt")
      list(APPEND CMAKE_CUDA_IMPLICIT_LINK_LIBRARIES "cudart")
    endif()
  else()
    # nvc++ -cuda implicitly links dynamically to libcudart.so. Setting this makes sure that CMake
    # does not add -lcudart_static and trigger errors due to mixed dynamic/static linkage.
    set(CMAKE_CUDA_RUNTIME_LIBRARY Shared)
  endif()

  # Patch CUDA_ARCHITECTURES support into older CMake versions
  if(${CMAKE_VERSION} VERSION_LESS 3.18)
    foreach(cuda_arch ${CMAKE_CUDA_ARCHITECTURES})
      string(
        APPEND CMAKE_CUDA_FLAGS
        " --generate-code=arch=compute_${cuda_arch},code=[compute_${cuda_arch},sm_${cuda_arch}]")
    endforeach()
  endif()

  # ~~~
  # Needed for the Eigen GPU support Warning suppression (Eigen GPU-related):
  # 3057 : Warning on ignoring __host__ annotation in some functions
  # 3085 : Warning on redeclaring a __host__ function as __host__ __device__
  # ~~~
  set(CMAKE_CUDA_FLAGS
      "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr -Xcudafe --diag_suppress=3057,--diag_suppress=3085"
  )

  if(CORENRN_ENABLE_NMODL)
    # NMODL supports both OpenACC and OpenMP target offload
    if(CORENRN_ENABLE_OPENMP AND CORENRN_ENABLE_OPENMP_OFFLOAD)
      set(CORENRN_ACCELERATOR_OFFLOAD "OpenMP")
    else()
      set(CORENRN_ACCELERATOR_OFFLOAD "OpenACC")
    endif()
  else()
    # MOD2C only supports OpenACC offload
    set(CORENRN_ACCELERATOR_OFFLOAD "OpenACC")
  endif()
endif()

# =============================================================================
# Project version from git and project directories
# =============================================================================
set(CN_PROJECT_VERSION ${PROJECT_VERSION})

# generate file with version number from git and nrnunits.lib file path
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/coreneuron/config/config.cpp.in
               ${PROJECT_BINARY_DIR}/coreneuron/config/config.cpp @ONLY)

# =============================================================================
# Include cmake modules after cmake options
# =============================================================================
include(OpenAccHelper)

# =============================================================================
# Common dependencies
# =============================================================================
find_package(PythonInterp REQUIRED)
find_package(Perl REQUIRED)

# =============================================================================
# Common build options
# =============================================================================
# build mod files for coreneuron
list(APPEND CORENRN_COMPILE_DEFS CORENEURON_BUILD)
set(CMAKE_REQUIRED_QUIET TRUE)
check_include_files(malloc.h have_malloc_h)
if(have_malloc_h)
  list(APPEND CORENRN_COMPILE_DEFS HAVE_MALLOC_H)
endif()

# =============================================================================
# Build option specific compiler flags
# =============================================================================
if(CORENRN_ENABLE_NMODL)
  # We use Eigen for "small" matrices with thread-level parallelism handled at a higher level; tell
  # Eigen not to try to multithread internally
  list(APPEND CORENRN_COMPILE_DEFS EIGEN_DONT_PARALLELIZE)
endif()
if(CORENRN_HAVE_NVHPC_COMPILER)
  # PGI with llvm code generation doesn't have necessary assembly intrinsic headers
  list(APPEND CORENRN_COMPILE_DEFS EIGEN_DONT_VECTORIZE=1)
  if(NOT CORENRN_ENABLE_GPU AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 21.11)
    # Random123 does not play nicely with NVHPC 21.11+'s detection of ABM features if it detects the
    # compiler to be PGI or NVHPC, see: https://github.com/BlueBrain/CoreNeuron/issues/724 and
    # https://github.com/DEShawResearch/random123/issues/6. In fact in GPU builds Random123
    # (mis)detects nvc++ as nvcc because we pass the -cuda option and we therefore avoid the
    # problem. If GPU support is disabled, we define R123_USE_INTRIN_H=0 to avoid the problem.
    list(APPEND CORENRN_COMPILE_DEFS R123_USE_INTRIN_H=0)
  endif()
  # CMake versions <3.19 used to add -A when using NVHPC/PGI, which makes the compiler excessively
  # pedantic. See https://gitlab.kitware.com/cmake/cmake/-/issues/20997.
  if(CMAKE_VERSION VERSION_LESS 3.19)
    list(REMOVE_ITEM CMAKE_CXX17_STANDARD_COMPILE_OPTION -A)
  endif()
endif()

if(CORENRN_ENABLE_SHARED)
  set(COMPILE_LIBRARY_TYPE "SHARED")
else()
  set(COMPILE_LIBRARY_TYPE "STATIC")
endif()

if(CORENRN_ENABLE_MPI)
  find_package(MPI REQUIRED)
  list(APPEND CORENRN_COMPILE_DEFS NRNMPI=1)
  # avoid linking to C++ bindings
  list(APPEND CORENRN_COMPILE_DEFS MPI_NO_CPPBIND=1)
  list(APPEND CORENRN_COMPILE_DEFS OMPI_SKIP_MPICXX=1)
  list(APPEND CORENRN_COMPILE_DEFS MPICH_SKIP_MPICXX=1)
else()
  list(APPEND CORENRN_COMPILE_DEFS NRNMPI=0)
  list(APPEND CORENRN_COMPILE_DEFS NRN_MULTISEND=0)
endif()

if(CORENRN_ENABLE_OPENMP)
  find_package(OpenMP QUIET)
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${ADDITIONAL_THREADSAFE_FLAGS}")
  endif()
endif()

list(APPEND CORENRN_COMPILE_DEFS LAYOUT=0)

if(NOT CORENRN_ENABLE_HOC_EXP)
  list(APPEND CORENRN_COMPILE_DEFS DISABLE_HOC_EXP)
endif()

# splay tree required for net_move
if(CORENRN_ENABLE_SPLAYTREE_QUEUING)
  list(APPEND CORENRN_COMPILE_DEFS ENABLE_SPLAYTREE_QUEUING)
endif()

if(NOT CORENRN_ENABLE_NET_RECEIVE_BUFFER)
  list(APPEND CORENRN_COMPILE_DEFS NET_RECEIVE_BUFFERING=0)
endif()

if(NOT CORENRN_ENABLE_TIMEOUT)
  list(APPEND CORENRN_COMPILE_DEFS DISABLE_TIMEOUT)
endif()

if(CORENRN_ENABLE_REPORTING)
  find_package(reportinglib)
  find_package(sonata)
  find_program(H5DUMP_EXECUTABLE h5dump)

  if(reportinglib_FOUND)
    list(APPEND CORENRN_COMPILE_DEFS ENABLE_BIN_REPORTS)
    set(ENABLE_BIN_REPORTS_TESTS ON)
  else()
    set(reportinglib_INCLUDE_DIR "")
    set(reportinglib_LIBRARY "")
  endif()
  if(sonata_FOUND)
    if(TARGET sonata::sonata_report)
      list(APPEND CORENRN_COMPILE_DEFS ENABLE_SONATA_REPORTS)
      set(ENABLE_SONATA_REPORTS_TESTS ON)
    else()
      message(SEND_ERROR "SONATA library was found but without reporting support")
    endif()
  endif()

  if(NOT reportinglib_FOUND AND NOT sonata_FOUND)
    message(SEND_ERROR "Neither reportinglib nor SONATA libraries were found")
  endif()

  include_directories(${reportinglib_INCLUDE_DIR})
  include_directories(${sonatareport_INCLUDE_DIR})
endif()

if(CORENRN_ENABLE_LEGACY_UNITS)
  set(CORENRN_USE_LEGACY_UNITS 1)
else()
  set(CORENRN_USE_LEGACY_UNITS 0)
endif()
list(APPEND CORENRN_COMPILE_DEFS CORENEURON_USE_LEGACY_UNITS=${CORENRN_USE_LEGACY_UNITS})
# Propagate Legacy Units flag to backends.
set(MOD2C_ENABLE_LEGACY_UNITS
    ${CORENRN_ENABLE_LEGACY_UNITS}
    CACHE BOOL "" FORCE)
set(NMODL_ENABLE_LEGACY_UNITS
    ${CORENRN_ENABLE_LEGACY_UNITS}
    CACHE BOOL "" FORCE)

if(CORENRN_ENABLE_MPI_DYNAMIC)
  if(NOT CORENRN_ENABLE_MPI)
    message(FATAL_ERROR "Cannot enable dynamic mpi without mpi")
  endif()
  list(APPEND CORENRN_COMPILE_DEFS CORENEURON_ENABLE_MPI_DYNAMIC)
endif()

if(CORENRN_ENABLE_PRCELLSTATE)
  set(CORENRN_NRN_PRCELLSTATE 1)
else()
  set(CORENRN_NRN_PRCELLSTATE 0)
endif()
if(MINGW)
  list(APPEND CORENRN_COMPILE_DEFS MINGW)
endif()

# =============================================================================
# NMODL specific options
# =============================================================================
if(CORENRN_ENABLE_NMODL)
  find_package(nmodl)
  if(NOT "${CORENRN_NMODL_DIR}" STREQUAL "" AND NOT nmodl_FOUND)
    message(FATAL_ERROR "Cannot find NMODL in ${CORENRN_NMODL_DIR}")
  endif()
  if(nmodl_FOUND)
    set(CORENRN_MOD2CPP_BINARY ${nmodl_BINARY})
    set(CORENRN_MOD2CPP_INCLUDE ${nmodl_INCLUDE})
    # path to python interface
    set(ENV{PYTHONPATH} "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
    set(CORENRN_NMODL_PYTHONPATH $ENV{PYTHONPATH})
  else()
    set(NMODL_ENABLE_PYTHON_BINDINGS
        OFF
        CACHE BOOL "Disable NMODL python bindings")
    include(AddNmodlSubmodule)
    set(CORENRN_MOD2CPP_BINARY ${CMAKE_BINARY_DIR}/bin/nmodl${CMAKE_EXECUTABLE_SUFFIX})
    set(CORENRN_MOD2CPP_INCLUDE ${CMAKE_BINARY_DIR}/include)
    set(ENV{PYTHONPATH} "$ENV{PYTHONPATH}")
    set(nmodl_PYTHONPATH "${CMAKE_BINARY_DIR}/lib")
    set(CORENRN_NMODL_PYTHONPATH "${nmodl_PYTHONPATH}:$ENV{PYTHONPATH}")
    set(NMODL_TARGET_TO_DEPEND nmodl)
  endif()
  include_directories(${CORENRN_MOD2CPP_INCLUDE})
  # set correct arguments for nmodl for cpu/gpu target
  set(CORENRN_NMODL_FLAGS
      ""
      CACHE STRING "Extra NMODL options such as passes")
else()
  include(AddMod2cSubmodule)
  set(NMODL_TARGET_TO_DEPEND mod2c_core)
  set(CORENRN_MOD2CPP_BINARY ${CMAKE_BINARY_DIR}/bin/mod2c_core${CMAKE_EXECUTABLE_SUFFIX})
  set(CORENRN_MOD2CPP_INCLUDE ${CMAKE_BINARY_DIR}/include)
endif()

# =============================================================================
# Profiler/Instrumentation Options
# =============================================================================
if(CORENRN_ENABLE_CALIPER_PROFILING)
  find_package(caliper REQUIRED)
  list(APPEND CORENRN_COMPILE_DEFS CORENEURON_CALIPER)
  set(CORENRN_CALIPER_LIB caliper)
endif()

if(CORENRN_ENABLE_LIKWID_PROFILING)
  find_package(likwid REQUIRED)
  list(APPEND CORENRN_COMPILE_DEFS LIKWID_PERFMON)
  # TODO: avoid this part, probably by using some likwid CMake target
  include_directories(${likwid_INCLUDE_DIRS})
endif()

# enable debugging code with extra logs to stdout
if(CORENRN_ENABLE_DEBUG_CODE)
  list(APPEND CORENRN_COMPILE_DEFS CORENRN_DEBUG CHKPNTDEBUG CORENRN_DEBUG_QUEUE INTERLEAVE_DEBUG)
endif()

# =============================================================================
# Common CXX flags : ignore unknown pragma warnings
# =============================================================================
# Do not set this when building wheels. The nrnivmodl workflow means that we do not know what
# compiler will be invoked with these flags, so we have to use flags that are as generic as
# possible.
if(NOT DEFINED NRN_WHEEL_BUILD OR NOT NRN_WHEEL_BUILD)
  list(APPEND CORENRN_EXTRA_CXX_FLAGS "${IGNORE_UNKNOWN_PRAGMA_FLAGS}")
endif()

# Add the main source directory
add_subdirectory(coreneuron)

# Extract the various compiler option strings to use inside nrnivmodl-core. Sets the global property
# CORENRN_LIB_LINK_FLAGS, which contains the arguments that must be added to the link line for
# `special` to link against `libcorenrnmech.{a,so}`
include(MakefileBuildOptions)

# Generate the nrnivmodl-core script and makefile using the options from MakefileBuildOptions
add_subdirectory(extra)

if(CORENRN_ENABLE_UNIT_TESTS)
  add_subdirectory(tests)
endif()

# =============================================================================
# Install cmake modules
# =============================================================================
get_property(CORENRN_NEURON_LINK_FLAGS GLOBAL PROPERTY CORENRN_NEURON_LINK_FLAGS)
configure_file(CMake/coreneuron-config.cmake.in CMake/coreneuron-config.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CMake/coreneuron-config.cmake" DESTINATION share/cmake)
install(EXPORT coreneuron DESTINATION share/cmake)

if(NOT CORENEURON_AS_SUBPROJECT)
  # =============================================================================
  # Setup Doxygen documentation
  # =============================================================================
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    # generate Doxyfile with correct source paths
    configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile)
    add_custom_target(
      doxygen
      COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  endif()

  # =============================================================================
  # Setup Sphinx documentation
  # =============================================================================
  find_package(Sphinx QUIET)
  if(SPHINX_FOUND)
    set(SPHINX_SOURCE ${PROJECT_SOURCE_DIR}/docs)
    set(SPHINX_BUILD ${PROJECT_BINARY_DIR}/docs/)

    add_custom_target(
      sphinx
      COMMAND ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating documentation with Sphinx")
  endif()

  # =============================================================================
  # Build full docs
  # =============================================================================
  if(DOXYGEN_FOUND AND SPHINX_FOUND)
    add_custom_target(
      docs
      COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target doxygen
      COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target sphinx
      COMMENT "Generating full documentation")
  else()
    add_custom_target(
      docs
      VERBATIM
      COMMAND echo "Please install docs requirements (see docs/README.md)!"
      COMMENT "Documentation generation not possible!")
  endif()
endif()
# =============================================================================
# Build status
# =============================================================================
message(STATUS "")
message(STATUS "Configured CoreNEURON ${PROJECT_VERSION}")
message(STATUS "")
message(STATUS "You can now build CoreNEURON using:")
message(STATUS "  cmake --build . --parallel 8 [--target TARGET]")
message(STATUS "You might want to adjust the number of parallel build jobs for your system.")
message(STATUS "Some non-default targets you might want to build:")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Target             |   Description")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "install             | Will install CoreNEURON to: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "docs                | Build full docs. Calls targets: doxygen, sphinx")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS " Build option       | Status")
message(STATUS "--------------------+--------------------------------------------------------")
message(STATUS "CXX COMPILER        | ${CMAKE_CXX_COMPILER}")
message(STATUS "COMPILE FLAGS       | ${CORENRN_CXX_FLAGS}")
message(STATUS "Build Type          | ${COMPILE_LIBRARY_TYPE}")
message(STATUS "MPI                 | ${CORENRN_ENABLE_MPI}")
if(CORENRN_ENABLE_MPI)
  message(STATUS "  DYNAMIC           | ${CORENRN_ENABLE_MPI_DYNAMIC}")
  if(CORENRN_ENABLE_MPI_DYNAMIC AND NRN_MPI_LIBNAME_LIST)
    # ~~~
    # for dynamic mpi, rely on neuron for list of libraries to build
    # this is to avoid cmake code duplication on the coreneuron side
    # ~~~
    list(LENGTH NRN_MPI_LIBNAME_LIST _num_mpi)
    math(EXPR num_mpi "${_num_mpi} - 1")
    foreach(val RANGE ${num_mpi})
      list(GET NRN_MPI_LIBNAME_LIST ${val} libname)
      list(GET NRN_MPI_INCLUDE_LIST ${val} include)
      message(STATUS "    LIBNAME         | core${libname}")
      message(STATUS "    INC             | ${include}")
    endforeach(val)
  else()
    message(STATUS "  INC               | ${MPI_CXX_INCLUDE_PATH}")
  endif()
endif()
message(STATUS "OpenMP              | ${CORENRN_ENABLE_OPENMP}")
message(STATUS "Use legacy units    | ${CORENRN_ENABLE_LEGACY_UNITS}")
message(STATUS "NMODL               | ${CORENRN_ENABLE_NMODL}")
if(CORENRN_ENABLE_NMODL)
  message(STATUS "  FLAGS             | ${CORENRN_NMODL_FLAGS}")
endif()
message(STATUS "MOD2CPP PATH        | ${CORENRN_MOD2CPP_BINARY}")
message(STATUS "GPU Support         | ${CORENRN_ENABLE_GPU}")
if(CORENRN_ENABLE_GPU)
  message(STATUS "  CUDA              | ${CUDAToolkit_LIBRARY_DIR}")
  message(STATUS "  Offload           | ${CORENRN_ACCELERATOR_OFFLOAD}")
  message(STATUS "  Unified Memory    | ${CORENRN_ENABLE_CUDA_UNIFIED_MEMORY}")
endif()
message(STATUS "Auto Timeout        | ${CORENRN_ENABLE_TIMEOUT}")
message(STATUS "Wrap exp()          | ${CORENRN_ENABLE_HOC_EXP}")
message(STATUS "SplayTree Queue     | ${CORENRN_ENABLE_SPLAYTREE_QUEUING}")
message(STATUS "NetReceive Buffer   | ${CORENRN_ENABLE_NET_RECEIVE_BUFFER}")
message(STATUS "Caliper             | ${CORENRN_ENABLE_CALIPER_PROFILING}")
message(STATUS "Likwid              | ${CORENRN_ENABLE_LIKWID_PROFILING}")
message(STATUS "Unit Tests          | ${CORENRN_ENABLE_UNIT_TESTS}")
message(STATUS "Reporting           | ${CORENRN_ENABLE_REPORTING}")
if(CORENRN_ENABLE_REPORTING)
  message(STATUS "  sonatareport_INC  | ${sonatareport_INCLUDE_DIR}")
  message(STATUS "  sonatareport_LIB  | ${sonatareport_LIBRARY}")
  message(STATUS "  reportinglib_INC  | ${reportinglib_INCLUDE_DIR}")
  message(STATUS "  reportinglib_LIB  | ${reportinglib_LIBRARY}")
endif()
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS " See documentation : https://github.com/BlueBrain/CoreNeuron/")
message(STATUS "--------------+--------------------------------------------------------------")
message(STATUS "")
