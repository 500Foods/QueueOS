# 003 — Slots and the publisher

Status: ready to build on 2026-11-25. `plans/README.md` names this file as the active slice. Nothing in it is built.

Plan 002 closed on 2026-11-24 and said the next plan was the smallest database-disk header, with enumeration and the Lua text screen staying behind it. The interview after that close did not stay that small. The notes are `plans/interview-log.md`, under "Proposals added 2026-11-25" and "Decided 2026-11-25". This draft is that cluster plus the publisher rule chosen the same day: the only slot that has completed a boot is not the one a publish drops.

Writing this down does not vend Lua or Jansson, does not change `Boot.efi`, and does not put bytes in the database image. The image stays the blank disk plan 002 recorded, hash `1b7af784841125887e688254229e282829e17245d0e47fe7280da9b68a30d9e1`, until the work below is built.

## 1. What is already built

The host list is `lua tests/boot-tui.lua`. It is not firmware. It reads ten slots, draws the framed mark, and does not start Base. `src/boot/slot-read.lua`, `src/boot/json-text.lua`, `src/boot/boot-screen.lua`, `src/boot/wordmark.lua`, and `src/boot/screen-geo.lua` are that stand-in. `ctest` runs `boot-tui`. Those files stay the behavior the firmware screen is measured against. They are not copied into the guest: the firmware console is CHAR16, sixteen foreground colors, eight background colors, and no ANSI.

`src/boot/x86-64/boot.c` still loads `\EFI\QueueOS\current\Base.efi` from the disk Boot itself came from. The comment in that file says the database disk is not read. `tests/README.md` says the same. `A-001-002` still requires the first 8 KiB of the database image to be zeros.

`tools/mkimage.c` is 985 lines. The cap is 1,000. This plan does not teach `mkimage.c` the slot format. A new host program writes the raw database. `mkimage` may exec that program. If the exec does not fit in the remaining lines, the image recipe calls the program from CMake and `mkimage` keeps creating the qcow2 from the raw file it is handed. The line count is checked before the edit. Spending the last fifteen lines on the format is how this file becomes the next one that cannot be opened.

## 2. The disk

No filesystem. The guest sees a linear disk. qcow2 stays host packaging.

Ten slots of 4,096 bytes start at byte 0. Slot `i` is at offset `(i - 1) * 4096`. Each slot is a JSON object at byte 0, with no leading whitespace, then a newline, then a line `sha256 ` and 64 lowercase hex digits of SHA-256 over the JSON bytes only, then a newline. The hash does not cover the rest of the slot, so a later boot-completion line does not change it and key order does not matter.

An empty slot is the two bytes `{}`, the newline, and the hash of those two bytes. That hash is `44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a`. The practical JSON limit is 4,023 bytes: 4,096 minus the newline, the `sha256 ` line, and its newline.

Classification, already implemented on the host:

- `{}` and a matching hash is empty.
- JSON, hash matches, `"OS"` is `QueueOS`: a candidate.
- JSON, hash missing or wrong: UNSAFE. Shown, not started.
- Not JSON: unreadable. An all-zero slot is unreadable, not empty.
- Hash matches, OS is something else, bytes are not `{}`: foreign.

Launchable means candidate, and one binary has role `base`, arch `x86-64`, an integer offset of at least 0, a length of at least 1, and a 64-hex sha256. `"file"` is a label. It is not a path. The arch token is the one this Boot was built for. There is no UEFI call that answers it.

The first 40,960 bytes are the slots. After that is the boot range, where Base and later pictures live. The database body starts at the end of the boot range and is empty in the first image. The body's start is one named constant, in one file, so it can move. The first setting is 8 MiB from the start of the disk. That number is not a format version. No second copy of it is written into readers, tests, or sample JSON. Boot trusts the offset in the slot, and it rejects an offset that lands in the slots or past the body start. The sample offset `8388608` inside `tests/boot-tui.lua` is a fixture. It is not this constant.

The first image carries one generation. The JSON is fixed text, not the build clock, so the qcow2 hash stays reproducible. Name `stub`, version `0`, published `2026-11-25T00:00:00Z`, OS `QueueOS`, one binary, role `base`, arch `x86-64`, file `Base.efi`. The bytes are the built `Base.efi`. Their offset is the first byte of the boot range, 40960. Their length and sha256 are computed from that file at image build. The other nine slots are empty. Nothing is written past the body start.

