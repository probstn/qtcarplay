# Raspberry Pi 64-bit Lite cross build

This project builds on macOS Apple Silicon through a Debian Trixie Docker container. The container is `linux/arm64`, so it runs fast on an M1 Pro while still using the Raspberry Pi filesystem as the sysroot. The generated binaries target Raspberry Pi OS Lite 64-bit.

Verified current target on 2026-05-16:

- Raspberry Pi OS Lite, 64-bit
- Debian 13 `trixie`
- Download image kernel line: 6.12
- Release date 21 Apr 2026
- Qt source version used here: 6.11.0

After running `apt full-upgrade` on `haflinger.local` on 2026-05-16, the Pi rebooted into `6.18.29+rpt-rpi-v8`.

## 1. Fresh Raspberry Pi OS install

Use Raspberry Pi Imager 2.x or newer and choose:

- OS: Raspberry Pi OS Lite (64-bit)
- Hostname: `haflinger.local`
- Username: `haflinger`
- Enable SSH

Boot the Pi, then confirm that SSH works:

```bash
ssh haflinger@haflinger.local 'uname -m; . /etc/os-release; echo "$PRETTY_NAME"'
```

Expected architecture is `aarch64`.

## 2. Prepare the Pi

From the Mac:

```bash
./scripts/rpi-prepare-pi.sh
```

This updates Raspberry Pi OS, installs the target development packages needed by Qt and QtCarplay, adds FFmpeg headers for Qt Multimedia and the app decoder, adds a udev rule for the CarPlay USB dongle vendor ID `1314`, and adds the user to `audio`, `video`, `render`, `input`, and `plugdev`.

Log out and back in, or reboot the Pi after this step:

```bash
ssh haflinger@haflinger.local 'sudo reboot'
```

## 3. Build the Docker SDK image

```bash
./scripts/rpi-docker-build-image.sh
```

The image contains CMake, Ninja, Qt build dependencies, and the Debian `aarch64-linux-gnu` compiler.

## 4. Sync the Raspberry Pi sysroot

```bash
./scripts/rpi-sync-sysroot.sh
```

The sysroot is copied to `.rpi-sdk/sysroot`. This is intentionally copied from the actual Pi, not guessed from a package list, because cross-compiling Qt needs the target headers, libraries, CMake packages, and pkg-config files to match the board.

## 5. Build Qt

```bash
./scripts/rpi-build-qt.sh
```

This downloads Qt `6.11.0`, checks Qt's published MD5 file, builds the required host Qt tools for the Docker container, then builds target Qt into:

```text
.rpi-sdk/qt-rpi-6.11.0
```

Target install prefix on the Pi is:

```text
/usr/local/qt6
```

The target Qt build includes `qtbase`, `qtshadertools`, `qtdeclarative`, and `qtmultimedia`, with `eglfs`, `linuxfb`, OpenGL ES 2, and X11/XCB disabled. Optional Qt Quick 3D is explicitly disabled so Qt Multimedia does not require the host `balsam` Quick 3D tool.

The build defaults to two parallel jobs and disables Qt precompiled headers because Qt can otherwise exceed Docker Desktop's default memory while compiling large C++ translation units. The Docker image also forces `C.UTF-8` so Qt tools do not emit locale warnings. Override parallelism only if Docker has more memory available:

```bash
QT_BUILD_PARALLEL=4 ./scripts/rpi-build-qt.sh
```

## 6. Build QtCarplay

```bash
./scripts/rpi-build-app.sh
```

The app is staged under:

```text
.rpi-sdk/package/opt/qtcarplay
```

## 7. Deploy to the Pi

```bash
./scripts/rpi-deploy.sh
```

This syncs Qt to `/usr/local/qt6`, the app to `/opt/qtcarplay`, and installs a systemd unit named `qtcarplay.service`.

Manual run:

```bash
ssh haflinger@haflinger.local '/opt/qtcarplay/run-qtcarplay.sh'
```

Start on boot:

```bash
ssh haflinger@haflinger.local 'sudo systemctl enable --now qtcarplay.service'
```

Check logs:

```bash
ssh haflinger@haflinger.local 'journalctl -u qtcarplay.service -f'
```

## Notes

- Passwords are not stored in the repository or scripts. Use your SSH password interactively, or install an SSH key with `ssh-copy-id haflinger@haflinger.local`.
- For a headless Lite install with a directly attached display, `eglfs`/KMS is the intended Qt platform. To test without EGLFS, run with `QT_QPA_PLATFORM=linuxfb`.
- Re-run `./scripts/rpi-sync-sysroot.sh` after installing or upgrading target libraries on the Pi.
