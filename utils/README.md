Scripts for Accelerometers
==========================

*"Python scripts which helpt to develop a Linux kernel tablet-mode module driver"*

# Introduction

Python scripts:

* `calculate_tilts.py`: caculates the tilt rotations (roll, pitch, yaw) of a given accelerometer
* `detect_tabletmode.py`: detects the tablet mode from two given accelerometers (touchscreen, keyboard)
* `smooth_data.py`: smooths data recorded into a *.csv* file
* `read_from_accels.py`: reads data from given accelerometers

Shell script:

* `convert_data_to_eureqa.sh`: converts data recorded into a *.csv* file for [Eureqa](https://www.nutonian.com/products/eureqa/) software

# Requirements

*python3* is required to run the scripts.

```
# pip3 install -r python/requirements.txt
```

# Read

Record every `0.5` second from both `iio:device0` and `iio:device1`
accelerometers located into the `/sys/bus/iio/devices/` sysfs directory
and whose the angle between the screen and the keyboard  is `0` degree:

```
$ ./record_accels.py iio:device0 iio:device1 0 0.5 2>/dev/null
```

> We print both on *stdout* and *stderr* so to avoid duplicating output, redirect for instance the *stderr* to `/dev/null`

# Tilts

```
$ FIXME
```

# Tablet Mode Detection

```
$ FIXME
```
