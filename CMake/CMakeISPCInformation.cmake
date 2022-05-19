# =============================================================================
# Copyright (c) 2016-2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================

# This file sets the basic flags for the FOO compiler
set(CMAKE_INCLUDE_FLAG_ISPC "-I")

if(NOT CMAKE_ISPC_COMPILE_OBJECT)
  set(CMAKE_ISPC_COMPILE_OBJECT "<CMAKE_ISPC_COMPILER> <FLAGS> <INCLUDES> <SOURCE> -o <OBJECT>")
endif()
set(CMAKE_ISPC_INFORMATION_LOADED 1)
