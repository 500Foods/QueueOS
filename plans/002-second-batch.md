# 002 — Second batch

Status: ready to implement. Nothing in this file is built yet.

Plan 001 closed its first batch on 2026-11-24. `results/000-005` is eighteen passes. The next run id is `000-006`. This plan is the rest of that slice: three failure cases on a work copy, one `pc` boot folder, and the Boot and Base sentences on that machine. It is also the place that records what the first boots changed, so a later reader can see the wrong turns beside the ones that held.

Writing this down does not build `images/a-003-pc-base`, does not add `test-second`, and does not accept a database format.

## 1. Diary

The first draft of plan 001 was a clean specification. The machine was less clean. These are the parts worth keeping.

### Python lasted one heading

Section 7 of plan 001 named Python 3 as the image builder and the harness, because QMP and a PPM sample looked like poor shell. The language rules then banned Python from the repository. `tools/mkimage.c` builds the disks. `tests/harness/` is Bash. QMP is one `socat` connection: `qmp_capabilities`, then the payload, `socat -t 0.5`, and an absolute screendump path. Split across two connections, the dump fails. The PPM sample is Bash, and it has to skip the header or it scores the ASCII as pixels.

During the virtio-scsi edit a `python3` one-liner was invoked anyway. It printed `skip` and changed no file. That is still a breach of the rule. It does not get a second outing, including for XML.

### The menu could not see a virtio block disk

The first attachment was virtio block. The hope was that the FAT label `QOS-ESP`, the GPT name `QueueOS-ESP`, or a virtio serial would become the firmware's device line. edk2's boot description walks USB strings, DiskInfo, network, LoadFile, and NVMe, then falls through to `Misc Device`. Every core description is prefixed with `UEFI `. A second disk with the same description becomes `UEFI Misc Device 2`. VirtioBlkDxe publishes BlockIo and a driver name. It does not publish DiskInfo. The menu ignores the labels.

Virtio-scsi was the attachment that worked. ScsiDiskDxe issues Inquiry and publishes DiskInfo. QEMU's `scsi-hd` vendor field is 8 bytes and the product field is 16. `QueueOS` fits. `Boot` and `Base` fit. The menu lines, after the firmware collapses extra spaces, are `UEFI QueueOS Boot` and `UEFI QueueOS Base`. The SCSI serial is a VPD page. It is not the menu line.

The blue device list on the proof boot also offered `EFI Firmware Setup` and `EFI Internal Shell`. The highlighted row was `UEFI QueueOS Boot`.

Plan 001's risk list said that if virtio block were invisible, the response was AHCI: `ich9-ahci` on q35, the machine IDE bus on `pc`, boot indexes kept, vendor and product kept if the fallback firmware would show them. Virtio block was invisible. AHCI was not the thing that made it visible. The fallback stays in section 5 for a later boot whose log shows the firmware never saw a disk. It is not applied in advance. The `pc` machine has not been booted.

### Ten seconds of menu was a progress bar

The request was a boot menu for 10 seconds, then Boot's countdown, then Base's countdown, then the device menu again. `-boot menu=on,splash-time=10000` does set a 10 second wait on this OVMF (`edk2-ovmf-20260812`). `GetFrontPageTimeoutFromQemu` reads the millisecond value and rounds up: `(10000 + 999) / 1000` is 10. What the screen draws during that wait is the TianoCore logo, the words `Start boot option`, and a white-on-black progress bar. Esc or F2 during the wait opens the firmware UI. Boot Manager is the device list. If the wait ends with no key, firmware loads Boot.

The blue `Please select boot device:` list is `BootManagerMenuApp`. It has no timeout. It is the return point after both programs have handed back. Putting that app first in the boot order would wait forever, and Boot would not auto-start. There is no QueueOS menu in this slice.

Boot counts from 10 to 1 under `QueueOS loading...`. Base counts from 10 to 1 under `QueueOS Loaded`. The harness quits a catalogue case when the sentence it wants has arrived, so the suite does not sit through either countdown. The headless `run` target does. Its deadline is `SECONDS + 70`, long enough for the progress bar, Boot's countdown, and Base's countdown, and it stops at `Please select boot device:`. A catalogue case's deadline is `SECONDS + 50`. `serial-absent` sleeps 25 seconds. Those numbers exist because the firmware pause was added after the first timeouts.

### The flash, and the color that does not repaint

After Base returns, firmware can print a line with whatever attribute was left behind. On the first boots that line was `BdsDxe: loading Boot0000 BootManagerMenuApp`, white on the red field, gone before a person could read it. Extra newlines did not hide it. `SetAttribute` does not repaint glyphs already on the screen. Setting red on red (`0x04 | 0x40`) after the Loaded sentence makes a later firmware line the same color as the field. The Loaded sentence stays white, because it was drawn under white on red (`0x0F | 0x40`). `ClearScreen` fills with the attribute already selected, which is why the attribute is set before the clear. The same red-on-red set happens again on Boot's success path, immediately before `EFI_SUCCESS`.

