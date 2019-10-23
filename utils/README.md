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

Read from both `iio:device0` and `iio:device1` accelerometers (located into the `/sys/bus/iio/devices/` sysfs directory) whose the angle between the screen and the keyboard  is `0` degree.

Run the script as following and press `<Enter>` key to read from accelerometers:

```
$ sudo python3 ./read_from_accels.py iio:device0 iio:device1 0 2>/dev/null
```

> `sudo` is required because the script listens to the keyboard's events.
> We print both on *stdout* and *stderr* so to avoid duplicating output,
> redirect for instance the *stderr* to `/dev/null`.


`sudo` is not required if you decide to set the main loop's delay (as below `0.5 sec`):

```
$ python3 ./read_from_accels.py iio:device0 iio:device1 0 0.5 2>/dev/null
```

# Tilts

This section deals with calculating tilts (roll, pitch and yaw)
from the accelerometers.

Run the `calculate_tilts.py` script as following:

```
$ sudo python3 ./calculate_tilts.py iio:device0

| IIO:DEVICE0
|------------------|------------------|------------------|
|Ax                |Ay                |Az                |
|========================================================|
|-94               |21                |-501              |
|------------------|------------------|------------------|
|------------------|------------------|------------------|
|roll              |pitch             |yaw               |
|========================================================|
|-10.88°           |2.36°             |-10.62°           |
|------------------|------------------|------------------|

```

# Tablet Mode Detection

This section deals with detecting the tablet mode based on the Eureqa formulas
or **empirical thresholds** (what we have chosen).


Run the `detect_tabletmode.py` script as following and press `<Enter>` key to print the detection's result:

```
$ sudo python3 ./detect_tabletmode.py iio:device0 iio:device1

TOUCHSCREEN         KEYBOARD         DISTANCES       ISTABLET

[-98, 36, -498]     [3, 7, -551]     [101, 29, 53]   True
```

> `sudo` is required because the script listens to the keyboard's events.

`sudo` is not required if you decide to set the main loop's delay (as below `0.1 sec`):

```
$ python3 ./detect_tabletmode.py iio:device0 iio:device1 0.1
```

# Smooth

Run the `detect_tabletmode.py` script as following (`true` means "CSV headers present"):

```
$ python3 ./smooth_data.py output_from_read_script.csv true
```
