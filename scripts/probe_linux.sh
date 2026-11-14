#!/usr/bin/env bash
set -euo pipefail
echo '=== architecture ==='
uname -m
echo '=== compiler ==='
g++ --version
echo '=== Qt ==='
qmake -query QT_VERSION
echo '=== OpenCV pkg-config ==='
pkg-config --modversion opencv4
pkg-config --cflags --libs opencv4
echo '=== cameras ==='
if command -v v4l2-ctl >/dev/null; then v4l2-ctl --list-devices; else echo 'v4l2-ctl is unavailable'; fi
echo 'Dependency discovery only; this does not certify runtime compatibility.'