The failure path does not change the attribute. `boot-fail.c` and a missing `Base.efi` may show the firmware line. That is acceptable. The success-path hide was the request. The bytes remain in `serial.log`. A test that those bytes are absent would be testing the firmware's silence, and this plan does not add one.

The red-pixel check needed no calibration. At least 40 percent of every 16th pixel, `R > 80`, `R > G + 40`, `R > B + 40`, held on the real PPMs.

### Base is on the wrong disk, on purpose

Boot still reaches Base by `LoadImage` of `\EFI\QueueOS\current\Base.efi` on the same device Boot was loaded from. The database disk is blank. The product string on that disk is `Base` anyway. Later, Base is the short name for the database partition, and the executable is fetched from it. Renaming the disk to Database now would make the menu honest for a week and wrong for the design. The scaffold sentence in `tests/README.md` stays.

The user's virt-manager domain `QueueOS a-001` was redefined to the same virtio-scsi disks while it was shut off. A boot of a copy of its variables file still found `UEFI QueueOS Boot` and `UEFI QueueOS Base`. Firmware dropped the stale `UEFI Misc Device` paths in that copy. The harness always starts from a fresh `cp -L` of the host template, so the suite does not depend on that NVRAM.

The image builder refreshes `QueueOS-UEFI.qcow2` and `QueueOS-Database.qcow2` in the guest directory only when `virsh` reports `shut off`, or when `virsh` cannot see the domain. Any other state skips the copy and does not fail the build. The builder does not define a domain, and it does not copy `OVMF_VARS.fd`.

This close-out found the domain shut off and the variables file 131072 bytes, mtime `2026-11-24 17:59:39 -0700`. An earlier note the same day said `17:23:57`. The close-out nearly wrote the older stamp into plan 001 as if it were still true. The file had been written in between. The rule is the file, not the memory of the file.

### Checks that were named and not grown

Plan 001's check table listed `annotation` as something the slice implemented. `tests/harness/run.sh` does not know that word. Every first-batch `case.conf` carries `mutate=none`. The runner does not read `mutate`. `serial-absent` is implemented, sleeps 25 seconds, and no first-batch case uses it. `BootFail.efi` is built, `efi-headers` checks it, and nothing installs it onto a disk.

`tools/mkimage.c` is 985 lines. The cap is 1000. The `pc` folder has to be another `image.conf`. `boot_folders()` already scans for those. Growing `mkimage.c` to special-case `pc` would spend the remaining lines on a machine name.

The failure sentence is `QueueOS loading... failed to start Base`. A fixed-string search for the Boot sentence also matches the failure sentence. Plan 001's pass rule for `D-003-002` said "contains the Boot sentence" and "does not contain `QueueOS Loaded`". A guest that printed only the failure sentence would have satisfied that search. Section 3 tightens the rule.

`tq all` runs the first batch. It stays on that list. A red second batch must not become the only thing `tq all` knows how to do.

The letter scheme past A, D, and E is still open in the interview log. This plan keeps using that working assumption. `A-003` is the `pc` image package. `D-003` is the failure package. `D-004` and `E-003` are the `pc` boots. `a-003-pc-base` is a folder name. It is not a package id.

## 2. Outside this plan

- The database header, generations, checksums, and fetching Base off the database disk. The database image stays blank. No test writes a header to force a mutation.
- The Lua Boot text TUI, recovery zsh, the updater, and the database engine.
- A virt-manager guest for `a-003`. No `vm.conf` unless one is asked for. `a-002` also has none.
- Starting `QueueOS a-001`. Editing its variables file. Editing any other domain.
- A new catalogue parameter. `tq all` remains the first batch.
- A commit.
- Switching the disks to AHCI before a boot has shown that the firmware never saw them.

## 3. The six cases

`tests/suites/second-batch.list` lists them in the order below. One QEMU per guest case. Mutations touch the case's `work/` copy. `images/` is unchanged, which means the UEFI qcow2 hash in the boot folder's manifest still matches after the case.

`case.conf` grows two keys the runner does not understand yet:

- `mutate` is `none`, `delete-bootx64`, `replace-boot-fail`, or `delete-base`.
- `annotation` is the fixed sentence. When the key is present, the harness writes that exact line to `annotation.txt` in the case result directory before it judges the serial log, and the case fails if the file does not contain it. The sentence comes from the case file. It is not scraped from firmware text.
- `forbid`, when present, is a fixed string that must be absent from `serial.log`. Repeated once per forbidden string, one per line.

`expect` stays the serial text. It is no longer also the annotation.

