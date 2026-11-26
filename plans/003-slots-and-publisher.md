# 003 — Slots and the publisher

Status: closed on 2026-11-25. The five steps are built. No following plan is ready. Plan 004 is not opened.

This file was opened ready to build on 2026-11-25. `plans/README.md` named it as the active slice. Nothing in it is built.

Steps 1 and 2 of section 8 are built. The sections below stay the original ask. A sentence this work made false has the correction next to it. Steps 3, 4, and 5 are not built. `Boot.efi` still loads Base from the ESP. This file is still the active slice.

Step 3 is built. Steps 4 and 5 are not. `Boot.efi` reads the virtio-scsi disk whose product is `Base` and starts the Base bytes from the chosen slot. Jansson 2.15.1 is vendored. Lua is not. This file is still the active slice.

Lua 5.5.1 is imported under `vendored/lua/`. `Boot.efi` does not link it yet, and the guest does not draw the list. The host test `lua-floor` runs `pcall`, `math.floor`, `math.max`, `math.min`, `2 ^ 53`, and `string.format` for `%.0f`, `%04x`, and `%2d`. Trig, logs, and `exp` return 0. The boot scripts do not call them.

Step 4 is built. `Boot.efi` links that Lua and draws the list. The loading sentence is still first. The medium shade `U+2592` is drawn as the light shade `U+2591`. Hover is yellow on the red field, because this firmware font has no underline. A relative Simple Pointer is not read. The absolute pointer is. Step 5 is not built. Base still does not write the completion line. This file is still the active slice.

Both pointers are read now. Reset is called before GetState. The list rows are black and the selected row is a red bar. The mark stays on the red field. The sentence above is the step 4 record.

Step 5 is built. Base reads the load options. After `QueueOS Loaded` is drawn, and before that sentence is written to the serial line, it writes `booted STAMP` into the named slot. The stamp is the firmware clock, `YYYY-MM-DDTHH:MM:SSZ`. GetTime is not asked to convert a timezone. A missing slot name, a failed clock, a hash that does not match, or a failed write leaves the disk alone, and the Loaded sentence is still printed. `results/000-017` is `E-001-001`: the list, then Loaded. Slot 1 of that work database gained `booted 2026-11-25T15:38:39Z`. The hash line was still `sha256 065486e9db58d99b687d3312c1c0fbe19c42ea2b920cc90f14aa9839c18e95ed`. `results/000-018` is `D-003-003`: the failure sentence, Loaded absent, and no `booted` line. The next run id is `000-019`. The five steps of section 8 are built. This file is still the active slice. Plan 004 is not opened.

The list drawn after that boot was adjusted. Clearing the console on every timer tick flickered while the selected row could launch, which on the stub image is the first row. Later paints rewrite the list and the buttons and leave the mark. Three blank rows sit between the mark and the list, and three between the list and the buttons. The buttons are boxes. Rows are drawn in boot priority, newest launchable first. The line has Pri, Name, Version, Release, Last Successful Boot, and Tag. Release is the published stamp as `YYYY-Mon-DD`. A `booted` line is `YYYY-Mon-DD (Www) HH:MM`, and the weekday is computed. Tag is the JSON string when the slot has one. The stub generation is still name `stub`, version `0`. The ten-second timer still launches. The number in `slot N` is still the disk slot. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `b982504e62b92adccadb90c271f6b9e4c4021246c645a94e395d6c0f3de6cb4d`. The shut-off `a-001` copies were refreshed. The domain was not started. `OVMF_VARS.fd` was not edited. `results/000-019` is `E-001-001` after that screen change: `QueueOS loading...` and then `QueueOS Loaded`. The database qcow2 was not rewritten by that guest. The next run id is `000-020`. The flicker, the pointer, the boxes, and the columns were not watched on a graphical console. Screenshot cases were not re-run.

