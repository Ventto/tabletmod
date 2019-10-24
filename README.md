Tablet Mode
===========

*"External Kernel module for 2-in-1 hybrid laptop"*

It detects according to accelerometers's calibration datas, if the laptop is in tablet position.
Consequently, it disables the internal keyboard and trackpad.

# Build

## As External Module

Build the module with the following command:

```
$ make KDIR=<path/to/kernel/tree/source>
```

## As Internal Module

This is an example:

1. Copy this repository as module directory into the kernel source tree.
   Let's say into the `drivers/input/` subdirectory.

2. Integrate the *tabletmod*'s `Kconfig` file into those of the
   `drivers/input/` (c.f [Kconfig section](#Kconfig)).
   Add the following line in to the `drivers/input/Kconfig` file:

```
source "drivers/input/tabletmod/Kconfig"
```

3. Add this module to the building chain, by adding the following line to
   the `drivers/input/Makefile` file:

```
obj-$(CONFIG_TABLET_MODE) += tabletmod/
```

4. Go to the kernel tree's root and run the `make` command

## Kconfig

A `Kconfig` file is available if you would like to integrate this module with
the kernel's configuration tree structure
(c.f [Documentation/kbuild/kconfig-language.txt](https://www.kernel.org/doc/Documentation/kbuild/kconfig-language.txt)).

# Run

Once installed, you can run the module as following:
```
# modprobe tabletmod [debug=1]
```

The `debug` option enables the debugging messages.

# Development

## Add the module to initramfs


Add the module name with its options to the `/etc/initramfs-tools/modules` file with the following command:

```
# echo "tabletmod debug=1" > /etc/initramfs-tools/modules
```

On Debian system, update the current kernel's initramfs (c.f `uname -r`) as following:

```
# update-initramfs -u
```

> For futher information, read the [man page](http://manpages.ubuntu.com/manpages/precise/man8/update-initramfs.8.html).

## DMI-based module autoloading


### How to autoload this module for specific platforms ?
The idea is to load laptop drivers automatically (and other
drivers which cannot be autoloaded otherwise), based on the DMI system
identification information of the BIOS.
We use the `MODULE_ALIAS(_dmi_)` macro to autoload this module according to a given `_dmi_` [modalias](https://wiki.archlinux.org/index.php/Modalias).

Here is an extract from [tabletmod.c](tabletmod.c) where we use the product name (`pv`) and the board name (`rn`) to identify the "Ordissimo Julia 2" platform. The `*` wildcard card means it will match anything. Note that the information are seperated by `:` colon character:

```c
[...]
// Platform: Ordissimo Julia 2
MODULE_ALIAS("dmi:bvn*:bvr*:bd*:svn*:pnGeoFlex3:pvr*:rvn*:rnS133AR700:*");
[...]
```

This is a description of all the fields :

```
bvn: BIOS Vendor  | svn: System Vendor   | rvn: Board Vendor  | cvn: Chassis Vendor
bvr: BIOS Version | pn:  Product Name    | rn:  Board Name    | ct:  Chassis Type
bd:  BIOS Date    | pvr: Product Version | rvr: Board Version | cvr: Chassis Version
```

The following command prints the complete modalias to match the current platform:

```
$ cat /sys/devices/virtual/dmi/id/modalias
```

For instance, the following DMI modalias matchs all profiles leveraging the `*` wildcard character:

```
dmi:bvn*:bvr*:bd*:svn*:pn*:pvr*:rvn*:rn*:rvr*:cvn*:ct*:cvr*
```

### How to test a DMI modalias ?

Create a `.hwdb` file which will contain the modalias to test following with an arbitrary `test=ok` constant:

```
# echo "dmi:<my_modalias>\ntest=ok" > /lib/udev/hwdb.d/99-tabletmod.hwdb
```

Update the hardware database with the following command:

```
# systemd-hwdb update
```

Test the modalias :

```
# systemd-hwdb query 'dmi:<my_modalias>'
dmi:<my_modalias>
test=ok

# systemd-hwdb query 'dmi:<bad_modalias>'   (no output if no match)
```