### D-003-001 — firmware never hands off

Image `a-001-q35-base`. `mutate=delete-bootx64` deletes `EFI/BOOT/BOOTX64.EFI` from the work copy and leaves `EFI/QueueOS/current/Boot.efi` in place. The harness uses a fresh variables file, so the removable-media path is the handoff plan 001 relied on. If this firmware finds `current/Boot.efi` anyway, the case fails, and the log is the lesson. Do not delete the current file to force a pass.

```text
check=serial-absent
expect=QueueOS loading...
mutate=delete-bootx64
annotation=firmware did not hand off to QueueOS-Boot
```

`QueueOS loading...` is also the prefix of the failure sentence, so a missing handoff that somehow still printed the failure sentence fails this case too. The 25 second sleep covers the 10 second progress bar and a margin. If the first real log shows our sentence arriving after that sleep, lengthen the sleep and write down why. If the menu or the shell prints our sentence, that is a failure. Do not shorten the expected text to make the case green.

### D-003-002 — Boot returns before Base

`mutate=replace-boot-fail` copies `build/efi/BootFail.efi` over both `EFI/BOOT/BOOTX64.EFI` and `EFI/QueueOS/current/Boot.efi` on the work copy. `boot-fail.c` reports `QueueOS loading...` and returns `EFI_LOAD_ERROR`. It does not call `LoadImage`, it does not count down, and it does not set red on red.

```text
check=serial-contains
expect=QueueOS loading...
forbid=failed to start Base
forbid=QueueOS Loaded
mutate=replace-boot-fail
annotation=QueueOS-Boot returned before starting Base
```

`failed to start Base` is the part of the failure sentence that the Boot sentence does not share. Forbidding that phrase is what keeps a missing-Base boot from passing as a BootFail boot.

### D-003-003 — Boot cannot start Base

`mutate=delete-base` deletes `EFI/QueueOS/current/Base.efi` from the work copy. Boot writes the failure sentence and returns an error. It does not change the console attribute.

```text
check=serial-contains
expect=QueueOS loading... failed to start Base
forbid=QueueOS Loaded
mutate=delete-base
annotation=QueueOS-Boot could not start Base
```

The failure sentence contains `QueueOS loading...`. The case requires the longer sentence.

### A-003-001 — the pc folder is the same ESP

No QEMU. Read `images/a-003-pc-base/image.conf`.

- `machine` is `pc`.
- `qemu` is `qemu-system-x86_64`.
- `display` is `graphical`.
- `database` is `current`.
- `firmware-code` and `firmware-vars` match `images/a-001-q35-base/image.conf`.
- `QueueOS-UEFI.qcow2` in the two folders compares equal.

The three UEFI disks of this era are one staged ESP. `a-003` is not a second image-build path.

### D-004-001 — pc prints the Boot sentence

Boot `images/a-003-pc-base`. `check=serial-contains`, `expect=QueueOS loading...`, `mutate=none`.

The harness already passes `machine` from `image.conf` to `-machine`, and it already copies that conf value into `meta.txt`. Those two facts can drift apart. When the guest starts, write the same token that was given to `-machine` into `machine.txt` beside `accel.txt`. This case passes when the serial line is present and `machine.txt` is `pc`.

### E-003-001 — pc prints Loaded after loading

Boot `images/a-003-pc-base`. `check=serial-order`, `mutate=none`. Loading precedes Loaded. `machine.txt` is `pc`.

Screenshot cases are not repeated on `pc`. The red-pixel proof already exists on q35. A `pc` screenshot can be added later if a boot shows the frame buffer is a different problem.

## 4. Mutating the work copy

The harness copies both qcow2 files into `work/` and points QEMU at those copies. The mutation runs after that copy and before QEMU starts. `images/` is not opened for writing.

`mkimage` talks to mtools as `<raw>@@1M`, because the ESP starts at 1 MiB. The mutation does the same to a raw conversion of the work copy:

1. `qemu-img convert -O raw` the work UEFI qcow2 to a raw file under `work/`.
2. Confirm the first GPT partition starts at 1 MiB. If it does not, fail the case. Do not invent a second copy of the partition math inside `mkimage.c`.
3. `mdel` or `mcopy` with `-i <raw>@@1M`, the same offset syntax the builder uses. `replace-boot-fail` overwrites both Boot paths from `build/efi/BootFail.efi`. `delete-bootx64` removes only `::/EFI/BOOT/BOOTX64.EFI`. `delete-base` removes only `::/EFI/QueueOS/current/Base.efi`.
4. `qemu-img convert -O qcow2` back over the work qcow2. QEMU keeps `format=qcow2`.
5. Leave directory timestamps alone. The work copy is not hashed into a manifest.

`losetup`, `qemu-nbd`, and `kpartx` need privileges. They are not the path. No new host package. `mtools` and `qemu-img` are already required by the first batch.

