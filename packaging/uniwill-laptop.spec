Name:           uniwill-laptop
Version:        1.0.3
Release:        1%{?dist}
Summary:        Uniwill / Medion ERAZER laptop extras
License:        GPL-2.0-only
URL:            https://github.com/JochemKuipers/uniwill-laptop
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  desktop-file-utils
Requires:       %{name}-dkms
Requires:       %{name}-control

%description
Metapackage for the Uniwill laptop DKMS driver and Qt control app.

%package dkms
Summary:        DKMS driver for Uniwill / Medion ERAZER laptops
Requires:       dkms
BuildArch:      noarch

%description dkms
Out-of-tree uniwill-laptop kernel module with a DMI match for the
MEDION ERAZER Major 15 X1.

%package control
Summary:        Qt control app for Uniwill / Medion ERAZER laptops
Requires:       %{name}-dkms
Requires:       qt6-qtbase

%description control
Desktop app to set keyboard RGB, the front LED strip, Fn lock, USB-C
power split, extra GPU power, and to read temperatures and fans.
Also installs an autostart OSD for performance modes and hotkey feedback.

%prep
%autosetup

%build
cmake -S control -B build \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=%{_prefix} \
	-DCMAKE_INSTALL_LIBEXECDIR=%{_libexecdir}
cmake --build build %{?_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build
install -d %{buildroot}/usr/src/uniwill-laptop-1.0
install -m 644 Makefile dkms.conf uniwill-acpi.c uniwill-wmi.c uniwill-wmi.h \
	%{buildroot}/usr/src/uniwill-laptop-1.0/
install -D -m 644 uniwill-laptop.conf %{buildroot}/usr/lib/modules-load.d/uniwill-laptop.conf
install -D -m 644 99-uniwill-laptop.rules %{buildroot}/usr/lib/udev/rules.d/99-uniwill-laptop.rules

%pre dkms
getent group plugdev >/dev/null || groupadd -r plugdev >/dev/null 2>&1 || true

%post dkms
dkms add -m uniwill-laptop -v 1.0 >/dev/null 2>&1 || true
dkms build -m uniwill-laptop -v 1.0 >/dev/null 2>&1 || true
dkms install -m uniwill-laptop -v 1.0 >/dev/null 2>&1 || true
udevadm control --reload >/dev/null 2>&1 || true

%preun dkms
if [ "$1" = "0" ]; then
	dkms remove -m uniwill-laptop -v 1.0 --all >/dev/null 2>&1 || true
fi

%files

%files dkms
/usr/src/uniwill-laptop-1.0/
/usr/lib/modules-load.d/uniwill-laptop.conf
/usr/lib/udev/rules.d/99-uniwill-laptop.rules

%files control
%{_bindir}/uniwill-control
%{_datadir}/applications/uniwill-control.desktop
%{_datadir}/applications/uniwill-osd.desktop
/etc/xdg/autostart/uniwill-osd.desktop

%changelog
* Sat Oct 03 2026 Jochem Kuipers <jochem@kuipers.cc> - 1.0.3-1
- Add platform performance modes and Control Center-style OSD

* Tue Sep 08 2026 Jochem Kuipers <jochem@kuipers.cc> - 1.0.1-1
- Add CoolerControl-compatible fan PWM control

* Mon Sep 07 2026 Jochem Kuipers <jochem@kuipers.cc> - 1.0-1
- Initial package for MEDION ERAZER Major 15 X1
