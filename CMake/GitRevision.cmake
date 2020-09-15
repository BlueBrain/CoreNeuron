# For now use simple approach to get version information as git is
# often avaialble on the machine where we are building from source

find_package(Git)

if(GIT_FOUND)
  # get last commit sha1
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVISION_SHA1
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  # get last commit date
  execute_process(
    COMMAND ${GIT_EXECUTABLE} show -s --format=%ci
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVISION_DATE
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  # remove extra double quotes
  set(CN_GIT_REVISION "${GIT_REVISION_SHA1} (${GIT_REVISION_DATE})")
else()
  set(CN_GIT_REVISION "unknown")
endif()