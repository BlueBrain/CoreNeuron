# Copyright (c) 2019, Blue Brain Project
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


include_directories(utils/randoms)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CORENEURON_PROJECT_SOURCE_DIR})
include_directories(${MPI_INCLUDE_PATH})

file(GLOB_RECURSE coreneuron_all_headers "*.h*")
file(GLOB_RECURSE coreneuron_all_templates "*.ipp")
file(GLOB_RECURSE coreneuron_cuda_files "*.cu")
file(GLOB_RECURSE coreneuron_all_c_files "nrniv/*.c*" "nrnmpi/*.c*" "nrnoc/*.c*"
                                         "nrnomp/*.c*" "utils/*.c*")
file(GLOB_RECURSE scopmath_c_file "scopmath_core/*.c*")
set(dimplic_c_file "mech/dimplic.cpp")


# set mod2c binary and files from built in
if(NOT EXISTS ${MOD2C})
    if(${ENABLE_ISPC_TARGET})
        message(FATAL_ERROR "ispc targets can only be generated with the external nmodl compiler")
    endif()
    set (MOD2C ${PROJECT_BINARY_DIR}/bin/mod2c_core)
endif()

set (MOD2C_BINARY ${MOD2C})
get_filename_component(mod2c_install_prefix ${MOD2C} DIRECTORY)
set (MOD2C_UNITS "${PROJECT_BINARY_DIR}/share/nrnunits.lib")
set(MOD2C env "MODLUNIT=${MOD2C_UNITS}" "PYTHONPATH=$ENV{PYTHONPATH}" ${MOD2C})

# Macro sets up build rule for .c files from .mod files.
# Parameters:
#    KEY name               An arbitrary name to keep track of output .c files
#    MODFILE input          The path to the mod file
#    OUTTYPE <SERIAL|ISPC>  The output type (optional, defaults to serial)
#
# Because mod2c_core wants to write the .c file in the same directory
# as the mod file, we copy the mod file to the binary directory first
#
# The macro appends the names of the output files to MOD2C_${name}_OUTPUTS
# and the names of the mod files (without directories) to MOD2C_${name}_MODS

