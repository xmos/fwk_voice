# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from __future__ import print_function
import sys
import xml.etree.ElementTree as ET


def main(show_arrays):
    filename = 'pytest_result.xml'
    tree = ET.parse(filename)
    properties = tree.findall(".//testcase/properties")

    for result in properties:
        for p in result.findall("property"):
            if p.get('name') == "suppression_arr" and not show_arrays:
                continue
            else:
                print("{}: {}".format(p.get('name'), p.get('value')))
        print('')


if __name__ == "__main__":
    show_arrays = False
    try:
        if sys.argv[1] == "--all":
            show_arrays = True
    except IndexError:
        pass
    main(show_arrays)
