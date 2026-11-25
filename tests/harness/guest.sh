# QEMU guest for one harness case. Sourced by run.sh.
# The serial file also receives the firmware console, so a sentence is
# matched as text inside a line. ConOut on this OVMF build is mirrored
# there with ANSI cursor sequences around the same words.

guest_stop() {
    if [[ -n ${GUEST_PID:-} ]] && kill -0 "$GUEST_PID" 2>/dev/null; then
        kill "$GUEST_PID" 2>/dev/null || true
        wait "$GUEST_PID" 2>/dev/null || true
    fi
    GUEST_PID=
}

guest_qmp() {
    local payload=$1
    {
        printf '%s\n' '{"execute":"qmp_capabilities"}'
        printf '%s\n' "$payload"
    } | socat -t 0.5 - "UNIX-CONNECT:${GUEST_SOCK}"
}

guest_accel() {
    local resp
    resp=$(guest_qmp '{"execute":"query-kvm"}' || true)
    if [[ $resp == *'"enabled": true'* || $resp == *'"enabled":true'* ]]; then
        printf '%s\n' kvm
    else
        printf '%s\n' tcg
    fi
}

guest_screendump() {
    local dest=$1 resp
    resp=$(guest_qmp "{\"execute\":\"screendump\",\"arguments\":{\"filename\":\"${dest}\"}}" || true)
    [[ -f $dest ]] || return 1
    [[ $resp == *'"error"'* ]] && return 1
    return 0
}

# Deadline is an absolute SECONDS value. The needle is literal text.
wait_for_text() {
    local needle=$1 deadline=$2
    while (( SECONDS < deadline )); do
        if [[ -s $GUEST_SERIAL ]] && grep -F -q -- "$needle" "$GUEST_SERIAL"; then
            return 0
        fi
        if [[ -n ${GUEST_PID:-} ]] && ! kill -0 "$GUEST_PID" 2>/dev/null; then
            echo "qemu exited before: $needle" >&2
            return 1
        fi
        sleep 0.2
    done
    echo "timed out waiting for: $needle" >&2
    return 1
}

ppm_is_p6() {
    local ppm=$1
    local magic size
    [[ -f $ppm ]] || return 1
    size=$(stat -c '%s' "$ppm")
    [[ $size -gt 1024 ]] || return 1
    magic=$(dd if="$ppm" bs=2 count=1 status=none 2>/dev/null || true)
    [[ $magic == P6 ]]
}

