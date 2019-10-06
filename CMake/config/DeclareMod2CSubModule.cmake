# Copyright (c) 2017, Blue Brain Project
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
include (ExternalProject)
include(FindPkgConfig)

find_path(MOD2C_PROJ NAMES CMakeLists.txt PATHS "${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c")
find_package_handle_standard_args(MOD2C REQUIRED_VARS MOD2C_PROJ)

if (NOT MOD2C_FOUND)
  find_package(Git 1.8.3 QUIET)
  if(NOT ${GIT_FOUND})
    message(FATAL_ERROR "git not found, clone repository with --recursive")
  endif()
  message (STATUS "Sub-module mod2c missing : running git submodule update --init --recursive")
  execute_process(COMMAND
    ${GIT_EXECUTABLE} submodule update --init --recursive -- ${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c
    WORKING_DIRECTORY ${CORENEURON_PROJECT_SOURCE_DIR})
else()
  message(STATUS "Using mod2c submodule from from ${MOD2C_PROJ}")
endif()

set(ExternalProjectCMakeArgs
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}
  -DCMAKE_C_COMPILER=${FRONTEND_C_COMPILER} -DCMAKE_CXX_COMPILER=${FRONTEND_CXX_COMPILER}
)

ExternalProject_Add(mod2c
  BUILD_ALWAYS 1
  SOURCE_DIR ${CORENEURON_PROJECT_SOURCE_DIR}/external/mod2c
  GIT_SUBMODULES
  CMAKE_ARGS ${ExternalProjectCMakeArgs}
)

