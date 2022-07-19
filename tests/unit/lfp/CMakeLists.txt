# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
add_executable(lfp_test_bin lfp.cpp)
target_link_libraries(lfp_test_bin coreneuron-all)
# Tell CMake *not* to run an explicit device code linker step (which will produce errors); let the
# NVHPC C++ compiler handle this implicitly.
#set_target_properties(lfp_test_bin PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
#add_dependencies(lfp_test_bin nrniv-core)
add_test(NAME lfp_test COMMAND ${TEST_EXEC_PREFIX} $<TARGET_FILE:lfp_test_bin>)
cpp_cc_configure_sanitizers(TARGET lfp_test_bin TEST lfp_test)
set_property(
  TEST lfp_test
  APPEND
  PROPERTY ENVIRONMENT OMP_NUM_THREADS=1)
