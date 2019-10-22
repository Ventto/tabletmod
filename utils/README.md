Utils
=====

*"Scripts which helpt to develop the kernel tablet-mode module"*

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

This section deals with reading raw values from accelerometers.

Record every `0.5` second from both `iio:device0` and `iio:device1`
accelerometers located into the `/sys/bus/iio/devices/` sysfs directory
and whose the angle between the screen and the keyboard  is `0` degree:

```
$ ./read_from_accels.py iio:device0 iio:device1 0 0.5 2>/dev/null
```

> We print both on *stdout* and *stderr* so to avoid duplicating output, redirect for instance the *stderr* to `/dev/null`

# Tilts

This section deals with calculating tilts (roll, pitch and yaw)
from the accelerometers.

Run the `calculate_tilts.py` script as following:

```
$ ./calculate_tilts.py iio:device0

 =  iio:device0  =
|-----|-----|-----|
|Ax   |Ay   |Az   |
|=================|
|1    |2    |3    |
|-----|-----|-----|

|-----|------|-----|
|roll |pitch |yaw  |
|==================|
|36.7°|32.31°|15.5°|
|-----|------|-----|
```

# Tablet Mode Detection

This section deals with detecting the tablet mode based on the Eureqa formulas
or empirical thresholds.


Run the `detect_tabletmode.py` script as following:

```
$ ./detect_tabletmode.py [OPTIONS...]
```
