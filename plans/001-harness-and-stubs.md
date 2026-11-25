# 001 — Harness and stubs

Status: first batch closed on 2026-11-24. `results/000-005` records eighteen passes. The second batch moved to [002 — Second batch](002-second-batch.md).

This file was opened as "ready to implement," and the next sentence said the stubs, the image builder, and the harness were not built yet. They are built. The sections below stay the original ask. Where a boot or a language rule made a sentence false, the correction sits next to it. The file is not rewritten to look as if those corrections were known on the first draft.

The boot disks live under `images/`, one folder per bootable system, as section 4 describes. `tests/test-queueos.lua` is the interactive catalogue in section 7.

The decision record stays in [interview-log.md](interview-log.md). This file says what to build for the first test batches, what those batches hold constant, and what they leave alone. Writing it down does not close a deferred experiment, and it does not turn a proposal into an accepted decision.

`brainstorming/DEVELOPMENT.md` milestone 1 is the slice this plan implements: a build, a minimal architecture-specific executable, QEMU/KVM, and diagnostic output. Milestone 2 (storage metadata, generation selection, loading the real next stage from the database) is a later plan. Section 16 of that note still asks open questions about the permanent toolchain, the permanent image format, and the full repository tree. This plan answers those only for the stub slice.

## 1. What this slice proves

An experimenter on this Fedora machine can build two disk images, boot them under QEMU, and see that our code ran.

QueueOS-Boot paints a red text screen, writes `QueueOS loading...`, and starts QueueOS-Base. QueueOS-Base paints a red text screen and writes `QueueOS Loaded`. The same sentences appear on the serial line. The harness keeps the serial log and, when the case asks for a display, a screenshot. When the run ends, the temporary machine is gone.

The programs are C stubs. They are the proof that the images, the firmware handoff, and the harness agree. They are not the Boot described in the interview: no Lua text TUI, no enumeration of database instances, no checksum of a generation, no fallback timer, no recovery prompt.

## 2. Decisions this slice uses

These are already accepted in the interview log. The implementer follows them and does not reopen them inside the stubs.

- The guest is a generic x86-64 PC. The first QEMU family is `q35`. The second is `pc` (i440FX). Versioned names such as `pc-q35-*` are snapshots of those families. `microvm` and `isapc` are outside this slice.
- Firmware for this guest is UEFI, using the OVMF files already installed on the host. Other boot pathways stay open and are not built here.
- QEMU is given two disk files. `QueueOS-Database.qcow2` is the database disk and has no filesystem. `QueueOS-UEFI.qcow2` is the boot disk. Its EFI System Partition is FAT32 and holds the Boot packages. qcow2 is host packaging. The guest sees two ordinary disks. The database has one working copy, versioned under `images/database/`. Each bootable system is a folder under `images/` and carries its own UEFI disk.
- The database file is the one that later platforms share unchanged. The first image carries only an x86-64 Base. There is no AArch64 Base in this slice.
- The Base executable that Boot must be able to fetch stays under about 500 KB. That figure is the executable, not a budget for the database engine.
- Boot packages on the ESP are three: current, previous, and a rescue image that updates are never allowed to replace. A planning size of a few megabytes per package is enough. The ESP is 64 MiB. Many recoverable OS versions belong on the database disk, once that format exists.
- The first guest hardware is one display, one keyboard, and one mouse. A serial-only run is a separate boot folder, `images/a-002-q35-serial`. It uses the same firmware and the same database version, and it has its own UEFI disk.
- Both stubs emit low-level diagnostics on the serial line.
- Test products are split three ways. `build/` is what we build. `tests/` holds the tests. `results/` holds what a run produces.
- A run number is a six-digit counter written `000-000`, incremented once per test invocation or per suite invocation. Case `A-001-001` from run `000-001` is written at `results/000-001/A-001-001`.
- Unity is the C unit-test framework. Blackbox tests stay separate from it.
- Letters name layers: A image, B build environment, C C unit tests, D Boot, E Base, F MCP, G Console, H Display Manager. A three-digit package sits inside a letter. `A-001` is the check package for the boot folder `images/a-001-q35-base`. `A-001-001` is one check inside it. A folder name is not a package id. `a-002-q35-serial` is a boot folder. It is not package `A-002`.
- The harness calls QEMU on files. It does not create a virtual machine as a lasting object.
- The concepts come from the Hydrogen suite in Philement. The layout does not. Hydrogen writes logs under its build tree. QueueOS writes them under `results/`. The interactive catalogue uses terminal.lua, the same library as Hydrogen's SchemaHelper, and it does not rewrite the README.

Letter assignment beyond this slice stays open in the interview log. The working assumption used here is the one already recorded: A checks image files, and the red-screen Boot check is a D test. Base's `QueueOS Loaded` line is an E test.

## 3. Outside this plan

The following stay where the interview log put them. This slice must not invent stand-ins and then treat the stand-ins as the design.

