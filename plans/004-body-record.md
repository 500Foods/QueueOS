# 004 — Body record

Status: closed on 2026-11-25. Both steps are built. No following plan is ready. Plan 005 is not opened.

It was opened ready to build on 2026-11-25. `plans/README.md` named this file as the active slice. It was drafted the same day and then marked ready. Nothing in it was built then.

Both steps are built. Host `ctest` was nine passes, including `body-page`. `results/000-028` is `E-004-001`: the planted line `stored 2026-11-25T00:00:00Z slot 1`, then `QueueOS Loaded`. The firmware console copies the drawn Loaded sentence onto the serial line, so that log line reads `QueueOS Loadedstored 2026-11-25T00:00:00Z slot 1` and the next line is `QueueOS Loaded`. The work page was replaced with `stored 2026-11-26T02:08:06Z slot 1` and the hash matched. Slot 1 gained `booted 2026-11-26T02:08:06Z`. The JSON hash line is `sha256 a530bd7751b40fe3324063b5e2b0d0597936025f46a3649547929a443a951d1f` on the `E-001-001` work copy, which is the hash of the new slot JSON. `results/000-029` is `E-001-001`: `QueueOS Loaded`, and no `stored ` line. The work page is `stored 2026-11-26T02:08:32Z slot 1` with a matching hash and a zero tail. Slot 1 gained `booted 2026-11-26T02:08:32Z`. `results/000-030` is `D-003-003`: the failure sentence, `QueueOS Loaded` absent, no `stored ` line, and the work page still zeros. The stamps are the firmware clock. Screenshot cases were not re-run. The shipped body page is still zeros. The database qcow2 is now `9bc491c85a6a39c181db306fcfe9ec4024ca8991325221da3d1ca74b7bdfa816`. The UEFI qcow2 is now `b1c57b16dc9647a8f1fd25f00a24bfc85840b351d0981286630efeefe26b4840`. Base in the slot is length 33280, sha256 `4eb267cb8a783a2e1cdf7c56156d80a53aa4973308dad8c97125c82056a4f795`. The shut-off `a-001` copies were refreshed to those two qcow2 hashes. Before that refresh the guest database copy was `6b610ce69a6319c3b7b59d28b57824b2c833d7a6aab9eee9a8b3fc0f0aa6c6d9`, which was not the repo hash this draft was written against. The domain was not started. `OVMF_VARS.fd` was not edited. Its hash is still `8af72aeb775c49d6e1545d74c587f04b410b5dafa6a9358932b1b9380d21ead0`. The next run id is `000-031`.

Plan 003 closed the same day. The interview after that close is `plans/interview-log.md`, under "Plan 004 interview". This draft is that cluster. Writing it down does not change `Base.efi` and does not put a record in the shipped database image. The body of that image stays zeros. The database qcow2 this draft was written against is `6d00aaece1af9d9737af226bb2fb1dcead34772b3d2680cf9b3677e80f85aa3e`. The UEFI qcow2 is `c748825771428c6e43283ddb831b604d5c681acdfb5aa236838da9939267d975`. The next run id is `000-028`. That id was used. The sentences above are the record of the boots.

## 1. What is already built

The database disk has no filesystem. Ten 4 KiB slots start at byte 0. The boot range holds Base. `QUEUEOS_BODY_START` in `src/common/disk.h` is 8 MiB, and that constant lives only in that file. The bytes from there to the end of the 64 MiB image are zeros.

Base draws `QueueOS Loaded`, then writes `booted STAMP` into the slot named by its load options, then prints that sentence on the serial line. The stamp is the firmware clock, `YYYY-MM-DDTHH:MM:SSZ`. GetTime is not asked to convert a timezone. The harness quits once `QueueOS Loaded` arrives, so a disk write has to finish before that serial line.

`queueos_slot_mark_booted` leaves an existing `booted` line unchanged. `queueos_base_complete` then sees identical slot bytes and returns. A second boot of the same slot does not refresh that stamp. The publisher treats that first line as the completion mark. `queueos_disk_write` returns 1 on success. The completion path ignores that return today.

`queueos_publish` copies the whole image and edits slots and the boot range. The body stays where it was copied. Base links the pool, `slot-text.c`, and `sha256.c`. The build keeps Jansson and Lua out of `Base.efi`. Boot reads slots. It does not read the body.

A guest test copies the database qcow2 into the run directory and writes that copy. `images/` is not opened for writing. `mutate=corrupt-base-bytes` is the existing way a case changes the work database.

## 2. The page

One 4 KiB page sits at `QUEUEOS_BODY_START`. `QUEUEOS_BODY_PAGE` is 4096, defined beside `QUEUEOS_BODY_START` in `src/common/disk.h`, and that is the only copy of the size. The store writes that page and leaves every later body byte alone.

The page is two text lines and then zeros.

```text
stored 2026-11-25T00:00:00Z slot 1
sha256 b5584f1334084037dd6be0610f569914deac15d8651c367ec5b7f98e5edc59bf
```

The first line is `stored `, the stamp, ` slot `, and the slot number in decimal with no leading zero. Slot 10 is `slot 10`. The hash covers those bytes and not the newline. The second line is `sha256 `, 64 lowercase hex digits, and a newline. The rest of the page is zeros.

The stamp is twenty characters: a four-digit year, a hyphen, a two-digit month, a hyphen, a two-digit day, `T`, a two-digit hour, a colon, a two-digit minute, a colon, a two-digit second, and `Z`. Year is 0001 through 9999. Month is 01 through 12. Day is 01 through 31. Hour is 00 through 23. Minute is 00 through 59. Second is 00 through 60. That is the form `format_stamp` in `src/base/x86-64/completion.c` already writes. `stamp_ok` in `slot-text.c` accepts any visible token, and it is not this check.

