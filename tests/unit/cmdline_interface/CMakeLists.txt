# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(cmd_interface_test_bin test_cmdline_interface.cpp)
target_link_libraries(cmd_interface_test_bin coreneuron-unit-test)
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
#set_target_properties(cmd_interface_test_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
#target_compile_options(cmd_interface_test_bin PRIVATE ${CORENEURON_BOOST_UNIT_TEST_COMPILE_FLAGS})
add_test(NAME cmd_interface_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:cmd_interface_test_bin>)
cpp_cc_configure_sanitizers(TARGET cmd_interface_test_bin TEST cmd_interface_test)