- The on-disk database format: headers, generations, checksums, directory slots, preferred versus last-known-good. `brainstorming/DATABASE.md` and `BOOTSTRAP.md` still have that as exploration. A later plan defines the smallest header Boot can checksum and fetch from.
- Fetching Base out of the database disk. Section 5 says what the stub does until that plan exists.
- The Lua Boot Script, the JSON UI description, the theme file, and the text TUI. That remains the Boot interface after this proof, including on serial, on non-UEFI PCs, on Apple Silicon without UEFI, and on RP2040-class devices. The creative budget for that TUI is the UEFI text palette (16 foreground colors, 8 background colors) and the required sharp box-drawing glyphs. Rounded corners stay unavailable for that TUI.
- Recovery zsh inside Boot, and the recovery jobs listed in the interview log.
- Programming UEFI `BootOrder` so a verified new package is first and the previous package is second. The rescue slot is created here so the layout is real. The updater that must refuse to replace it is a later plan.
- The database engine, buffer pools, queues, schedules, rights, and boot-success records. Those are the large early investment, and they start once a stub Base is visible and callable. They are not squeezed into these stubs.
- QueueOS-Desktop, QueueOS-Console, QueueOS-MCP, QueueOS-Remote, and the display manager. MCP is the likely first session the harness will use to drive Base, and that is an F plan.
- Cluster trust, version sharing, application placement, and Desktop crash handling.
- AArch64 guests and physical hardware. The harness should be shaped so a later folder can name different firmware and a different QEMU binary. This plan does not require those runs to pass.
- README's "give it a try" tone, and the CLOC workflow that rewrites README. Leave those. The README used to link to `docs/README.md` while `docs/` was an empty directory. That sentence now points at `brainstorming/` and `plans/`. An architecture manual waits until an accepted design has somewhere to live. The CLOC badge counts Markdown and Lua and skips the C sources.

## 4. The image folders

Bootable systems live under `images/`, one folder per system. A folder holds what one full test cycle needs to boot. The database is versioned once, because it changes rarely, and each boot folder points at a version.

```text
images/
  database/
    current                         one line, the version name in use
    0001/
      QueueOS-Database.qcow2
      manifest.sha256
  a-001-q35-base/
    image.conf
    manifest.sha256
    QueueOS-UEFI.qcow2
    QueueOS-Database.qcow2          symlink to the resolved database version
  a-002-q35-serial/
    image.conf
    manifest.sha256
    QueueOS-UEFI.qcow2
    QueueOS-Database.qcow2          same kind of symlink
  a-003-pc-base/                  plan 002; the folder does not exist yet
    image.conf
    manifest.sha256
    QueueOS-UEFI.qcow2
    QueueOS-Database.qcow2          same kind of symlink
```

`images/database/current` is tracked. Its first line is `0001`. A later version is a sibling directory, and `current` changes when that version becomes the working copy. `database=current` in a boot folder means read that file and use `images/database/<name>/QueueOS-Database.qcow2`. A version name in the key selects that directory directly.

`image.conf` is tracked. The harness reads it. One `name=value` pair per line:

| Key | a-001-q35-base | a-002-q35-serial | a-003-pc-base |
|---|---|---|---|
| `name` | `a-001-q35-base` | `a-002-q35-serial` | `a-003-pc-base` |
| `qemu` | `qemu-system-x86_64` | `qemu-system-x86_64` | `qemu-system-x86_64` |
| `machine` | `q35` | `q35` | `pc` |
| `display` | `graphical` | `serial` | `graphical` |
| `database` | `current` | `current` | `current` |
| `firmware-code` | `/usr/share/OVMF/OVMF_CODE.fd` | the same path | the same path |
| `firmware-vars` | `/usr/share/OVMF/OVMF_VARS.fd` | the same path | the same path |

`a-001-q35-base` and `a-002-q35-serial` are the first batch, and those folders exist. `a-003-pc-base` is drawn in the tree above and was not created here. Plan 002 creates it, and its UEFI disk is the same staged ESP bytes as the other two. `a-003-pc-base` is the `pc` machine. It is not package `A-002`, and `a-002-q35-serial` is not that package either.

The symlink `QueueOS-Database.qcow2` is a build product. `*.qcow2` is ignored, so the builder recreates the symlink and Git does not store it. The harness resolves the symlink and copies the target bytes into the run's `work/` directory. QEMU never opens the file under `images/`.

Both qcow2 files are virtual size 64 MiB.

| File | Guest-visible contents |
|---|---|
| `QueueOS-Database.qcow2` | A blank disk. No MBR signature, no GPT, no filesystem. |
| `QueueOS-UEFI.qcow2` | A GPT disk with one EFI System Partition, FAT32, type GUID `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`. |

The ESP label is `QOS-ESP`. The GPT partition name is `QueueOS-ESP`. The first usable LBA is 1 MiB aligned. One partition covers the rest of the disk through the last usable LBA. A protective MBR with a single `0xEE` entry sits at LBA 0.

ESP tree for this image:

```text
EFI/BOOT/BOOTX64.EFI
EFI/QueueOS/current/Boot.efi
EFI/QueueOS/current/Base.efi
EFI/QueueOS/previous/.keep
EFI/QueueOS/rescue/Boot.efi
EFI/QueueOS/rescue/RESCUE
```

`EFI/BOOT/BOOTX64.EFI` is a byte-for-byte copy of `EFI/QueueOS/current/Boot.efi`. With a blank OVMF variable store, firmware loads the removable-media path `\EFI\BOOT\BOOTX64.EFI`. That is the handoff this slice uses. `previous/` holds only `.keep`, a zero-length file, so the directory exists on FAT. `rescue/RESCUE` is the ASCII bytes `rescue\n`. `rescue/Boot.efi` is the same stub binary as current. The marker is what a later updater test will use to prove the slot was not replaced. This slice has no updater.

Nothing in the image programs NVRAM `Boot####` entries. Building that order is the updater plan.

The image builder is the host C program `tools/mkimage.c`, run from CMake. It must work without root. `losetup`, `qemu-nbd`, and `kpartx` are on this machine and still need privileges, so they are not the path. The program writes the GPT itself, formats the partition with `mkfs.fat -F 32 -s 1 --invariant`, and copies files with mtools (`mmd`, `mcopy`, using the `@@` byte-offset syntax against the raw image). `-s 1` is 512-byte clusters, so this 64 MiB disk has enough clusters for FAT32. `--invariant` freezes the volume id. mtools still writes the clock into directory entries, and the program clears those fields so a second build matches the first. `qemu-img convert -O qcow2` writes each boot folder's UEFI image. `qemu-img create -f qcow2` writes the blank database image for the version named by `current`. The program then creates each boot folder's database symlink. Intermediate raw files stay under `build/` and are already ignored.

