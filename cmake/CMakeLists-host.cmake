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

# Host build tools. pe-reloc repairs the EFI image after objcopy.
# mkimage writes the GPT and calls mkfs.fat, mtools, and qemu-img.
# Both are C. The checks that ctest runs are Bash.

add_executable(pe-reloc "${CMAKE_SOURCE_DIR}/tools/pe-reloc.c")
target_compile_options(pe-reloc PRIVATE -Wall -Wextra -Werror)

add_executable(mkimage
    "${CMAKE_SOURCE_DIR}/tools/mkimage.c"
    "${CMAKE_SOURCE_DIR}/tools/host-hash.c"
)
target_include_directories(mkimage PRIVATE "${CMAKE_SOURCE_DIR}/tools")
target_compile_options(mkimage PRIVATE -Wall -Wextra -Werror)
