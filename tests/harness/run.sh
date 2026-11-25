#!/usr/bin/env bash
# QueueOS harness. One run id per invocation. Cases in a suite share it.
# QEMU opens copies under the case work directory, never the images in
# the repository and never the host firmware variable store.
#
# usage:
#   run.sh --root DIR --build DIR --suite FILE
#   run.sh --root DIR --build DIR --case ID
#   run.sh --root DIR --build DIR --boot FOLDER

set -euo pipefail

ROOT=
BUILD=
SUITE=
CASE_ID=
BOOT=

die() {
    echo "harness: $1" >&2
    exit 1
}

usage() {
    die "usage: run.sh --root DIR --build DIR (--suite FILE | --case ID | --boot FOLDER)"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --root) [[ $# -ge 2 ]] || usage; ROOT=$2; shift 2 ;;
        --build) [[ $# -ge 2 ]] || usage; BUILD=$2; shift 2 ;;
        --suite) [[ $# -ge 2 ]] || usage; SUITE=$2; shift 2 ;;
        --case) [[ $# -ge 2 ]] || usage; CASE_ID=$2; shift 2 ;;
        --boot) [[ $# -ge 2 ]] || usage; BOOT=$2; shift 2 ;;
        *) usage ;;
    esac
done

[[ -n $ROOT && -n $BUILD ]] || usage
modes=0
[[ -n $SUITE ]] && modes=$((modes + 1))
[[ -n $CASE_ID ]] && modes=$((modes + 1))
[[ -n $BOOT ]] && modes=$((modes + 1))
[[ $modes -eq 1 ]] || usage

HARNESS=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
# shellcheck source=host-checks.sh
source "$HARNESS/host-checks.sh"
# shellcheck source=guest.sh
source "$HARNESS/guest.sh"

conf_get() {
    local key=$1 file=$2 line trimmed k v
    [[ -f $file ]] || return 1
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

qemu_version() {
    local line
    line=$(qemu-system-x86_64 --version 2>/dev/null | head -n 1 || true)
    [[ -n $line ]] || line=unknown
    printf '%s' "$line"
}

alloc_run() {
    local file=$ROOT/results/run-counter cur major minor n
    mkdir -p "$ROOT/results"
    if [[ -f $file ]]; then
        cur=$(head -n 1 "$file" | tr -d '[:space:]')
    else
        cur=000-001
    fi
    [[ $cur =~ ^[0-9]{3}-[0-9]{3}$ ]] || die "run counter is not NNN-NNN"
    RUN=$cur
    major=${cur%-*}
    minor=${cur#*-}
    n=$((10#$major * 1000 + 10#$minor + 1))
    printf '%03d-%03d\n' $((n / 1000)) $((n % 1000)) > "$file"
    RUN_DIR=$ROOT/results/$RUN
    mkdir -p "$RUN_DIR"
    : > "$RUN_DIR/summary.txt"
}

write_meta() {
    local dir=$1 result=$2 machine=$3 accel=$4 code=$5 vars=$6 shot=$7
    cat > "$dir/meta.txt" <<EOF
case=${CASE_LABEL}
run=${RUN}
qemu=$(qemu_version)
machine=${machine}
accel=${accel}
firmware-code=${code}
firmware-vars=${vars}
screenshot=${shot}
result=${result}
EOF
}

run_one() {
    local id=$1
    local conf=$ROOT/tests/cases/$id/case.conf
    local dir check image expect image_dir
    local machine=- accel=- code=- vars=- shot=no result=fail
    [[ -f $conf ]] || die "missing $conf"
    id_from=$(conf_get id "$conf") || die "$id has no id"
    [[ $id_from == "$id" ]] || die "$id does not match its case.conf"
    check=$(conf_get check "$conf") || die "$id has no check"
    image=$(conf_get image "$conf" || true)
    expect=$(conf_get expect "$conf" || true)
    dir=$RUN_DIR/$id
    mkdir -p "$dir"
    CASE_LABEL=$id
    GUEST_DIR=$dir
    if [[ -n $image ]]; then
        image_dir=$ROOT/images/$image
        [[ -f $image_dir/image.conf ]] || die "$id image is missing"
        machine=$(conf_get machine "$image_dir/image.conf")
        code=$(conf_get firmware-code "$image_dir/image.conf")
        vars=$(conf_get firmware-vars "$image_dir/image.conf")
    fi
    case $check in
        screenshot-present|screenshot-red) shot=yes ;;
        *) shot=no ;;
    esac
    set +e
    case $check in
        tool-presence) host_tool_presence ;;
        build-products) host_build_products ;;
        unity) host_unity ;;
        image-checksum|image-database-blank|image-esp|image-payloads|image-base-size)
            bash "$ROOT/tests/host/check-images.sh" --root "$ROOT" --check "$check"
            ;;
        serial-contains|serial-order|serial-absent|screenshot-present|screenshot-red|screenshot-not-requested)
            [[ -n $image ]] || die "$id has no image"
            guest_case "$image_dir" "$check" "$expect"
            ;;
        *)
            echo "unknown check: $check" >&2
            false
            ;;
    esac >"$dir/check.log" 2>&1
    local status=$?
    set -e
    if [[ -f $dir/accel.txt ]]; then
        accel=$(head -n 1 "$dir/accel.txt")
    fi
    if (( status == 0 )); then
        result=pass
    else
        result=fail
        FAILS=$((FAILS + 1))
    fi
    write_meta "$dir" "$result" "$machine" "$accel" "$code" "$vars" "$shot"
    printf '%s %s\n' "$id" "$result" | tee -a "$RUN_DIR/summary.txt"
}

run_boot() {
    local image_dir=$ROOT/images/$BOOT
    local dir
    [[ -f $image_dir/image.conf ]] || die "missing $image_dir/image.conf"
    dir=$RUN_DIR/boot
    mkdir -p "$dir"
    CASE_LABEL=boot
    GUEST_DIR=$dir
    set +e
    if [[ -n ${DISPLAY:-} || -n ${WAYLAND_DISPLAY:-} ]]; then
        guest_window "$image_dir"
    else
        guest_sequence "$image_dir"
    fi
    local status=$?
    set -e
    local result=fail
    (( status == 0 )) && result=pass
    local accel=-
    [[ -f $dir/accel.txt ]] && accel=$(head -n 1 "$dir/accel.txt")
    write_meta "$dir" "$result" \
        "$(conf_get machine "$image_dir/image.conf")" \
        "$accel" \
        "$(conf_get firmware-code "$image_dir/image.conf")" \
        "$(conf_get firmware-vars "$image_dir/image.conf")" \
        yes
    printf '%s %s\n' boot "$result" | tee -a "$RUN_DIR/summary.txt"
    echo "result: $dir"
    return "$status"
}

alloc_run
FAILS=0
echo "run $RUN"

if [[ -n $BOOT ]]; then
    run_boot
    exit $?
fi

if [[ -n $CASE_ID ]]; then
    run_one "$CASE_ID"
else
    [[ -f $SUITE ]] || die "missing suite $SUITE"
    while IFS= read -r line || [[ -n ${line:-} ]]; do
        line=${line#"${line%%[![:space:]]*}"}
        line=${line%"${line##*[![:space:]]}"}
        [[ -z $line || $line == \#* ]] && continue
        run_one "$line"
    done < "$SUITE"
fi

echo "summary: $RUN_DIR/summary.txt"
if (( FAILS > 0 )); then
    exit 1
fi
exit 0
