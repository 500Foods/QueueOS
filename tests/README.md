# Tests

`src/boot/x86-64/boot.c` loads Base from `\EFI\QueueOS\current\Base.efi` on the boot disk. That path is a scaffold until Boot can fetch Base from the database disk.

That scaffold is gone. Boot reads the virtio-scsi disk whose product is `Base`, chooses the newest launchable slot, and starts those bytes. The load options are ASCII `slot N` and a trailing NUL. Base does not read them yet. Base reads them and writes `booted STAMP` after the hash line of that slot. The stamp is the firmware clock. The JSON and the hash line stay as they were.

Base writes one page at the 8 MiB mark after that slot line. The page is `stored STAMP slot N` and a sha256 of the stored line. When the page already holds a record, Base prints that stored line before `QueueOS Loaded`.

That page is a JSON ring. The writer emits a compact array of objects with the keys `stored` and `slot`, in that order, then a sha256 of those JSON bytes. Base prints the newest stored sentence and appends this boot. A full page drops the oldest object. A zero page or a rejected page prints nothing and is replaced by a one-element array. Base links the Jansson Boot already uses.

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`sentences` checks the three lines in `src/common/diagnostics.h`. `publisher` checks the host publisher against a raw image in memory: a first publish, one stored copy of an identical hash, a publish that does not fit, and a full set of slots whose only completed boot is the oldest. `body-page` checks the JSON ring: a zero page, the sample array, a refused page, an append, the full-page drop, and a publish that leaves the page in place. `efi-headers` checks that `build/efi/Boot.efi`, `Base.efi`, and `BootFail.efi` are PE32+ EFI applications with a base-relocation table. `image-a001` checks the `a-001-q35-base` disk and the database image that carries the stub generation. `slot-json` checks the guest classifier: empty, unreadable, the stub candidate, a bad hash, a foreign OS, newest-wins with the lower index on a tie, the body range, and the empty-string digest. `lua-floor` links vendored Lua 5.5.1 with the firmware shims and no C library. It checks `pcall`, `math.floor`, and `string.format`.

`cmake --build build --target test-first` runs the first batch through `tests/harness/run.sh`. `cmake --build build --target test-second` runs the second batch under its own run id. `tq all` stays the first batch. One run id is `results/<run>/`, and each case keeps `serial.log`, `meta.txt`, `machine.txt` when a guest was started, `annotation.txt` when the case names one, and a screenshot when the case asked for one. The annotation is the case file's sentence. It is not read out of the firmware log.

Firmware holds its start-boot progress for ten seconds, then loads Boot. Boot draws `QueueOS loading...` and counts down from 10 before it starts Base. Base draws `QueueOS Loaded` and counts down from 10.

That Boot countdown is gone. After the loading sentence, Boot draws the Lua list for about ten seconds and then starts the chosen slot. Base's countdown is unchanged. Before Boot returns success, the console attribute is left red on red, so a firmware line printed with that leftover attribute matches the field. The Loaded sentence stays white. The failure path does not change the attribute. Firmware then draws its boot-device menu in its own colors and waits for a key. The disks are virtio-scsi. The menu names are `UEFI QueueOS Boot` and `UEFI QueueOS Base`. The same attachment boots `images/a-003-pc-base`, which is machine `pc` and the same ESP bytes as `a-001-q35-base`. The headless `run` target keeps that menu in the serial log and in `screenshot-menu.ppm`. Catalogue cases stop at the sentence they check.

The second batch mutates a work copy of the `a-001` ESP for three failure cases, checks that the `pc` folder is that same ESP, and checks the Boot and Base sentences on `pc`. `D-003-003` is the exception: it flips one byte of the work-copy database image at offset 40960 and leaves the ESP alone. `images/` is not opened for writing. The red-pixel check on the q35 Boot screenshot did not need calibration. The thresholds held. Screenshot cases are not repeated on `pc`.

`lua tests/boot-tui.lua` is a host stand-in for the Boot list. It is not `Boot.efi`. No arguments shows ten empty slots. A path is a database disk: qcow2 is converted to a temporary raw file, that copy is what is read, and `images/` is not written. The screen is one stack: the framed QueueOS mark, the ten slots in a column the width of that mark, and Explore and Launch under the list. Spare rows and columns center the stack. A terminal too small for the frame uses the one-line word and scrolls the slots. Explore replaces the list with a scrollable view of that slot's JSON and replaces the buttons with Back. Launch prints the chosen Base and does not start it. `ctest` runs `boot-tui`, which is `lua tests/boot-tui.lua --check`.
