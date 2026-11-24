#!/usr/bin/env bash
# Check a built stub is a PE32+ EFI application.
# Machine is AMD64 (0x8664) and the subsystem is EFI application (10).
# The second argument is Base.efi and must stay under the 500 KiB tripwire.
# This does not boot anything.
#
# usage: check-efi.sh Boot.efi Base.efi BootFail.efi

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck source=pe-read.sh
source "$script_dir/pe-read.sh"

if [[ $# -ne 3 ]]; then
    echo "usage: check-efi.sh Boot.efi Base.efi BootFail.efi" >&2
    exit 1
fi

boot=$1
base=$2
fail=$3

for path in "$boot" "$base" "$fail"; do
    if ! pe_read "$path"; then
        echo "$path: ${pe_error}" >&2
        exit 1
    fi
    if (( pe_machine != 0x8664 || pe_magic != 0x20b || pe_subsystem != 10 )); then
        printf '%s: machine=%#x magic=%#x subsystem=%s\n' \
            "$path" "$pe_machine" "$pe_magic" "$pe_subsystem" >&2
        exit 1
    fi
    if (( (pe_characteristics & 0x0001) != 0 || pe_reloc_size == 0 )); then
        printf '%s: relocations stripped or empty (characteristics=%#x reloc=%#x+%s)\n' \
            "$path" "$pe_characteristics" "$pe_reloc_rva" "$pe_reloc_size" >&2
        exit 1
    fi
    printf '%s: pe32+ efi-app %s bytes, reloc %s bytes\n' \
        "$path" "$pe_size" "$pe_reloc_size"
done

base_size=$(stat -c '%s' "$base")
if (( base_size >= 500 * 1024 )); then
    echo "$base: $base_size bytes is not under 500 KiB" >&2
    exit 1
fi
exit 0
