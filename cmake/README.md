# CMake

The lists in this directory are the QueueOS build. The root `CMakeLists.txt` includes them so `cmake -S . -B build` keeps the repository root as the source directory.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Hydrogen splits its build the same way, one file per concern. QueueOS starts with the files that have a job now.

| File | Job |
|---|---|
| `CMakeLists.txt` | Includes the other lists. The root file calls `project()`. Configuring `cmake/` by itself is refused. |
| `CMakeLists-version.cmake` | `major.minor.build`. The build count is `git rev-list --count HEAD`. |
| `CMakeLists-init.cmake` | Host C defaults and CTest. |
| `CMakeLists-host.cmake` | Host test `sentences`, plus the `pe-reloc` and `mkimage` tools. |
| `CMakeLists-efi.cmake` | Freestanding x86-64 Boot, Base, and BootFail stubs, plus the `efi-headers` check. |
| `CMakeLists-image.cmake` | `images` target and the `image-a001` check. |
| `CMakeLists-harness.cmake` | `test-first`, `test-case`, and `run`. |

`test-second` waits until `images/a-003-pc-base` exists. Debug, sanitizer, and coverage variants wait until a program needs them.

`QUEUEOS_VERSION` is the three-part version. CMake's `PROJECT_VERSION` is not that string.
