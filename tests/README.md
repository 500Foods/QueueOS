# Tests

`src/boot/x86-64/boot.c` loads Base from `\EFI\QueueOS\current\Base.efi` on the boot disk. That path is a scaffold until Boot can fetch Base from the database disk.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`sentences` checks the three lines in `src/common/diagnostics.h`. `efi-headers` checks that `build/efi/Boot.efi`, `Base.efi`, and `BootFail.efi` are PE32+ EFI applications with a base-relocation table. `image-a001` checks the `a-001-q35-base` disk and the blank database. The QEMU suite is not in this tree yet.
