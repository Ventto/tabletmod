#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: read_from_accels.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
    Record data from the triple-axis accelerometers
    as Linux kernel's IIO devices (iio:device0 and iio:device1)
"""

import signal
import time
import os
import sys
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

def print_accels_data(iio_ts, iio_kb):
    """ Print the angle values from two triple-axis accelerometers and
        a timestamp based on the system clock

    :iio_ts: iio device name of the keyboard's accelerometer located into
             the `/sys/bus/iio/devices` directory
    :iio_kb: iio device name of the touchscreen's accelerometer located into
             the `/sys/bus/iio/devices` directory
    """

    ang_ts = get_data_from_accel(iio_ts)
    ang_kb = get_data_from_accel(iio_kb)

    print(*ang_ts, sep=";", end=";")
    print(*ang_kb, sep=";", end=";")
    print(*ang_ts, file=sys.stderr, sep=";", end=";")
    print(*ang_kb, file=sys.stderr, sep=";", end=";")

def usage(progpath):
    """
    Print the usage of the script
    """

    print("Usage:", os.path.basename(progpath), "IIO_DEVICE1 IIO_DEVICE2 ANGLE [DELAY]")
    print("\nArguments:\n  IIO_DEVICE1\tkeyboard's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")
    print("  IIO_DEVICE2\ttouchscreen's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")
    print("  ANGLE\t\tangle in degree between the laptop's screen and keyboard,")
    print("\t\tit is an interger value.")
    print("  DELAY\t\tdelay in seconds between each record")

def checkargs(argv):
    """
    Check the script's arguments
    """

    if len(argv) < 4:
        usage(argv[0])
        return 2

    angle = argv[3]
    if not angle.isdigit() or (int(angle) < 0 or int(angle) > 360):
        usage(argv[0])
        return 2

    if len(argv) == 5:
        delay = argv[4]
        try:
            float(delay)
        except ValueError:
            usage(argv[0])
            return 2

    for i in range(1, 3):
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
    _angle = int(argv[3])
    _delay = RECORD_DATA_DELAY if len(argv) != 5 else float(argv[4])

    print("ts_ax;ts_ay;ts_az;kb_ax;kb_ay;kb_az;is_tablet")
    print("ts_ax;ts_ay;ts_az;kb_ax;kb_ay;kb_az;is_tablet", file=sys.stderr)
    while True:
        if _angle >= 20:
            keyboard.wait('end')

        print_accels_data(_iio_tp, _iio_kb)
        print(_angle)
        print(_angle, file=sys.stderr)

        if _angle < 20:
            time.sleep(_delay)

    return 0

if __name__ == "__main__":
    main(sys.argv)
