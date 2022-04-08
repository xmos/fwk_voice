import py_vnr.run_wav_vnr as rwv
import py_vnr.vnr as vnr
import argparse
import audio_wav_utils as awu
import scipy.io.wavfile
import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
import os
import sys
sys.path.append(os.path.join(os.getcwd(), "../shared_src/python"))
import run_xcoreai

def test_mel(plot_results=False):
    run_xcoreai.run("../../../build/examples/bare-metal/test_mel/bin/avona_test_mel.xe")

    vnr_obj = vnr.Vnr(model_file=None)    
    dtype=np.dtype([('r', np.int32),('q', np.int32)])
    # read dut fft output that will be used as input to the python mel
    with open("dut_fft.bin", "rb") as fdut:        
        dut_fft_out = np.fromfile(fdut, dtype=dtype)
        #print("dut_fft_out from file", dut_fft_out[0])
        dut_fft_out = [complex(*item) for item in dut_fft_out]
        dut_fft_out = np.array(dut_fft_out, dtype=np.complex128)
    # Read dut FFT output exponents
    with open("dut_fft_exp.bin", "rb") as fdut:
        dut_fft_out_exp = np.fromfile(fdut, dtype=np.int32)

    # read dut mel output
    with open("dut_mel.bin", "rb") as fdut:        
        dut_mel_mant = np.fromfile(fdut, dtype=np.int64)
        dut_mel_mant = np.array(dut_mel_mant, dtype=np.float64)
        with open("dut_mel_exp.bin", "rb") as fdutexp:        
            dut_mel_exp = np.fromfile(fdutexp, dtype=np.int32)
            dut_mel = np.array(dut_mel_mant*(float(2)**dut_mel_exp), dtype=np.float64)
    
    # Convert FFT output to float
    frames = len(dut_fft_out_exp)
    print(f"{frames} frames sent to python")

    ref_X_spect_full = np.empty(0, dtype=np.complex128)
    ref_mel = np.empty(0, dtype=np.float64)
    for fr in range(frames):
        exp = dut_fft_out_exp[fr]
        X_spect = np.array(dut_fft_out[fr*257:(fr+1)*257] * (float(2)**exp))
        ref_X_spect_full = np.append(ref_X_spect_full, X_spect)

        out_spect = np.abs(X_spect)**2
        #if(fr == 66):
        #    print("abs_spect\n",out_spect[:20])
        out_spect = np.dot(out_spect, vnr_obj.mel_fbank)        
        ref_mel = np.append(ref_mel, out_spect)

    max_diff_per_frame = np.empty(0, dtype=np.float64) 
    for i in range(0, len(ref_mel), vnr_obj.mel_filters):
        fr = i/vnr_obj.mel_filters
        ref = ref_mel[i:i+vnr_obj.mel_filters]
        dut = dut_mel[i:i+vnr_obj.mel_filters]
        max_diff_per_frame = np.append(max_diff_per_frame, np.max(np.abs(ref-dut)))
        #if fr == 66:
        #    print("ref mel\n",ref)
        #    print("dut mel\n",dut)
        #    print(f"frame {i} diff {max(abs(ref-dut))}, max_ref {max(abs(ref))}, max_dut {max(abs(dut))}")
    
    if(plot_results):
        fig,ax = plt.subplots(2)
        fig.set_size_inches(12,10)
        ax[0].set_title('mel output')
        ax[0].plot(ref_mel)
        ax[0].plot(dut_mel)
        ax[1].set_title('max mel output diff per frame')
        ax[1].plot(max_diff_per_frame)
        fig_instance = plt.gcf() #get current figure instance so we can save in a file later
        plt.show()
        plot_file = "ref_dut_mel_compare.png"
        fig.savefig(plot_file)
        
if __name__ == "__main__":
    test_mel(plot_results=True)