The field outside the mark, the list frame, and the buttons is now the light shade `U+2591`. The mark's "ueue" is still the medium shade in the picture, drawn as that same light shade, and the inside of the mark stays a solid field so the letters stay readable. The list has a frame the width of the mark. When all ten rows fit, that frame is also the mark's height. The Tag column is gone. A named foreign or unsafe row keeps its name on the line. The firmware text caret stays hidden. It is an underscore. The pointer is the glyph `U+25BA`, drawn when a pointer device is present. `results/000-020` is `E-001-001` after that change. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `1d852e245fc09510dc80c66bde3a2ed1797e4e1ac7107e81792a486fa8ba4bb3`. The shut-off `a-001` UEFI copy was refreshed. The database copy already matched and was left as it was. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-021`. The graphical look was not watched. Screenshot cases were not re-run.

The user watched that screen. The right edge was a solid red column, the last cell left unpainted so a full row does not scroll. The left column is now the same. The list border and the button boxes are solid spaces. The line-drawing characters are gone from those. The logo keeps its frame. The list text is one column in from the border, and a blank row sits between the titles and the slots. A full list is one row taller than the mark. Space is under Explore and Enter is under Launch, in the field color. The Space key explores. The pointer glyph was in the center and did not move. The wait is now WaitForEvent on a relative timer, so a pointer driver's timer can run. Stall does not. `results/000-021` is `E-001-001` after that change. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `df835417b1900b3f2b94802c2db24633191e5fc77b076b2398120f438348fb6b`. The shut-off `a-001` UEFI copy was refreshed. mkimage also refreshed the database copy, and that copy was put back. Its hash is `dd0ff34a3a48c3734a4d4e230e72ed8ffefa4ed8942f71bfc4ec8440ab585736`. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-022`. The new layout and whether the pointer moves were not watched. Screenshot cases were not re-run.

The user watched that screen. The pointer still did not move. The logo border is solid spaces. The list border is black spaces, and the list is ten columns wider than the mark. A sha256 line in Explore is 83 columns and fits in that width. One red space border holds Explore and Launch, with six columns between them. `results/000-022` is `E-001-001` after that change. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `509e17a2f87db9720d796843fbbfa09a6263ff2d6dff5c1607b127f5ac7452f2`. The shut-off `a-001` UEFI copy was refreshed. mkimage refreshed the database copy, and that copy was put back. Its hash is still `dd0ff34a3a48c3734a4d4e230e72ed8ffefa4ed8942f71bfc4ec8440ab585736`. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-023`. The new layout was not watched. Screenshot cases were not re-run.

The user watched that layout. ueue was light gray, and the button box was full of the shade. ueue is now white. The button box is solid red, and the shade stays outside it. The pointer still had no Absolute Pointer from this firmware. virt-manager delivers the grabbed pointer to the USB tablet, so Boot now reads that tablet through USB I/O. The first image treated the copied config descriptor as a pointer. `results/000-023` is `E-001-001` and it failed: a general-protection fault after `QueueOS loading...`. The call now fills a caller buffer. A host QEMU then moved the tablet to the origin and the yellow glyph left the center of a 1280x800 framebuffer for the corner. `results/000-024` is `E-001-001` after that fix. Host `ctest` was eight passes, including `tablet-report`. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `e4e97bfa31c744429d90edc59e0f4861054af54519510e3a816cda85cebf2840`. The shut-off `a-001` UEFI copy was refreshed. mkimage refreshed the database copy, and that copy was put back. Its hash is still `dd0ff34a3a48c3734a4d4e230e72ed8ffefa4ed8942f71bfc4ec8440ab585736`. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-025`. The new layout was not watched in virt-manager. Screenshot cases were not re-run.

The user confirmed the mouse on that image. The pointer is now the full block `U+2588`, still yellow on black. Each button has its own solid red box. Four columns of the shade sit between those outer edges. The button says Launch. The whole seconds sit on the middle row of the gap under the list, in white, and that line is empty when the timer is not running. A pointer move restores the cell it left and draws the block. It repaints a list row or a button word only when the hotspot under it changes. The first field copies one `U+2591` cell and stamps it across the interior when the firmware cell is 8 by 19. Otherwise it writes one row at a time from C. Columns 1 and the last column stay solid. `results/000-025` is `E-001-001` and it passed. Host `ctest` was eight passes. The database qcow2 hash is still `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. The UEFI qcow2 is now `c3d8aa4b274c8706316b38bfab4bc11807c1dbb6531058c5ed21a203b587f56c`. The shut-off `a-001` UEFI copy was refreshed. mkimage refreshed the database copy, and that copy was put back. Its hash is still `dd0ff34a3a48c3734a4d4e230e72ed8ffefa4ed8942f71bfc4ec8440ab585736`. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-026`. virt-manager was not watched. Screenshot cases were not re-run.

