#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
	build-essential debhelper cmake pkg-config qt6-base-dev dh-dkms dkms fakeroot
dpkg-buildpackage -us -uc -b
mkdir -p dist
mv -f ../uniwill-laptop_*.deb ../uniwill-laptop-dkms_*.deb ../uniwill-control_*.deb dist/ 2>/dev/null || true
mv -f ../*.buildinfo ../*.changes dist/ 2>/dev/null || true
ls -l dist/*.deb
