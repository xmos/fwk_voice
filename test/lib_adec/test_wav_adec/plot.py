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

class log_fields:
    def __init__(self, name):
        self.name = name

def log_frames(start, end, c, py, xc):
    print(f"(erle_ratio Ov_Error, Ov_Error_shad, Ov_Input, shadow_reset_count, shadow_better_count, shadow_flag)")
    for i in range(start, end):
        print(f"frame {i}: c: ({'{:.3f}'.format(c.erle_ratio[i])}, {'{:.3f}'.format(c.ov_Error[i])}, {'{:.3f}'.format(c.ov_Error_shad[i])}, {'{:.3f}'.format(c.ov_Y[i])}, {c.shadow_reset_count[i]}, {c.shadow_better_count[i]}, {c.shadow_flag[i]})")
        print(f"frame {i}: py: ({'{:.3f}'.format(py.erle_ratio[i])}, {'{:.3f}'.format(py.ov_Error[i])}, {'{:.3f}'.format(py.ov_Error_shad[i])}, {'{:.3f}'.format(py.ov_Y[i])}, {py.shadow_reset_count[i]}, {py.shadow_better_count[i]}, {py.shadow_flag[i]})")
        print(f"frame {i}: xc: ({'{:.3f}'.format(xc.erle_ratio[i])})")
        print("\n")

