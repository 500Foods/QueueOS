# Work-copy ESP mutations. Sourced by run.sh.
# images/ is not opened for writing. The database copy is not touched.
# losetup, qemu-nbd, and kpartx need privileges, so they are not the path.
# mtools uses the same @@1M offset as mkimage, and only after the first
# GPT partition is shown to start at 1 MiB. A different start fails the
# case instead of teaching this script a second partition layout.

# Assemble a little-endian integer from GPT bytes. Host endian does not matter.
le_uint() {
    local file=$1 skip=$2 width=$3
    local acc=0 place=1 count=0 b
    for b in $(od -An -tu1 -N "$width" -j "$skip" -- "$file"); do
        acc=$((acc + b * place))
        count=$((count + 1))
        if (( count < width )); then
            place=$((place * 256))
        fi
    done
    printf '%s' "$acc"
    (( count == width ))
}

# 512-byte sectors: the EFI PART signature lives at byte 512, and LBA
# 2048 is 1 MiB. Any other first-partition start is a failed case.
esp_starts_at_1m() {
    local raw=$1
    local sig entry_lba entry_size start_off start_lba
    sig=$(dd if="$raw" bs=1 skip=512 count=8 status=none 2>/dev/null || true)
    [[ $sig == "EFI PART" ]] || return 1
    entry_lba=$(le_uint "$raw" 584 8) || return 1
    entry_size=$(le_uint "$raw" 596 4) || return 1
    (( entry_lba >= 1 && entry_size >= 48 )) || return 1
    start_off=$((entry_lba * 512 + 32))
    start_lba=$(le_uint "$raw" "$start_off" 8) || return 1
    (( start_lba == 2048 ))
}

apply_mutation() {
    local mode=${1:-none}
    local work raw spec next
    if [[ $mode == none ]]; then
        return 0
    fi
    work=$GUEST_DIR/work/QueueOS-UEFI.qcow2
    raw=$GUEST_DIR/work/uefi.raw
    next=$GUEST_DIR/work/uefi-mutated.qcow2
    if [[ ! -f $work ]]; then
        echo "work UEFI image is missing" >&2
        return 1
    fi
    if ! qemu-img convert -O raw -- "$work" "$raw"; then
        echo "could not read the work UEFI image" >&2
        return 1
    fi
    if ! esp_starts_at_1m "$raw"; then
        echo "first GPT partition does not start at 1 MiB" >&2
        return 1
    fi
    spec=${raw}@@1M
    case $mode in
        delete-bootx64)
            if ! mdel -i "$spec" ::/EFI/BOOT/BOOTX64.EFI; then
                echo "could not delete EFI/BOOT/BOOTX64.EFI" >&2
                return 1
            fi
            ;;
        delete-base)
            if ! mdel -i "$spec" ::/EFI/QueueOS/current/Base.efi; then
                echo "could not delete EFI/QueueOS/current/Base.efi" >&2
                return 1
            fi
            ;;
        replace-boot-fail)
            if [[ ! -f $BUILD/efi/BootFail.efi ]]; then
                echo "missing BootFail.efi" >&2
                return 1
            fi
            if ! mcopy -D o -D O -i "$spec" "$BUILD/efi/BootFail.efi" ::/EFI/BOOT/BOOTX64.EFI; then
                echo "could not replace EFI/BOOT/BOOTX64.EFI" >&2
                return 1
            fi
            if ! mcopy -D o -D O -i "$spec" "$BUILD/efi/BootFail.efi" ::/EFI/QueueOS/current/Boot.efi; then
                echo "could not replace EFI/QueueOS/current/Boot.efi" >&2
                return 1
            fi
            ;;
        *)
            echo "unknown mutate: $mode" >&2
            return 1
            ;;
    esac
    if ! qemu-img convert -O qcow2 -- "$raw" "$next"; then
        echo "could not write the mutated work image" >&2
        return 1
    fi
    if ! mv -f -- "$next" "$work"; then
        echo "could not replace the work UEFI image" >&2
        return 1
    fi
    rm -f -- "$raw"
}
