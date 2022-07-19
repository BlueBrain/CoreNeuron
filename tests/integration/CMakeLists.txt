# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

if(CORENRN_ENABLE_MPI_DYNAMIC)
  # ~~~
  # In case of submodule building we don't know the MPI launcher and mpi
  # distribution being used. So for now just skip these tests and rely on
  # neuron to test dynamic mpi mode. For coreneuron build assume are just
  # building single generic mpi library libcorenrn_mpi.<suffix>
  # ~~~
  if(CORENEURON_AS_SUBPROJECT)
    message(INFO "CoreNEURON integration tests are disabled with dynamic MPI")
    return()
  else()
    set(CORENRN_MPI_LIB_ARG
        "--mpi-lib ${PROJECT_BINARY_DIR}/lib/lib${CORENRN_MPI_LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
  endif()
endif()

set(COMMON_ARGS "--tstop 100. --celsius 6.3 --mpi ${CORENRN_MPI_LIB_ARG}")
set(MODEL_STATS_ARG "--model-stats")
set(RING_DATASET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ring")
set(RING_COMMON_ARGS "--datpath ${RING_DATASET_DIR} ${COMMON_ARGS}")
set(RING_GAP_COMMON_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/ring_gap ${COMMON_ARGS}")
set(PERMUTE1_ARGS "--cell-permute 1")
set(PERMUTE2_ARGS "--cell-permute 2")
set(CUDA_INTERFACE "--cuda-interface")
if(CORENRN_ENABLE_GPU)
  set(GPU_ARGS "--gpu")
  set(permutation_modes 1 2)
else()
  set(permutation_modes 0 1)
endif()

# List of tests with arguments
set(TEST_CASES_WITH_ARGS
    "ring!${RING_COMMON_ARGS} ${MODEL_STATS_ARG} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring"
    "ring_binqueue!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_binqueue --binqueue"
    "ring_multisend!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_multisend --multisend"
    "ring_spike_buffer!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer --spikebuf 1"
    "ring_gap!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap"
    "ring_gap_binqueue!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_binqueue --binqueue"
    "ring_gap_multisend!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_multisend --multisend"
)
set(test_suffixes "" "_binqueue" "_multisend")
foreach(cell_permute ${permutation_modes})
  list(APPEND test_suffixes "_permute${cell_permute}")
  list(
    APPEND
    TEST_CASES_WITH_ARGS
    "ring_permute${cell_permute}!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute${cell_permute} --cell-permute=${cell_permute}"
    "ring_gap_permute${cell_permute}!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute${cell_permute} --cell-permute=${cell_permute}"
  )
  # As reports require MPI, do not add test if report is enabled.
  if(NOT CORENRN_ENABLE_REPORTING)
    list(APPEND test_suffixes "_serial_permute${cell_permute}")
    list(
      APPEND
      TEST_CASES_WITH_ARGS
      "ring_serial_permute${cell_permute}!${GPU_ARGS} --cell-permute=${cell_permute} --tstop 100. --celsius 6.3 --datpath ${RING_DATASET_DIR} ${MODEL_STATS_ARG} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_serial_permute${cell_permute}"
    )
  endif()
endforeach()

if(CORENRN_ENABLE_GPU)
  list(APPEND test_suffixes "_permute2_cudaInterface")
  list(
    APPEND
    TEST_CASES_WITH_ARGS
    "ring_permute2_cudaInterface!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute2_cudaInterface ${PERMUTE2_ARGS} ${CUDA_INTERFACE}"
    "ring_gap_permute2_cudaInterface!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute2_cudaInterface ${PERMUTE2_ARGS} ${CUDA_INTERFACE}"
  )
endif()

# ~~~
# There are no directories for permute and multisend related tests,
# create them and copy reference spikes
# ~~~
foreach(data_dir "ring" "ring_gap")
  # Naïve foreach(test_suffix ${test_suffixes}) does not seem to handle empty suffixes correctly.
  list(LENGTH test_suffixes num_suffixes)
  math(EXPR num_suffixes_m1 "${num_suffixes} - 1")
  foreach(suffix_index RANGE 0 ${num_suffixes_m1})
    list(GET test_suffixes ${suffix_index} test_suffix)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}/out.dat.ref"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${data_dir}${test_suffix}/")
  endforeach()
endforeach()
# test without ring_gap version
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/ring/out.dat.ref"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer/")

# names of all tests added
set(CORENRN_TEST_NAMES "")

# Configure test scripts
foreach(args_line ${TEST_CASES_WITH_ARGS})
  string(REPLACE "!" ";" string_line ${args_line})
  set(test_num_processors 1)
  if(MPI_FOUND)
    # serial test run without srun or mpiexec
    if(args_line MATCHES "ring_serial.*")
      string(REPLACE ";" " " SRUN_PREFIX "")
    else()
      set(test_num_processors 2)
      string(REPLACE ";" " " SRUN_PREFIX "${TEST_MPI_EXEC_BIN};-n;${test_num_processors}")
    endif()
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(integration_test.sh.in ${TEST_NAME}/integration_test.sh @ONLY)
  add_test(
    NAME ${TEST_NAME}_TEST
    COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/integration_test.sh
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
  set_tests_properties(${TEST_NAME}_TEST PROPERTIES PROCESSORS ${test_num_processors})
  cpp_cc_configure_sanitizers(TEST ${TEST_NAME}_TEST)
  list(APPEND CORENRN_TEST_NAMES ${TEST_NAME}_TEST)
endforeach()

if(CORENRN_ENABLE_REPORTING)
  foreach(TEST_NAME "1")
    set(SIM_NAME "reporting_${TEST_NAME}")
    set(CONFIG_ARG "${TEST_NAME}")
    configure_file(reportinglib/${TEST_NAME}.conf.in ${SIM_NAME}/${TEST_NAME}.conf @ONLY)
    configure_file(reportinglib/reporting_test.sh.in ${SIM_NAME}/reporting_test.sh @ONLY)
    configure_file(reportinglib/${TEST_NAME}.check.in ${SIM_NAME}/${TEST_NAME}.check @ONLY)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/reportinglib/test_ref.out" DESTINATION "${SIM_NAME}/")
    add_test(
      NAME ${SIM_NAME}
      COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/reporting_test.sh
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
    cpp_cc_configure_sanitizers(TEST ${SIM_NAME})
    list(APPEND CORENRN_TEST_NAMES ${SIM_NAME})
  endforeach()
endif()