Pictures stay out. A thumbnail would be a blob with role `thumbnail`. It is legal and absent. It is not base64 in the JSON. UPX is out.

## 3. Where a completed boot is recorded

The interview said Base writes the outcome where the next Boot can read it, and left the location alone. This draft puts it in the spare bytes of the same slot, immediately after the hash line's newline:

```text
booted 2026-11-25T00:00:00Z
```

One ASCII line. Absent means this slot has not completed a boot. The line is not part of the hash. Rewriting the JSON to store it would make a good slot look UNSAFE, or would force Base to republish the generation. Explore shows the line as a status on the slot. It does not pretend the line is inside the JSON.

A completion counts only when the slot is a candidate and the hash matches. An UNSAFE slot with a `booted` line is still UNSAFE.

The success presumption in the interview is unchanged and still unfinished: Base has finished the initial database load, the buffer pools, and hardware enumeration, and is ready for a session. The stub Base does none of that. In this slice, Base writes the line when it has drawn `QueueOS Loaded`. That stamp is a stand-in. It is not the finished criterion. Boot tells Base which slot it started by setting the loaded image's load options to the ASCII `slot N` before `StartImage`. Base reads its own load options. A stub that was not started that way writes no line.

## 4. The publisher

The publisher is a host C program. It reads a raw database image and writes that same file. It does not open `images/` itself. A qcow2 is converted by the caller, and the caller converts back. Tests use a copy.

It reads all ten slots. It gathers the binaries those slots still name. An identical sha256 keeps one copy. Bytes a slot still names stay at that offset. New bytes go in a free gap inside the boot range, or at the end of what is already used there. They do not enter the slots and they do not enter the body. If the new bytes do not fit, the publish stops and the image is unchanged. A reorg is not implied. Reorg, when a later plan builds it, is requested on purpose and does not move the body.

The new slot is committed before any old slot is dropped. The only good generation is not overwritten in place as the way of publishing its replacement.

Which slot receives the new generation:

1. The lowest empty slot.
2. Otherwise the lowest unreadable, foreign, or UNSAFE slot.
3. Otherwise the oldest candidate by `published`. The same timestamp yields the higher slot index.
4. If that candidate is the only slot that has completed a boot, skip it and take the oldest of the rest by the same rule. Once any newer slot has also completed a boot, the older completion may be dropped like any other candidate.

A publish that has nowhere to go under those rules stops and leaves the image unchanged. The first image has nine empty slots, so the rule is quiet until the slots fill.

Binaries nobody names after the new slot is committed can be dropped on the next publish. This plan drops them only when the publish is already rewriting the set. It does not move the ones that remain.

## 5. Firmware Boot

Boot gains a pool allocator: `malloc` and `free` over `AllocatePool` and `FreePool`. Jansson and Lua are linked statically into `Boot.efi`. Nothing is loaded at runtime. Both are imported under `vendored/` with the upstream names, the upstream license, and a `VERSION` file. The trees are not reformatted to satisfy the line cap or the dash rule.

Jansson parses the ten slots. Lua draws the list, runs the timer, and pretty-prints Explore. The picture and the stack geometry already live in `wordmark.lua` and `screen-geo.lua`. The firmware script uses those rules. The drawing underneath is the UEFI text console: sharp box corners, block `U+2588`, light shade for the "ueue" letters if the glyph is present, and the VGA palette. The host screen's medium shade `U+2592` and its 256-color silver are not assumed. If the firmware font has no medium shade, the "ueue" letters use the light shade `U+2591` and the plan records the substitution. The red field stays.

Pointer input exists when the firmware reports a Simple Pointer or an Absolute Pointer. A serial console has neither, and the keyboard path is enough there. There is no SGR mouse on the guest.

SHA-256 in the guest is a small C file in the tree, shared with the publisher. The host list may keep calling `sha256sum`. The two hashes have to agree on `{}` and on the stub generation.

