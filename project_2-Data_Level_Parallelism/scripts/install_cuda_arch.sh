#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  install_cuda_arch.sh check
  install_cuda_arch.sh install
  install_cuda_arch.sh reinstall-driver

Supported target: Arch Linux x86_64 with an NVIDIA GeForce GTX 10-series GPU.
The installer uses the AUR packages nvidia-580xx-dkms and cuda-12.9.
EOF
}

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

check_platform() {
    [[ -r /etc/arch-release ]] ||
        fail "this script supports Arch Linux only"
    [[ "$(uname -m)" == "x86_64" ]] ||
        fail "only the x86_64 architecture is supported"
}

check_status() {
    check_platform
    printf 'Platform: Arch Linux\nArchitecture: %s\n' "$(uname -m)"

    if command -v lspci >/dev/null 2>&1; then
        lspci -nn -d 10de:
    else
        printf 'lspci: not found (install pciutils to identify the NVIDIA GPU)\n'
    fi

    if command -v nvidia-smi >/dev/null 2>&1; then
        nvidia-smi
    else
        printf 'nvidia-smi: not found\n'
    fi

    if command -v nvcc >/dev/null 2>&1; then
        nvcc --version
    elif [[ -x /opt/cuda-12.9/bin/nvcc ]]; then
        /opt/cuda-12.9/bin/nvcc --version
    else
        printf 'nvcc: not found\n'
    fi
}

if [[ $# -ne 1 ]]; then
    usage >&2
    exit 2
fi

action=$1
case "$action" in
    check)
        check_status
        exit 0
        ;;
    install|reinstall-driver)
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

check_platform
command -v pacman >/dev/null 2>&1 || fail "pacman was not found"
command -v lspci >/dev/null 2>&1 ||
    fail "lspci was not found; install pciutils, then rerun this script"

gpu_info=$(lspci -nn -d 10de: | grep -E 'VGA compatible controller|3D controller' || true)
[[ -n "$gpu_info" ]] || fail "no NVIDIA GPU was detected"
grep -Eqi 'GeForce GTX 10[0-9]{2}' <<<"$gpu_info" ||
    fail "this installer is for GTX 10-series (Pascal) GPUs; detected: $gpu_info"

[[ $EUID -ne 0 ]] ||
    fail "do not run this script as root; AUR helpers must run as a regular user"

if command -v yay >/dev/null 2>&1; then
    aur_helper=yay
elif command -v paru >/dev/null 2>&1; then
    aur_helper=paru
else
    fail "install yay or paru first; review AUR build files before accepting them"
fi
command -v sudo >/dev/null 2>&1 || fail "sudo is required"
sudo -v

kernel_headers=()
for kernel_package in linux linux-lts linux-zen linux-hardened; do
    if pacman -Qq "$kernel_package" >/dev/null 2>&1; then
        kernel_headers+=("${kernel_package}-headers")
    fi
done

if ((${#kernel_headers[@]} == 0)) &&
    [[ ! -e "/usr/lib/modules/$(uname -r)/build" ]]; then
    fail "matching kernel headers are missing; install headers for the running kernel, then rerun"
fi

printf 'Updating Arch Linux and installing GTX 10-series compatible CUDA packages.\n'
printf 'The AUR helper (%s) will build AUR packages; inspect its PKGBUILD prompts.\n' "$aur_helper"

"$aur_helper" -Syu --needed base-devel dkms "${kernel_headers[@]}"

case "$action" in
    install)
        "$aur_helper" -S --needed nvidia-580xx-dkms cuda-12.9
        ;;
    reinstall-driver)
        if [[ "$aur_helper" == "yay" ]]; then
            "$aur_helper" -S --rebuildall nvidia-580xx-dkms nvidia-580xx-utils
        else
            "$aur_helper" -S --rebuild nvidia-580xx-dkms nvidia-580xx-utils
        fi
        ;;
esac

cat <<'EOF'
Installation completed. Reboot to load the NVIDIA kernel module:
  sudo reboot

Then verify with:
  nvidia-smi
  nvcc --version
EOF
