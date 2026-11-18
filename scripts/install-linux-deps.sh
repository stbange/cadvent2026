#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build dependencies for the Linux targets (Ubuntu 24.04 / Debian).
# SDL3 itself is fetched and built by CMake; these are its system backends
# plus Mesa (llvmpipe) for headless GPU tests.
set -euo pipefail
SUDO=""
if [ "$(id -u)" -ne 0 ]; then SUDO="sudo"; fi
$SUDO apt-get update
$SUDO apt-get install -y --no-install-recommends \
  build-essential clang cmake ninja-build pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev \
  libxfixes-dev libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols \
  libgl-dev libegl-dev libgles-dev libdrm-dev libgbm-dev \
  libasound2-dev libpulse-dev libdbus-1-dev libudev-dev \
  libgl1-mesa-dri libegl-mesa0 mesa-utils
