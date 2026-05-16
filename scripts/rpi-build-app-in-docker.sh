#!/usr/bin/env bash
set -euo pipefail

: "${RPI_QT_TARGET:?}"
: "${RPI_APP_PREFIX:?}"

rm -rf /work/.rpi-sdk/package

"${RPI_QT_TARGET}/bin/qt-cmake" -S /work -B /work/build-rpi -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/work/cmake/rpi-aarch64-toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX="${RPI_APP_PREFIX}"

cmake --build /work/build-rpi --parallel
DESTDIR=/work/.rpi-sdk/package cmake --install /work/build-rpi --prefix "${RPI_APP_PREFIX}"