# Sample every 16th pixel. Pass when at least 40 percent are red-dominant.
ppm_is_red() {
    local ppm=$1 header=0 byte newlines=0
    local raw=$GUEST_DIR/work/rgb counts
    [[ $(dd if="$ppm" bs=2 count=1 status=none) == P6 ]] || return 1
    for byte in $(od -An -tu1 -N 128 -v "$ppm"); do
        header=$((header + 1))
        if (( byte == 10 )); then
            newlines=$((newlines + 1))
            if (( newlines == 3 )); then
                break
            fi
        fi
    done
    (( newlines == 3 && header > 0 )) || return 1
    dd if="$ppm" of="$raw" bs="$header" skip=1 status=none
    if ! counts=$(od -An -tu1 -w3 -v "$raw" | awk '
        NF == 3 && (NR % 16) == 1 {
            total++
            if ($1+0 > 80 && $1+0 > $2+40 && $1+0 > $3+40) red++
        }
        END {
            if (total < 1) exit 1
            printf "%d %d\n", red, total
            exit (red * 100 >= total * 40 ? 0 : 1)
        }
    '); then
        echo "red sample ${counts:-failed}" >&2
        return 1
    fi
    echo "red sample $counts"
}

prepare_work() {
    local image_dir=$1
    local db version
    mkdir -p "$GUEST_DIR/work"
    version=$(conf_get database "$image_dir/image.conf") || {
        echo "image.conf has no database" >&2
        return 1
    }
    if [[ $version == current ]]; then
        version=$(head -n 1 "$ROOT/images/database/current" | tr -d '[:space:]')
    fi
    db=$ROOT/images/database/$version/QueueOS-Database.qcow2
    [[ -f $db ]] || { echo "missing $db" >&2; return 1; }
    cp -L -- "$(conf_get firmware-vars "$image_dir/image.conf")" "$GUEST_DIR/work/OVMF_VARS.fd"
    cp -f -- "$image_dir/QueueOS-UEFI.qcow2" "$GUEST_DIR/work/QueueOS-UEFI.qcow2"
    cp -f -- "$db" "$GUEST_DIR/work/QueueOS-Database.qcow2"
}

start_guest() {
    local image_dir=$1 show_window=$2
    local qemu machine display code vars
    local -a args
    qemu=$(conf_get qemu "$image_dir/image.conf")
    machine=$(conf_get machine "$image_dir/image.conf")
    display=$(conf_get display "$image_dir/image.conf")
    code=$(conf_get firmware-code "$image_dir/image.conf")
    vars=$GUEST_DIR/work/OVMF_VARS.fd
    rm -f "$GUEST_SOCK" "$GUEST_SERIAL"
    args=(
        "$qemu"
        -machine "${machine},accel=kvm:tcg"
        -m 256
        -no-reboot
        -no-shutdown
        -boot menu=on,splash-time=10000
        -drive "if=pflash,format=raw,readonly=on,unit=0,file=${code}"
        -drive "if=pflash,format=raw,unit=1,file=${vars}"
        -drive "if=none,id=esp,format=qcow2,file=${GUEST_DIR}/work/QueueOS-UEFI.qcow2"
        -drive "if=none,id=db,format=qcow2,file=${GUEST_DIR}/work/QueueOS-Database.qcow2"
        -device virtio-scsi-pci,id=scsi0
        -device scsi-hd,bus=scsi0.0,drive=esp,bootindex=1,vendor=QueueOS,product=Boot,serial=qos-boot
        -device scsi-hd,bus=scsi0.0,drive=db,bootindex=2,vendor=QueueOS,product=Base,serial=qos-base
        -serial "file:${GUEST_SERIAL}"
        -qmp "unix:${GUEST_SOCK},server=on,wait=off"
        -name queueos-test
    )
    if [[ $show_window == window ]]; then
        args+=(-vga std -device qemu-xhci -device usb-kbd -device usb-tablet)
    elif [[ $display == graphical ]]; then
        args+=(-vga std -display none -device qemu-xhci -device usb-kbd -device usb-tablet)
    else
        args+=(-vga none -display none)
    fi
    # The token is taken from the -machine argument, not from a second
    # read of image.conf. meta.txt records the conf value on its own.
    write_machine_token "${args[@]}" || return 1
    "${args[@]}" >"$GUEST_DIR/qemu.err" 2>&1 &
    GUEST_PID=$!
    local _try
    for _try in $(seq 1 50); do
        [[ -S $GUEST_SOCK ]] && break
        sleep 0.05
    done
    [[ -S $GUEST_SOCK ]] || { echo "qmp socket did not appear" >&2; return 1; }
    guest_accel >"$GUEST_DIR/accel.txt"
}

# machine.txt sits beside accel.txt. The string is the machine token
# inside the -machine argument, before the accel list.
write_machine_token() {
    local prev="" arg
    for arg in "$@"; do
        if [[ $prev == -machine ]]; then
            printf '%s\n' "${arg%%,*}" > "$GUEST_DIR/machine.txt"
            return 0
        fi
        prev=$arg
    done
    echo "qemu command has no -machine" >&2
    return 1
}

# One catalogue case. Screenshot checks dump when EXPECT appears.
# serial-order waits for the Base sentence. serial-absent runs a short
# window and requires EXPECT to stay missing. A mutation is applied to
# the work copy after the copy and before QEMU starts.
guest_case() {
    local image_dir=$1 check=$2 expect=$3
    local deadline shot
    GUEST_DIR=$(cd "$GUEST_DIR" && pwd)
    GUEST_SERIAL=$GUEST_DIR/serial.log
    GUEST_SOCK=$GUEST_DIR/qmp.sock
    prepare_work "$image_dir" || return 1
    apply_mutation "${CASE_MUTATE:-none}" || return 1
    start_guest "$image_dir" headless || { guest_stop; return 1; }
    deadline=$((SECONDS + 50))
    case $check in
        serial-contains)
            wait_for_text "$expect" "$deadline" || { guest_stop; return 1; }
            ;;
        serial-order)
            wait_for_text "QueueOS Loaded" "$deadline" || { guest_stop; return 1; }
            ;;
        serial-absent)
            sleep 25
            ;;
        screenshot-present|screenshot-red)
            wait_for_text "$expect" "$deadline" || { guest_stop; return 1; }
            sleep 0.2
            shot=$GUEST_DIR/screenshot.ppm
            guest_screendump "$shot" || { guest_stop; echo "screendump failed" >&2; return 1; }
            ;;
        screenshot-not-requested)
            wait_for_text "$expect" "$deadline" || { guest_stop; return 1; }
            ;;
        *)
            guest_stop
            echo "unknown guest check: $check" >&2
            return 1
            ;;
    esac
    guest_stop
    case $check in
        serial-contains)
            grep -F -q -- "$expect" "$GUEST_SERIAL"
            ;;
        serial-order)
            guest_order
            ;;
        serial-absent)
            if grep -F -q -- "$expect" "$GUEST_SERIAL"; then
                echo "serial contains $expect" >&2
                return 1
            fi
            ;;
        screenshot-present)
            ppm_is_p6 "$GUEST_DIR/screenshot.ppm"
            ;;
        screenshot-red)
            ppm_is_p6 "$GUEST_DIR/screenshot.ppm" && ppm_is_red "$GUEST_DIR/screenshot.ppm"
            ;;
        screenshot-not-requested)
            [[ ! -e $GUEST_DIR/screenshot.ppm ]]
            ;;
    esac || return 1
    judge_forbids || return 1
    judge_annotation || return 1
    judge_machine "$image_dir"
}

