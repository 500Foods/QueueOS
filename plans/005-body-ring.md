# 005 — Body ring

Status: opened ready to build on 2026-11-25. `plans/README.md` names this file as the active slice. Plan 004 closed the same day. The interview after that close is `plans/interview-log.md`, under "Plan 005 interview". This file is that cluster.

Nothing in it is built. The shipped body page is still zeros. The database qcow2 this draft was written against is `9bc491c85a6a39c181db306fcfe9ec4024ca8991325221da3d1ca74b7bdfa816`. The UEFI qcow2 is `b1c57b16dc9647a8f1fd25f00a24bfc85840b351d0981286630efeefe26b4840`. The next run id is `000-031`.

Both steps are built. Host `ctest` was nine passes, including `body-page`. `results/000-031` is `E-004-001`: the planted sentence `stored 2026-11-25T00:00:00Z slot 1`, then `QueueOS Loaded`. The firmware console copies the drawn Loaded sentence onto the serial line, so that log line reads `QueueOS Loadedstored 2026-11-25T00:00:00Z slot 1` and the next line is `QueueOS Loaded`. The work page is the planted object and `{"stored":"2026-11-26T06:06:48Z","slot":1}`, hash `20f030182db458c40d42615ec5cff82d6a2502a0469b91707b657c793d5ed5f8`, tail zero. Slot 1 gained `booted 2026-11-26T06:06:48Z`. `results/000-032` is `E-001-001`: `QueueOS Loaded`, and no `stored ` line. The work page is `[{"stored":"2026-11-26T06:07:14Z","slot":1}]`, hash `fee8bb9bddfb09897e1a6c160b90c23ab34b8943ed20b9b625f358d7d8a7f5f4`, tail zero. Slot 1 gained `booted 2026-11-26T06:07:14Z`. The slot JSON hash line is `sha256 ce6a62441aedc7e2930a2d50b5a739868231b23a827fc78ab3563477765430cc`. `results/000-033` is `D-003-003`: the failure sentence, `QueueOS Loaded` absent, no `stored ` line, no `booted` line, and the work page still zeros. The stamps are the firmware clock. Screenshot cases were not re-run. The shipped body page is still zeros. The database qcow2 is now `5d8a2b01f6ddf7afdaeec20c0a4ae4782121452567189d39ccc1a51e31391e3c`. The UEFI qcow2 is now `913ad1d3cbefc1fcf8bf11f65ed131021dafcc70d801959dd7b5103bc2868980`. Base in the slot is length 93696, sha256 `a9dc3bfd803dee4c43b807397e727a146e29dfe36404b8807abd4cef6f286030`. The shut-off `a-001` copies were refreshed to those two qcow2 hashes. The domain was not started. `OVMF_VARS.fd` was not edited. Its hash is still `8af72aeb775c49d6e1545d74c587f04b410b5dafa6a9358932b1b9380d21ead0`. The next run id is `000-034`.

## 1. What is already built

The database disk has no filesystem. Ten 4 KiB slots start at byte 0. The boot range holds Base. `QUEUEOS_BODY_START` in `src/common/disk.h` is 8 MiB. `QUEUEOS_BODY_PAGE` is 4096, beside that constant, and that is the only copy of the size. The bytes from the page to the end of the 64 MiB image are zeros.

Plan 004 put one text record on that page. Two lines: `stored STAMP slot N`, then `sha256` and 64 lowercase hex digits of that stored line. Base printed the line, then replaced the page. The publisher copies the image and leaves the page where it was copied. Boot does not read the body. Base links the pool, `slot-text.c`, `body-page.c`, and `sha256.c`. Jansson is linked into `Boot.efi` and is not linked into `Base.efi`.

`queueos_body_read` and `queueos_body_write` are the page. `queueos_base_complete` reads the page, prints a record, writes the page, and then `base.c` prints `QueueOS Loaded`. The harness quits once that sentence arrives, so the page write has to finish before it. A guest test copies the database qcow2 into the run directory. `images/` is not opened for writing. `mutate=plant-body-record` is the existing way a case plants the sample page.

## 2. The page

The same page becomes a ring. The writer emits one canonical text. Jansson reads it. `json_dumps` is not the writer: Jansson walks object keys in hashtable order, and that order follows the hash seed, so a dump would not be a stable hash. The canonical text is the hash input.

A one-element page looks like this. The hash covers the JSON bytes and not the newline.

```text
[{"stored":"2026-11-25T00:00:00Z","slot":1}]
sha256 fe46aee0aa27eade88658f2037557cde0a585d52ef1291cec0805225d38b6570
```

Each object is `{"stored":"STAMP","slot":N}` with those keys in that order, no spaces, and the slot in decimal with no leading zero. Slot 10 is two digits. The stamp is the same twenty-character clock `stamp_form` already accepts. Objects are separated by a comma. The array is wrapped in `[` and `]`. The second line is `sha256 `, 64 lowercase hex digits, and a newline. The rest of the page is zeros.

Slot 1's object is 42 bytes. Slot 10's is 43. A page of one-digit slots holds 93 objects. The 94th does not fit, and the oldest is dropped. A page of slot-10 objects holds 91. The fit is the encoded size, not a second copy of those counts. `QUEUEOS_BODY_ROWS` in `body-page.c` is 96, which is only the stack bound. A parsed array longer than that is not a ring.

