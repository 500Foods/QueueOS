# Host checks for the harness. Sourced by run.sh. No QEMU.

host_tool_presence() {
    local -a missing=()
    local tool
    local -a tools=(
        gcc objcopy cmake bash
        qemu-system-x86_64 qemu-img
        mkfs.fat mmd mcopy
    )
    for tool in "${tools[@]}"; do
        command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
    done
    [[ -f /usr/share/OVMF/OVMF_CODE.fd ]] || missing+=("/usr/share/OVMF/OVMF_CODE.fd")
    [[ -f /usr/share/OVMF/OVMF_VARS.fd ]] || missing+=("/usr/share/OVMF/OVMF_VARS.fd")
    if [[ -e /dev/kvm ]]; then
        echo "kvm present"
    else
        echo "kvm absent"
    fi
    if (( ${#missing[@]} > 0 )); then
        echo "missing: ${missing[*]}" >&2
        return 1
    fi
}

host_build_products() {
    local version folder db
    [[ -f $BUILD/efi/Boot.efi ]] || { echo "missing Boot.efi" >&2; return 1; }
    [[ -f $BUILD/efi/Base.efi ]] || { echo "missing Base.efi" >&2; return 1; }
    version=$(head -n 1 "$ROOT/images/database/current" | tr -d '[:space:]')
    [[ -n $version ]] || { echo "database version is empty" >&2; return 1; }
    db=$ROOT/images/database/$version/QueueOS-Database.qcow2
    [[ -f $db ]] || { echo "missing $db" >&2; return 1; }
    [[ -f $ROOT/images/database/$version/manifest.sha256 ]] || {
        echo "missing database manifest" >&2
        return 1
    }
    for folder in a-001-q35-base a-002-q35-serial; do
        [[ -f $ROOT/images/$folder/QueueOS-UEFI.qcow2 ]] || {
            echo "missing $folder UEFI image" >&2
            return 1
        }
        [[ -L $ROOT/images/$folder/QueueOS-Database.qcow2 ]] || {
            echo "missing $folder database symlink" >&2
            return 1
        }
        [[ -f $ROOT/images/$folder/manifest.sha256 ]] || {
            echo "missing $folder manifest" >&2
            return 1
        }
        [[ -f $ROOT/images/$folder/image.conf ]] || {
            echo "missing $folder image.conf" >&2
            return 1
        }
    done
    echo "build-products ok"
}

host_unity() {
    [[ -x $BUILD/sentences ]] || { echo "sentences binary is missing" >&2; return 1; }
    "$BUILD/sentences"
}

# a-003 is the same staged ESP as a-001, described as machine pc.
host_pc_same() {
    local pc=$ROOT/images/a-003-pc-base
    local base=$ROOT/images/a-001-q35-base
    local key got want
    [[ -f $pc/image.conf ]] || { echo "missing a-003 image.conf" >&2; return 1; }
    [[ -f $pc/QueueOS-UEFI.qcow2 ]] || { echo "missing a-003 UEFI image" >&2; return 1; }
    [[ -f $base/QueueOS-UEFI.qcow2 ]] || { echo "missing a-001 UEFI image" >&2; return 1; }
    got=$(conf_get name "$pc/image.conf") || { echo "a-003 has no name" >&2; return 1; }
    [[ $got == a-003-pc-base ]] || { echo "a-003 name is ${got}" >&2; return 1; }
    got=$(conf_get machine "$pc/image.conf") || { echo "a-003 has no machine" >&2; return 1; }
    [[ $got == pc ]] || { echo "a-003 machine is ${got}" >&2; return 1; }
    got=$(conf_get qemu "$pc/image.conf") || { echo "a-003 has no qemu" >&2; return 1; }
    [[ $got == qemu-system-x86_64 ]] || { echo "a-003 qemu is ${got}" >&2; return 1; }
    got=$(conf_get display "$pc/image.conf") || { echo "a-003 has no display" >&2; return 1; }
    [[ $got == graphical ]] || { echo "a-003 display is ${got}" >&2; return 1; }
    got=$(conf_get database "$pc/image.conf") || { echo "a-003 has no database" >&2; return 1; }
    [[ $got == current ]] || { echo "a-003 database is ${got}" >&2; return 1; }
    for key in firmware-code firmware-vars; do
        got=$(conf_get "$key" "$pc/image.conf") || { echo "a-003 has no ${key}" >&2; return 1; }
        want=$(conf_get "$key" "$base/image.conf") || { echo "a-001 has no ${key}" >&2; return 1; }
        [[ $got == "$want" ]] || { echo "a-003 ${key} does not match a-001" >&2; return 1; }
    done
    if ! cmp -s -- "$pc/QueueOS-UEFI.qcow2" "$base/QueueOS-UEFI.qcow2"; then
        echo "a-003 UEFI image differs from a-001" >&2
        return 1
    fi
    if [[ -e $pc/vm.conf ]]; then
        echo "a-003 has vm.conf" >&2
        return 1
    fi
    echo "image-pc-same ok"
}
