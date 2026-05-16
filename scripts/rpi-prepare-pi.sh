#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

remote_script="$(ssh "${RPI_SSH}" 'mktemp')"
trap 'ssh "${RPI_SSH}" "rm -f ${remote_script@Q}" >/dev/null 2>&1 || true' EXIT

ssh "${RPI_SSH}" "cat > ${remote_script@Q}" <<'REMOTE'
set -euo pipefail

apt update
apt full-upgrade -y
apt install -y \
    build-essential \
    ffmpeg \
    libasound2-dev \
    libavcodec-dev \
    libavdevice-dev \
    libavfilter-dev \
    libavformat-dev \
    libavutil-dev \
    libdrm-dev \
    libegl-dev \
    libgbm-dev \
    libgles-dev \
    libinput-dev \
    libpulse-dev \
    libswresample-dev \
    libswscale-dev \
    libudev-dev \
    libusb-1.0-0-dev \
    libxkbcommon-dev \
    mesa-common-dev \
    pkg-config \
    rsync

tee /etc/udev/rules.d/70-qtcarplay-dongle.rules >/dev/null <<'RULE'
SUBSYSTEM=="usb", ATTR{idVendor}=="1314", MODE="0660", GROUP="plugdev", TAG+="uaccess"
RULE
udevadm control --reload-rules
udevadm trigger || true

usermod -aG audio,video,render,input,plugdev "${SUDO_USER:-haflinger}"
REMOTE

ssh -tt "${RPI_SSH}" "sudo bash ${remote_script@Q}"
