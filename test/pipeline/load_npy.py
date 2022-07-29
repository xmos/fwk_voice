import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import os

import argparse

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", nargs='?',
                        help=".npy file to load")
    parser.add_argument('--input2', type=str,
                        help="optional 2nd input .npy file")
    args = parser.parse_args()
    return args


args = parse_arguments()

input_dut = np.load(args.input)
input_ref = np.empty(0, dtype=np.float64)
if args.input2 != None:
    input_ref = np.load(args.input2)
    max_diff = np.max(np.abs(input_dut - input_ref))
    print(f"max_diff = {max_diff}")

#print(input_dut[:100])
#print(input_ref[:100])

plt.plot(input_dut, label="DUT")
if len(input_ref) != 0:
    plt.plot(input_ref, '--', label="REF")
plt.legend(loc="upper right")
fig = plt.gcf()
plt.show()
#plt.plot((input_dut - input_ref))
#plt.show()
name = Path(os.path.basename(args.input))
name = "plot_" + os.path.splitext(name)[0] + ".png"
fig.savefig(name)