# forbid= lines are fixed strings that must be absent from serial.log.
judge_forbids() {
    local line
    [[ -n ${CASE_FORBIDS:-} ]] || return 0
    [[ -f $GUEST_SERIAL ]] || { echo "serial.log is missing" >&2; return 1; }
    while IFS= read -r line; do
        [[ -z $line ]] && continue
        if grep -F -q -- "$line" "$GUEST_SERIAL"; then
            echo "serial contains forbidden text: $line" >&2
            return 1
        fi
    done <<< "$CASE_FORBIDS"
}

# The annotation is the case file's sentence, written before this judgment.
# It is not scraped from firmware text.
judge_annotation() {
    [[ -n ${CASE_ANNOTATION:-} ]] || return 0
    [[ -f $GUEST_DIR/annotation.txt ]] || { echo "annotation.txt is missing" >&2; return 1; }
    if ! grep -F -x -q -- "$CASE_ANNOTATION" "$GUEST_DIR/annotation.txt"; then
        echo "annotation.txt does not contain the case sentence" >&2
        return 1
    fi
}

# Pass when the token QEMU was given matches image.conf. A pc case then
# requires machine.txt to be pc, because that file says so.
judge_machine() {
    local image_dir=$1 want got
    want=$(conf_get machine "$image_dir/image.conf") || {
        echo "image.conf has no machine" >&2
        return 1
    }
    [[ -f $GUEST_DIR/machine.txt ]] || { echo "machine.txt is missing" >&2; return 1; }
    got=$(head -n 1 "$GUEST_DIR/machine.txt")
    [[ $got == "$want" ]] || {
        echo "machine.txt is ${got}, image.conf machine is ${want}" >&2
        return 1
    }
}

guest_order() {
    local first second
    first=$(grep -n -F -m 1 'QueueOS loading...' "$GUEST_SERIAL" | cut -d: -f1)
    second=$(grep -n -F -m 1 'QueueOS Loaded' "$GUEST_SERIAL" | cut -d: -f1)
    [[ -n $first && -n $second && $first -lt $second ]]
}

# Headless run target. Keep Boot, Base, and the firmware boot menu.
guest_sequence() {
    local image_dir=$1
    local deadline
    GUEST_DIR=$(cd "$GUEST_DIR" && pwd)
    GUEST_SERIAL=$GUEST_DIR/serial.log
    GUEST_SOCK=$GUEST_DIR/qmp.sock
    prepare_work "$image_dir" || return 1
    start_guest "$image_dir" headless || { guest_stop; return 1; }
    deadline=$((SECONDS + 70))
    wait_for_text "QueueOS loading..." "$deadline" || { guest_stop; return 1; }
    sleep 0.2
    guest_screendump "$GUEST_DIR/screenshot-boot.ppm" || { guest_stop; echo "boot screendump failed" >&2; return 1; }
    wait_for_text "QueueOS Loaded" "$deadline" || { guest_stop; return 1; }
    sleep 0.2
    guest_screendump "$GUEST_DIR/screenshot-base.ppm" || { guest_stop; echo "base screendump failed" >&2; return 1; }
    wait_for_text "Please select boot device:" "$deadline" || { guest_stop; return 1; }
    sleep 0.4
    guest_screendump "$GUEST_DIR/screenshot-menu.ppm" || { guest_stop; echo "menu screendump failed" >&2; return 1; }
    guest_qmp '{"execute":"quit"}' >/dev/null 2>&1 || true
    wait "$GUEST_PID" 2>/dev/null || true
    GUEST_PID=
}

guest_window() {
    local image_dir=$1
    GUEST_DIR=$(cd "$GUEST_DIR" && pwd)
    GUEST_SERIAL=$GUEST_DIR/serial.log
    GUEST_SOCK=$GUEST_DIR/qmp.sock
    prepare_work "$image_dir" || return 1
    start_guest "$image_dir" window || { guest_stop; return 1; }
    echo "guest window is open; serial log is $GUEST_SERIAL"
    wait "$GUEST_PID" || true
    GUEST_PID=
}
