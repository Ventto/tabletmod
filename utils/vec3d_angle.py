#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: vec3d_angle.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
"""

import math

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

def main():
    """
    Entry point of the script
    """

    vec_tp = get_coordinates("iio:device0")
    vec_kb = get_coordinates("iio:device1")

    print("vec_tp : ", vec_tp)
    print("vec_kb : ", vec_kb)
    print("angle : ", (angle(vec_tp, vec_kb) * 180 / math.pi))

if __name__ == "__main__":
    main()
