#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: vec3d_angle.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
"""

import signal
import time
import os
import sys
import math

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

def roll(vec):
    """Get the Roll angle of a given vector

    :vec: 3-cells array
    :returns: float as angle in radian

    """

    return math.atan(vec[0] / math.sqrt(math.pow(vec[1], 2)
                                        + math.pow(vec[2], 2)))

def pitch(vec):
    """Get the Pitch angle of a given vector

    :vec: 3-cells array
    :returns: float as angle in radian

    """

    return math.atan(vec[1] / math.sqrt(math.pow(vec[0], 2)
                                        + math.pow(vec[2], 2)))

def yaw(vec):
    """Get the Yaw angle of a given vector

    :vec: 3-cells array
    :returns: float as angle in radian

    """

    return math.atan(math.sqrt(math.pow(vec[0], 2)
                               + math.pow(vec[1], 2)) / vec[2])

def to_titl_vec(gproj_vec):
    """ Returns titl angles (in radian) vector from the gravity's projection
        vector

    :gproj_vec: projection of the gravity vector on the axes

    :returns: vector of triple tilt angles in radian
    """

    return [roll(gproj_vec), pitch(gproj_vec), yaw(gproj_vec)]

def to_titl_indegree(vec):
    """ Converts a triple-tilt angle vector from radian to degree

    :vec: triple-tilt angle vector in radian

    :returns: vector of triple tilt angles in degree
    """
    return [round(roll(vec)  * (180 / math.pi), 2),
            round(pitch(vec) * (180 / math.pi), 2),
            round(yaw(vec)   * (180 / math.pi), 2)]

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

    rot_ts = to_titl_vec(ang_ts)
    rot_ts = to_titl_indegree(rot_ts)

    rot_kb = to_titl_vec(ang_kb)
    rot_kb = to_titl_indegree(rot_kb)

    print(*ang_ts, sep=";", end=";")
    print(*rot_ts, sep=";", end=";")
    print(*ang_kb, sep=";", end=";")
    print(*rot_kb, sep=";", end=";")
    print(int(round(time.time() * 1000)))

def usage(progpath):
    """
    Print the usage of the script
    """

    print("Usage:", os.path.basename(progpath), "IIO_DEVICE1 IIO_DEVICE2")
    print("\nArguments:\n  IIO_DEVICE1\tkeyboard's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")
    print("  IIO_DEVICE2\ttouchscreen's iio device name located ")
    print("\t\tinto the `/sys/bus/iio/devices` directory.")

def checkargs(argv):
    """
    Check the script's arguments
    """

    if len(argv) != 3:
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

    iio_tp, iio_kb = argv[1], argv[2]

    print("ts_ax;ts_ay;ts_az;ts_rx;ts_ry;ts_rz;kb_ax;kb_ay;kb_az;kb_rx;kb_ry;kb_rz;timestamp")
    for _ in range(10):
        print_accels_data(iio_tp, iio_kb)

    return 0

if __name__ == "__main__":
    main(sys.argv)