The disk Boot reads is the virtio-scsi disk whose product is `Base`. The ESP product stays `Boot`. Boot reads the slot region and the one Base range with EFI block I/O. `LoadImage` gets the Base bytes in a buffer. There is no FAT on that disk and no `fs0:` guess.

The list behavior is the one the host screen already runs. Ten slots, in slot order. Newest launchable candidate selected, ties to the lower slot, otherwise slot 1. Ten seconds on a launchable row, reset by a key or a pointer, paused on Explore, never fired on an empty, UNSAFE, unreadable, or foreign row. Launch starts that Base. The host tool's "print the record and return" behavior stays on the host tool.

The first guest proof can be a plain text list on the way to the Lua screen. The plain list does not remain as a second interface. The slice is done when the Lua screen is what the guest draws.

`Boot.efi` may grow. That was accepted in the interview. The source files still stop at 1,000 lines. `boot.c` stays the entry. The reader, the hasher, the allocator, and the screen shim are other files.

## 6. What this does to the tests

`A-001-002` says the database image is a blank 64 MiB disk and checks that the first 8 KiB are zeros. That sentence becomes false. The case becomes a check that the ten slots and the one Base are the image in section 2, that the body is zeros, and that the disk still has no MBR and no GPT. The old title stays in this paragraph so the change is visible. The manifest hash changes with the bytes. `A-001-001` keeps comparing the built files to the manifest.

`D-003-003` deletes `Base.efi` on the ESP and expects Boot to fail. After this plan, deleting that file does not test the fetch. The failure moves to the database copy: a generation whose Base bytes do not match, or whose Base is absent. Boot prints the existing failure sentence and does not start Base. `D-003-001` and `D-003-002` still mutilate the ESP Boot program. They stay.

The runs that expect `QueueOS loading...` before `QueueOS Loaded` stay on those sentences. The harness may still quit when the sentence arrives. It does not have to sit through the new ten-second list. A headless `run` does, and its deadline has to cover the list as well as the firmware progress bar and Base's countdown. The deadline is raised when a real boot shows the old one is short, not before.

`images/a-001-q35-base/vm.conf` will refresh the domain's database qcow2 when the domain is shut off, because the bytes changed. The plan does not start the domain, does not edit `OVMF_VARS.fd`, and does not add a `vm.conf` for `a-003`.

## 7. Outside this plan

- The reorg command. Publish stops when the bytes do not fit.
- The updater, the rescue package, and firmware boot order.
- The database engine, queues, and anything Base does after the Loaded stamp besides writing the one completion line.
- A finished definition of "completed a boot" beyond that stamp.
- Thumbnails, and any edit of a slot from the Explore view.
- Recovery zsh.
- A virt-manager guest for `a-003`.
- A commit.
- A new letter of the catalogue. This plan keeps A for the image, D for Boot, and E for Base.

The separate yes for marking this file ready was given on 2026-11-25. That yes is the status line. It is not a claim that any of the work has been done.

## 8. Order

1. The shared hasher and the publisher, with host tests: empty image, first publish, identical sha256 stored once, publish that does not fit leaves the file alone, and a full disk whose only completion is the oldest slot keeps that slot.
2. The image recipe writes section 2. `A-001-002` changes as section 6 says. `lua tests/boot-tui.lua` pointed at the new qcow2 shows `stub` and does not start Base.
3. The pool allocator, vendored Jansson, and Boot reading the `Base` disk. A guest boot starts `Base.efi` from the slot. Serial shows the loading sentence and then the Loaded sentence. The ESP path is gone.
4. Vendored Lua and the screen shim. The guest draws the same stack as the host list. Explore and the timer behave as section 5 says.
5. Base writes the completion line for the slot named in its load options. A second host publish against a copy of that image keeps the slot.

Each step leaves the tree bootable. Step 3 is the one that changes what a guest does. It does not land before step 2's image exists.

## 9. Done

- `ctest` passes, including `boot-tui` and the image checks.
- The publisher tests in section 8 pass.
- A headless boot of `a-001` reaches `QueueOS Loaded` with Base's bytes coming from the database disk.
- The host list, pointed at that qcow2, shows one launchable `stub` and nine empty slots.
- `tools/mkimage.c` is at or under 1,000 lines.
- No new Python, and no file outside `vendored/` violates the name or length rules.