A boot folder may also carry `vm.conf`. The file is optional. It names a local Virtual Machine Manager copy that should track that folder, and folders without it are left alone. `a-001-q35-base` has one. Later folders get one only when a guest is added for them. The keys are `name` (the libvirt domain on `qemu:///system`) and `directory` (the folder that holds the guest's disk copies). After writing the images, the builder refreshes `QueueOS-UEFI.qcow2` and `QueueOS-Database.qcow2` in that directory. The database copy is the resolved symlink target. Bytes that already match are left untouched. `OVMF_VARS.fd` is guest NVRAM and is not refreshed. The builder does not define the domain. It copies when `virsh` reports `shut off`, and when `virsh` cannot see the domain. Any other domain state leaves the copies in place and does not fail the build. A missing directory prints one line and does not fail the build. A `vm.conf` that is present but missing `name` or `directory` fails the build.

Each boot folder's `manifest.sha256` lists the sha256 of that folder's UEFI qcow2 and of the four ESP payloads `BOOTX64.EFI`, `current/Boot.efi`, `current/Base.efi`, and `rescue/Boot.efi`. `images/database/<version>/manifest.sha256` lists the sha256 of that version's database qcow2. A-001 checks the `a-001-q35-base` manifest and the database version that folder resolves. It does not repeat those checks for `a-002-q35-serial`.

## 5. Temporary handoff

The accepted direction is that Boot, while enumerating, retrieves the x86-64 Base executable from the database disk. This slice does not define that disk's format, so it cannot do that retrieval yet.

Until the header plan exists, the stub image places Base at `\EFI\QueueOS\current\Base.efi` on the boot disk, and Boot starts that file. This is a test scaffold. Source comments and `tests/README.md` say so in one sentence. The database image stays blank so a later format is not preempted by an accidental header.

Boot loads Base from the same device Boot itself was loaded from. It does not guess `fs0:` versus `fs1:`. The database disk has no FAT filesystem, so it will not appear as a file-system handle. The file path is `\EFI\QueueOS\current\Base.efi`. The call is `LoadImage` followed by `StartImage`.

If that start fails, Boot writes the failure sentence from section 6 on the serial line and returns an error status to firmware. It does not scan the database disk for another Base.

## 6. The two programs

Both are x86-64 PE32+ EFI applications, subsystem EFI application (value 10), machine `0x8664`.

Firmware calls the entry point with the Microsoft x64 calling convention. Host gcc on this machine uses the System V convention. `efi_main` and every function pointer that calls a UEFI service are declared with `__attribute__((ms_abi))`. Helpers that firmware does not call may stay System V. This is the same boundary the interview already drew for in-process calls versus calls that cross into another environment. Here the other environment is firmware.

Build them with the host gcc and binutils. gnu-efi, mingw, clang, and nasm are not required for this slice. Flags:

- `-ffreestanding -fno-stack-protector -fno-stack-check -fno-builtin`
- `-fpic -fshort-wchar -mno-red-zone -maccumulate-outgoing-args`
- `-fno-unwind-tables -fno-asynchronous-unwind-tables -fcf-protection=none`
- `-fvisibility=hidden -fno-plt`
- `-Wall -Wextra -Werror`
- link `-nostdlib -shared` with `-Bsymbolic` passed to the linker, not as a gcc `-B` prefix
- link `--build-id=none --hash-style=sysv -z nocombreloc -z max-page-size=0x1000 -z common-page-size=0x1000`
- a small EFI linker script, entry `efi_main`, `.text` starting at `0x1000`
- `objcopy --output-target=efi-app-x86_64` of `.text`, `.rodata`, and `.data`
- `tools/pe-reloc` writes the PE base-relocation table afterwards

This gcc is built with CET, and its default link adds a build id and a GNU hash. Those sections do not belong in the PE. `-Bsymbolic` has to be a linker flag: written bare, gcc treats it as a search prefix and leaves PLT slots firmware will never fill. Entry address 0 makes this objcopy drop the optional header, so the linker script leaves the first page for the DOS and PE headers. `--target` on this binutils sets both the input format and the output format, so the build uses `--output-target`. objcopy still marks the image relocations-stripped. `tools/pe-reloc` reads `R_X86_64_RELATIVE` from the ELF and writes `.reloc`. `src/common/memory.c` holds `memcpy`, `memset`, and one pointer to itself so that relative fixup exists.

Provide tiny `memcpy` and `memset` in the tree. gcc emits calls to them even in freestanding code. Do not link libc.

`src/common/diagnostics.h` is the only copy of the sentences. Unity and both EFI programs include it. The text is ASCII:

| Name | Bytes |
|---|---|
| Boot sentence | `QueueOS loading...` |
| Base sentence | `QueueOS Loaded` |
| Boot failure sentence | `QueueOS loading... failed to start Base` |

Screen output converts those bytes to CHAR16 (`uint16_t`, not `wchar_t`) and stops at a NUL. Serial output writes the same bytes, then CR LF.

On entry, each program:

1. Writes a diagnostic line to the serial port: `QueueOS-Boot: entry` or `QueueOS-Base: entry`.
2. If `ConOut` is present, sets the attribute to white on red (`0x0F | 0x40`), then clears the screen, then prints its sentence. `ClearScreen` fills with the background that is already selected, so the attribute comes first. Boot prints the Boot sentence. Base prints the Base sentence. The screen does not print the diagnostic lines.
3. If `ConOut` is missing, skips the screen and still writes the sentence to the serial port. A serial-only QEMU config may have no text console. The program still runs.

The serial device is the 16550 at I/O port `0x3F8`, 115200 8N1, which is QEMU's default UART. Initialize it from the stub. Wait for the transmit-holding-register-empty bit before each byte. Firmware's text protocol is CHAR16 and does not write these ASCII lines to that UART. The explicit port writes are what `serial.log` contains.

Boot then holds its screen for ten seconds before it starts Base. Each second it redraws `QueueOS loading...` and the remaining count, and it writes `QueueOS-Boot: countdown N` on the serial line. The loading sentence itself is already on the serial line before the first count, so a case that is waiting for that sentence does not sit through the countdown. On success Boot writes `QueueOS-Boot: base started` after `StartImage` returns, then sets the console attribute to red on red (`0x04 | 0x40`) without clearing. On failure it writes the failure sentence and returns an error, and it does not change the attribute, so the loading sentence stays visible. Base, after its sentence, writes `QueueOS-Base: ready`, then counts down from 10 the same way Boot does (`QueueOS-Base: countdown N` on the serial line, the sentence and the count on screen). It then sets the same red-on-red attribute. The Loaded sentence was already drawn in white and is left in place. The harness does not use either pause as a timer. It quits QEMU itself once the serial line it is waiting for has arrived.

After Base and Boot both return, firmware draws its own boot-device menu, including the line `Please select boot device:`. Before that menu sets its own colors, firmware may print a line with whatever attribute QueueOS left behind. Red on red makes that line the same color as the field. The menu then sets its own colors and stays visible. That menu waits for a key. It is not a QueueOS shutdown sentence, and QueueOS has already returned, so the stubs cannot pause it. The headless `run` target keeps the serial log through that line and saves a screenshot of the menu. A catalogue case still stops at the sentence it is checking. The serial log still contains the firmware line. The attribute only changes how the screen draws it.

`current/Base.efi` must be smaller than 500 KiB. The stub will be far smaller. The check is a tripwire for the accepted limit.

Suggested sources, and no empty directories beyond these:

```text
src/common/diagnostics.h
src/common/memory.c
src/arch/x86-64/efi/types.h
src/arch/x86-64/efi/serial.h
src/arch/x86-64/efi/serial.c
src/arch/x86-64/efi/text.h
src/arch/x86-64/efi/text.c
src/boot/x86-64/boot.c
src/boot/x86-64/boot-fail.c
src/boot/x86-64/link.lds
src/base/x86-64/base.c
tools/pe-reloc.c
tools/mkimage.c
tools/host-hash.c
```

`boot-fail.c` prints the Boot sentence and returns `EFI_LOAD_ERROR` without calling `LoadImage`. It was built with the first EFI targets so `efi-headers` could see three PE files. No first-batch case installs it. Plan 002 is what copies it onto a run's work disk.

## 7. Repository layout this slice adds

```text
CMakeLists.txt               includes cmake/
cmake/                       CMake scripts, one file per concern
src/                         as in section 6
tools/mkimage.c              disk images
tools/pe-reloc.c             PE base relocations
tools/host-hash.c            crc32 and sha256 for the image builder
images/database/current      tracked pointer, first line 0001
images/a-001-q35-base/image.conf
images/a-001-q35-base/vm.conf    optional local guest copy
images/a-002-q35-serial/image.conf
vendored/unity/              stock ThrowTheSwitch Unity, plus VERSION
tests/test-queueos.lua       interactive catalogue
tests/tq-mouse.lua
tests/README.md
tests/host/                  Unity sentence runner and Bash checks
tests/harness/run.sh
tests/cases/                 one directory per case, each with case.conf
tests/suites/first-batch.list
build/                       generated, already gitignored
results/                     generated, gitignored
```

`/results/` is ignored. `*.qcow2` is already ignored. The qcow2 files and the database symlinks are build products under `images/`. `image.conf`, an optional `vm.conf`, and `images/database/current` are tracked. Do not negate the qcow2 rule to check disk images in.

`tests/test-queueos.lua` is the catalogue a person opens. It runs on Lua 5.5 and uses terminal.lua 0.1.0, the library Hydrogen's SchemaHelper uses. The script asks `luarocks --lua-version=5.5 path` for the module path, so the alias does not have to export `LUA_PATH`. The alias itself stays in the user's shell: `alias tq='lua /mnt/extra/Projects/QueueOS/tests/test-queueos.lua'`.

With no arguments, on a terminal, it opens one full-screen list. The background is red on every cell. The word QueueOS is silver, and the Q and the OS are bold white. The version beside it is `major.minor.build` from `cmake/CMakeLists-version.cmake`: major 1, minor 0, and the build count is `git rev-list --count HEAD`. A click selects a row. The wheel moves the selection. `tests/tq-mouse.lua` is the SGR 1006 tracking, the same approach as Hydrogen's SchemaHelper, and the catalogue turns it off before restoring the terminal.

Each row is a case directory `tests/cases/<id>/` that contains `case.conf`. The row shows the directory name, the `title=` line, the last run, and the result. The last run is the highest `results/<run>/<id>/` whose `meta.txt` exists. The result is that file's `result=` line, `pass` or `fail`. A case that has never run shows `none`. No cases yet: the screen says so. Up and down move the selection, Page Up and Page Down move a page, `r` rescans, `q` or Esc quits. Ctrl-C quits too. The screen does not start QEMU.

`all` is the only run parameter in this step. With no cases it prints a line and exits 0. The draft said that, once cases existed and the harness did not, `all` would print that the runner was a later step and exit non-zero. The harness exists. `all` calls `tests/harness/run.sh` on `tests/suites/first-batch.list`. The Lua file still has no QEMU command of its own. Further parameters wait until there is a case to run.

If stdout is not a terminal, no arguments prints the same rows as text and exits 0. `--help` prints usage and exits 0. Anything else prints usage and exits non-zero.

The catalogue file stays the list. Running a case stays in the harness. Split the Lua before a file approaches 1000 lines.

`brainstorming/DEVELOPMENT.md` shows a larger tree (`src/storage`, `tests/integration`, `docs/`, and so on). That tree is illustrative. Create a directory when this slice puts a file in it.

Third-party source that this tree builds lives in `vendored/<project>/`, one upstream per folder, with its license and a `VERSION` file. Unity is the first. Firmware on the host and luarocks modules are not vendored. QueueOS sources do not go in that tree.

CMake is the front door. The lists live in `cmake/` and the root `CMakeLists.txt` includes them. Configure with `cmake -S . -B build`. The draft named Python 3 as the image builder and the harness, on the grounds that QMP and the PPM sample were poor shell, and added that Python was not part of the OS. The language rules then kept Python out of the repository entirely. `tools/mkimage.c` builds the images. The harness is Bash. QMP is one `socat` connection that sends `qmp_capabilities` before the payload. The PPM sample is Bash.

| Command | What it does |
|---|---|
| `cmake --build build` | Build the host tests, both EFI stubs, the current database image, and the `a-001` and `a-002` boot folders. |
| `cmake --build build --target test-first` | Run `tests/suites/first-batch.list` under one new run id. Exit non-zero if any case fails. |
| `cmake --build build --target test-case` | Run the case named by `-DCASE=D-001-001` under one new run id. |
| `cmake --build build --target test-second` | Not a target. Plan 002 adds it with the second-batch list and `a-003-pc-base`. Plan 002 closed with that target in place. |
| `cmake --build build --target run` | Boot `images/a-001-q35-base` and leave the screen up. |
| `cmake --build build --target clean` | Remove build products under `build/`. Leave `images/` disks and `results/` in place. |

`sentences`, `efi-headers`, and `image-a001` run under `ctest --test-dir build`. `test-first` runs the first batch. The draft said the image targets and `test-first` would arrive later. They have arrived. `run` allocates a run id and writes `serial.log` there. If `DISPLAY` or `WAYLAND_DISPLAY` is set, QEMU gets a window and the process stays in the foreground until the guest exits or the user interrupts it. Otherwise it uses the headless capture and prints the result path. `tq` with no arguments is the catalogue. `tq all` calls the harness on the first-batch list.

## 8. Run ids and result folders

The counter file is `results/run-counter`. It contains one token, such as `000-001`. If the file is missing, the next id is `000-001`. The harness takes the current value, formats the run directory from it, then writes the next value. Arithmetic drops the hyphen, adds one, and writes `%03d-%03d`. After `000-999` comes `001-000`. Deleting `results/` resets the counter. That is acceptable. The counter is not committed.

One invocation of `test-first`, `test-case`, `test-second`, or `run` consumes one id. Cases inside a suite share that id. The first harness runs cases one after another. It does not launch several QEMU processes at once.

```text
results/000-001/summary.txt
results/000-001/D-001-001/serial.log
results/000-001/D-001-001/screenshot.ppm
results/000-001/D-001-001/meta.txt
results/000-001/D-001-001/work/
```

`work/` holds the per-run copies: a fresh `OVMF_VARS.fd`, and copies of both qcow2 files from the case's boot folder. The database copy is the resolved symlink target, not the symlink. QEMU must not open the files under `images/` or the host's `OVMF_VARS.fd` for writing. Firmware writes the variable store. A case that mutates an ESP (second batch) mutates only its copy under `work/`.

`meta.txt` records `case`, `run`, `qemu` version, `machine`, `accel` (`kvm` or `tcg`), both firmware paths, whether a screenshot was requested, and `result` (`pass` or `fail`).

`summary.txt` lists every case in the invocation and its result.

A second `test-first` must leave the sha256 manifest unchanged. The suite uses copies and never rebuilds the images unless CMake considers them stale.

## 9. Harness

`tests/harness/run.sh` is the blackbox runner. The CMake test targets call it. Each `tests/cases/<id>/case.conf` names a boot folder and a check. The harness reads that folder's `image.conf` for the machine, the display, the firmware, and the database version.

```text
id=D-001-001
title=q35 graphical serial contains the Boot sentence
image=a-001-q35-base
check=serial-contains
expect=QueueOS loading...
mutate=none
```

Checks. `annotation` is the row the runner never grew. The others are what the first batch runs.

| Check | Pass rule |
|---|---|
| `tool-presence` | Every tool in section 10 is on `PATH` or at the firmware path recorded there. `/dev/kvm` is recorded and is not required. |
| `build-products` | The build has produced both EFI binaries, the current database qcow2 and its manifest, and the UEFI qcow2, symlink, and manifest for `a-001-q35-base` and `a-002-q35-serial`. |
| `unity` | The named host Unity binary exits 0. |
| `image-checksum` | Recomputed sha256 values match `manifest.sha256`. |
| `image-database-blank` | Database virtual size is 64 MiB. The first 8 KiB of the converted raw image is zeros. Offset 510 is not `55 AA`. Offset 512 is not `EFI PART`. |
| `image-esp` | UEFI image has a protective MBR, a GPT header, one ESP partition, and a FAT32 BPB whose label is `QOS-ESP`. |
| `image-payloads` | The ESP tree in section 4 is present. `BOOTX64.EFI` matches `current/Boot.efi` byte for byte. Both Boot and Base are PE32+ EFI applications, machine `0x8664`, subsystem 10. `rescue/RESCUE` is `rescue\n`. |
| `image-base-size` | `current/Base.efi` is smaller than 500 KiB. |
| `serial-contains` | `serial.log` contains `expect` at least once. On this firmware the console is also mirrored to the serial file, with ANSI sequences around the sentence, so the match is the text rather than a line that contains nothing else. |
| `serial-order` | The first occurrence of `QueueOS loading...` precedes the first occurrence of `QueueOS Loaded`. |
| `serial-absent` | `serial.log` does not contain `expect`. The runner sleeps 25 seconds, then fails if the text is present. No first-batch case uses it. |
| `screenshot-present` | `screenshot.ppm` exists, is a binary P6 PPM, and is larger than 1 KiB. |
| `screenshot-red` | At least 40 percent of sampled pixels are red-dominant: `R > 80`, `R > G + 40`, and `R > B + 40`. Sample every 16th pixel. |
| `screenshot-not-requested` | `meta.txt` says a screenshot was not requested, and no `screenshot.ppm` was written. |
| `annotation` | Specified here for failure cases: `annotation.txt` would contain the case's `expect` line. The runner never grew this check, and overloading `expect` was the wrong shape. Plan 002 splits `annotation=` from `expect=`. |

Image checks do not start QEMU. They read `images/a-001-q35-base/` and the database version that folder's `image.conf` resolves.

### QEMU command

The harness builds the command from the boot folder. `a-001-q35-base` expands to this shape. Paths point at the per-run copies. The binary is the `qemu=` value. `-machine` is `machine` plus `accel=kvm:tcg`.

```text
qemu-system-x86_64
  -machine q35,accel=kvm:tcg
  -m 256
  -no-reboot
  -no-shutdown
  -drive if=pflash,format=raw,readonly=on,unit=0,file=<OVMF_CODE>
  -drive if=pflash,format=raw,unit=1,file=<OVMF_VARS copy>
  -boot menu=on,splash-time=10000
  -drive if=none,id=esp,format=qcow2,file=<UEFI copy>
  -drive if=none,id=db,format=qcow2,file=<database copy>
  -device virtio-scsi-pci,id=scsi0
  -device scsi-hd,bus=scsi0.0,drive=esp,bootindex=1,vendor=QueueOS,product=Boot,serial=qos-boot
  -device scsi-hd,bus=scsi0.0,drive=db,bootindex=2,vendor=QueueOS,product=Base,serial=qos-base
  -serial file:<serial.log>
  -vga std
  -display none
  -device qemu-xhci
  -device usb-kbd
  -device usb-tablet
  -qmp unix:<qmp.sock>,server=on,wait=off
  -name queueos-test
```

Use the non-secure-boot firmware pair. Secure boot would reject these unsigned stubs.

- Code: `/usr/share/OVMF/OVMF_CODE.fd`
- Variables: `/usr/share/OVMF/OVMF_VARS.fd`

The edk2 directory on this machine also has 4 MiB qcow2 firmware. Leave it alone while the `.fd` pair works. Do not copy firmware into the repository.

`accel=kvm:tcg` lets QEMU fall back when `/dev/kvm` cannot be opened. `meta.txt` records which accelerator actually ran (the QEMU log line, or a probe). A pass does not require KVM. A silent fallback is still visible in the meta file.

`display=serial` drops `-vga std`, the USB devices, and the tablet, and sets `-vga none -display none`. It keeps `-serial file:` and the same firmware. `a-002-q35-serial` is that command.

`display=graphical` with `machine=pc` is the same argument list as `a-001-q35-base` with `-machine pc,accel=kvm:tcg`. `a-003-pc-base` is that command, and it is second-batch work.

Deprecation warnings from `-drive if=pflash` do not fail a case.

Screenshot path: the harness waits until `serial.log` contains the line that case needs, sends QMP `qmp_capabilities` and `screendump`, then sends `quit`. Overall timeout is 50 seconds. The firmware start-boot progress holds for ten seconds before Boot runs, and a case that waits for `QueueOS Loaded` also waits out Boot's countdown. On timeout it kills QEMU, marks the case failed, and keeps the logs. `-no-shutdown` leaves the frame buffer available if the guest returns from the EFI app before the dump.

The red-pixel check is allowed one calibration after the first real PPM if OVMF's drawn red falls outside the thresholds above. Calibration changes the thresholds in this plan's check, records the captured reason in `tests/README.md`, and does not weaken the serial-line checks.

A D-case screenshot is taken when `QueueOS loading...` appears, which is before the countdown finishes and before Base starts, so the frame is Boot's red screen. An E-case screenshot is taken when `QueueOS Loaded` appears, during Base's countdown. The serial log is the record of the sentences. The screenshot proves a red display was produced. Do not OCR the PPM. The firmware boot menu is captured by the headless `run` target, not by these cases.

### Failure annotations

The draft said second-batch failure cases would write `annotation.txt` in the case directory before judging the serial log. That write was not built. The sentences below are the ones plan 002 still uses. The annotation is a fixed sentence, so the result says what kind of failure was observed. The harness copies it from the case. It does not parse it out of firmware text.

| Situation | Annotation line |
|---|---|
| `\EFI\BOOT\BOOTX64.EFI` removed from the run copy | `firmware did not hand off to QueueOS-Boot` |
| Boot returns an error without starting Base | `QueueOS-Boot returned before starting Base` |
| `current/Base.efi` removed from the run copy | `QueueOS-Boot could not start Base` |

A missing Boot file passes when our sentences are absent and that first annotation is present. Firmware is then doing what the interview described: the EFI file is missing, so firmware does not run our program. A hang after handoff is a different mode. These cases do not claim that firmware walked the database disk.

## 10. Host tools the first batch requires

Recorded by `B-001-001`. All of these were present on the development machine when this plan was written (QEMU 10.2.2, KVM node present, edk2 OVMF under `/usr/share/OVMF`).

- `gcc`, `objcopy`, `cmake`, `bash`
- `qemu-system-x86_64`, `qemu-img`
- `mkfs.fat`, `mmd`, `mcopy`
- `/usr/share/OVMF/OVMF_CODE.fd` and `OVMF_VARS.fd`

`parted` is installed and is not required, because the image builder writes the GPT. A missing tool fails `B-001-001` with the name it looked for. The suite lists B first so the failure happens before any QEMU run.

## 11. First batch

`tests/suites/first-batch.list` runs in this order. Eighteen cases. Implement these, then run them, before starting section 12.

### B-001 — build environment

| Case | Check | Pass |
|---|---|---|
| `B-001-001` | `tool-presence` | Section 10 is satisfied. |
| `B-001-002` | `build-products` | Stubs exist, and the current database image plus the `a-001` and `a-002` boot folders exist under `images/`. |

### C-001 — sentences, on the host, no QEMU

| Case | Check | Pass |
|---|---|---|
| `C-001-001` | `unity` | Boot sentence equals `QueueOS loading...`. |
| `C-001-002` | `unity` | Base sentence equals `QueueOS Loaded`. |
| `C-001-003` | `unity` | Failure sentence equals `QueueOS loading... failed to start Base`. |

Unity is ThrowTheSwitch Unity 2.7.0, MIT, imported under `vendored/unity/` with the version written in `vendored/unity/VERSION`. The import is the stock `src/` files and the license. Do not copy the Hydrogen Unity tree, and do not edit the imported files. The runners are ordinary host binaries. They include `diagnostics.h` and do not compile the EFI sources.

### A-001 — q35 image package, no QEMU

| Case | Check | Pass |
|---|---|---|
| `A-001-001` | `image-checksum` | Manifest matches the files just built. This is the checksum check the interview used as the example. |
| `A-001-002` | `image-database-blank` | Database image is a blank 64 MiB disk. |
| `A-001-003` | `image-esp` | UEFI image is GPT plus one FAT32 ESP. |
| `A-001-004` | `image-payloads` | ESP tree, PE headers, rescue marker, Boot copy. |
| `A-001-005` | `image-base-size` | Base payload is under 500 KiB. |

`A-001` checks `images/a-001-q35-base` and the database version it resolves. The UEFI bytes are not specific to the q35 chipset. `a-002-q35-serial` is a boot folder, not a second image package, and these checks do not repeat against it. The `pc` folder is `images/a-003-pc-base`, checked in the second batch.

`ctest` runs the same five image checks as `image-a001` through `tests/host/check-images.sh`, plus `sentences` and `efi-headers`. The catalogue has the eighteen first-batch cases. `tq all` runs that list.

### D-001 — q35 graphical Boot

Boots `images/a-001-q35-base`.

| Case | Check | Pass |
|---|---|---|
| `D-001-001` | `serial-contains` | `QueueOS loading...` is in the serial log. `meta.txt` records machine `q35` and the firmware paths. |
| `D-001-002` | `screenshot-present` | A real PPM was written. |
| `D-001-003` | `screenshot-red` | The sampled frame is red-dominant. |

### D-002 — q35 serial-only Boot

Boots `images/a-002-q35-serial`. Same firmware and the same database version as `a-001-q35-base`. No display device.

| Case | Check | Pass |
|---|---|---|
| `D-002-001` | `serial-contains` | `QueueOS loading...` is in the serial log. |
| `D-002-002` | `screenshot-not-requested` | The case asked for no screenshot. |

### E-001 — q35 graphical Base

Boots `images/a-001-q35-base`.

| Case | Check | Pass |
|---|---|---|
| `E-001-001` | `serial-order` | `QueueOS loading...` comes before `QueueOS Loaded`. |
| `E-001-002` | `screenshot-present` | A PPM was written after the Base line. |

### E-002 — q35 serial-only Base

Boots `images/a-002-q35-serial`.

| Case | Check | Pass |
|---|---|---|
| `E-002-001` | `serial-order` | The same order, with no display device. |

D and E are separate cases even though a single guest run prints both lines. Run QEMU once per case. A Boot that prints its sentence and fails to start Base can pass D and fail E.

## 12. Second batch

Moved to [002 — Second batch](002-second-batch.md) after `results/000-005`. The original ask was six cases: three mutations of an `a-001` work copy (missing `BOOTX64.EFI`, `BootFail.efi` in place of Boot, missing `Base.efi`), then `images/a-003-pc-base` as machine `pc` with the same ESP bytes, then the Boot and Base sentences on that machine. Plan 002 holds the case files, the pass rules, and the corrections. The table that used to live here is not repeated, so this file cannot drift into a second contract.

What the boots changed, in short:

- The first disk attachment was virtio block. The firmware menu drew `UEFI Misc Device` and `UEFI Misc Device 2`. FAT labels, GPT names, and a virtio-blk serial never reach that menu. Section 9's command is virtio-scsi, vendor `QueueOS`, products `Boot` and `Base`. The menu then drew `UEFI QueueOS Boot` and `UEFI QueueOS Base`. The AHCI fallback in the old section 12 was not used. Plan 002 keeps it for a boot whose log shows the firmware never saw a disk, and does not switch ahead of that boot.
- `-boot menu=on,splash-time=10000` is the ten-second `Start boot option` progress on this OVMF. The blue `Please select boot device:` list is where the guest returns, and that list waits for a key.
- `mutate=` is in every first-batch `case.conf`. The runner does not read it. The annotations in section 9 were specified and not written.

A later boot folder may set `qemu` to `qemu-system-aarch64` and name a firmware path that is not `OVMF_CODE.fd`. No case in the first batch references it, and plan 002 does not add one. The suite does not fail because that platform is absent.

## 13. Order of work

1. Ignore `/results/`. Done. The catalogue can read that tree without Git storing it.
2. The catalogue `tests/test-queueos.lua` is in place. It lists cases. `all` calls the harness. The Lua file does not boot a guest by itself.
3. Add `diagnostics.h` and the three Unity cases under `vendored/unity`. Run them with `ctest`. Done.
4. Add the freestanding EFI support and both stubs. Done. `ctest` passes `sentences` and `efi-headers`. The red screen was debugged with the harness, which is what this step was trying to protect.
5. Add `tools/mkimage.c`, the `image.conf` files, the database version, the `a-001` and `a-002` folders, and the A-001 checks. Done. `image-a001` passes. The builder is host C and the checks are Bash. `a-003` waits in plan 002. `images/a-001-q35-base/vm.conf` names the local guest copy that the image build refreshes. `tools/mkimage.c` is 985 lines. Plan 002 adds `a-003` as another `image.conf`, which the existing folder scan already picks up.
6. Add the harness and the D and E cases. Done. Each case names its boot folder. The runner is `tests/harness/run.sh`. `cmake --build build --target test-first` runs the first batch.
7. Run `test-first` and fix the slice until the first batch passes. Done. `results/000-005` is eighteen passes. The counter's next id is `000-006`.
8. Second batch. Moved to plan 002. `test-second` does not exist yet. Plan 002 added it. `results/000-007` is the six passes.
9. `tests/README.md` covers the first batch: how to run it, where a result goes, the scaffold sentence from section 5, the two countdowns, the menu names, and the red-on-red attribute. The two-suite note waits until plan 002 has a second suite. The red-pixel calibration in section 9 was not required. The thresholds held.
10. Done. `brainstorming/` stays exploration. The stub sentences and the ESP-resident Base are a scaffold.

## 14. Done when

Met on this machine:

- `results/000-005/summary.txt` shows all eighteen first-batch cases passed. Earlier runs occupy `000-001` through `000-004`. The counter advanced. This close-out did not launch another `test-first` to re-prove that a second invocation leaves `manifest.sha256` alone. `A-001-001` inside `000-005` is the checksum check that passed.
- The headless `run` path captured the red Boot screen, the red Base screen, and the firmware device menu. The virt-manager guest `QueueOS a-001` is what was watched by hand. The image refresh does not copy `OVMF_VARS.fd`. This close-out did not start the domain. It was shut off, and the variables file was 131072 bytes with mtime `2026-11-24 17:59:39 -0700`. An earlier note from the same day said `17:23:57`. The file has been written since that note. Leave it alone.
- `current/Base.efi` is the small stub, under the 500 KiB tripwire, and `A-001-002` still sees a blank database image.
- No source file, image builder, or test writes a database header, a generation record, or a boot-success flag.

Moved to plan 002:

- `test-second` exits 0, with the six second-batch cases passed. Met in plan 002 as `results/000-007`, followed by `results/000-008` for the first batch.

## 15. What the first real runs actually did

These were written as risks. The ones that fired are recorded as history. The ones that are still load-bearing stay as rules.

- `ConOut` and the serial port are different devices. A stub that only calls `OutputString` passes a screenshot glance and fails every serial assertion. The port writes in section 6 are part of the stub.
- A graphical case that assumes `ConOut` is never null faults the serial-only case. The null check in section 6 stays.
- Opening the host `OVMF_VARS.fd` directly makes later runs nondeterministic. The harness copies it. The image refresh leaves the virt-manager copy alone.
- The Microsoft x64 attribute on UEFI function pointers is load-bearing. A System V call into `OutputString` faults before any sentence appears.
- The red-pixel thresholds did not need the calibration in section 9. Leave them as written.
- Virtio block was invisible to this OVMF menu. The lines were `UEFI Misc Device` and `UEFI Misc Device 2`. Virtio-scsi fixed the names. AHCI was not the fix. See section 12.
- `SetAttribute` does not repaint glyphs already drawn. Red on red hides the firmware line that follows a successful return, and it leaves `QueueOS Loaded` white. The failure path does not change the attribute.
- FAT does not reliably store an empty directory. `previous/.keep` is why that slot has a file.
- AArch64 firmware, when that plan arrives, is a different file and a different QEMU binary. Reusing `OVMF_CODE.fd` there will not boot.

## 16. What the next plans will have to pick up

Plan 002 is written and is the next slice to build. The rest of this list is an order implied by the interview, not a set of accepted designs.

1. Plan 002: the six second-batch cases, the `pc` folder, and the harness mutations. Build that before the database header.
2. The smallest database-disk header that lets Boot checksum an instance and fetch the x86-64 Base, replacing the scaffold in section 5.
3. Enumeration, latest-by-timestamp selection, and the timer that highlights the most recent successful boot when the latest attempt failed. The success presumption in the interview log is the starting point, and it is still not a finished criterion.
4. The Lua text TUI on top of the C shim, after the stub screen has been proven.
5. The updater: write a new package, verify it, then set firmware order to new, then previous. Never replace rescue.
6. The database engine inside Base, which is the work the interview expects to dominate the early schedule once these batches are green.
