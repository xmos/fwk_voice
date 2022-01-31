import numpy as np
import matplotlib.pyplot as plt

def get_erle_ratio(input_energy, output_energy):
    eps = np.finfo(float).eps
    ratio = np.where(
        input_energy > 0.0,
        np.where(
            output_energy > 0.0,
            ((input_energy) / (output_energy)),
            1.0,
        ),
        0,
    )
    return ratio

def test():
    with open("debug_log.bin", 'r') as f:
        logs = np.array([float(l) for l in f.readlines()], dtype=float)

    num_fields = 9
    deinterleaved = [logs[i::num_fields] for i in range(num_fields)]
    ov_Error = np.array(deinterleaved[0], dtype=np.float64)
    ov_Error_shad = np.array(deinterleaved[1], dtype=np.float64)
    ov_Y = np.array(deinterleaved[2], dtype=np.float64)
    shadow_flag = np.array(deinterleaved[3], dtype=np.int32)
    shadow_reset_count = np.array(deinterleaved[4], dtype=np.int32)
    shadow_better_count = np.array(deinterleaved[5], dtype=np.int32)
    ema_error = np.array(deinterleaved[6], dtype=np.float64)
    ema_y = np.array(deinterleaved[7], dtype=np.float64)
    peak_to_avg_ratio = np.array(deinterleaved[8], dtype=np.float64)
    #print(f"shadow_better_count[110:130] = {shadow_better_count[110:130]}")
    #print(f"shadow_flag[110:130] = {shadow_flag[110:130]}")
    #print(f"ov_Error[110:130] = {ov_Error[110:130]}")
    #print(f"ov_Error_shad[110:130] = {ov_Error_shad[110:130]}")
    #print(f"ov_Y[110:130] = {ov_Y[110:130]}")
    erle_ratio = get_erle_ratio(ema_y, ema_error)

  

    #Read python output
    ema_y_py = np.array(np.loadtxt("near_power_py.txt"), dtype=np.float64)
    ema_error_py = np.array(np.loadtxt("aec_out_power_py.txt"), dtype=np.float64)
    shadow_flag_py = np.array(np.loadtxt("shadow_flag_py.txt"), dtype=np.int32)
    shadow_reset_count_py = np.array(np.loadtxt("shadow_reset_count_py.txt"), dtype=np.int32)
    shadow_better_count_py = np.array(np.loadtxt("shadow_better_count_py.txt"), dtype=np.int32)
    ov_Error_py = np.array(np.loadtxt("Ov_Error_py.txt"), dtype=np.float64)
    ov_Error_shad_py = np.array(np.loadtxt("Ov_Error_shad_py.txt"), dtype=np.float64)
    ov_Y_py = np.array(np.loadtxt("Ov_Input_py.txt"), dtype=np.float64)
    peak_to_avg_ratio_py = np.array(np.loadtxt("peak_to_average_ratio_py.txt"), dtype=np.float64)
    print(f"shadow_better_count_py[110:130] = {shadow_better_count_py[110:130]}")
    print(f"shadow_flag_py[110:130] = {shadow_flag_py[110:130]}")

   
    erle_ratio_py = get_erle_ratio(ema_y_py, ema_error_py)

    fig, axes = plt.subplots(2,4)
    axes[0,0].set_title("erle_ratio")
    axes[0,0].set_ylim(0,7)
    axes[0,0].plot([i for i in range(len(erle_ratio))], erle_ratio[:], color='b')
    axes[0,0].plot([i for i in range(len(erle_ratio_py))], erle_ratio_py[:], color='g')

    axes[0,1].set_title("ov_Error_shad")
    axes[0,1].plot([i for i in range(len(ov_Error_shad))], ov_Error_shad, color='b')
    axes[0,1].plot([i for i in range(len(ov_Error_shad_py))], ov_Error_shad_py, color='g')

    axes[0,2].set_title("ov_Error")
    axes[0,2].plot([i for i in range(len(ov_Error))], ov_Error, color='b')
    axes[0,2].plot([i for i in range(len(ov_Error_py))], ov_Error_py, color='g')

    axes[0,3].set_title("ov_Y")
    axes[0,3].plot([i for i in range(len(ov_Y))], ov_Y, color='b')
    axes[0,3].plot([i for i in range(len(ov_Y_py))], ov_Y_py, color='g')

    axes[1,0].set_title("shadow_better_count")
    axes[1,0].plot([i for i in range(len(shadow_better_count))], shadow_better_count, color='b')
    axes[1,0].plot([i for i in range(len(shadow_better_count_py))], shadow_better_count_py, color='g')

    axes[1,1].set_title("shadow_reset_count")
    axes[1,1].plot([i for i in range(len(shadow_reset_count))], shadow_reset_count, color='b')
    axes[1,1].plot([i for i in range(len(shadow_reset_count_py))], shadow_reset_count_py, color='g')

    axes[1,2].set_title("shadow_flag")
    axes[1,2].plot([i for i in range(len(shadow_flag))], shadow_flag, color='b')
    axes[1,2].plot([i for i in range(len(shadow_flag_py))], shadow_flag_py, color='g')

    axes[1,3].set_title("peak_to_avg_ratio")
    axes[1,3].plot([i for i in range(len(peak_to_avg_ratio))], peak_to_avg_ratio, color='b')
    axes[1,3].plot([i for i in range(len(peak_to_avg_ratio_py))], peak_to_avg_ratio_py, color='g')
    plt.show()

if __name__ == "__main__":
    test()


