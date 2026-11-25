# Tests

`src/boot/x86-64/boot.c` loads Base from `\EFI\QueueOS\current\Base.efi` on the boot disk. That path is a scaffold until Boot can fetch Base from the database disk.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`sentences` checks the three lines in `src/common/diagnostics.h`. `efi-headers` checks that `build/efi/Boot.efi`, `Base.efi`, and `BootFail.efi` are PE32+ EFI applications with a base-relocation table. `image-a001` checks the `a-001-q35-base` disk and the blank database.

`cmake --build build --target test-first` runs the first batch through `tests/harness/run.sh`. `cmake --build build --target test-second` runs the second batch under its own run id. `tq all` stays the first batch. One run id is `results/<run>/`, and each case keeps `serial.log`, `meta.txt`, `machine.txt` when a guest was started, `annotation.txt` when the case names one, and a screenshot when the case asked for one. The annotation is the case file's sentence. It is not read out of the firmware log.

Firmware holds its start-boot progress for ten seconds, then loads Boot. Boot draws `QueueOS loading...` and counts down from 10 before it starts Base. Base draws `QueueOS Loaded` and counts down from 10. Before Boot returns success, the console attribute is left red on red, so a firmware line printed with that leftover attribute matches the field. The Loaded sentence stays white. The failure path does not change the attribute. Firmware then draws its boot-device menu in its own colors and waits for a key. The disks are virtio-scsi. The menu names are `UEFI QueueOS Boot` and `UEFI QueueOS Base`. The same attachment boots `images/a-003-pc-base`, which is machine `pc` and the same ESP bytes as `a-001-q35-base`. The headless `run` target keeps that menu in the serial log and in `screenshot-menu.ppm`. Catalogue cases stop at the sentence they check.

The second batch mutates a work copy of the `a-001` ESP for three failure cases, checks that the `pc` folder is that same ESP, and checks the Boot and Base sentences on `pc`. `images/` is not opened for writing. The red-pixel check on the q35 Boot screenshot did not need calibration. The thresholds held. Screenshot cases are not repeated on `pc`.
