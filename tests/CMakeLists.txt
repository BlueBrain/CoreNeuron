# =============================================================================
# Copyright (C) 2016-2019 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================

include(FindSLURM QUIET)
include(TestHelpers)

include_directories(${CORENEURON_PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

if(NOT Boost_USE_STATIC_LIBS)
  add_definitions(-DBOOST_TEST_DYN_LINK=TRUE)
endif()

# Tests
if(Boost_FOUND)
  if(CORENRN_ENABLE_UNIT_TESTS)
    add_subdirectory(unit/endian)
    add_subdirectory(unit/sdprintf)
    add_subdirectory(unit/cmdline_interface)
    add_subdirectory(unit/interleave_info)
    add_subdirectory(unit/alignment)
    add_subdirectory(unit/queueing)
  endif()
else()
  message(STATUS "Boost not found, unit tests disabled")
endif()

if(CORENRN_ENABLE_FUNCTIONAL_TESTS)
  add_subdirectory(integration)
else()
  message(STATUS "Functional tests disabled")
endif()
