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




# List of tests with arguments
set(TEST_CASE_ARGS
  "ring!--datpath=${CMAKE_CURRENT_SOURCE_DIR}/ring --outpath=${CMAKE_CURRENT_BINARY_DIR}/ring --tstop=100. --celsius=6.3 -mpi"
  "ring_gap!--datpath=${CMAKE_CURRENT_SOURCE_DIR}/ring_gap --outpath=${CMAKE_CURRENT_BINARY_DIR}/ring_gap --tstop=100. --celsius=6.3 -mpi")



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
      set(TEST_ARGS "--datpath=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --tstop=100. -mpi")
    elseif(${SUBTEST_NAME} MATCHES "forwardskip")
      set(TEST_ARGS "--datpath=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --forwardskip=5000. --tstop=100. -mpi")
    elseif(${SUBTEST_NAME} MATCHES "spikereplay")
      set(TEST_ARGS "--datpath=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME} --outpath=${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME} --pattern=${CMAKE_CURRENT_SOURCE_DIR}/${SIM_NAME}/out.std --tstop=100. -mpi")
    endif()

    configure_file(integration_test.sh.in ${SIM_NAME}/integration_test.sh @ONLY)

    add_test(NAME ${TEST_NAME}_${SUBTEST_NAME}_TEST
             COMMAND "/bin/sh" ${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}/integration_test.sh
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SIM_NAME}")
  endfunction(exec_test)

endif()
