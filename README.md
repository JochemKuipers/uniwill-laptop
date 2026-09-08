# uniwill-laptop
Linux kernel driver for various Uniwill laptops. The driver itself should be shipped with the mainline kernel
starting with kernel version 6.19. This repository mainly contains experimental features for testers to test.

## Packages

Binaries for **v1.0** are on [GitHub Releases](https://github.com/JochemKuipers/uniwill-laptop/releases/tag/v1.0) (Debian `.deb`, Fedora `.rpm`, and source RPM).

### Debian / PikaOS

```sh
curl -fsSL https://jochemkuipers.github.io/apt-repo/jochem.sources \
  | sudo tee /etc/apt/sources.list.d/jochem.sources
sudo apt update
sudo apt install uniwill-laptop
```

That metapackage pulls in the DKMS driver and the Qt control app.

### Arch Linux (AUR)

```sh
yay -S uniwill-laptop-dkms uniwill-control
```

### Fedora

Install the RPMs from the [v1.0 release](https://github.com/JochemKuipers/uniwill-laptop/releases/tag/v1.0):

```sh
sudo dnf install ./uniwill-laptop-dkms-1.0.1-1.fc42.noarch.rpm \
  ./uniwill-laptop-control-1.0.1-1.fc42.x86_64.rpm
```

## From source
You can build the kernel modules by simply executing `make`. Keep in mind that you need a recent enough linux kernel (>= 7.2.0)
and the linux kernel headers installed.

You can then load the kernel modules by executing `insmod uniwill-laptop.ko` with superuser privileges.

## Development

This driver is based on [qc71_laptop](https://github.com/pobrn/qc71_laptop) and [tuxedo-driver](https://github.com/tuxedocomputers/tuxedo-drivers).
All knowledge was retrieved using reverse engineering, so be careful when testing this driver!