The database work copy is never mutated.

## 5. The pc machine, and the fallback that is still waiting

`images/a-003-pc-base/image.conf` is the table in plan 001 section 4: machine `pc`, graphical, same firmware paths, `database=current`. No `vm.conf`.

The first `pc` boot uses the virtio-scsi command already in the harness. Vendor `QueueOS`, products `Boot` and `Base`, boot indexes 1 and 2.

If that boot never prints a Boot line, and the serial log shows the firmware never saw a disk, switch virtio-scsi to the fallback once, for every boot folder, and record the attachment in `tests/README.md`. On q35 the fallback is an `ich9-ahci` controller and two disks. On `pc` it is the machine IDE bus, used the same way. Keep the boot indexes. Keep the vendor and product strings if that firmware will show them. A single invisible `pc` boot is not permission to change q35 if q35 is still drawing the named disks. The fallback is one attachment for the suite only when the same invisibility shows up on the machine that already works. Until a log says the disk was invisible, the command stays virtio-scsi.

## 6. Order of work

1. Teach the harness `mutate`, `annotation`, `forbid`, and `machine.txt`. Keep `mutate=none` working for the eighteen existing cases.
2. Add `D-003-001`, `D-003-002`, and `D-003-003` against `a-001-q35-base`. Run those three. Adjust the 25 second absence window only from a log that shows why.
3. Add `images/a-003-pc-base/image.conf` and let the existing image target build it. Add `A-003-001`. Confirm the UEFI bytes match `a-001`.
4. Boot `pc` once by hand through the harness before declaring the chipset fine. If the disk is invisible, apply section 5 and record it. If the disk is seen, leave the command alone.
5. Add `D-004-001` and `E-003-001`.
6. Add `tests/suites/second-batch.list` and the CMake target `test-second`. `cmake/README.md` already says the target waits until `a-003` exists. Add it in this step, with the folder.
7. Run `test-second` until the six cases pass. Then run `test-first` once so the mutations have not broken the first batch. That second `test-first` consumes a new run id.
8. Extend `tests/README.md` so it covers both suites, the result directory, the scaffold sentence, and the note that red-pixel calibration was not required. Mention the attachment only if section 5 actually changed it.
9. Leave `brainstorming/` as exploration. The stub sentences and the ESP-resident Base stay a scaffold in anything this plan writes.

`tq all` is unchanged. `test-second` is the door for this list.

## 7. Done when

- `test-second` exits 0 and the summary shows the six cases passed.
- A following `test-first` exits 0.
- The case mutations do not change `images/`. The `a-001` UEFI manifest still matches.
- `a-003`'s UEFI qcow2 matches `a-001`'s byte for byte.
- `machine.txt` for the two `pc` boots is `pc`.
- The database image is still blank. No header, generation, or boot-success flag was written.
- `a-003` has no `vm.conf`.
- The failure cases do not set red on red. That is a property of `boot.c` and `boot-fail.c`, and this plan does not "fix" the failure path to hide firmware text.
- The host `OVMF_VARS.fd` and `/mnt/speedy/500_VMs/QueueOS-a-001/OVMF_VARS.fd` are not written by these runs. Check the mtime after the suite. Do not trust a mtime copied into this file.

## 8. Already paid for

These broke once. They are easy to reintroduce while adding the six cases.

- Root `CMakeLists.txt` contains a literal `project()` call.
- `_XOPEN_SOURCE` is 700. `LINKER:-Bsymbolic` and the linker script are passed without an extra set of quotes. `--hash-style=sysv` keeps `.hash`. `-fcf-protection=none`.
- The catalogue's TTY test does not redirect file descriptor 1.
- The variables copy is `cp -L`. The blank-database probe is `cmp -s -n 8192`. A manifest row is the digest, two spaces, then the name.
- `guest_order` compares the two line numbers. `guest_sequence` fails if the screendump fails. `ppm_is_red` skips the header.
- A proof boot unsets `DISPLAY` and `WAYLAND_DISPLAY`. A set display makes `run` open a window and wait.
- Deprecation text from `-drive if=pflash` does not fail a case. A pass does not require KVM.
- shellcheck SC2054 on a quoted QEMU comma is a false positive. The comma stays inside the argument.
- Do not kill a QEMU whose `-name` is not `queueos-test`.
- Do not call `python3`.
- `tools/mkimage.c` has fifteen lines of room. Spend them only if the folder scan cannot see `a-003`.

## 9. After this

The next plan is the smallest database-disk header that lets Boot checksum an instance and fetch the x86-64 Base, replacing the ESP scaffold. Enumeration, the Lua text TUI, the updater, and the database engine stay behind that header. They are listed in plan 001 section 16. They are not started from this file.
