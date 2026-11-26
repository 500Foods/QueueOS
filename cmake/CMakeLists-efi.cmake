# x86-64 EFI stubs. Host gcc and binutils. gnu-efi, mingw, clang, and
# nasm are not used. The .efi files are build products under build/efi/.

find_program(QUEUEOS_OBJCOPY objcopy REQUIRED)

set(QUEUEOS_EFI_CFLAGS
    -ffreestanding
    -fno-stack-protector
    -fno-stack-check
    -fno-builtin
    -fpic
    -fshort-wchar
    -mno-red-zone
    -maccumulate-outgoing-args
    -fno-asynchronous-unwind-tables
    -fno-unwind-tables
    -fcf-protection=none
    -fvisibility=hidden
    -fno-plt
    -Wall
    -Wextra
    -Werror
)

# -Bsymbolic is a linker flag. Passing it bare is gcc's -B prefix and does
# not bind calls locally. A PLT entry would be left for a dynamic linker
# the firmware does not run. Hidden visibility keeps the same boundary.

set(QUEUEOS_EFI_SUPPORT
    "${CMAKE_SOURCE_DIR}/src/common/memory.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/serial.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/text.c"
)

# memory.c lives next to the header the plan already named. The compiler
# still calls memcpy from freestanding code, and libc is not on this link.
set(QUEUEOS_EFI_INCLUDES
    "${CMAKE_SOURCE_DIR}/src/common"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi"
)

set(QUEUEOS_EFI_DIR "${CMAKE_BINARY_DIR}/efi")
file(MAKE_DIRECTORY "${QUEUEOS_EFI_DIR}")

# Extra objcopy -j arguments follow output_name. Boot passes -j .bss.
# Without that section, uninitialized statics use the ELF .bss address,
# and objcopy places .reloc at the same address. The firmware then reads
# the relocation bytes as those statics.
function(queueos_add_efi target source output_name)
    add_executable(${target} ${source} ${QUEUEOS_EFI_SUPPORT})
    add_dependencies(${target} pe-reloc)
    target_compile_options(${target} PRIVATE ${QUEUEOS_EFI_CFLAGS})
    target_include_directories(${target} PRIVATE ${QUEUEOS_EFI_INCLUDES})
    target_link_options(${target} PRIVATE
        -nostdlib
        -shared
        "LINKER:-Bsymbolic"
        "LINKER:-e,efi_main"
        "LINKER:-T,${CMAKE_SOURCE_DIR}/src/boot/x86-64/link.lds"
        "LINKER:--no-undefined"
        "LINKER:--build-id=none"
        "LINKER:--hash-style=sysv"
        "LINKER:-z,nocombreloc"
        "LINKER:-z,max-page-size=0x1000"
        "LINKER:-z,common-page-size=0x1000"
    )
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${target}"
        RUNTIME_OUTPUT_DIRECTORY "${QUEUEOS_EFI_DIR}"
        LINKER_LANGUAGE C
    )

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${QUEUEOS_OBJCOPY}"
                --output-target=efi-app-x86_64
                -j .text -j .rodata -j .data
                ${ARGN}
                "$<TARGET_FILE:${target}>"
                "${QUEUEOS_EFI_DIR}/${output_name}"
        # objcopy leaves IMAGE_FILE_RELOCS_STRIPPED set. pe-reloc reads
        # the ELF relative fixups and writes the PE .reloc table.
        COMMAND $<TARGET_FILE:pe-reloc>
                "$<TARGET_FILE:${target}>"
                "${QUEUEOS_EFI_DIR}/${output_name}"
        COMMENT "objcopy ${output_name}"
        VERBATIM
    )
endfunction()

queueos_add_efi(queueos_boot "${CMAKE_SOURCE_DIR}/src/boot/x86-64/boot.c" "Boot.efi" -j .bss)
queueos_add_efi(queueos_base "${CMAKE_SOURCE_DIR}/src/base/x86-64/base.c" "Base.efi" -j .bss)
queueos_add_efi(queueos_boot_fail "${CMAKE_SOURCE_DIR}/src/boot/x86-64/boot-fail.c" "BootFail.efi")

# Base and BootFail stay free of Jansson and Lua. Boot reads the slots
# and draws the list. Base writes the completion line and one body page,
# so it links the pool, the slot text, the body page, and the disk move.
# The script bytes are generated.
set(QUEUEOS_GUEST_SCRIPTS_C "${CMAKE_BINARY_DIR}/guest-scripts.c")
add_custom_command(
    OUTPUT "${QUEUEOS_GUEST_SCRIPTS_C}"
    COMMAND $<TARGET_FILE:lua-embed>
            "${QUEUEOS_GUEST_SCRIPTS_C}"
            wordmark "${CMAKE_SOURCE_DIR}/src/boot/wordmark.lua"
            screen-geo "${CMAKE_SOURCE_DIR}/src/boot/screen-geo.lua"
            boot-screen "${CMAKE_SOURCE_DIR}/src/boot/boot-screen.lua"
            json-text "${CMAKE_SOURCE_DIR}/src/boot/json-text.lua"
            guest-screen "${CMAKE_SOURCE_DIR}/src/boot/guest-screen.lua"
    DEPENDS lua-embed
            "${CMAKE_SOURCE_DIR}/src/boot/wordmark.lua"
            "${CMAKE_SOURCE_DIR}/src/boot/screen-geo.lua"
            "${CMAKE_SOURCE_DIR}/src/boot/boot-screen.lua"
            "${CMAKE_SOURCE_DIR}/src/boot/json-text.lua"
            "${CMAKE_SOURCE_DIR}/src/boot/guest-screen.lua"
    VERBATIM
)
target_sources(queueos_boot PRIVATE
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/pool.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/c-shim.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/setjmp.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/lua-libc.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/disk-io.c"
    "${CMAKE_SOURCE_DIR}/src/boot/x86-64/base-disk.c"
    "${CMAKE_SOURCE_DIR}/src/boot/x86-64/screen-run.c"
    "${CMAKE_SOURCE_DIR}/src/boot/x86-64/field-fill.c"
    "${CMAKE_SOURCE_DIR}/src/boot/x86-64/usb-tablet.c"
    "${CMAKE_SOURCE_DIR}/src/boot/tablet-report.c"
    "${CMAKE_SOURCE_DIR}/src/boot/slot-json.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
    "${QUEUEOS_GUEST_SCRIPTS_C}"
    ${QUEUEOS_JANSSON_SOURCES}
    ${QUEUEOS_LUA_SOURCES}
)
target_include_directories(queueos_boot PRIVATE
    "${CMAKE_SOURCE_DIR}/src/boot"
    "${CMAKE_SOURCE_DIR}/src/boot/jansson-config"
    "${CMAKE_SOURCE_DIR}/vendored/jansson/src"
    "${CMAKE_SOURCE_DIR}/vendored/lua/src"
)
target_sources(queueos_base PRIVATE
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/pool.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/c-shim.c"
    "${CMAKE_SOURCE_DIR}/src/arch/x86-64/efi/disk-io.c"
    "${CMAKE_SOURCE_DIR}/src/base/x86-64/completion.c"
    "${CMAKE_SOURCE_DIR}/src/common/slot-text.c"
    "${CMAKE_SOURCE_DIR}/src/common/body-page.c"
    "${CMAKE_SOURCE_DIR}/src/common/sha256.c"
)

add_test(
    NAME efi-headers
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/host/check-efi.sh"
            "${QUEUEOS_EFI_DIR}/Boot.efi"
            "${QUEUEOS_EFI_DIR}/Base.efi"
            "${QUEUEOS_EFI_DIR}/BootFail.efi"
)
