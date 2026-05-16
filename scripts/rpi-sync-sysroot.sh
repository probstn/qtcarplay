#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

mkdir -p \
    "${RPI_SYSROOT}/lib" \
    "${RPI_SYSROOT}/usr/include" \
    "${RPI_SYSROOT}/usr/lib" \
    "${RPI_SYSROOT}/usr/share/pkgconfig"

rsync -az --delete "${RPI_SSH}:/lib/" "${RPI_SYSROOT}/lib/"
rsync -az --delete "${RPI_SSH}:/usr/include/" "${RPI_SYSROOT}/usr/include/"
rsync -az --delete "${RPI_SSH}:/usr/lib/" "${RPI_SYSROOT}/usr/lib/"
rsync -az --delete "${RPI_SSH}:/usr/share/pkgconfig/" "${RPI_SYSROOT}/usr/share/pkgconfig/" || true

ssh "${RPI_SSH}" 'test ! -d /opt/vc' || {
    mkdir -p "${RPI_SYSROOT}/opt/vc"
    rsync -az --delete "${RPI_SSH}:/opt/vc/" "${RPI_SYSROOT}/opt/vc/"
}

"${repo_root}/scripts/rpi-docker-run.sh" python3 /work/scripts/rpi-fix-sysroot-links.py /work/.rpi-sdk/sysroot