macro(mod2c_target)
    set(options)
    set(oneValueArgs TARGET MODFILE KEY)
    set(multiValueArgs)
    cmake_parse_arguments(mod2c "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if("${mod2c_MODFILE}" STREQUAL "")
        message(FATAL_ERROR "mod2c_target requires a MODFILE argument")
    endif()
    if("${mod2c_KEY}" STREQUAL "")
        message(FATAL_ERROR "mod2c_target requires a KEY argument")
    endif()

    get_filename_component(mod2c_source_ ${mod2c_MODFILE} ABSOLUTE)
    get_filename_component(mod2c_modname_ ${mod2c_MODFILE} NAME)

    if("${mod2c_TARGET}" STREQUAL "serial")
        string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
        set(nmodl_modearg "--c")
    elseif("${mod2c_TARGET}" STREQUAL "ispc")
        file(STRINGS ${mod2c_MODFILE} mod2c_mod_artcell REGEX "ARTIFICIAL_CELL")
        if(NOT "${mod2c_mod_artcell}" STREQUAL "")
            string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
            set(nmodl_modearg "--c")
        else()
            string(REGEX REPLACE "\\.mod$" ".ispc" mod2c_outname_ "${mod2c_modname_}")
            string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_wrapper_outname_ "${mod2c_modname_}")
            set(nmodl_modearg "--ispc")
        endif()
    elseif("${mod2c_TARGET}" STREQUAL "")
        # default case
        string(REGEX REPLACE "\\.mod$" ".cpp" mod2c_outname_ "${mod2c_modname_}")
        set(nmodl_modearg "--c")
    else()
        message(SEND_ERROR "mod2c_target only supports serial and ispc generation for now: \"${mod2c_TARGET}\"")
    endif()


    set(mod2c_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_outname_}")
    list(APPEND MOD2C_${mod2c_KEY}_OUTPUTS "${mod2c_output_}")
    if(DEFINED mod2c_wrapper_outname_)
        set(mod2c_wrapper_output_ "${CMAKE_CURRENT_BINARY_DIR}/${mod2c_wrapper_outname_}")
        list(APPEND MOD2C_${mod2c_KEY}_OUTPUTS "${mod2c_wrapper_output_}")
        unset(mod2c_wrapper_outname_)
    endif()
    list(APPEND MOD2C_${mod2c_KEY}_MODS "${mod2c_modname_}")
    if(ENABLE_NMODL AND NMODL_FOUND)
        add_custom_command(OUTPUT "${mod2c_output_}" "${mod2c_wrapper_output_}"
                DEPENDS ${mod2c_MODFILE} ${MOD2C_BINARY}
                COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${MOD2C} "${mod2c_modname_}" -o "${CMAKE_CURRENT_BINARY_DIR}" host ${nmodl_modearg}
                    ${NMODL_EXTRA_FLAGS_LIST}
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    else()
        add_custom_command(OUTPUT "${mod2c_output_}"
                DEPENDS ${mod2c_MODFILE} mod2c ${MOD2C_BINARY}
                COMMAND ${CMAKE_COMMAND} -E copy "${mod2c_source_}" "${CMAKE_CURRENT_BINARY_DIR}"
                COMMAND ${MOD2C} "${mod2c_modname_}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endmacro()

# Function to check if mod file of specific filename is already added
# to the target. When user specify multiple search directory paths then
# it's important to check if unique filenames exist.
# Parameters:
#   filepath_list   list of absolute filepath of all mod files found
#   filename        (only) filename of mod file

function(file_in_filepath_list filepath_list filename if_exist)
    foreach(filepath ${filepath_list})
        get_filename_component(newfile "${filepath}" NAME)
        if("${newfile}" STREQUAL "${filename}")
            set(${if_exist} TRUE PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

# Standard MOD files are always built
set(INBUILT_MOD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/mech/modfile")
set(INBUILT_MOD_FILES expsyn.mod exp2syn.mod  hh.mod  netstim.mod  passive.mod  pattern.mod stim.mod)

if(${ENABLE_ISPC_TARGET})
    set(M2C_TARGET ispc)
else()
    set(M2C_TARGET serial)
endif()

foreach(MOD ${INBUILT_MOD_FILES})
    mod2c_target(KEY INBUILT_MOD MODFILE "${INBUILT_MOD_PATH}/${MOD}" TARGET ${M2C_TARGET})
endforeach()

# For 'non-standard' mod files, need to generate registering function in mod_func.c
set(MOD_FUNC_C_PL "${CMAKE_CURRENT_SOURCE_DIR}/mech/mod_func.c.pl")

# to work around no acc ability to pass function pointers as arguments,
# some translated c files depend on a _kinderiv.h file that is constructed
# by kinderiv.py

set(KINDERIV_PY "${CMAKE_CURRENT_SOURCE_DIR}/kinderiv.py")

set_source_files_properties(${MOD2C_INBUILT_MOD_OUTPUTS} PROPERTIES GENERATED TRUE)


if(ENABLE_ISPC_TARGET)
    set(MOD2C_ISPC_INBUILT_MOD_OUTPUTS ${MOD2C_INBUILT_MOD_OUTPUTS})
    list(FILTER MOD2C_ISPC_INBUILT_MOD_OUTPUTS INCLUDE REGEX ".*\.ispc$")
    set_source_files_properties(${MOD2C_ISPC_INBUILT_MOD_OUTPUTS} PROPERTIES LANGUAGE ISPC)
    set_source_files_properties(${MOD2C_ISPC_INBUILT_MOD_OUTPUTS} PROPERTIES COMPILE_FLAGS "-I${nmodl_INCLUDE}")
endif()


# Add generated sources from MOD files
set(ADDITIONAL_MECH_C_FILES ${dimplic_c_file})
set(GENERATED_MECH_C_FILES ${ADDITIONAL_MECH_C_FILES}  ${MOD2C_INBUILT_MOD_OUTPUTS})

# artificial cells must be on cpu, defaul nrnran123.c is for cpu, nrn_setup.cpp uses nrnran123 for only memory calculation purpose which should use cpu version of nrnran123
set(NOACC_MECH_C_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/netstim.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/netstim_inhpoisson.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/pattern.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/randoms/nrnran123.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nrniv/nrn_setup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nrniv/global_vars.cpp)

if(ENABLE_OPENACC)
    set_source_files_properties(${GENERATED_MECH_C_FILES} PROPERTIES COMPILE_FLAGS "")
    set_source_files_properties(${NOACC_MECH_C_FILES} PROPERTIES COMPILE_FLAGS "-DDISABLE_OPENACC")
    if(${CMAKE_C_COMPILER_ID} STREQUAL "PGI")
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/scopmath_core/sparse_thread.c PROPERTIES COMPILE_FLAGS "-ta=tesla:nollvm")
    endif()
endif()

set(link_cudacoreneuron)
set(link_reportinglib)

#@TODO: CMake should have option for arch
if(ENABLE_OPENACC)
    cuda_add_library("cudacoreneuron" ${coreneuron_cuda_files} OPTIONS
        -arch=sm_30
        -gencode=arch=compute_30,code=sm_30
        -gencode=arch=compute_50,code=sm_50
        -gencode=arch=compute_52,code=sm_52
        -gencode=arch=compute_52,code=compute_52
        -Xcompiler -fPIC
    )
    set(link_cudacoreneuron cudacoreneuron)
endif()

if(ENABLE_REPORTINGLIB)
    set(link_reportinglib ${reportinglib_LIBRARY})
endif()



############################################################
# Target libs
############################################################

#
# The coreneuron lib (only internal mechanisms)
#
add_library(coreneuron ${COMPILE_LIBRARY_TYPE} ${coreneuron_all_headers} ${coreneuron_all_templates}
                       ${coreneuron_all_c_files} ${cudacorenrn_objs} ${MOD2C_INBUILT_MOD_OUTPUTS})
set_target_properties(coreneuron PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_link_libraries(coreneuron ${MPI_CXX_LIBRARIES}
    ${link_reportinglib} ${link_cudacoreneuron} ${CUDA_LIBRARIES} ${CALIPER_LIB} ${CALIPER_MPI_LIB} ${likwid_LIBRARIES})

set_target_properties(coreneuron PROPERTIES
                      VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                      SOVERSION ${VERSION_MAJOR}
                      CLEAN_DIRECT_OUTPUT 1)

set_property(TARGET coreneuron PROPERTY POSITION_INDEPENDENT_CODE ON)

# Extract link defs to the whole project
get_target_property(CORENRN_LINK_LIBS coreneuron LINK_LIBRARIES)
if(NOT CORENRN_LINK_LIBS)
    set(CORENRN_LINK_LIBS "")
endif()
set(CORENRN_LINK_LIBS "${CORENRN_LINK_LIBS}" PARENT_SCOPE)

# The scopmath lib - should not contain dimplic
add_library(scopmath STATIC ${coreneuron_all_headers} ${scopmath_c_file})
set_target_properties(scopmath PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/coreneuron/)

#
# The "complete" lib (coreneuron + opt mechs)
#
set(ENGINEMECH_C "mech/enginemech.cpp")
add_library(corenrnmech ${COMPILE_LIBRARY_TYPE} ${coreneuron_all_headers} ${ENGINEMECH_C} ${ADDITIONAL_MECH_C_FILES})
set_target_properties(corenrnmech PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# Public link interface (default), so that end binary links with all, even if they'r static
target_link_libraries(corenrnmech scopmath coreneuron)


# Copy to build directory as well
configure_file(${MOD_FUNC_C_PL} ${CMAKE_BINARY_DIR}/share/coreneuron/mod_func.c.pl @ONLY)
configure_file(${KINDERIV_PY} ${CMAKE_BINARY_DIR}/share/coreneuron/kinderiv.py)
configure_file(${dimplic_c_file} ${CMAKE_BINARY_DIR}/share/coreneuron/dimplic.cpp)
configure_file(${ENGINEMECH_C} ${CMAKE_BINARY_DIR}/share/coreneuron/enginemech.cpp)
configure_file("engine.h" ${CMAKE_BINARY_DIR}/include/coreneuron/engine.h)
configure_file("enginemech.h" ${CMAKE_BINARY_DIR}/include/coreneuron/enginemech.h)
configure_file("nrniv/profiler_interface.h" ${CMAKE_BINARY_DIR}/include/coreneuron/nrniv/profiler_interface.h)

#########################################################
# INSTALL
#########################################################

install(TARGETS coreneuron corenrnmech ${link_cudacoreneuron}
        EXPORT coreneuron
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION $<INSTALL_INTERFACE:include>)

install(FILES "engine.h" "enginemech.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coreneuron)

install(PROGRAMS ${MOD2C_BINARY}
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ${MOD2C_UNITS}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mod2c)

if(ENABLE_HEADER_INSTALL)
    install(DIRECTORY .
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/coreneuron
            FILES_MATCHING PATTERN "*.h*" PATTERN "*.ipp" PATTERN "*.ispc")

    install(FILES ${KINDERIV_PY} ${MOD_FUNC_C_PL} ${dimplic_c_file} ${ENGINEMECH_C}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/coreneuron)

    install(TARGETS scopmath
            EXPORT coreneuron
            DESTINATION ${CMAKE_INSTALL_DATADIR}/coreneuron)
endif()
