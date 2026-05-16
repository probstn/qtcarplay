#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/scripts/rpi-sdk-env.sh"

"${repo_root}/scripts/rpi-docker-run.sh" /work/scripts/rpi-build-app-in-docker.sh
