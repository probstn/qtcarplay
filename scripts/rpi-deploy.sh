#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

if [[ ! -d "${RPI_QT_TARGET}" ]]; then
    echo "missing ${RPI_QT_TARGET}; run scripts/rpi-build-qt.sh first" >&2
    exit 1
fi

if [[ ! -d "${SDK_ROOT}/package${RPI_APP_PREFIX}" ]]; then
    echo "missing app package; run scripts/rpi-build-app.sh first" >&2
    exit 1
fi

rsync -az --delete "${RPI_QT_TARGET}/" "${RPI_SSH}:/tmp/qt6-rpi/"
rsync -az --delete "${SDK_ROOT}/package${RPI_APP_PREFIX}/" "${RPI_SSH}:/tmp/qtcarplay/"

ssh "${RPI_SSH}" "sudo mkdir -p '${RPI_QT_PREFIX}' '${RPI_APP_PREFIX}' && \
    sudo rsync -a --delete /tmp/qt6-rpi/ '${RPI_QT_PREFIX}/' && \
    sudo rsync -a --delete /tmp/qtcarplay/ '${RPI_APP_PREFIX}/' && \
    sudo tee '${RPI_APP_PREFIX}/run-qtcarplay.sh' >/dev/null && \
    sudo chmod +x '${RPI_APP_PREFIX}/run-qtcarplay.sh'" <<REMOTE
#!/usr/bin/env bash
set -euo pipefail

export QT_ROOT="${RPI_QT_PREFIX}"
export LD_LIBRARY_PATH="\${QT_ROOT}/lib:\${LD_LIBRARY_PATH:-}"
export PATH="\${QT_ROOT}/bin:\${PATH}"
export QML_IMPORT_PATH="\${QT_ROOT}/qml"
export QT_PLUGIN_PATH="\${QT_ROOT}/plugins"
export QT_QPA_FONTDIR="\${QT_QPA_FONTDIR:-/usr/share/fonts/truetype/dejavu}"
export QT_QPA_PLATFORM="\${QT_QPA_PLATFORM:-eglfs}"
export QT_QPA_EGLFS_INTEGRATION="\${QT_QPA_EGLFS_INTEGRATION:-eglfs_kms}"

exec "${RPI_APP_PREFIX}/bin/appQtCarplay" "\$@"
REMOTE

ssh "${RPI_SSH}" "sudo tee /etc/systemd/system/qtcarplay.service >/dev/null && sudo systemctl daemon-reload" <<REMOTE
[Unit]
Description=Qt CarPlay
After=multi-user.target

[Service]
User=${RPI_USER}
SupplementaryGroups=audio video render input plugdev
Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_INTEGRATION=eglfs_kms
ExecStart=${RPI_APP_PREFIX}/run-qtcarplay.sh
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
REMOTE

ssh "${RPI_SSH}" "sudo tee /etc/udev/rules.d/99-qtcarplay-dongle.rules >/dev/null && sudo udevadm control --reload-rules && sudo udevadm trigger" <<'REMOTE'
SUBSYSTEM=="usb", ATTR{idVendor}=="1314", ATTR{idProduct}=="1520", MODE="0660", GROUP="plugdev", TAG+="uaccess"
SUBSYSTEM=="usb", ATTR{idVendor}=="1314", ATTR{idProduct}=="1521", MODE="0660", GROUP="plugdev", TAG+="uaccess"
REMOTE
