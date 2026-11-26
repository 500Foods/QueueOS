#!/usr/bin/env bash
# A-001 checks for images/a-001-q35-base. No QEMU.
# usage: check-images.sh --root DIR --build DIR [--check NAME]
# NAME selects one A-001 check. The default runs all five.

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck source=pe-read.sh
source "$script_dir/pe-read.sh"

DISK_BYTES=$((64 * 1024 * 1024))
ESP_TYPE=28732ac11ff8d211ba4b00a0c93ec93b
ROOT=
BUILD=
CHECK=all

die() {
    echo "$1" >&2
    exit 1
}

usage() {
    die "usage: check-images.sh --root DIR --build DIR [--check NAME]"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --root)
            [[ $# -ge 2 ]] || usage
            ROOT=$2
            shift 2
            ;;
        --build)
            [[ $# -ge 2 ]] || usage
            BUILD=$2
            shift 2
            ;;
        --check)
            [[ $# -ge 2 ]] || usage
            CHECK=$2
            shift 2
            ;;
        *)
            usage
            ;;
    esac
done
[[ -n $ROOT && -n $BUILD ]] || usage

conf_get() {
    local key=$1 file=$2 line trimmed k v
    while IFS= read -r line || [[ -n ${line:-} ]]; do
        trimmed=${line#"${line%%[![:space:]]*}"}
        trimmed=${trimmed%"${trimmed##*[![:space:]]}"}
        [[ -z $trimmed || $trimmed == \#* ]] && continue
        [[ $trimmed == *=* ]] || continue
        k=${trimmed%%=*}
        v=${trimmed#*=}
        k=${k%"${k##*[![:space:]]}"}
        k=${k#"${k%%[![:space:]]*}"}
        v=${v#"${v%%[![:space:]]*}"}
        v=${v%"${v##*[![:space:]]}"}
        if [[ $k == "$key" ]]; then
            printf '%s' "$v"
            return 0
        fi
    done < "$file"
    return 1
}

sha256_file() {
    local sum
    sum=$(sha256sum -- "$1") || return 1
    printf '%s' "${sum%% *}"
}

hex_at() {
    od -An -tx1 -N "$3" -j "$2" "$1" | tr -d '[:space:]'
}

declare -A MANIFEST
load_manifest() {
    local line digest name
    MANIFEST=()
    while IFS= read -r line || [[ -n ${line:-} ]]; do
        [[ -z ${line//[[:space:]]/} ]] && continue
        digest=${line%%  *}
        name=${line#*  }
        MANIFEST["$name"]=$digest
    done < "$1"
}

resolve_database() {
    local folder=$1 version
    version=$(conf_get database "$folder/image.conf") || die "image.conf has no database"
    if [[ $version == current ]]; then
        version=$(head -n 1 "$ROOT/images/database/current" | tr -d '[:space:]')
    fi
    [[ -n $version ]] || die "images/database/current has no version"
    printf '%s' "$ROOT/images/database/$version/QueueOS-Database.qcow2"
}

require_efi() {
    local path=$1
    if ! pe_read "$path"; then
        die "$(basename "$path") is not a relocatable PE32+ EFI application"
    fi
    if (( pe_machine != 0x8664 || pe_magic != 0x20b || pe_subsystem != 10 )); then
        die "$(basename "$path") is not a relocatable PE32+ EFI application"
    fi
    if (( (pe_characteristics & 0x0001) != 0 || pe_reloc_size == 0 )); then
        die "$(basename "$path") is not a relocatable PE32+ EFI application"
    fi
}

check_database() {
    local folder=$1 database=$2 link target info vsize raw
    link=$(readlink -f "$folder/QueueOS-Database.qcow2") || die "database symlink does not resolve to the version image"
    target=$(readlink -f "$database") || die "database symlink does not resolve to the version image"
    [[ $link == "$target" ]] || die "database symlink does not resolve to the version image"
    info=$(qemu-img info "$database") || die "qemu-img info failed"
    vsize=$(printf '%s\n' "$info" | sed -n 's/.*(\([0-9][0-9]*\) bytes).*/\1/p' | head -n 1)
    [[ $vsize == "$DISK_BYTES" ]] || die "database virtual size is ${vsize}"
    raw=$TMP/db.raw
    qemu-img convert -O raw "$database" "$raw"
    [[ $(stat -c '%s' "$raw") -eq $DISK_BYTES ]] || die "converted database is not 64 MiB"
    [[ $(hex_at "$raw" 510 2) != 55aa ]] || die "database has an MBR signature"
    [[ $(hex_at "$raw" 512 8) != 4546492050415254 ]] || die "database has a GPT signature"
    [[ -x $BUILD/db-image ]] || die "db-image is missing"
    [[ -f $BUILD/efi/Base.efi ]] || die "Base.efi is missing"
    "$BUILD/db-image" --check "$raw" "$BUILD/efi/Base.efi"
    echo "image-database ok"
}

check_esp() {
    local raw=$1 index off type used used_off start start_hi end end_hi part label kind
    [[ $(hex_at "$raw" 510 2) == 55aa ]] || die "UEFI image has no protective MBR signature"
    [[ $(hex_at "$raw" 450 1) == ee ]] || die "protective MBR type is not 0xEE"
    [[ $(hex_at "$raw" 512 8) == 4546492050415254 ]] || die "UEFI image has no GPT header"
    used=0
    used_off=0
    for ((index = 0; index < 128; index++)); do
        off=$((1024 + index * 128))
        type=$(hex_at "$raw" "$off" 16)
        if [[ $type != 00000000000000000000000000000000 ]]; then
            used=$((used + 1))
            used_off=$off
        fi
    done
    [[ $used -eq 1 ]] || die "GPT has $used partitions"
    [[ $(hex_at "$raw" "$used_off" 16) == "$ESP_TYPE" ]] || die "partition type is not an ESP"
    start=$(pe_u32 "$raw" $((used_off + 32))) || die "ESP starts at LBA ?"
    start_hi=$(pe_u32 "$raw" $((used_off + 36))) || die "ESP starts at LBA ?"
    [[ $start_hi -eq 0 && $start -eq 2048 ]] || die "ESP starts at LBA $start"
    end=$(pe_u32 "$raw" $((used_off + 40))) || die "ESP has no sectors"
    end_hi=$(pe_u32 "$raw" $((used_off + 44))) || die "ESP has no sectors"
    [[ $end_hi -eq 0 && $end -gt $start ]] || die "ESP has no sectors"
    part=$((start * 512))
    label=$(dd if="$raw" bs=1 skip=$((part + 71)) count=11 status=none)
    [[ $label == 'QOS-ESP    ' ]] || die "ESP label is ${label}"
    kind=$(dd if="$raw" bs=1 skip=$((part + 82)) count=8 status=none)
    [[ $kind == 'FAT32   ' ]] || die "ESP filesystem type is ${kind}"
    echo "image-esp ok"
}

check_checksum() {
    local folder=$1 extracted=$2 name actual
    load_manifest "$folder/manifest.sha256"
    [[ -n ${MANIFEST[QueueOS-UEFI.qcow2]+x} ]] || die "manifest has no UEFI image"
    actual=$(sha256_file "$folder/QueueOS-UEFI.qcow2")
    [[ ${MANIFEST[QueueOS-UEFI.qcow2]} == "$actual" ]] || die "UEFI qcow2 hash does not match the manifest"
    for name in \
        EFI/BOOT/BOOTX64.EFI \
        EFI/QueueOS/current/Boot.efi \
        EFI/QueueOS/current/Base.efi \
        EFI/QueueOS/rescue/Boot.efi
    do
        [[ -n ${MANIFEST[$name]+x} ]] || die "manifest has no $name"
        actual=$(sha256_file "$extracted/$name")
        [[ ${MANIFEST[$name]} == "$actual" ]] || die "$name hash does not match the manifest"
    done
    echo "image-checksum ok"
}

check_payloads() {
    local extracted=$1 boot current base rescue marker keep
    boot=$extracted/EFI/BOOT/BOOTX64.EFI
    current=$extracted/EFI/QueueOS/current/Boot.efi
    base=$extracted/EFI/QueueOS/current/Base.efi
    rescue=$extracted/EFI/QueueOS/rescue/Boot.efi
    marker=$extracted/EFI/QueueOS/rescue/RESCUE
    keep=$extracted/EFI/QueueOS/previous/.keep
    cmp -s "$boot" "$current" || die "BOOTX64.EFI is not a copy of current/Boot.efi"
    cmp -s "$rescue" "$current" || die "rescue/Boot.efi is not the current Boot stub"
    printf 'rescue\n' | cmp -s - "$marker" || die "rescue marker is not rescue\\n"
    [[ -f $keep && ! -s $keep ]] || die "previous/.keep is missing or not empty"
    require_efi "$boot"
    require_efi "$current"
    require_efi "$base"
    echo "image-payloads ok"
}

check_base_size() {
    local base=$1/EFI/QueueOS/current/Base.efi
    [[ -f $base ]] || die "current/Base.efi is missing"
    [[ $(stat -c '%s' "$base") -lt $((500 * 1024)) ]] || die "Base.efi is $(stat -c '%s' "$base") bytes"
    echo "image-base-size ok"
}

folder=$ROOT/images/a-001-q35-base
database=$(resolve_database "$folder")
[[ -f $database ]] || die "missing $database"
[[ -f $folder/QueueOS-UEFI.qcow2 ]] || die "missing UEFI qcow2"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

prepare_uefi() {
    qemu-img convert -O raw "$folder/QueueOS-UEFI.qcow2" "$TMP/uefi.raw"
    mkdir -p "$TMP/tree"
    mcopy -i "$TMP/uefi.raw@@1M" -s "::/" "$TMP/tree"
}

case $CHECK in
    all)
        check_database "$folder" "$database"
        prepare_uefi
        check_esp "$TMP/uefi.raw"
        check_checksum "$folder" "$TMP/tree"
        check_payloads "$TMP/tree"
        check_base_size "$TMP/tree"
        ;;
    image-database)
        check_database "$folder" "$database"
        ;;
    image-esp)
        prepare_uefi
        check_esp "$TMP/uefi.raw"
        ;;
    image-checksum)
        prepare_uefi
        check_checksum "$folder" "$TMP/tree"
        ;;
    image-payloads)
        prepare_uefi
        check_payloads "$TMP/tree"
        ;;
    image-base-size)
        prepare_uefi
        check_base_size "$TMP/tree"
        ;;
    *)
        die "unknown image check: $CHECK"
        ;;
esac
exit 0
