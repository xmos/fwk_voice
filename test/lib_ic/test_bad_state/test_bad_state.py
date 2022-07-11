import os
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pytest
import scipy.io.wavfile as wavfile

import acoustic_performance_tests.core as aptc
import IC
import room_acoustic_pipeline as rap
from room_acoustic_pipeline import helpers
from test_wav_ic import run_ic

sys.path.append('../../../../py_ic/tests/')
import ic_test_helpers as ith
sys.path.append('../../share/python/')
import py_vs_c_utils as pvc
import xtagctl
import xscope_fileio

# some mess to get the list of IRs
home = Path(os.environ.get('hydra_audio_PATH', '~/hydra_audio'), "acoustic_team_test_audio")

audio_dirs = ['speech', 'point_noise', 'playback_audio', 'ambient_noise']
audio_path = [home / ad for ad in audio_dirs]
audio_list = [helpers.files_of_type(ap, 'wav') for ap in audio_path]

imp_path = home / 'impulse'
imp_list = helpers.files_of_type(imp_path, 'npy')

# some possible parameters
gain = 50
noise_level = gain
noise_pos = 2
speech_name = "007_podcast"
speech_pos = 3

exe_dir = '../../../../build/test/lib_ic/test_bad_state/bin/'
xe = os.path.join(exe_dir, 'fwk_voice_test_bad_state.xe')

def run_xcore(conf_data, xe):
    conf_data.astype(np.int32).tofile('conf.bin')
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, xe)

@pytest.mark.parametrize("room", ["lab"])
@pytest.mark.parametrize("speech_level", [0])
@pytest.mark.parametrize("noise_name", ["006_Pink", "015_Silence"])
def test_bad_state(room, speech_level, noise_name):

    # some constants:
    fs = 16000
    length_secs = 10

    # load config
    conf_path = '../../shared/config/ic_conf_no_adapt_control.json'
    ic_conf = pvc.json_to_dict(conf_path)
    ic_conf["vnr_model"] = "../../modules/lib_vnr/python/model/model_output/model_qaware.tflite"
    delay = ic_conf["y_channel_delay"]
    phases = ic_conf["phases"]
    proc_frame_length = ic_conf["proc_frame_length"]
    frame_advance = ic_conf["frame_advance"]
    f_bin_count = (proc_frame_length // 2) + 1

    speech_snr = gain + speech_level

    # make room pipeline spec
    noise_spec, speech_spec, length_samps = aptc.make_rap_spec_ic(fs, room, 
                                                                noise_name, noise_level, noise_pos,
                                                                speech_name, speech_pos, speech_snr, 
                                                                length_secs)
    noise_ir = np.load(imp_path / imp_list[noise_spec[2]])
    speech_ir = np.load(imp_path / imp_list[speech_spec[2]])

    ideal_speech_cancellation_H = ith.calc_ideal_fd_filter(speech_ir, delay, phases, f_bin_count, proc_frame_length, frame_advance)
    ideal_noise_cancellation_H = ith.calc_ideal_fd_filter(noise_ir, delay, phases, f_bin_count, proc_frame_length, frame_advance)

    # run room pipeline
    mic_sig, out_array, in_array = rap.room_sim(utterance=speech_spec, 
                                                point_noise=noise_spec,
                                                signal_len=length_samps, 
                                                return_unsquashed=True, 
                                                return_signals=True)

    src_path = os.path.abspath('src/')
    prev_path = os.getcwd()
    os.chdir(src_path)
    wavfile.write('input.wav', fs, mic_sig.astype(np.int32))

    num_words_H = (1 + (f_bin_count * 2)) * phases # H_hat[ph][bin_count] + exponents
    # initialise IC to cancel the speech, do adapt
    conf_data_cancel_speech = np.empty(0, dtype=np.int32)
    conf_data_cancel_speech = np.append(conf_data_cancel_speech, np.array([num_words_H, 0], dtype=np.int32)) 
    conf_data_cancel_speech = np.append(conf_data_cancel_speech, np.array(pvc.float_to_int32(ideal_speech_cancellation_H), dtype=np.int32))
    run_xcore(conf_data_cancel_speech, xe)
    sr, out_data_adapt_bad = wavfile.read('output.wav')

    # initialise IC to cancel the noise, don't adapt
    conf_data_cancel_noise = np.empty(0, dtype=np.int32)
    conf_data_cancel_noise = np.append(conf_data_cancel_noise, np.array([num_words_H, 2], dtype=np.int32))
    conf_data_cancel_noise = np.append(conf_data_cancel_noise, np.array(pvc.float_to_int32(ideal_noise_cancellation_H), dtype=np.int32))
    run_xcore(conf_data_cancel_noise, xe)
    sr, out_data_fixed_good = wavfile.read('output.wav')

    t, adapt_bad = ith.leq_smooth(out_data_adapt_bad, fs, 0.05)
    t, fixed_good = ith.leq_smooth(out_data_fixed_good, fs, 0.05)

    # check after 3 seconds we have converged to be better than the fixed good filter (because it should leak)
    average_fixed_good = np.mean(fixed_good[(t>3)*(t<5)])
    average_adapt_bad  = np.mean(adapt_bad[(t>3)*(t<5)])
    assert average_adapt_bad > average_fixed_good

    if __name__ == "__main__":
        t2, original_speech = ith.leq_smooth(out_array[delay:, 0, 0], fs, 0.05)

        plt.plot(t, adapt_bad, label="adapt_bad")
        plt.plot(t, fixed_good, label="fixed_good")
        plt.plot(t2, original_speech, linestyle='--', label="raw_speech")
        plt.ylim(np.array([-10, 40])-50)
        plt.ylabel("level (dB)")
        plt.xlabel("Time (s)")
        plt.xlim([0, 5])
        plt.title("Input-output VNR, lab podcast, pink noise, speech level %d dB"%speech_level)
        plt.legend()
        plt.grid()
        plt.show()
    

if __name__ =="__main__":
    test_bad_state("lab", 0, "006_Pink")