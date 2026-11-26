# Disk images. mkimage writes the GPT. mkfs.fat and mtools fill the ESP.
# qemu-img wraps qcow2. The EFI stubs are the payloads. QEMU is a later
# target.

set(QUEUEOS_DATABASE_RAW "${CMAKE_BINARY_DIR}/database.raw")

add_custom_command(
    OUTPUT "${QUEUEOS_DATABASE_RAW}"
    COMMAND $<TARGET_FILE:db-image>
            "${QUEUEOS_DATABASE_RAW}"
            "${QUEUEOS_EFI_DIR}/Base.efi"
    DEPENDS db-image queueos_base
    COMMENT "QueueOS database image"
    VERBATIM
)

add_custom_target(images ALL
    COMMAND $<TARGET_FILE:mkimage>
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
            --database-raw "${QUEUEOS_DATABASE_RAW}"
    DEPENDS queueos_boot queueos_base mkimage "${QUEUEOS_DATABASE_RAW}"
    COMMENT "QueueOS disk images"
    VERBATIM
)

add_test(
    NAME image-a001
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/host/check-images.sh"
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
)
