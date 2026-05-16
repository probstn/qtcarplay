#!/usr/bin/env bash
set -euo pipefail

: "${QT_VERSION:?}"
: "${RPI_SYSROOT:?}"
: "${RPI_QT_HOST:?}"
: "${RPI_QT_TARGET:?}"
: "${RPI_QT_PREFIX:?}"
: "${QT_BUILD_PARALLEL:=2}"

src_root="/work/.rpi-sdk/src"
archive="qt-everywhere-src-${QT_VERSION}.tar.xz"
src_dir="${src_root}/qt-everywhere-src-${QT_VERSION}"
host_build="/work/.rpi-sdk/build/qt-host-${QT_VERSION}"
target_build="/work/.rpi-sdk/build/qt-rpi-${QT_VERSION}"
base_url="https://download.qt.io/official_releases/qt/${QT_VERSION%.*}/${QT_VERSION}/single"

install_host_file() {
    local mode="$1"
    local src="$2"
    local dst="$3"

    if [[ -e "${src}" ]]; then
        install -D -m "${mode}" "${src}" "${dst}"
    fi
}

install_host_dir_contents() {
    local src="$1"
    local dst="$2"

    if [[ -d "${src}" ]]; then
        mkdir -p "${dst}"
        cp -a "${src}/." "${dst}/"
    fi
}

host_prefix_ready() {
    local required

    for required in \
        "${RPI_QT_HOST}/bin/qt-cmake" \
        "${RPI_QT_HOST}/bin/qsb" \
        "${RPI_QT_HOST}/libexec/moc" \
        "${RPI_QT_HOST}/libexec/rcc" \
        "${RPI_QT_HOST}/libexec/qmlcachegen" \
        "${RPI_QT_HOST}/libexec/qmltyperegistrar" \
        "${RPI_QT_HOST}/lib/cmake/Qt6HostInfo/Qt6HostInfoConfig.cmake" \
        "${RPI_QT_HOST}/lib/cmake/Qt6CoreTools/Qt6CoreToolsTargets.cmake" \
        "${RPI_QT_HOST}/lib/cmake/Qt6QmlTools/Qt6QmlToolsTargets.cmake" \
        "${RPI_QT_HOST}/lib/cmake/Qt6ShaderToolsTools/Qt6ShaderToolsToolsTargets.cmake"; do
        [[ -e "${required}" ]] || return 1
    done
}

install_host_metadata() {
    local module_build cmake_dir dst

    for module_build in "${host_build}"/qtbase "${host_build}"/qtdeclarative "${host_build}"/qtshadertools "${host_build}"/qtsvg; do
        [[ -d "${module_build}/lib/cmake" ]] || continue
        for cmake_dir in "${module_build}"/lib/cmake/Qt6*; do
            [[ -d "${cmake_dir}" ]] || continue
            dst="${RPI_QT_HOST}/lib/cmake/$(basename "${cmake_dir}")"
            install_host_dir_contents "${cmake_dir}" "${dst}"
        done
    done

    install_host_dir_contents \
        "${src_dir}/qtbase/cmake/QtBuildInternals/QtStandaloneTestTemplateProject" \
        "${RPI_QT_HOST}/lib/cmake/Qt6BuildInternals/QtStandaloneTestTemplateProject"

    install_host_file 0644 "${src_dir}/qtbase/cmake/QtBuildInternals/QtBuildInternalsHelpers.cmake" \
        "${RPI_QT_HOST}/lib/cmake/Qt6BuildInternals/QtBuildInternalsHelpers.cmake"
    install_host_file 0644 "${host_build}/qtbase/mkspecs/qconfig.pri" \
        "${RPI_QT_HOST}/mkspecs/qconfig.pri"
    install_host_file 0644 "${host_build}/qtbase/mkspecs/qmodule.pri" \
        "${RPI_QT_HOST}/mkspecs/qmodule.pri"
    install_host_file 0644 "${host_build}/qtbase/lib/cmake/Qt6/qt-configure-module-flags.txt" \
        "${RPI_QT_HOST}/lib/cmake/Qt6/qt-configure-module-flags.txt"

    install_host_file 0755 "${host_build}/qtbase/bin/qt-cmake" "${RPI_QT_HOST}/bin/qt-cmake"
    install_host_file 0755 "${host_build}/qtbase/bin/qt-cmake-create" "${RPI_QT_HOST}/bin/qt-cmake-create"
    install_host_file 0755 "${host_build}/qtbase/bin/qt-configure-module" "${RPI_QT_HOST}/bin/qt-configure-module"
    install_host_file 0755 "${host_build}/qtbase/libexec/qt-cmake-private" "${RPI_QT_HOST}/libexec/qt-cmake-private"
    install_host_file 0755 "${host_build}/qtbase/libexec/qt-cmake-standalone-test" "${RPI_QT_HOST}/libexec/qt-cmake-standalone-test"
    install_host_file 0644 "${host_build}/qtbase/libexec/qt-cmake-private-install.cmake" \
        "${RPI_QT_HOST}/libexec/qt-cmake-private-install.cmake"
    install_host_file 0755 "${host_build}/qtbase/libexec/qt-internal-configure-tests" \
        "${RPI_QT_HOST}/libexec/qt-internal-configure-tests"
    install_host_file 0755 "${host_build}/qtbase/libexec/qt-internal-configure-examples" \
        "${RPI_QT_HOST}/libexec/qt-internal-configure-examples"

    host_prefix_ready || {
        echo "The host Qt prefix is still missing required tools or CMake metadata." >&2
        return 1
    }
}

