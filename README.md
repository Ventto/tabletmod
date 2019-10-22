Tablet Mode
===========

*"External Kernel module for 2-in-1 hybrid laptop"*

It detects according to accelerometers's calibration datas,
if the laptop is in tablet position.
Consequently, it disables the internal keyboard and trackpad.

# Build

## As External Module

Build the module with the following command:

```
$ make KDIR=<path/to/kernel/tree/source>
```

## As Internal Module

This is an example

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

# Kconfig

A `Kconfig` file is available if you would like to integrate this module with
the kernel's configuration tree structure
(c.f [Documentation/kbuild/kconfig-language.txt](https://www.kernel.org/doc/Documentation/kbuild/kconfig-language.txt)).
