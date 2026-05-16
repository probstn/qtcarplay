#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

tty_args=()
if [[ -t 0 && -t 1 ]]; then
    tty_args=(-it)
fi

docker run --rm "${tty_args[@]}" \
    --platform linux/arm64 \
    -e LANG=C.UTF-8 \
    -e LC_ALL=C.UTF-8 \
    -e QT_VERSION \
    -e QT_BUILD_PARALLEL \
    -e RPI_SYSROOT=/work/.rpi-sdk/sysroot \
    -e RPI_QT_HOST=/work/.rpi-sdk/qt-host-${QT_VERSION} \
    -e RPI_QT_TARGET=/work/.rpi-sdk/qt-rpi-${QT_VERSION} \
    -e RPI_QT_PREFIX \
    -e RPI_APP_PREFIX \
    -v "${repo_root}:/work" \
    -w /work \
    "${RPI_DOCKER_IMAGE}" \
    "$@"