That id was used. The user watched that screen. A block was left in the top-left cell. A glyph is written only when the cursor move succeeds. The sample cell is cleared before the shade, and the corner is a space and then the shade after either fill. The left solid column was put back as the shade when the pointer had not remembered the cell. A move on that column now remembers a space. Both solid margins are drawn again after a full paint and after a pointer move, and then the block. The buttons read Explore Image and Launch Image. One extra space sits inside the border on each side of the caption. Eight columns of the shade sit between the outer edges when the row has room, and that gap is never under four. Hover and a click cover the whole box, border included, and they stop at the gap. Back reads Back to Images and uses that same box. `countdown` is a JSON integer. Under 10, or a value that is not a number, does not arm and does not advance. Over 30 counts as 30. Selecting an image starts that image's count. Movement starts it over. The database holds two stub generations and one copy of the Base bytes. Slot 1 is name `stub`, version `1.0.1`, published `2026-11-25T00:00:01Z`, countdown 10. Slot 2 is name `stub`, version `1.0.0`, published `2026-11-25T00:00:00Z`, countdown 30. Both release dates show 2026-Nov-25. `results/000-026` is `E-001-001` and it passed. Host `ctest` was eight passes. The database qcow2 hash is now `6d00aaece1af9d9737af226bb2fb1dcead34772b3d2680cf9b3677e80f85aa3e`. The UEFI qcow2 is now `32b0ac75891b9e549fd10f2d6483a000a6bf51ee04441ff284a859edec915a4e`. That build found the `a-001` guest running and left both copies. The domain was shut off afterwards. A later image build refreshed both copies to those hashes. The domain was not started. `OVMF_VARS.fd` was not edited. The next run id is `000-027`. virt-manager was not watched after this change. Screenshot cases were not re-run.

That id was used. Back names Esc, in the same field color as Space and Enter. Explore draws a one-column scrollbar on the right edge of the JSON frame. The up and down arrows are bright red on black. The thumb is the red bar on the black track, and it marks the visible window. A click on an arrow moves one line. A click on the track moves a window. The QEMU tablet wheel is a signed byte. Wheel-up is negative and moves toward the start, three lines a notch, and one report applies at most four notches. The host list already used the terminal wheel. `results/000-027` is `E-001-001` and it passed. Host `ctest` was eight passes. The database qcow2 hash is still `6d00aaece1af9d9737af226bb2fb1dcead34772b3d2680cf9b3677e80f85aa3e`. The UEFI qcow2 is now `c748825771428c6e43283ddb831b604d5c681acdfb5aa236838da9939267d975`. The shut-off `a-001` copies were refreshed to those hashes. The domain was not started. `OVMF_VARS.fd` was not edited. The picture was not watched. Screenshot cases were not re-run. The five steps are built. This file is closed. No following plan is ready. Plan 004 is not opened. The next run id is `000-028`.

Plan 002 closed on 2026-11-24 and said the next plan was the smallest database-disk header, with enumeration and the Lua text screen staying behind it. The interview after that close did not stay that small. The notes are `plans/interview-log.md`, under "Proposals added 2026-11-25" and "Decided 2026-11-25". This draft is that cluster plus the publisher rule chosen the same day: the only slot that has completed a boot is not the one a publish drops.

Writing this down does not vend Lua or Jansson, does not change `Boot.efi`, and does not put bytes in the database image. The image stays the blank disk plan 002 recorded, hash `1b7af784841125887e688254229e282829e17245d0e47fe7280da9b68a30d9e1`, until the work below is built.

