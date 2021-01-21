# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

set(COMMON_ARGS "--tstop 100. --celsius 6.3 --mpi")
set(MODEL_STATS_ARG "--model-stats")
set(RING_DATASET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ring")
set(RING_COMMON_ARGS "--datpath ${RING_DATASET_DIR} ${COMMON_ARGS}")
set(RING_GAP_COMMON_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/ring_gap ${COMMON_ARGS}")
set(PERMUTE1_ARGS "--cell-permute 1")
set(PERMUTE2_ARGS "--cell-permute 2")
if(CORENRN_ENABLE_GPU)
  set(GPU_ARGS "--gpu")
endif()

# List of tests with arguments
set(TEST_CASES_WITH_ARGS
    "ring!${RING_COMMON_ARGS} ${MODEL_STATS_ARG} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring"
    "ring_binqueue!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_binqueue --binqueue"
    "ring_multisend!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_multisend --multisend"
    "ring_spike_buffer!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer --spikebuf 1"
    "ring_permute1!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute1 ${PERMUTE1_ARGS}"
    "ring_permute2!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute2 ${PERMUTE2_ARGS}"
    "ring_gap!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap"
    "ring_gap_binqueue!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_binqueue --binqueue"
    "ring_gap_multisend!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_multisend --multisend"
    "ring_gap_permute1!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute1 ${PERMUTE1_ARGS}"
    "ring_gap_permute2!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute2 ${PERMUTE2_ARGS}"
)

set(NEGATIVE_TEST_CASES_WITH_ARGS
    "ring_different_seed!${RING_COMMON_ARGS} ${GPU_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_different_seed --seed 123456"
)

# ~~~
# There are no directories for permute and multisend related tests,
# create them and copy reference spikes
# ~~~
foreach(data_dir "ring" "ring_gap")
  foreach(
    test_suffix
    "multisend"
    "binqueue"
    "savestate_permute0"
    "savestate_permute1"
    "savestate_permute2"
    "permute1"
    "permute2")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}/out.dat.ref"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${data_dir}_${test_suffix}/")
  endforeach()
endforeach()

# base tests : ring, ring with gap and ring duplication
foreach(data_dir "ring" "ring_gap")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}/out.dat.ref"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${data_dir}/")
endforeach()

# test for small spike buffer
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/ring/out.dat.ref"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer/")

# Configure test scripts
foreach(args_line ${TEST_CASES_WITH_ARGS})
  string(REPLACE "!" ";" string_line ${args_line})
  if(MPI_FOUND)
    string(REPLACE ";" " " SRUN_PREFIX "${TEST_MPI_EXEC_BIN};-n;2")
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(integration_test.sh.in ${TEST_NAME}/integration_test.sh @ONLY)
  add_test(
    NAME ${TEST_NAME}_TEST
    COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/integration_test.sh
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
endforeach()

foreach(args_line ${NEGATIVE_TEST_CASES})
  string(REPLACE "!" ";" string_line ${args_line})
  if(MPI_FOUND)
    string(REPLACE ";" " " SRUN_PREFIX "${TEST_MPI_EXEC_BIN};-n;2")
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(negative_integration_test.sh.in ${TEST_NAME}/negative_integration_test.sh @ONLY)
  add_test(
    NAME ${TEST_NAME}_TEST
    COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/negative_integration_test.sh
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
endforeach()

if(CORENRN_ENABLE_REPORTING)
  foreach(TEST_NAME "1")
    set(SIM_NAME "reporting_${TEST_NAME}")
    set(CONFIG_ARG "${TEST_NAME}")
    configure_file(reportinglib/${TEST_NAME}.conf.in ${SIM_NAME}/${TEST_NAME}.conf @ONLY)
    configure_file(reportinglib/reporting_test.sh.in ${SIM_NAME}/reporting_test.sh @ONLY)
    configure_file(reportinglib/${TEST_NAME}.check.in ${SIM_NAME}/${TEST_NAME}.check @ONLY)
    add_test(
      NAME ${SIM_NAME}
      COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/reporting_test.sh
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
  endforeach()
endif()
