# Shared PE32+ field reader for the host checks.
# Sourced by check-efi.sh and check-images.sh. Running it directly does nothing useful.

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "source tests/host/pe-read.sh from a check" >&2
    exit 1
fi

# pe_read PATH fills pe_machine, pe_magic, pe_subsystem, pe_characteristics,
# pe_reloc_rva, pe_reloc_size, and pe_size. On failure it sets pe_error.
pe_read() {
    local path=$1 raw b1 b2 b3 b4
    pe_error=
    pe_machine=0
    pe_magic=0
    pe_subsystem=0
    pe_characteristics=0
    pe_reloc_rva=0
    pe_reloc_size=0
    pe_size=0

    if [[ ! -f $path ]]; then
        pe_error="missing file"
        return 1
    fi
    pe_size=$(stat -c '%s' "$path") || {
        pe_error="cannot stat"
        return 1
    }

    raw=$(od -An -tx1 -N2 -j 0 "$path" | tr -d '[:space:]') || {
        pe_error="missing MZ header"
        return 1
    }
    if [[ $raw != 4d5a ]]; then
        pe_error="missing MZ header"
        return 1
    fi

    local e_lfanew
    e_lfanew=$(pe_u32 "$path" 60) || {
        pe_error="missing PE signature"
        return 1
    }
    if (( e_lfanew < 64 || e_lfanew + 160 > pe_size )); then
        pe_error="missing PE signature"
        return 1
    fi
    raw=$(od -An -tx1 -N4 -j "$e_lfanew" "$path" | tr -d '[:space:]') || {
        pe_error="missing PE signature"
        return 1
    }
    if [[ $raw != 50450000 ]]; then
        pe_error="missing PE signature"
        return 1
    fi

    pe_machine=$(pe_u16 "$path" $((e_lfanew + 4))) || return 1
    pe_characteristics=$(pe_u16 "$path" $((e_lfanew + 22))) || return 1
    local optional=$((e_lfanew + 24))
    pe_magic=$(pe_u16 "$path" "$optional") || return 1
    pe_subsystem=$(pe_u16 "$path" $((optional + 68))) || return 1
    pe_reloc_rva=$(pe_u32 "$path" $((optional + 152))) || return 1
    pe_reloc_size=$(pe_u32 "$path" $((optional + 156))) || return 1
    return 0
}

pe_u16() {
    local file=$1 offset=$2 raw b1 b2
    raw=$(od -An -tx1 -N2 -j "$offset" "$file") || return 1
    read -r b1 b2 <<<"$raw" || return 1
    if [[ -z ${b1:-} || -z ${b2:-} ]]; then
        return 1
    fi
    echo $((16#$b2 * 256 + 16#$b1))
}

pe_u32() {
    local file=$1 offset=$2 raw b1 b2 b3 b4
    raw=$(od -An -tx1 -N4 -j "$offset" "$file" | tr -d '\n') || return 1
    read -r b1 b2 b3 b4 <<<"$raw" || return 1
    if [[ -z ${b1:-} || -z ${b2:-} || -z ${b3:-} || -z ${b4:-} ]]; then
        return 1
    fi
    echo $((16#$b4 * 16777216 + 16#$b3 * 65536 + 16#$b2 * 256 + 16#$b1))
}
