# Included from the repository root CMakeLists.txt.
# Configure with:  cmake -S . -B build
# Build with:      cmake --build build
# Host tests:      ctest --test-dir build --output-on-failure
#
# Configuring this directory by itself is refused. CMAKE_SOURCE_DIR has
# to be the repository root so src/, tests/, and vendored/ resolve.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    message(FATAL_ERROR
        "Configure the repository root: cmake -S . -B build")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-version.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-init.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-host.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-efi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-image.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-harness.cmake")
