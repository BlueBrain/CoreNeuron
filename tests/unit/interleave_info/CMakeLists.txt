# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(interleave_info_bin check_constructors.cpp)
target_link_libraries(interleave_info_bin coreneuron-all)
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
#set_target_properties(interleave_info_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
#separate_arguments(CORENRN_ACC_FLAGS UNIX_COMMAND "${NVHPC_ACC_COMP_FLAGS}")
target_compile_options(interleave_info_bin PRIVATE ${CORENRN_ACC_FLAGS} ${CORENEURON_BOOST_UNIT_TEST_COMPILE_FLAGS})
add_test(NAME interleave_info_constructor_test COMMAND ${TEST_EXEC_PREFIX}
                                                       $<TARGET_FILE:interleave_info_bin>)
cpp_cc_configure_sanitizers(TARGET interleave_info_bin TEST interleave_info_constructor_test)
