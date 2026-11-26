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

# Jansson 2.15.1. dtoa.c stays in the import and is not compiled.
# The hand-written jansson_config.h is the substitution the upstream
# header describes. -DNDEBUG keeps assert() from calling the C library.
set(QUEUEOS_JANSSON_SOURCES
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/dump.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/error.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/hashtable.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/hashtable_seed.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/load.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/memory.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/pack_unpack.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/strbuffer.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/strconv.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/utf.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/value.c"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src/version.c"
)
# -include stdint.h supplies uint32_t. The upstream files include it only
# when configure has defined HAVE_STDINT_H. That header is not edited.
set(QUEUEOS_JANSSON_FLAGS -Wno-error -DNDEBUG -include stdint.h)
set_source_files_properties(${QUEUEOS_JANSSON_SOURCES} PROPERTIES
    COMPILE_OPTIONS "${QUEUEOS_JANSSON_FLAGS}")

# Lua 5.5.1. lua.c and luac.c stay in the import and are not compiled.
# Boot links these objects. The host test below runs one snippet.
set(QUEUEOS_LUA_SOURCES
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lapi.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lauxlib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lbaselib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lcode.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lcorolib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lctype.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ldblib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ldebug.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ldo.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ldump.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lfunc.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lgc.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/linit.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/liolib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/llex.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lmathlib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lmem.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/loadlib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lobject.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lopcodes.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/loslib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lparser.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lstate.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lstring.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lstrlib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ltable.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ltablib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/ltm.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lundump.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lutf8lib.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lvm.c"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src/lzio.c"
)
set(QUEUEOS_LUA_FLAGS
    -Wno-error -Wall -ffreestanding -fno-builtin -fno-stack-protector
    -fno-asynchronous-unwind-tables -fno-unwind-tables)
set_source_files_properties(${QUEUEOS_LUA_SOURCES} PROPERTIES
    COMPILE_OPTIONS "${QUEUEOS_LUA_FLAGS}")

include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-version.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-init.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-host.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-efi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-image.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CMakeLists-harness.cmake")
