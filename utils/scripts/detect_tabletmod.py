#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: detect_tabletmod.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
    Detect the tablet mode from two triple-axis accelerometers's data
    as Linux kernel's IIO devices
"""

import signal
import time
import os
import sys
import math
import keyboard

RECORD_DATA_DELAY = 3
RECORD_DATA_WITH_KB_DELAY = 0.2
IIO_DEVICES_SYSPATH = "/sys/bus/iio/devices"

def get_data_from_accel(device_name):
    """Get coordinate from a given IIO device name (ex: iio:device0)
    The names are attributed by the Linux kernel.

    @param device_name:  Name of the IIO device

    @return:  3-cells array as vector
    """

    vec3 = []

    for axis in ["x", "y", "z"]:
        with open(IIO_DEVICES_SYSPATH + "/" + device_name + "/in_accel_" +
                  axis + "_raw", 'r') as reader:
            axis_data = int(reader.read())
            vec3.append(axis_data)

    return vec3


def detect_tabletmode_eureqa(ang_kb, ang_ts):
    """ Detect the tablet mode

        Implementation of the Eureqa formula
        istablet =
            49 * less(-34, kb_ax)
            + kb_ax * less(ts_ay, ts_az)
            + kb_az * less(-36, ts_ay + ts_az)
            - 89 - kb_az * less(ts_ay, -14)
            - kb_ax * less(ts_az, kb_az
                                    * less(kb_ax, kb_ax
                                                    * less(ts_ay, ts_az)))

    :ang_kb: [Ax; Ay; Az]
    :ang_ts: [Ax; Ay; Az]

    : returns: Boolean, True if the 2-in-1 laptop is in tablet mode
    """

    res = 49 * 1 if -34 < ang_kb[0] else 0
    res += ang_kb[0] * 1 if ang_ts[1] < ang_ts[2] else 0
    res += ang_kb[2] * 1 if -36 < (ang_ts[1] + ang_ts[2]) else 0
    res += - 89 - ang_kb[2] * 1 if ang_ts[1] < -14 else 0

    tmp = ang_kb[0] * 1 if ang_ts[1] < ang_ts[2] else 0
    tmp = ang_kb[2] * 1 if ang_kb[0] < tmp else 0
    tmp = -ang_kb[0] * 1 if ang_ts[2] < tmp else 0

    res += tmp

    return res > 0

def detect_tabletmode_thresholds(ang_kb, ang_ts):
    """ Detect the tablet mode

        Empirical implementation according to different rotations
        ex: XZ, YZ, ZXY

    :ang_kb: [Ax; Ay; Az]
    :ang_ts: [Ax; Ay; Az]

    : returns: Boolean, True if the 2-in-1 laptop is in tablet mode
    """
    ##
    # Touchscreen
    # XZ Rotation: forward
    if ang_ts[1] < 0 and ang_ts[2] < 500:
        return True
    # XY Rotations
    if ang_ts[1] < 320:
        ## XY Rotation: left or right
        if (ang_ts[0] > 380 or ang_ts[0] < -380):
            return True
    ## XYZ Rotation: forward-left
    if ang_ts[0] > -150 and ang_ts[1] < 250 and ang_ts[2] > 360:
        return True
    ##
    # Keyboard
    # XZ Rotation: forward
    if ang_kb[0] < 0 and ang_kb[2] > -400:
        return True
    ## XZ Rotation: backward
    if ang_kb[0] < 410 and ang_kb[2] > 280:
        return True
    ## YZ Rotation: right
    if ang_kb[1] > -445 and ang_kb[2] > -260:
        return True
    ## YZ Rotation: left
    if ang_kb[1] > 480 and ang_kb[2] > -230:
        return True
    ## XYZ Rotation: forward-left
    if ang_kb[0] > 220 and ang_kb[1] < -350 and ang_kb[2] > -335:
        return True
    return False

def print_accels_data(iio_ts, iio_kb):
    """ Print the angle values from two triple-axis accelerometers and
        a timestamp based on the system clock

    :iio_ts: iio device name of the keyboard's accelerometer located into
             the `/sys/bus/iio/devices` directory
    :iio_kb: iio device name of the touchscreen's accelerometer located into
             the `/sys/bus/iio/devices` directory
    """

    ang_kb = get_data_from_accel(iio_kb)
    ang_ts = get_data_from_accel(iio_ts)

    print("ts: (", end="")
    print(*ang_ts, sep=";", end="")
    print(") ", end="")
    print(" kb: (", end="")
    print(*ang_kb, sep=";", end="")
    print(") ", end="")
    print(detect_tabletmode_thresholds(ang_kb, ang_ts))

def usage(progpath):
    """
    Print the usage of the script
    """

    print("Usage:", os.path.basename(progpath), "IIO_DEVICE1 IIO_DEVICE2 [DELAY]")
    print("\nArguments:\n  IIO_DEVICE1\tkeyboard's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")
    print("  IIO_DEVICE2\ttouchscreen's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")
    print("  DELAY\t\tdelay in seconds between each record")

def checkargs(argv):
    """
    Check the script's arguments
    """

    if len(argv) < 3:
        usage(argv[0])
        return 2

    if len(argv) == 4:
        delay = argv[3]
        try:
            float(delay)
        except ValueError:
            usage(argv[0])
            return 2

    for i in range(1, 2):
        iiodev_path = IIO_DEVICES_SYSPATH + "/" + argv[i]
        if not os.path.exists(iiodev_path):
            print(iiodev_path, ":", "path does not exist")
            return 1
    return 0

def main(argv):
    """
    Entry point of the script
    """

    signal.signal(signal.SIGINT, lambda sig, frame: sys.exit(0))

    ret = checkargs(argv)
    if ret != 0:
        return ret

    _iio_tp = argv[1]
    _iio_kb = argv[2]
    _delay = RECORD_DATA_DELAY if len(argv) != 4 else float(argv[3])

    while True:
        print_accels_data(_iio_tp, _iio_kb)
        time.sleep(_delay)

    return 0

if __name__ == "__main__":
    main(sys.argv)
