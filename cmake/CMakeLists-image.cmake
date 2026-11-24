# Disk images. mkimage writes the GPT. mkfs.fat and mtools fill the ESP.
# qemu-img wraps qcow2. The EFI stubs are the payloads. QEMU is a later
# target.

add_custom_target(images ALL
    COMMAND $<TARGET_FILE:mkimage>
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
    DEPENDS queueos_boot queueos_base mkimage
    COMMENT "QueueOS disk images"
    VERBATIM
)

add_test(
    NAME image-a001
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/host/check-images.sh"
            --root "${CMAKE_SOURCE_DIR}"
)