Step 2 did put the section 2 generation on that disk. The hash above is the blank disk this file was written against. Two image builds of the stub disk produced `2e716b95531bf7b7bf2b3ed21a5a61f942a453779fb14dfc6a7494d06bbb921c`. Lua and Jansson are still not vendored. `Boot.efi` is unchanged.

Step 3 vendored Jansson 2.15.1 under `vendored/jansson/` and changed `Boot.efi`. Lua is still not vendored. The database qcow2 hash is still the stub hash above. The UEFI qcow2 was rewritten because `Boot.efi` changed.

Lua 5.5.1 is now under `vendored/lua/`, with `doc/readme.html` for the license the upstream README names. `lua.c` and `luac.c` are not compiled. The database hash is unchanged.

Step 5 changed `Base.efi`, so the stub slot and the database qcow2 changed. The previous stub Base was length 5632, sha256 `d98adbbfce0de152376cc2ee282841def911041bfb4138dbb3ab30df4ad5235d`, and the database qcow2 was `2e716b95531bf7b7bf2b3ed21a5a61f942a453779fb14dfc6a7494d06bbb921c`. The new Base is length 30720, sha256 `a8644f770d17c46aa98781d44ea87c441a9ab03e6f5d6061155095004056b594`. The slot JSON hash is `065486e9db58d99b687d3312c1c0fbe19c42ea2b920cc90f14aa9839c18e95ed`. Two image builds produced database qcow2 `30cb0b3cd89a4bdc166459e4fe73162a0496d270cba5ee44979eaf3118ef3dbd`. `Boot.efi` also changed, because the disk find, read, and write moved to `src/arch/x86-64/efi/disk-io.c`. Base.efi is still copied onto the ESP, so the UEFI qcow2 changed too. Its hash is `bf6a55e2fde8787b0b9544b0f2ff33340ba48e3e328be8e69dc42170b66dd3be`. Plan 002's UEFI hash `d9261692d561b8ea7bf29a710e83353eb140ae64aa7625825133cfff139be84b` stays the earlier record. The shut-off `a-001` copies were refreshed. The domain was not started.

## 1. What is already built

The host list is `lua tests/boot-tui.lua`. It is not firmware. It reads ten slots, draws the framed mark, and does not start Base. `src/boot/slot-read.lua`, `src/boot/json-text.lua`, `src/boot/boot-screen.lua`, `src/boot/wordmark.lua`, and `src/boot/screen-geo.lua` are that stand-in. `ctest` runs `boot-tui`. Those files stay the behavior the firmware screen is measured against. They are not copied into the guest: the firmware console is CHAR16, sixteen foreground colors, eight background colors, and no ANSI.

`src/boot/x86-64/boot.c` still loads `\EFI\QueueOS\current\Base.efi` from the disk Boot itself came from. The comment in that file says the database disk is not read. `tests/README.md` says the same. `A-001-002` still requires the first 8 KiB of the database image to be zeros.

That load path is still the firmware. `A-001-002` no longer requires those zeros. Its title is "Database image carries the stub generation" and its check is `image-database`. The old title stays in section 6. `tests/README.md` now says the database image carries the stub generation, and it still says Boot loads Base from the ESP.

That ESP load is gone. `boot.c` binds the pool, reads the Base disk, and passes `LoadImage` a buffer. The child's load options are the ASCII text `slot N` and a trailing NUL. Base does not read them yet. `tests/README.md` says the same.

Base reads them. The completion line is the step 5 paragraph above. `tests/README.md` says the same.

`tools/mkimage.c` is 985 lines. The cap is 1,000. This plan does not teach `mkimage.c` the slot format. A new host program writes the raw database. `mkimage` may exec that program. If the exec does not fit in the remaining lines, the image recipe calls the program from CMake and `mkimage` keeps creating the qcow2 from the raw file it is handed. The line count is checked before the edit. Spending the last fifteen lines on the format is how this file becomes the next one that cannot be opened.

The file is 988 lines. It takes `--database-raw` and only converts that file to qcow2. `tools/db-image.c` writes the raw disk. CMake runs `db-image`, then `mkimage`. The slot format is not in `mkimage.c`.

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

