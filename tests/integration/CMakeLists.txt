# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


set(COMMON_ARGS "--tstop 100. --celsius 6.3 -mpi")
set(RING_COMMON_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/ring ${COMMON_ARGS}")
set(RING_GAP_COMMON_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/ring_gap ${COMMON_ARGS}")
set(PERMUTE1_ARGS "--cell-permute 1 --nwarp 8")
set(PERMUTE2_ARGS "--cell-permute 2 --nwarp 16")

# List of tests with arguments
set(TEST_CASE_ARGS
    "ring!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring"
    "ring_binqueue!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_binqueue --binqueue"
    "ring_multisend!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_multisend --multisend"
    "ring_spike_buffer!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_spike_buffer -b 1"
    "ring_duplicate!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_duplicate -z 3"
    "ring_permute1!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute1 ${PERMUTE1_ARGS}"
    "ring_permute2!${RING_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_permute2 ${PERMUTE2_ARGS}"

    "ring_gap!${RING_GAP_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap"
    "ring_gap_binqueue!${RING_GAP_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_binqueue --binqueue"
    "ring_gap_multisend!${RING_GAP_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_multisend --multisend"
    "ring_gap_permute1!${RING_GAP_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute1 ${PERMUTE1_ARGS}"
    "ring_gap_permute2!${RING_GAP_COMMON_ARGS} --outpath ${CMAKE_CURRENT_BINARY_DIR}/ring_gap_permute2 ${PERMUTE2_ARGS}")

# There are no directories for permute and multisend related tests, create them and copy ref spikes
foreach(data_dir "ring" "ring_gap")
    foreach(test_suffix "multisend" "binqueue" "permute1" "permute2")
      file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}/out.dat.ref" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/${data_dir}_${test_suffix}/")
    endforeach()
endforeach()
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/ring/out.dat.ref" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/ring_spike_buffer/")

#Configure test scripts
foreach (args_line ${TEST_CASE_ARGS})
  string(REPLACE "!" ";" string_line ${args_line})
  if(MPI_FOUND)
    string(REPLACE ";" " " SRUN_PREFIX  "${TEST_MPI_EXEC_BIN};-n;2")
  endif()
  list(GET string_line 0 TEST_NAME)
  list(GET string_line 1 TEST_ARGS)
  set(SIM_NAME ${TEST_NAME})
  configure_file(integration_test.sh.in ${TEST_NAME}/integration_test.sh @ONLY)
  add_test(NAME ${TEST_NAME}_TEST
           COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}/integration_test.sh
           WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}")
endforeach()

# This test is for coreneuron having all the mechanisms integrated into the
# library. Runs when the corresponding condition is utilized plus the
# executable is built.
# Set of 10Cells tests
if(ADDITIONAL_MECHS AND CORENEURON_MAIN)

  function(exec_test TEST_NAME SUBTEST_NAME)
if(MPI_FOUND)
    string(REPLACE ";" " " SRUN_PREFIX  "${TEST_MPI_EXEC_BIN};-n;2")
endif()
    set(SIM_NAME "${TEST_NAME}/${SUBTEST_NAME}")
    if(${SUBTEST_NAME} MATCHES "generic")
      set(TEST_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --tstop 100. -mpi")
    elseif(${SUBTEST_NAME} MATCHES "forwardskip")
      set(TEST_ARGS "--datpath=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --forwardskip 5000. --tstop 100. -mpi")
    elseif(${SUBTEST_NAME} MATCHES "spikereplay")
      set(TEST_ARGS "--datpath ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --pattern ${CMAKE_CURRENT_SOURCE_DIR}/${SIM_NAME}/out.std --tstop 100. -mpi")
    endif()

    configure_file(integration_test.sh.in ${SIM_NAME}/integration_test.sh @ONLY)

    add_test(NAME ${TEST_NAME}_${SUBTEST_NAME}_TEST
             COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/integration_test.sh
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
  endfunction(exec_test)

endif()
