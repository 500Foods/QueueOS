# QEMU harness. tests/harness/run.sh is Bash. test-second waits until
# images/a-003-pc-base exists. A case id is selected at configure time:
#   cmake -S . -B build -DCASE=D-001-001
#   cmake --build build --target test-case

set(CASE "" CACHE STRING "Case id for the test-case target, such as D-001-001")

add_custom_target(test-first
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/harness/run.sh"
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
            --suite "${CMAKE_SOURCE_DIR}/tests/suites/first-batch.list"
    DEPENDS sentences images
    USES_TERMINAL
    VERBATIM
)

add_custom_target(test-case
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/harness/run.sh"
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
            --case "${CASE}"
    DEPENDS sentences images
    USES_TERMINAL
    VERBATIM
)

add_custom_target(run
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/harness/run.sh"
            --root "${CMAKE_SOURCE_DIR}"
            --build "${CMAKE_BINARY_DIR}"
            --boot a-001-q35-base
    DEPENDS images
    USES_TERMINAL
    VERBATIM
)
