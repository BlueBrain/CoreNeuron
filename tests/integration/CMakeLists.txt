# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

if(CORENRN_ENABLE_GPU)
    set(GPU_COMMON "--gpu")
endif()
set(COMMON_ARGS "${GPU_COMMON} --mpi --tstop 100. config --celsius 6.3")
set(RING_DATASET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ring")
set(RING_COMMON_ARGS "${COMMON_ARGS} input --datpath ${RING_DATASET_DIR}")
set(RING_SPIKEBUFFER_ARGS "${COMMON_ARGS} --spikebuf 1 input --datpath ${RING_DATASET_DIR}")
set(RING_DUPLICATE_ARGS "${COMMON_ARGS} --multiple 3 input --datpath ${RING_DATASET_DIR}")
set(RING_GAP_COMMON_ARGS "${COMMON_ARGS} input --datpath ${CMAKE_CURRENT_SOURCE_DIR}/ring_gap")
set(PERMUTE1_ARGS "gpu --cell-permute 1 --nwarp 8")
set(PERMUTE2_ARGS "gpu --cell-permute 2 --nwarp 16")
if(CORENRN_ENABLE_GPU)
  set(GPU_ARGS "gpu --nwarp 8")
endif()

# List of tests with arguments
set(TEST_CASE_ARGS
    "ring!${RING_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring"
    "ring_binqueue!${RING_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_binqueue spike --binqueue"
    "ring_multisend!${RING_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_multisend spike --multisend"
    "ring_spike_buffer!${RING_SPIKEBUFFER_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer"
    "ring_duplicate!${RING_DUPLICATE_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_duplicate"
    "ring_permute1!${RING_COMMON_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute1 ${PERMUTE1_ARGS}"
    "ring_permute2!${RING_COMMON_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute2 ${PERMUTE2_ARGS}"
    "ring_gap!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap"
    "ring_gap_binqueue!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_binqueue spike --binqueue"
    "ring_gap_multisend!${RING_GAP_COMMON_ARGS} ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_multisend spike --multisend"
    "ring_gap_permute1!${RING_GAP_COMMON_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute1 ${PERMUTE1_ARGS}"
    "ring_gap_permute2!${RING_GAP_COMMON_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute2 ${PERMUTE2_ARGS}")

set(NEGATIVE_TEST_CASE_ARGS "ring_different_seed!${RING_COMMON_ARGS} --seed 123456 ${GPU_ARGS} output --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_different_seed")

# There are no directories for permute and multisend related tests, create them and copy ref spikes
foreach(data_dir "ring" "ring_gap")
  foreach(test_suffix
          "multisend"
          "binqueue"
          "savestate_permute0"
          "savestate_permute1"
          "savestate_permute2"
          "permute1"
          "permute2")
    file(COPY
         "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}/out.dat.ref"
         DESTINATION
         "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}_${test_suffix}/")
  endforeach()
endforeach()

file(COPY
     "${CMAKE_CURRENT_SOURCE_DIR}/ring/out.dat.ref"
     DESTINATION
     "${CMAKE_CURRENT_SOURCE_DIR}/ring_spike_buffer/")

file(GLOB INPUT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ring/*.dat")

# Configure test scripts
foreach(args_line ${TEST_CASES_WITH_ARGS})
  string(REPLACE "!"
                 ";"
                 string_line
                 ${args_line})
  if(MPI_FOUND)
    string(REPLACE ";"
                   " "
                   SRUN_PREFIX
                   "${TEST_MPI_EXEC_BIN};-n;2")
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(integration_test.sh.in ${TEST_NAME}/integration_test.sh @ONLY)
  add_test(NAME ${TEST_NAME}_TEST
           COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/integration_test.sh
           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
endforeach()

foreach(args_line ${NEGATIVE_TEST_CASES_WITH_ARGS})
  string(REPLACE "!"
                 ";"
                 string_line
                 ${args_line})
  if(MPI_FOUND)
    string(REPLACE ";"
                   " "
                   SRUN_PREFIX
                   "${TEST_MPI_EXEC_BIN};-n;2")
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(negative_integration_test.sh.in ${TEST_NAME}/negative_integration_test.sh @ONLY)
  add_test(NAME ${TEST_NAME}_TEST
           COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/negative_integration_test.sh
           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
endforeach()

if(CORENRN_ENABLE_REPORTINGLIB)
  foreach(TEST_NAME "1")
    set(SIM_NAME "reporting_${TEST_NAME}")
    set(CONFIG_ARG "${TEST_NAME}")
    configure_file(reportinglib/${TEST_NAME}.conf.in ${SIM_NAME}/${TEST_NAME}.conf @ONLY)
    configure_file(reportinglib/reporting_test.sh.in ${SIM_NAME}/reporting_test.sh @ONLY)
    configure_file(reportinglib/${TEST_NAME}.check.in ${SIM_NAME}/${TEST_NAME}.check @ONLY)
    add_test(NAME ${SIM_NAME}
             COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/reporting_test.sh
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
  endforeach()
endif()