A page is a record when both lines match and every byte after the hash newline is zero. Zeros, a bad hash, a bad stamp, a slot outside 1..10, uppercase hex, a leading zero on the slot, or a non-zero tail mean there is no record. The reader reports the stored line without its newline when the page is a record.

This page is the one record from the interview. A later plan can replace it with a table. The text lives in `src/common/body-page.c` with `src/common/body-page.h`. `slot-text.c` stays the slot format. Base and the host test link the new file. Boot does not.

## 3. What Base does

The screen stays `QueueOS Loaded`. The serial line for a record is the stored line itself, the same characters the page holds before the newline.

On a boot that has a slot number and a stamp:

1. The slot line is written the way it is now. An existing `booted` line stays the first stamp. That path still counts as this boot having a slot to store.
2. Base reads the page. When the page is a record, Base prints that stored line on the serial port.
3. Base writes a new page. The stamp is the one this boot already formatted for the slot line. The slot number is the load-options slot. One GetTime covers both writes.
4. Base prints `QueueOS Loaded`.

A zero page prints nothing before the new page is written. A rejected page prints nothing, and the new page replaces it.

The page write follows a slot path that accepted this boot. No slot number, no clock, or a slot `queueos_slot_mark_booted` refuses leaves the page as it was, and no stored line is printed. When the slot bytes change, the page is written only after `queueos_disk_write` returns 1 for that slot. A failed page write leaves the page as it was. `QueueOS Loaded` is still printed.

An existing `booted` line used to return before any further disk write. That return stays for the slot bytes and no longer skips the page. The page stamp can be later than the slot's first `booted` line. The publisher keeps using the slot line.

## 4. What stays where it is

Boot still reads slots and draws the list. The host list and Explore do not show this page. The shipped image still has a zero body. `A-001-002` stays a check of that zero body. Rebuilding the image after `Base.efi` changes will change the database qcow2 and the UEFI qcow2, because Base lives in the boot range and is also copied onto the ESP. The body of the shipped image stays zeros. `Boot.efi` stays as it is unless a build shows otherwise.

`tests/README.md` gains a sentence that Base writes this page after the slot line and prints an older stored line before `QueueOS Loaded`. The scaffold paragraph in that file stays the diary it already is.

The `a-001` domain is not started. `OVMF_VARS.fd` is not edited. A shut-off copy that an image build refreshes is put back when the plan's own rule for that guest says so. Plan 003's last image build refreshed those copies and left the domain shut off. This plan follows that.

## 5. Tests

Host test `body-page`, in `tests/host/test-body-page.c`:

- A zero page is not a record.
- The sample page in section 2 reads back as `stored 2026-11-25T00:00:00Z slot 1`.
- Slot 10 round-trips. Its stored line is `stored 2026-11-25T00:00:00Z slot 10` and its hash is `6217a9917cf2f265febf9e1001ea9ac6ea04095701a603be149e996375bf0c12`.
- A flipped hash, a bad stamp, uppercase hex, `slot 01`, a slot of 0 or 11, and one non-zero byte after the hash line are not records.
- `queueos_body_write` lays down a page that `queueos_body_read` accepts, and the bytes after the page in a larger buffer stay as they were.
- A publish of a generation on an image that already holds a good page leaves that page, and leaves a non-zero byte past the page, unchanged.

`E-004-001` is a q35 guest on `a-001-q35-base`. `mutate=plant-body-record` writes the section 2 page at offset `QUEUEOS_BODY_START` of the work database and leaves `images/` alone. The shape follows `corrupt-base-bytes`. `check=serial-order`. `expect` is `stored 2026-11-25T00:00:00Z slot 1`. Serial-order still waits for `QueueOS Loaded`. When the case sets `expect`, the log has to contain that line as well. Cases that omit `expect` stay as they are. The first and second batch lists are left as those batches. The case is run with `run.sh --case E-004-001`.

`E-001-001` is the empty-body boot. The serial log reaches `QueueOS Loaded` and has no `stored ` line. The work database then holds a well-formed page for slot 1. That inspection is recorded in this file the way plan 003 recorded the first `booted` line. `D-003-003` still fails closed, and its work page stays without a stored line.

Screenshot cases stay unrun. The screen is unchanged.

## 6. Outside this plan

- Queues, SQL, buffer pools, and any second page.
- A table format for the body. This page can be replaced later.
- Refreshing the slot's `booted` line on a later boot.
- A finished definition of a successful boot beyond the slot stamp and this page.
- Thumbnails, Explore editing, the updater, the rescue package, firmware boot order, recovery zsh, and the reorg command.
- Teaching Boot or the host list to read the page.
- Linking Jansson or Lua into Base.
- A new letter of the catalogue. This plan uses E.
- A commit.

## 7. Order

1. `body-page.c`, the host test, and the publish preservation check. The guest still boots as it does today. The shipped body stays zeros.
2. Base reads, prints, and writes the page. `plant-body-record` and `E-004-001` land with that step. `E-001-001` and `D-003-003` are the other guest proofs.

Each step leaves the tree bootable. Step 2 is the one that changes what a guest does.

## 8. Done

- `ctest` passes, including `body-page`. Host `ctest` was nine passes.
- `E-004-001` prints the planted line and then `QueueOS Loaded`. `results/000-028` passed.
- `E-001-001` prints `QueueOS Loaded` and no stored line, and the work page is a record. `results/000-029` passed. The page is `stored 2026-11-26T02:08:32Z slot 1`.
- `D-003-003` fails closed, with no stored line. `results/000-030` passed. The work page is zeros.
