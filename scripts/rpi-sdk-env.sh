#!/usr/bin/env bash
set -euo pipefail

export QT_VERSION="${QT_VERSION:-6.11.0}"
export RPI_HOST="${RPI_HOST:-haflinger.local}"
export RPI_USER="${RPI_USER:-haflinger}"
export RPI_SSH="${RPI_SSH:-${RPI_USER}@${RPI_HOST}}"

export SDK_ROOT="${SDK_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/.rpi-sdk}"
export RPI_SYSROOT="${RPI_SYSROOT:-${SDK_ROOT}/sysroot}"
export RPI_QT_HOST="${RPI_QT_HOST:-${SDK_ROOT}/qt-host-${QT_VERSION}}"
export RPI_QT_TARGET="${RPI_QT_TARGET:-${SDK_ROOT}/qt-rpi-${QT_VERSION}}"
export RPI_QT_PREFIX="${RPI_QT_PREFIX:-/usr/local/qt6}"
export RPI_APP_PREFIX="${RPI_APP_PREFIX:-/opt/qtcarplay}"
export RPI_DOCKER_IMAGE="${RPI_DOCKER_IMAGE:-qtcarplay-rpi-qt:${QT_VERSION}-trixie-arm64}"
export QT_BUILD_PARALLEL="${QT_BUILD_PARALLEL:-2}"