Jansson parses the ten slots. Lua draws the list, runs the timer, and pretty-prints Explore. The picture and the stack geometry already live in `wordmark.lua` and `screen-geo.lua`. The firmware script uses those rules. The drawing underneath is the UEFI text console: sharp box corners, block `U+2588`, light shade for the "ueue" letters if the glyph is present, and the VGA palette. The host screen's medium shade `U+2592` and its 256-color silver are not assumed. If the firmware font has no medium shade, the "ueue" letters use the light shade `U+2591` and the plan records the substitution. The red field stays. The mark and the gaps stay on that field. The list rows are black, and the selected row is white on red.

Pointer input exists when the firmware reports a Simple Pointer or an Absolute Pointer. A serial console has neither, and the keyboard path is enough there. There is no SGR mouse on the guest.

SHA-256 in the guest is a small C file in the tree, shared with the publisher. The host list may keep calling `sha256sum`. The two hashes have to agree on `{}` and on the stub generation.

The disk Boot reads is the virtio-scsi disk whose product is `Base`. The ESP product stays `Boot`. Boot reads the slot region and the one Base range with EFI block I/O. `LoadImage` gets the Base bytes in a buffer. There is no FAT on that disk and no `fs0:` guess.

The list behavior is the one the host screen already runs. Ten slots, in slot order. The rows are now drawn in boot priority. Newest launchable first, an equal published stamp keeps the lower slot, and the rows that cannot launch follow in slot order. The selected value is still the disk slot Boot names in `slot N`. Newest launchable candidate selected, ties to the lower slot, otherwise slot 1. Ten seconds on a launchable row, reset by a key or a pointer, paused on Explore, never fired on an empty, UNSAFE, unreadable, or foreign row. Launch starts that Base. The host tool's "print the record and return" behavior stays on the host tool.

The first guest proof can be a plain text list on the way to the Lua screen. The plain list does not remain as a second interface. The slice is done when the Lua screen is what the guest draws.

`Boot.efi` may grow. That was accepted in the interview. The source files still stop at 1,000 lines. `boot.c` stays the entry. The reader, the hasher, the allocator, and the screen shim are other files.

## 6. What this does to the tests

`A-001-002` says the database image is a blank 64 MiB disk and checks that the first 8 KiB are zeros. That sentence becomes false. The case becomes a check that the ten slots and the one Base are the image in section 2, that the body is zeros, and that the disk still has no MBR and no GPT. The old title stays in this paragraph so the change is visible. The manifest hash changes with the bytes. `A-001-001` keeps comparing the built files to the manifest.

That change is in the tree. `image-a001` passed, including the UEFI manifest comparison. The database manifest is the stub hash recorded under the status line. The UEFI qcow2 was left in place because its bytes already matched. The shut-off `a-001` guest copy of the database qcow2 was refreshed. The domain was not started.

`D-003-003` deletes `Base.efi` on the ESP and expects Boot to fail. After this plan, deleting that file does not test the fetch. The failure moves to the database copy: a generation whose Base bytes do not match, or whose Base is absent. Boot prints the existing failure sentence and does not start Base. `D-003-001` and `D-003-002` still mutilate the ESP Boot program. They stay.

That move is in the tree. `D-003-003` is `mutate=corrupt-base-bytes`. The work copy of the database qcow2 has one byte flipped at offset 40960, the first byte of the stub Base. `results/000-012` passed: the failure sentence is present and `QueueOS Loaded` is absent. `images/` was not written.

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

Steps 1 and 2 are done. Host `ctest` was five passes: `sentences`, `boot-tui`, `publisher`, `efi-headers`, and `image-a001`. The publisher cases are the empty-object hash, a first publish on a zero image, a first publish on ten empty slots, one stored copy of an identical hash, a publish that does not fit, the only completion keeping the oldest slot, and an UNSAFE slot with a `booted` line still being replaced. `lua tests/boot-tui.lua` on `images/database/0001/QueueOS-Database.qcow2`, with stdout not a terminal, printed slot 1 as `stub  0  2026-11-25T00:00:00Z` and slots 2 through 10 as `empty`, and it returned without starting Base. The built `Base.efi` is at offset 40960. From the body start through the end of the 64 MiB disk, the bytes are zero. A second image build produced the same database qcow2 hash.

