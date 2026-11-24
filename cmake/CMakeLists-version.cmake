# QueueOS version: major.minor.build
#
# The first number is the major release. The second is the minor release.
# The third is the build count: the number of commits reachable from HEAD.
# A tree with no git history is 1.0.0. Uncommitted edits do not bump it.
#
# CMake's own PROJECT_VERSION is not this string. CMake treats a third
# number as a patch. QUEUEOS_VERSION is the one the rest of the tree uses.
# tests/test-queueos.lua reads QUEUEOS_VERSION_MAJOR and
# QUEUEOS_VERSION_MINOR from this file and counts commits the same way.

set(QUEUEOS_VERSION_MAJOR 1)
set(QUEUEOS_VERSION_MINOR 0)

set(QUEUEOS_BUILD 0)
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE QUEUEOS_BUILD
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT QUEUEOS_BUILD MATCHES "^[0-9]+$")
    set(QUEUEOS_BUILD 0)
endif()

set(QUEUEOS_VERSION
    "${QUEUEOS_VERSION_MAJOR}.${QUEUEOS_VERSION_MINOR}.${QUEUEOS_BUILD}")

message(STATUS "QueueOS version ${QUEUEOS_VERSION}")