def test():
    with open("debug_log.bin", 'r') as f:
        logs = np.array([float(l) for l in f.readlines()], dtype=float)

    num_fields_c = 9
    deinterleaved = [logs[i::num_fields_c] for i in range(num_fields_c)]
    c_logs = log_fields("c")
    py_logs = log_fields("py")
    xc_logs = log_fields("xc")
    
    #Read c output
    c_logs.ov_Error = np.array(deinterleaved[0], dtype=np.float64)
    c_logs.ov_Error_shad = np.array(deinterleaved[1], dtype=np.float64)
    c_logs.ov_Y = np.array(deinterleaved[2], dtype=np.float64)
    c_logs.shadow_flag = np.array(deinterleaved[3], dtype=np.int32)
    c_logs.shadow_reset_count = np.array(deinterleaved[4], dtype=np.int32)
    c_logs.shadow_better_count = np.array(deinterleaved[5], dtype=np.int32)
    c_logs.ema_error = np.array(deinterleaved[6], dtype=np.float64)
    c_logs.ema_y = np.array(deinterleaved[7], dtype=np.float64)
    c_logs.peak_to_avg_ratio = np.array(deinterleaved[8], dtype=np.float64)
    c_logs.erle_ratio = get_erle_ratio(c_logs.ema_y, c_logs.ema_error)

    #Read xc output
    num_fields_xc = 6
    l = np.array(np.loadtxt("xc_log.txt"), dtype=np.float64)
    deinterleaved = [l[i::num_fields_xc] for i in range(num_fields_xc)]
    xc_logs.ema_error = np.array(deinterleaved[0], dtype=np.float64)
    xc_logs.ema_y = np.array(deinterleaved[1], dtype=np.float64)
    xc_logs.shadow_flag = np.array(deinterleaved[2], dtype=np.int32)
    xc_logs.ov_Error = np.array(deinterleaved[3], dtype=np.float64)
    xc_logs.ov_Error_shad = np.array(deinterleaved[4], dtype=np.float64)
    xc_logs.ov_Y = np.array(deinterleaved[5], dtype=np.float64)
    xc_logs.erle_ratio = get_erle_ratio(xc_logs.ema_y, xc_logs.ema_error)

    #Read python output
    py_logs.ema_y = np.array(np.loadtxt("near_power_py.txt"), dtype=np.float64)
    py_logs.ema_error = np.array(np.loadtxt("aec_out_power_py.txt"), dtype=np.float64)
    py_logs.shadow_flag = np.array(np.loadtxt("shadow_flag_py.txt"), dtype=np.int32)
    py_logs.shadow_reset_count = np.array(np.loadtxt("shadow_reset_count_py.txt"), dtype=np.int32)
    py_logs.shadow_better_count = np.array(np.loadtxt("shadow_better_count_py.txt"), dtype=np.int32)
    py_logs.ov_Error = np.array(np.loadtxt("Ov_Error_py.txt"), dtype=np.float64)
    py_logs.ov_Error_shad = np.array(np.loadtxt("Ov_Error_shad_py.txt"), dtype=np.float64)
    py_logs.ov_Y = np.array(np.loadtxt("Ov_Input_py.txt"), dtype=np.float64)
    py_logs.peak_to_avg_ratio = np.array(np.loadtxt("peak_to_average_ratio_py.txt"), dtype=np.float64) 
    py_logs.erle_ratio = get_erle_ratio(py_logs.ema_y, py_logs.ema_error)

    fig, axes = plt.subplots(2,4)
    axes[0,0].set_title("erle_ratio")
    axes[0,0].set_ylim(0,7)
    axes[0,0].plot([i for i in range(len(c_logs.erle_ratio))], c_logs.erle_ratio[:], color='b')
    axes[0,0].plot([i for i in range(len(py_logs.erle_ratio))], py_logs.erle_ratio[:], color='g')
    axes[0,0].plot([i for i in range(len(xc_logs.erle_ratio))], xc_logs.erle_ratio[:], 'm--')

    axes[0,1].set_title("ov_Error_shad")
    axes[0,1].plot([i for i in range(len(c_logs.ov_Error_shad))], c_logs.ov_Error_shad, color='b')
    axes[0,1].plot([i for i in range(len(py_logs.ov_Error_shad))], py_logs.ov_Error_shad, color='g')
    axes[0,1].plot([i for i in range(len(xc_logs.ov_Error_shad))], xc_logs.ov_Error_shad, 'm--')

    axes[0,2].set_title("ov_Error")
    axes[0,2].plot([i for i in range(len(c_logs.ov_Error))], c_logs.ov_Error, color='b')
    axes[0,2].plot([i for i in range(len(py_logs.ov_Error))], py_logs.ov_Error, color='g')
    axes[0,2].plot([i for i in range(len(xc_logs.ov_Error))], xc_logs.ov_Error, 'm--')

    axes[0,3].set_title("ov_Y")
    axes[0,3].plot([i for i in range(len(c_logs.ov_Y))], c_logs.ov_Y, color='b')
    axes[0,3].plot([i for i in range(len(py_logs.ov_Y))], py_logs.ov_Y, color='g')
    axes[0,3].plot([i for i in range(len(xc_logs.ov_Y))], xc_logs.ov_Y, 'm--')

    axes[1,0].set_title("shadow_better_count")
    axes[1,0].plot([i for i in range(len(c_logs.shadow_better_count))], c_logs.shadow_better_count, color='b')
    axes[1,0].plot([i for i in range(len(py_logs.shadow_better_count))], py_logs.shadow_better_count, color='g')

    axes[1,1].set_title("shadow_reset_count")
    axes[1,1].plot([i for i in range(len(c_logs.shadow_reset_count))], c_logs.shadow_reset_count, color='b')
    axes[1,1].plot([i for i in range(len(py_logs.shadow_reset_count))], py_logs.shadow_reset_count, color='g')

    axes[1,2].set_title("shadow_flag")
    axes[1,2].plot([i for i in range(len(c_logs.shadow_flag))], c_logs.shadow_flag, 'b')
    axes[1,2].plot([i for i in range(len(py_logs.shadow_flag))], py_logs.shadow_flag, 'g')
    axes[1,2].plot([i for i in range(len(xc_logs.shadow_flag))], xc_logs.shadow_flag, 'm--')

    axes[1,3].set_title("peak_to_avg_ratio")
    axes[1,3].plot([i for i in range(len(c_logs.peak_to_avg_ratio))], c_logs.peak_to_avg_ratio, color='b')
    axes[1,3].plot([i for i in range(len(py_logs.peak_to_avg_ratio))], py_logs.peak_to_avg_ratio, color='g')

    log_frames(0, 100, c_logs, py_logs, xc_logs)
    plt.show()

if __name__ == "__main__":
    test()


