#!/bin/sh
# Install out-of-tree uniwill-laptop via DKMS and autoload it.
set -eu

SRC="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
DEST=/usr/src/uniwill-laptop-1.0

rm -rf "$DEST"
mkdir -p "$DEST"
cp "$SRC/Makefile" "$SRC/dkms.conf" "$SRC/uniwill-acpi.c" "$SRC/uniwill-wmi.c" "$SRC/uniwill-wmi.h" "$DEST/"

if ! dkms status -m uniwill-laptop -v 1.0 | grep -q uniwill-laptop; then
	dkms add -m uniwill-laptop -v 1.0
fi
dkms build -m uniwill-laptop -v 1.0
dkms install -m uniwill-laptop -v 1.0 --force

cp "$SRC/uniwill-laptop.conf" /etc/modules-load.d/uniwill-laptop.conf
cp "$SRC/99-uniwill-laptop.rules" /etc/udev/rules.d/99-uniwill-laptop.rules
udevadm control --reload

if lsmod | grep -q '^uniwill_laptop'; then
	rmmod uniwill_laptop
fi
modprobe uniwill-laptop
udevadm trigger --action=add --subsystem-match=leds --subsystem-match=platform || true

plat=/sys/devices/platform/INOU0000:00
for f in fn_lock super_key_enable touchpad_toggle_enable rainbow_animation breathing_in_suspend ctgp_offset usb_c_power_priority ac_auto_boot usb_powershare_high; do
	if [ -e "$plat/$f" ]; then
		chgrp plugdev "$plat/$f"
		chmod 664 "$plat/$f"
	fi
done
for led in /sys/class/leds/uniwill:*; do
	for f in brightness multi_intensity; do
		if [ -e "$led/$f" ]; then
			chgrp plugdev "$led/$f"
			chmod 664 "$led/$f"
		fi
	done
done
