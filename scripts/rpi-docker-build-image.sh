#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

docker build \
    --platform linux/arm64 \
    -f "${repo_root}/docker/qt-rpi/Dockerfile" \
    -t "${RPI_DOCKER_IMAGE}" \
    "${repo_root}"
