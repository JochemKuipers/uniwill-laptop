#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER=1.0.4
docker run --rm -v "$ROOT:/src:ro" -v "$ROOT/dist:/out" -w /tmp fedora:42 bash -lc "
set -euo pipefail
dnf install -y rpm-build cmake gcc-c++ qt6-qtbase-devel desktop-file-utils systemd-rpm-macros make tar gzip
mkdir -p /root/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
tar -C /src --exclude=.git --exclude=dist --exclude=build --exclude=control/build --exclude=debian/build-control \
	-czf /root/rpmbuild/SOURCES/uniwill-laptop-${VER}.tar.gz --transform 's,^,uniwill-laptop-${VER}/,' .
cp /src/packaging/uniwill-laptop.spec /root/rpmbuild/SPECS/
rpmbuild -ba /root/rpmbuild/SPECS/uniwill-laptop.spec
cp /root/rpmbuild/RPMS/*/*.rpm /out/
cp /root/rpmbuild/SRPMS/*.rpm /out/
ls -l /out/*.rpm
"
