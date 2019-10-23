#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: calculate_tilts.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
    Caculate and print the triple-axis tilts of two accelerometers
    as Linux kernel's IIO devices
"""

import signal
import os
import sys
import math
from columnar import columnar

IIO_DEVICES_SYSPATH = "/sys/bus/iio/devices"

def dotproduct(vec1, vec2):
    """Dot production of two vectors

    @param vec1:  a 3-cells array as vector
    @param vec2:  a 3-cells array as vector

    @return:  float
    """
    return sum((a*b) for a, b in zip(vec1, vec2))

def length(vec):
    """Get the length of a vector

    @param vec:  a 3-cells array as vector

    @return:  float
    """

    return math.sqrt(dotproduct(vec, vec))

def angle(vec1, vec2):
    """Get the angle between two vectors

    @param vec1:  a 3-cells array as vector
    @param vec2:  a 3-cells array as vector

    @return:  float
    """

    return math.acos(dotproduct(vec1, vec2) / (length(vec1) * length(vec2)))

def get_coordinates(device_name):
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

def usage():
    """
    Usage
    """

    print("Usage:", os.path.basename(sys.argv[0]), "IIO_DEV [IIO_DEVS...]\n")
    print("Argument:\n")
    print("  IIO_DEV    accelerometer's name as IIO device located into")
    print("             the \"/sys/bus/iio/devices/\" directory.\n")

def checkargs(argv):
    """
    Checks on command line arguments

    :returns: 0: on success
              1: internal error
              2: bad args
    """

    if len(argv) < 2:
        usage()
        return 2

    for name in argv[1:]:
        if not os.path.exists(IIO_DEVICES_SYSPATH + "/" + name):
            print(name, ": IIO device (or accelerometer) not found")
            return 1
    return 0

def main():
    """
    Entry point of the script
    """

    signal.signal(signal.SIGINT, lambda sig, frame: sys.exit(0))

    ret = checkargs(sys.argv)
    if ret != 0:
        sys.exit(ret)

    iio_list = sys.argv[1:]

    for accel_name in iio_list:
        vec = get_coordinates(accel_name)
        vec_str = [list(map(lambda rot: str(rot), vec))]

        axis_rots = [yaw(vec), pitch(vec), roll(vec)]
        axis_rots = list(map(lambda rot: round(rot * 180 / math.pi, 2), axis_rots))
        axis_rots_strs = [list(map(lambda rot: str(rot) + "°", axis_rots))]

        print("| ", accel_name.upper())
        headers = ["Ax", "Ay", "Az"]
        print(columnar(vec_str, headers, max_column_width=18,
                       min_column_width=18)[:-1])
        headers = ["roll", "pitch", "yaw"]
        print(columnar(axis_rots_strs, headers, max_column_width=18,
                       min_column_width=18))

if __name__ == "__main__":
    main()
