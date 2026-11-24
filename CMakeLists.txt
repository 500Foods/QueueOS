cmake_minimum_required(VERSION 3.20)

# The build scripts live in cmake/. project() stays in this file because
# CMake requires a direct call here. The source directory is the repository root.
project(QueueOS
    DESCRIPTION "QueueOS"
    LANGUAGES C
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/CMakeLists.txt")
