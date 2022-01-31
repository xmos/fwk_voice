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

  num_fields = 8
  deinterleaved = [logs[i::num_fields] for i in range(num_fields)]
  ov_Error = np.array(deinterleaved[0], dtype=np.float64)
  ov_Error_shad = np.array(deinterleaved[1], dtype=np.float64)
  ov_Y = np.array(deinterleaved[2], dtype=np.float64)
  shadow_flag = np.array(deinterleaved[3], dtype=np.int32)
  shadow_reset_count = np.array(deinterleaved[4], dtype=np.int32)
  shadow_better_count = np.array(deinterleaved[5], dtype=np.int32)
  ema_error = np.array(deinterleaved[6], dtype=np.float64)
  ema_y = np.array(deinterleaved[7], dtype=np.float64)

  erle_ratio = get_erle_ratio(ema_y, ema_error)
  print(erle_ratio[:100])

  print(shadow_flag[:50])
  print(shadow_better_count[:50])

  fig, ax = plt.subplots()
  ax.plot([i for i in range(len(erle_ratio))], erle_ratio)
  plt.show()

if __name__ == "__main__":
    test()


