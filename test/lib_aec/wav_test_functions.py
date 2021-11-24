import numpy as np

def disco_check(h, phases, frame_advance):
    ''' discontinuity checker
    We expect the samples at the end of the frame to be a similar magnitude to those in the middle
    If the samples at the edges have a much larger magnitude, this indicates that there are likely 
    discontinuities at the frame boundaries.
    '''
    edge_ratio = np.zeros(phases-1)
    for p in range(phases-1):
        edge = frame_advance*(p+1) # filter samples on frame edges
        mid = int(frame_advance*(p+0.5)) # filter samples in middle of frame
        edge_ratio[p] = np.mean(np.abs(h[edge-1:edge+2]))/np.mean(np.abs(h[mid-1:mid+2]))

    if np.mean(edge_ratio) > 5:
        print("Failed discontinuity check, score %f"%np.mean(edge_ratio))
        return False
    else:
        print("Passed discontinuity check, score %f"%np.mean(edge_ratio))
        return True


def deconverge_check(in_leq, out_leq):
    ''' deconvergence checker
    Check that there is some attenuation of the input signal
    '''
    atten = out_leq[-1] - in_leq[-1]
    if atten > -10:
        print("Failed deconvergence check, atten %f dB"%atten)
        return False
    else:
        print("Passed deconvergence check, atten %f dB"%atten)
        return True


def calc_attenuation_time(time, output, target_attenuation):
    attenuation = output - output[0]
    target_idx = np.argmax(attenuation < target_attenuation)
    atten_time = time[target_idx]

    print("Time to %d dB attenuation is %f s"%(target_attenuation, atten_time))
    return atten_time


def calc_convergence_rate(time, output):

    idx_2 = np.searchsorted(time, 2)
    convergence_rate = (output[0]-output[idx_2])/2.0

    print("Convergence rate is %f dB/s"%convergence_rate)
    return convergence_rate


def calc_max_attenuation(output):
    attenuation = output - output[0]
    max_atten = np.min(attenuation)
    print("Max attenuation is %f dB"%(max_atten))
    return max_atten

def calc_atten_difference(output1, output2, start_ind, stop_ind):
    atten1 = output1 - output1[0]
    atten2 = output2 - output2[0]
    diff = np.mean(abs(atten1[start_ind:stop_ind] - atten2[start_ind:stop_ind]))
    print("Average attenuation difference is %f dB"%(diff))
    return diff


def calc_deconvergence(output, fs, stop_adapt_samples, restart_adapt_samples):
    attenuation = output - output[0]
    win_len = int(fs * 0.05)
    in_ind = stop_adapt_samples // win_len - 2
    out_ind = restart_adapt_samples // win_len + 2
    in_atten = attenuation[in_ind]
    out_atten = attenuation[out_ind]
    atten_diff = abs(in_atten - out_atten)
    atten_percent = atten_diff/abs(in_atten)*100
    print('Deconvergence is %f dB'%(atten_diff))
    return atten_diff, atten_percent


def leq(x):
    return 10 * np.log10(np.mean(x ** 2.0))

def leq_smooth(x, fs, T):
    len_x = x.shape[0]
    win_len = int(fs * T)
    win_count = len_x // win_len
    len_y = win_len * win_count

    y = np.reshape(x[:len_y], (win_len, win_count), 'F')

    leq = 10 * np.log10(np.mean(y ** 2.0, axis=0))
    t = np.arange(win_count) * T

    return t, leq

def make_impulse(RT, t=None, fs=None):
    scale = 0.005
    scale_noise = 0.00005
    a = 3.0 * np.log(10.0) / RT
    if t is None:
        t = np.arange(2.0*RT*fs) / fs
    N = t.shape[0]
    h = np.zeros(N)
    e = np.exp(-a*t)
    reflections = N // 100
    reflection_index = np.random.randint(N, size=reflections)
    for n, idx in enumerate(reflection_index):
        if n % 2 == 0:
            flip = 1
        else:
            flip = -1
        h[idx] = flip * scale * t[idx] * e[idx]
    h += scale_noise * np.random.randn(t.shape[0]) * e
    return h