Step 3 is done. Host `ctest` was six passes. The new one is `slot-json`: empty, unreadable, the stub candidate, a bad hash, a foreign OS, newest-wins with the lower index on a tie, the body range, and the empty-string digest. `results/000-010` is the first guest of this Boot. It drew the loading sentence and the countdown, then a general-protection fault. Disk Info begins with a 16-byte interface GUID, and the call used that GUID as the inquiry function. The struct now keeps the GUID. `results/000-011` is `E-001-001`: `QueueOS loading...` and then `QueueOS Loaded`, with Base's bytes taken from the database disk. The database qcow2 was not rewritten. The next run id is `000-013`.

`results/000-013` is `E-001-001` again, after `vsnprintf` learned a real `%f` and width padding. That file is linked into `Boot.efi`. The guest still reached `QueueOS Loaded`. The database qcow2 was not rewritten. The next run id is `000-014`.

Step 4 is done. Lua 5.5.1 is linked into `Boot.efi`. The scripts are embedded. Nothing is read from the ESP at runtime. The guest draws the framed mark, the ten slots, and Explore and Launch. The timer still launches the newest launchable slot when nobody presses a key. `results/000-014` drew the loading sentence and then a general-protection fault inside `string.format`. Uninitialized statics use the ELF `.bss` address, and objcopy had placed `.reloc` at that same address, so the ctype table was relocation bytes. Boot's objcopy now keeps `.bss`. The loader zeros it. `results/000-015` is `E-001-001`: the list, then `QueueOS Loaded`. `results/000-016` is `D-003-003`: the list, then the failure sentence, and `QueueOS Loaded` is absent. The database qcow2 was not rewritten. Host `ctest` was seven passes, including `lua-floor`. The next run id is `000-017`. Step 5 is not built.

Step 5 is done. `queueos_slot_mark_booted` writes the `booted` line the publisher already trusts. The JSON and the hash line stay as they are. The keep-oldest host test calls that function, so a second publish on a full image keeps the only completed slot. A flipped hash, and a non-zero tail that is not a booted line, are refused and the slot bytes stay. Base links the pool, so its objcopy keeps `.bss`. `.reloc` does not share that address. BootFail still has no `.bss`. Host `ctest` was seven passes. The guest proof and the new image hashes are in the status paragraphs. The next run id is `000-019`. That id was used. `results/000-019` is `E-001-001` after the list change in the status paragraphs. The next run id is `000-020`. That id was used. `results/000-020` is `E-001-001` after the shade field, the list frame, and the pointer glyph. The next run id is `000-021`. That id was used. `results/000-021` is `E-001-001` after the solid edge columns, the blank title row, and the pointer wait. The next run id is `000-022`. That id was used. `results/000-022` is `E-001-001` after the wider black list and the red button border. The next run id is `000-023`. That id was used. `results/000-023` is `E-001-001` and it failed: a general-protection fault after the loading line, from reading the USB config descriptor as a pointer. The next run id is `000-024`. That id was used. `results/000-024` is `E-001-001` after the tablet read was fixed. The next run id is `000-025`. That id was used. `results/000-025` is `E-001-001` after the solid pointer, the separate buttons, and the countdown line. The next run id is `000-026`. That id was used. `results/000-026` is `E-001-001` after the corner clear, the solid margins, the wider buttons, and the two stub generations. The next run id is `000-027`. That id was used. `results/000-027` is `E-001-001` after Esc on Back and the JSON scrollbar. This slice is closed. The next run id is `000-028`.

## 9. Done

- `ctest` passes, including `boot-tui` and the image checks.
- The publisher tests in section 8 pass.
- A headless boot of `a-001` reaches `QueueOS Loaded` with Base's bytes coming from the database disk.
- The host list, pointed at that qcow2, shows one launchable `stub` and nine empty slots. That was the one-generation image. The image now has two launchable `stub` rows, versions `1.0.1` and `1.0.0`, and eight empty slots.
- `tools/mkimage.c` is at or under 1,000 lines.
- No new Python, and no file outside `vendored/` violates the name or length rules.