The page is a ring when the hash matches the JSON bytes, the tail is zeros, and Jansson reads a JSON array of those objects. An object has both keys and no third key. `stored` is a stamp. `slot` is a JSON integer from 1 to 10. An empty array is a ring with no row. Spaces are allowed. Key order is allowed to differ from the writer's order. A newline inside the JSON splits the page the way a slot's first newline splits JSON from the hash, so a pretty-printed array is not a ring. Zeros, a bad hash, a bad stamp, a slot outside 1..10, a real number, uppercase hex, a non-zero tail, or the plan 004 text line are not a ring.

`queueos_body_read` returns the newest object's sentence, `stored STAMP slot N`, the same characters plan 004 printed. The slot out-parameter receives that object's slot. An empty array and a rejected page return 0, and the out-parameter is left alone. `queueos_body_write` appends one object when the page is a ring, dropping the oldest until the encoding fits. A zero page or a rejected page is replaced by a one-element array. A refused stamp or slot leaves the caller's bytes unchanged. The new text is parsed with Jansson before it is copied into the caller's page.

The text stays in `src/common/body-page.c`. `slot-text.c` stays the slot format. Base and the host test link Jansson the way Boot already does. Boot does not link `body-page.c`.

## 3. What Base does

The screen stays `QueueOS Loaded`. The serial line for a ring that already has a row is the newest stored sentence.

On a boot that has a slot number and a stamp:

1. The slot line is written the way it is now. An existing `booted` line stays the first stamp.
2. Base reads the page. When the page is a ring with a row, Base prints that newest sentence.
3. Base appends this boot's stamp and slot. One GetTime covers the slot line and the new object.
4. Base prints `QueueOS Loaded`.

A zero page prints nothing, then becomes a one-element array. A rejected page prints nothing, and the one-element array replaces it. The plan 004 text page is a rejected page.

The page write follows a slot path that accepted this boot. No slot number, no clock, or a slot `queueos_slot_mark_booted` refuses leaves the page as it was. When the slot bytes change, the page is written only after `queueos_disk_write` returns 1 for that slot. A failed page read or a failed page write leaves the disk page as it was. `QueueOS Loaded` is still printed.

## 4. What stays where it is

Boot still reads slots and draws the list. The host list and Explore do not show this page. The shipped image still has a zero body. Rebuilding the image after `Base.efi` changes will change the database qcow2 and the UEFI qcow2, because Base lives in the boot range and is also copied onto the ESP. The body of the shipped image stays zeros.

`tests/README.md` gains a sentence that the page is this ring. The scaffold paragraph in that file stays the diary it already is. `plant-body-record` plants the section 2 page. `E-004-001` still expects `stored 2026-11-25T00:00:00Z slot 1`.

The `a-001` domain is not started. `OVMF_VARS.fd` is not edited. A shut-off copy that an image build refreshes is left refreshed, which is what plan 004 did on its last build. A running guest is left alone.

## 5. Tests

Host test `body-page`:

- A zero page is not a ring. A null page or a null line is refused. A refused stamp or slot leaves the caller's bytes unchanged.
- The sample page in section 2 reads back as `stored 2026-11-25T00:00:00Z slot 1`.
- Slot 10 round-trips. Its JSON is `[{"stored":"2026-11-25T00:00:00Z","slot":10}]` and its hash is `9aa471dae1ea3306b0deeae9cd8581c09f0fa59d7d63b1f1e051a2d9e114ced0`.
- A second write keeps the first object and the read sentence is the second object.
- An empty array reads as no sentence. The next write is a one-element array.
- A reordered object is a ring. The next write rewrites it in the canonical order and keeps the stamp.
- A flipped hash, uppercase hex, a non-zero tail, a third key, a real number, a bad stamp, and the plan 004 text line are not a ring. Writing after the text line replaces it.
- Ninety-three one-digit objects fit. The ninety-fourth write drops the oldest stamp and keeps the next one.
- `queueos_body_write` leaves the bytes after the page in a larger buffer as they were.
- A publish of a generation on an image that already holds a ring leaves that page, and leaves a non-zero byte past the page, unchanged.

`E-004-001` plants the section 2 page and expects `stored 2026-11-25T00:00:00Z slot 1`, then `QueueOS Loaded`. `E-001-001` reaches `QueueOS Loaded` with no `stored ` line, and the work page is a one-element ring for slot 1. `D-003-003` fails closed, with no stored line, and its work page stays zeros. Screenshot cases stay unrun.

## 6. Outside this plan

- Queues, SQL, buffer pools, a second page, a header, and a page map.
- Teaching Boot or the host list to read the ring.
- Refreshing the slot's `booted` line on a later boot.
- A finished definition of a successful boot beyond the slot stamp and this page.
- Thumbnails, Explore editing, the updater, the rescue package, firmware boot order, recovery zsh, and the reorg command.
- Linking Lua into Base.
- A new letter of the catalogue. This plan uses E.
- A commit.

## 7. Order

1. The ring in `body-page.c`, Jansson linked into Base and the host test, and the host checks. The shipped body stays zeros.
2. `plant-body-record` plants the sample array. `E-004-001`, `E-001-001`, and `D-003-003` are the guest proofs.

Step 2 is the one that changes what a guest prints. Both steps leave the tree bootable. The guest proofs use run ids from `000-031`.

## 8. Done

- `ctest` passes, including `body-page`. Host `ctest` was nine passes.
- `E-004-001` prints the planted sentence and then `QueueOS Loaded`. `results/000-031` passed. The work page holds the planted object and the new one.
- `E-001-001` prints `QueueOS Loaded` and no stored line, and the work page is a one-element ring. `results/000-032` passed. The page is `[{"stored":"2026-11-26T06:07:14Z","slot":1}]`.
- `D-003-003` fails closed, with no stored line. `results/000-033` passed. The work page is zeros.
