# Host tests. These link the development machine's C library.
# They include src/common/diagnostics.h and do not compile the EFI sources.
# Unity itself stays in vendored/unity and is not modified.

set(QUEUEOS_UNITY_DIR "${CMAKE_SOURCE_DIR}/vendored/unity/src")

add_executable(sentences
    "${CMAKE_SOURCE_DIR}/tests/host/test-sentences.c"
    "${QUEUEOS_UNITY_DIR}/unity.c"
)

target_include_directories(sentences PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${QUEUEOS_UNITY_DIR}"
)

target_compile_options(sentences PRIVATE -Wall -Wextra -Werror)

add_test(NAME sentences COMMAND sentences)

# Host stand-in for the Boot list. It reads sample slots and does not boot.
add_test(
    NAME boot-tui
    COMMAND lua "${CMAKE_SOURCE_DIR}/tests/boot-tui.lua" --check
)

# Host build tools. pe-reloc repairs the EFI image after objcopy.
# mkimage writes the GPT and calls mkfs.fat, mtools, and qemu-img.
# Both are C. The checks that ctest runs are Bash.

add_executable(pe-reloc "${CMAKE_SOURCE_DIR}/tools/pe-reloc.c")
target_compile_options(pe-reloc PRIVATE -Wall -Wextra -Werror)

add_executable(mkimage
    "${CMAKE_SOURCE_DIR}/tools/mkimage.c"
    "${CMAKE_SOURCE_DIR}/tools/host-hash.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
)
target_include_directories(mkimage PRIVATE
    "${CMAKE_SOURCE_DIR}/tools"
    "${CMAKE_SOURCE_DIR}/src/common"
)
target_compile_options(mkimage PRIVATE -Wall -Wextra -Werror)

# Publisher rewrites a raw database image in place. The tests call the
# same code. The tool does not open images/ itself.
add_executable(publisher
    "${CMAKE_SOURCE_DIR}/tools/publisher.c"
    "${CMAKE_SOURCE_DIR}/src/common/publish.c"
    "${CMAKE_SOURCE_DIR}/src/common/slot-text.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
)
target_include_directories(publisher PRIVATE "${CMAKE_SOURCE_DIR}/src/common")
target_compile_options(publisher PRIVATE -Wall -Wextra -Werror)

add_executable(publisher-test
    "${CMAKE_SOURCE_DIR}/tests/host/test-publisher.c"
    "${CMAKE_SOURCE_DIR}/src/common/publish.c"
    "${CMAKE_SOURCE_DIR}/src/common/slot-text.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
    "${QUEUEOS_UNITY_DIR}/unity.c"
)
target_include_directories(publisher-test PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${QUEUEOS_UNITY_DIR}"
)
target_compile_options(publisher-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME publisher COMMAND publisher-test)

add_executable(body-page-test
    "${CMAKE_SOURCE_DIR}/tests/host/test-body-page.c"
    "${CMAKE_SOURCE_DIR}/src/common/body-page.c"
    "${CMAKE_SOURCE_DIR}/src/common/publish.c"
    "${CMAKE_SOURCE_DIR}/src/common/slot-text.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
    "${QUEUEOS_UNITY_DIR}/unity.c"
)
target_include_directories(body-page-test PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${QUEUEOS_UNITY_DIR}"
)
target_compile_options(body-page-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME body-page COMMAND body-page-test)

# First database disk. Ten slots and the stub Base. mkimage only wraps qcow2.
add_executable(db-image
    "${CMAKE_SOURCE_DIR}/tools/db-image.c"
    "${CMAKE_SOURCE_DIR}/src/common/publish.c"
    "${CMAKE_SOURCE_DIR}/src/common/slot-text.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
)
target_include_directories(db-image PRIVATE "${CMAKE_SOURCE_DIR}/src/common")
target_compile_options(db-image PRIVATE -Wall -Wextra -Werror)

add_executable(slot-json-test
    "${CMAKE_SOURCE_DIR}/tests/host/test-slot-json.c"
    "${CMAKE_SOURCE_DIR}/src/boot/slot-json.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
    ${QUEUEOS_JANSSON_SOURCES}
    "${QUEUEOS_UNITY_DIR}/unity.c"
)
target_include_directories(slot-json-test PRIVATE
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/boot"
    "${CMAKE_SOURCE_DIR}/src/boot/jansson-config"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src"
    "${QUEUEOS_UNITY_DIR}"
)
target_compile_options(slot-json-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME slot-json COMMAND slot-json-test)

add_executable(tablet-report-test
    "${CMAKE_SOURCE_DIR}/tests/host/test-tablet-report.c"
    "${CMAKE_SOURCE_DIR}/src/boot/tablet-report.c"
    "${QUEUEOS_UNITY_DIR}/unity.c"
)
target_include_directories(tablet-report-test PRIVATE
    "${CMAKE_SOURCE_DIR}/src/boot"
    "${QUEUEOS_UNITY_DIR}"
)
target_compile_options(tablet-report-test PRIVATE -Wall -Wextra -Werror)
add_test(NAME tablet-report COMMAND tablet-report-test)

# Vendored Lua, linked with the firmware shims and no C library.
# The snippet is pcall, math.floor, and string.format.
set(QUEUEOS_LUA_FLOOR_OURS
    "${CMAKE_SOURCE_DIR}/tests/host/test-lua-floor.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/lua-libc.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/setjmp.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/c-shim.c"
    "${CMAKE_SOURCE_DIR}/src/common/memory.c"
)
set_source_files_properties(${QUEUEOS_LUA_FLOOR_OURS} PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-ffreestanding;-fno-builtin;-fno-stack-protector;-fno-asynchronous-unwind-tables;-fno-unwind-tables")
add_executable(lua-floor-test
    ${QUEUEOS_LUA_FLOOR_OURS}
    ${QUEUEOS_LUA_SOURCES}
)
target_include_directories(lua-floor-test PRIVATE
    "${CMAKE_SOURCE_DIR}/vendored/lua/src"
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi"
)
target_link_options(lua-floor-test PRIVATE
    -nostdlib
    -no-pie
    "LINKER:-e,test_main"
    "LINKER:--no-undefined"
    "LINKER:--build-id=none"
)
add_test(NAME lua-floor COMMAND lua-floor-test)

add_executable(lua-embed "${CMAKE_SOURCE_DIR}/tools/lua-embed.c")
target_compile_options(lua-embed PRIVATE -Wall -Wextra -Werror)
