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
