from dut_var import dut_var
import sys
import numpy as np
path_to_ref = '../../../../../lib_interference_canceller_develop/lib_interference_canceller/python/'
sys.path.insert(1, path_to_ref)



with open(path_to_ref + 'ref_var.npy', 'rb') as f:
    ref_var = np.load(f)


print(ref_var.shape)
print(dut_var.shape)

        
pr = 3

if len(ref_var.shape) == 3:
    for frame in range(ref_var.shape[0]):
        for phase in range(ref_var.shape[1]):
            print("ref:", ref_var[frame][phase][:pr])
            print("dut:", dut_var[frame][phase][:pr])
            print()

else:
    for frame in range(ref_var.shape[0]):
        print("ref:", ref_var[frame][:pr])
        print("dut:", dut_var[frame][:pr])
        print()