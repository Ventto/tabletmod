#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
File: filter_accels.py
Author: Thomas "Ventto" Venriès
Email: thomas.venries@gmail.com
Github: https://github.com/Ventto
Description:
    Apply a high pass filter to the data recorded by the `record_accels.py`
    script
"""

import csv
import os
import signal
import sys

SEPARATOR_EUREQA = ' '

def high_pass_filter(vec_last, vec):
    """
    High pass filter apply to a XYZ vector

    :vec_last: previous vector recorded (and filtered)
    :vec: current vector on which we apply the filter

    :returns: XYZ vector
    """
    # Ramp-speed - play with this value until satisfied
    filter_factor = 0.1
    # High-pass filter to eliminate gravity
    vec[0] = vec[0] * filter_factor + vec_last[0] * (1.0 - filter_factor)
    vec[1] = vec[1] * filter_factor + vec_last[1] * (1.0 - filter_factor)
    vec[2] = vec[2] * filter_factor + vec_last[2] * (1.0 - filter_factor)
    return vec

def apply_filter_to_accels_data(accels_last, accels):
    """
    Apply high pass filter on both accelerometers data from the CSV file

    :accels_last: last stored data from both accelerometers
                  [x1, y1, z1, x2, y2, z2, ...]
    :accels: current data from both accelerometers

    :returns: filtered accels
    """
    if not accels_last:
        return accels
    accel1_last = high_pass_filter(accels_last[0:3], accels[0:3])
    accel2_last = high_pass_filter(accels_last[3:6], accels[3:6])
    return accel1_last + accel2_last + [accels[6]]

def print_accel_data_to_csv(data, sep=SEPARATOR_EUREQA):
    """
    Print the accelerometers's data

    :data: data from both accelerometers
    :sep: Data separator for the printed output
    """

    round_val = 0
    data = list(map(lambda val: round(val, round_val), data))
    for val in data:
        print(val, end=sep)
    print()

def apply_filter_to_csv(filepath, has_header, sep=SEPARATOR_EUREQA):
    """
    Print the filtered accelerometers data

    Sample from a CSV file:

    ```
    ts_ax;ts_ay;ts_az;kb_ax;kb_ay;kb_az;is_tablet
    -106;15;-504;18;14;-547;0
    ```

    :filepath: CSV file path
    :has_header: True if the CSV file starts with a header row
    :sep: Data separator for the printed output
    """
    with open(filepath) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=SEPARATOR_EUREQA)
        accels_last = []
        for row in csv_reader:
            if has_header:
                has_header = False
                continue

            accels_data = list(map(float, row))
            accels_last = apply_filter_to_accels_data(accels_last, accels_data)
            print_accel_data_to_csv(accels_last, sep)

def usage(progpath):
    """
    Print the usage of the script

    :progpath: sys.argv[0]
    """

    print("Usage:", os.path.basename(progpath), "ACCEL_CAPTURE_CSV HAS_HEADER")
    print()

def checkargs(argv):
    """
    Check the script's arguments
    """

    if len(argv) != 3:
        usage(argv[0])
        return 2

    if not os.path.exists(argv[1]):
        print(argv[1], ":", "path does not exist")
        return 1

    has_header = argv[2]
    if has_header not in ("true", "false"):
        usage(argv[0])
        return 2

    return 0

def main(argv):
    """
    Entry point of the script
    """

    signal.signal(signal.SIGINT, lambda sig, frame: sys.exit(1))

    ret = checkargs(argv)
    if ret != 0:
        return ret

    _has_header = argv[2] == "true"
    _csv_file = argv[1]

    apply_filter_to_csv(_csv_file, _has_header)

    return 0

if __name__ == "__main__":
    main(sys.argv)
