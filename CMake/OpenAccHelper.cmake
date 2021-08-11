# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

# =============================================================================
# Prepare compiler flags for GPU target
# =============================================================================
if(CORENRN_ENABLE_GPU)
  # cuda unified memory support
  if(CORENRN_ENABLE_CUDA_UNIFIED_MEMORY)
    set(UNIFIED_MEMORY_DEF -DUNIFIED_MEMORY)
  endif()
  # This is a lazy way of getting the major/minor versions separately without parsing
  # ${CMAKE_CUDA_COMPILER_VERSION}
  find_package(CUDAToolkit 9.0 REQUIRED)
  # Be a bit paranoid
  if(NOT ${CMAKE_CUDA_COMPILER_VERSION} STREQUAL ${CUDAToolkit_VERSION})
    message(
      FATAL_ERROR
        "CUDA compiler (${CMAKE_CUDA_COMPILER_VERSION}) and toolkit (${CUDAToolkit_VERSION}) versions are not the same!"
    )
  endif()
  set(CUDA_PROFILING_DEF -DCUDA_PROFILING)
  # -acc enables OpenACC support, -cuda links CUDA libraries and (very importantly!) seems to be
  # required so make the NVHPC compiler do the device code linking. Otherwise the explicit CUDA
  # device code (.cu files in libcudacoreneuron) has to be linked in a separate, earlier, step,
  # which causes problems with interoperability with OpenACC. See
  # https://github.com/BlueBrain/CoreNeuron/issues/607 for more information about this. -gpu=cudaX.Y
  # ensures that OpenACC code is compiled with the same CUDA version as is used for the explicit
  # CUDA code.
  set(PGI_ACC_FLAGS "-acc -cuda -gpu=cuda${CUDAToolkit_VERSION_MAJOR}.${CUDAToolkit_VERSION_MINOR}")
  # Make sure that OpenACC code is generated for the same compute capabilities as the explicit CUDA
  # code. Otherwise there may be confusing linker errors. We cannot rely on nvcc and nvc++ using the
  # same default compute capabilities as each other, particularly on GPU-less build machines.
  foreach(compute_capability ${CMAKE_CUDA_ARCHITECTURES})
    string(APPEND PGI_ACC_FLAGS ",cc${compute_capability}")
  endforeach()
  # disable very verbose diagnosis messages and obvious warnings for mod2c
  set(PGI_DIAG_FLAGS "--diag_suppress 161,177,550")
  # avoid PGI adding standard compliant "-A" flags
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION --c++11)
  set(CMAKE_CXX14_STANDARD_COMPILE_OPTION --c++14)
  set(CORENRN_ACC_GPU_DEFS "${UNIFIED_MEMORY_DEF} ${CUDA_PROFILING_DEF}")
  set(CORENRN_ACC_GPU_FLAGS "${PGI_ACC_FLAGS} ${PGI_DIAG_FLAGS}")

  add_definitions(${CORENRN_ACC_GPU_DEFS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CORENRN_ACC_GPU_FLAGS}")
endif()

# =============================================================================
# Set global property that will be used by NEURON to link with CoreNEURON
# =============================================================================
if(CORENRN_ENABLE_GPU)
  set_property(
    GLOBAL
    PROPERTY
      CORENEURON_LIB_LINK_FLAGS
      "${PGI_ACC_FLAGS} -rdynamic -lrt -Wl,--whole-archive -L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech -L${CMAKE_INSTALL_PREFIX}/lib -lcoreneuron -lcudacoreneuron -Wl,--no-whole-archive"
  )
else()
  set_property(GLOBAL PROPERTY CORENEURON_LIB_LINK_FLAGS
                               "-L${CMAKE_HOST_SYSTEM_PROCESSOR} -lcorenrnmech")
endif(CORENRN_ENABLE_GPU)