mkdir -p "${src_root}" /work/.rpi-sdk/build

if [[ ! -d "${src_dir}" ]]; then
    if [[ ! -f "${src_root}/${archive}" ]]; then
        curl -L --fail --continue-at - "${base_url}/${archive}" -o "${src_root}/${archive}"
        curl -L --fail "${base_url}/md5sums.txt" -o "${src_root}/md5sums.txt"
        (cd "${src_root}" && md5sum -c --ignore-missing md5sums.txt)
    fi
    tar -C "${src_root}" -xf "${src_root}/${archive}"
fi

if ! host_prefix_ready; then
    cmake -S "${src_dir}" -B "${host_build}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
        -DCMAKE_INSTALL_PREFIX="${RPI_QT_HOST}" \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
        -DQT_BUILD_SUBMODULES="qtbase;qtshadertools;qtdeclarative"
    cmake --build "${host_build}" --target host_tools --parallel "${QT_BUILD_PARALLEL}"
    cmake --install "${host_build}" --component host_tools
    cmake --install "${host_build}" --component Devel
    install_host_metadata
fi

cmake -S "${src_dir}" -B "${target_build}" -G Ninja \
    -UQT_FEATURE_alsa \
    -UQT_FEATURE_pulseaudio \
    -UQT_FEATURE_ffmpeg \
    -UQT_FEATURE_ffmpeg_stubs \
    -UFEATURE_alsa \
    -UFEATURE_pulseaudio \
    -UFEATURE_ffmpeg \
    -UFEATURE_ffmpeg_stubs \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON \
    -DCMAKE_TOOLCHAIN_FILE=/work/cmake/rpi-aarch64-toolchain.cmake \
    -DCMAKE_INSTALL_PREFIX="${RPI_QT_PREFIX}" \
    -DCMAKE_STAGING_PREFIX="${RPI_QT_TARGET}" \
    -DQT_HOST_PATH="${RPI_QT_HOST}" \
    -DQT_BUILD_EXAMPLES=OFF \
    -DQT_BUILD_TESTS=OFF \
    -DQT_NO_FEATURE_AUTO_RESET=ON \
    -DQT_BUILD_SUBMODULES="qtbase;qtshadertools;qtdeclarative;qtmultimedia" \
    -DBUILD_qtquick3d=OFF \
    -DINPUT_opengl=es2 \
    -DQT_FEATURE_xcb=OFF \
    -DFEATURE_xlib=OFF \
    -DFEATURE_egl_x11=OFF \
    -DQT_FEATURE_eglfs=ON \
    -DQT_FEATURE_eglfs_kms=ON \
    -DQT_FEATURE_linuxfb=ON \
    -DFEATURE_alsa=ON \
    -DFEATURE_pulseaudio=OFF \
    -DFEATURE_ffmpeg=OFF \
    -DQT_FEATURE_spatialaudio_quick3d=OFF

cmake --build "${target_build}" --parallel "${QT_BUILD_PARALLEL}"
cmake --install "${target_build}"
