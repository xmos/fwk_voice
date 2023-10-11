import librosa as lr
import librosa.display as lrdp
import matplotlib.pyplot as plt
import soundfile
import numpy as np
import argparse
from build_mel import build_uut

np.set_printoptions(threshold=np.inf, linewidth=200)

MEL_SPEC_OPTION_T = {"MEL_SPEC_SMALL": 0, "MEL_SPEC_LARGE": 1}
MEL_SPEC_PAD_MODE_T = {
    "MEL_SPEC_PAD_MODE_CONSTANT": 0,
    "MEL_SPEC_PAD_MODE_REFLECT": 1,
    "MEL_SPEC_PAD_MODE_SYMMETRIC": 2,
}

MEL_SPEC_COMMON_TOP_DIM_SZ = 1
MEL_SPEC_COMMON_LOW_DIM_SZ = 4
MEL_SPEC_COMMON_MIN_LOG10_IN = 1e-10
MEL_SPEC_COMMON_DB_DYNAMIC_RANGE = 80
MEL_SPEC_COMMON_CENTRE = True
MEL_SPEC_COMMON_PAD_MODE = MEL_SPEC_PAD_MODE_T["MEL_SPEC_PAD_MODE_REFLECT"]
MEL_SPEC_SMALL_N_FFT = 512
MEL_SPEC_SMALL_N_SAMPLES = 6400
MEL_SPEC_SMALL_N_MEL = 64
MEL_SPEC_SMALL_N_FRAMES = 26
MEL_SPEC_SMALL_HOP = 256
MEL_SPEC_SMALL_SCALE = 3.325621485710144e-1
MEL_SPEC_SMALL_ZERO_POINT = 12
MEL_SPEC_SMALL_PRE_DB_OFFSET = 0
MEL_SPEC_SMALL_TRIM_START = 0
MEL_SPEC_SMALL_TRIM_END = 0
MEL_SPEC_SMALL_FRAME_DIM_SZ = MEL_SPEC_SMALL_N_FRAMES
MEL_SPEC_SMALL_TOP_DIM_SZ = MEL_SPEC_COMMON_TOP_DIM_SZ
MEL_SPEC_SMALL_LOW_DIM_SZ = MEL_SPEC_COMMON_LOW_DIM_SZ
MEL_SPEC_SMALL_MIN_LOG10_IN = MEL_SPEC_COMMON_MIN_LOG10_IN
MEL_SPEC_SMALL_DB_DYNAMIC_RANGE = MEL_SPEC_COMMON_DB_DYNAMIC_RANGE
MEL_SPEC_SMALL_CENTRE = MEL_SPEC_COMMON_CENTRE
MEL_SPEC_SMALL_PAD_MODE = MEL_SPEC_COMMON_PAD_MODE
MEL_SPEC_LARGE_N_FFT = 1024
MEL_SPEC_LARGE_N_SAMPLES = 84800
MEL_SPEC_LARGE_N_MEL = 128
MEL_SPEC_LARGE_N_FRAMES = 166
MEL_SPEC_LARGE_HOP = 512
MEL_SPEC_LARGE_SCALE = 3.921553026884794e-3
MEL_SPEC_LARGE_ZERO_POINT = 128
MEL_SPEC_LARGE_PRE_DB_OFFSET = 1e-8
MEL_SPEC_LARGE_TRIM_START = 4
MEL_SPEC_LARGE_TRIM_END = 4
MEL_SPEC_LARGE_FRAME_DIM_SZ = 158
MEL_SPEC_LARGE_TOP_DIM_SZ = MEL_SPEC_COMMON_TOP_DIM_SZ
MEL_SPEC_LARGE_LOW_DIM_SZ = MEL_SPEC_COMMON_LOW_DIM_SZ
MEL_SPEC_LARGE_MIN_LOG10_IN = MEL_SPEC_COMMON_MIN_LOG10_IN
MEL_SPEC_LARGE_DB_DYNAMIC_RANGE = MEL_SPEC_COMMON_DB_DYNAMIC_RANGE
MEL_SPEC_LARGE_CENTRE = MEL_SPEC_COMMON_CENTRE
MEL_SPEC_LARGE_PAD_MODE = MEL_SPEC_COMMON_PAD_MODE


def test_mel_spec(input_filename, data_blocks, option, opy, oc, quantise, db, sm):
    assert option in ("small", "large")

    if option == "small":
        option = MEL_SPEC_OPTION_T["MEL_SPEC_SMALL"]
        fs = 16000
        n_fft = MEL_SPEC_SMALL_N_FFT
        n_samples = MEL_SPEC_SMALL_N_SAMPLES
        n_mel = MEL_SPEC_SMALL_N_MEL
        n_frames = MEL_SPEC_SMALL_N_FRAMES
        hop = MEL_SPEC_SMALL_HOP
        scale = MEL_SPEC_SMALL_SCALE
        zero_point = MEL_SPEC_SMALL_ZERO_POINT
        top_dim = MEL_SPEC_SMALL_TOP_DIM_SZ
        low_dim = MEL_SPEC_SMALL_LOW_DIM_SZ
        frame_dim = MEL_SPEC_SMALL_FRAME_DIM_SZ
        top_trim = MEL_SPEC_SMALL_TRIM_START
        end_trim = MEL_SPEC_SMALL_TRIM_END
        min_log10_in = MEL_SPEC_SMALL_MIN_LOG10_IN
        db_dynamic_range = MEL_SPEC_SMALL_DB_DYNAMIC_RANGE
        pre_db_offset = MEL_SPEC_SMALL_PRE_DB_OFFSET
    else:
        option = MEL_SPEC_OPTION_T["MEL_SPEC_LARGE"]
        fs = 16000
        n_fft = MEL_SPEC_LARGE_N_FFT
        n_samples = MEL_SPEC_LARGE_N_SAMPLES
        n_mel = MEL_SPEC_LARGE_N_MEL
        n_frames = MEL_SPEC_LARGE_N_FRAMES
        hop = MEL_SPEC_LARGE_HOP
        scale = MEL_SPEC_LARGE_SCALE
        zero_point = MEL_SPEC_LARGE_ZERO_POINT
        top_dim = MEL_SPEC_LARGE_TOP_DIM_SZ
        low_dim = MEL_SPEC_LARGE_LOW_DIM_SZ
        frame_dim = MEL_SPEC_LARGE_FRAME_DIM_SZ
        top_trim = MEL_SPEC_LARGE_TRIM_START
        end_trim = MEL_SPEC_LARGE_TRIM_END
        min_log10_in = MEL_SPEC_LARGE_MIN_LOG10_IN
        db_dynamic_range = MEL_SPEC_LARGE_DB_DYNAMIC_RANGE
        pre_db_offset = MEL_SPEC_LARGE_PRE_DB_OFFSET

    ffi, c_func, test = build_uut()
    print(ffi, c_func)

    data, sr = soundfile.read(input_filename, dtype=np.int16)
    assert sr == fs
    assert data.size >= n_samples * data_blocks

    c_melspecs = list()
    py_melspecs = list()
    for i in range(data_blocks):
        start_idx = i * n_samples
        data_block = data[start_idx : start_idx + n_samples]

        py_melspec = lr.feature.melspectrogram(
            y=data_block.astype(np.float32) / ((2**15) - 1),
            sr=fs,
            n_fft=n_fft,
            hop_length=hop,
            n_mels=n_mel,
            pad_mode="reflect",
        )

        # Take db if wanted
        if db:
            py_melspec = lr.power_to_db(
                py_melspec + pre_db_offset, amin=min_log10_in, top_db=db_dynamic_range
            )
        # Subtract mean if wanted
        if sm:
            py_melspec -= np.mean(py_melspec)
        # Quantise
        if quantise:
            py_melspec = (py_melspec + zero_point) * scale
        py_melspec = np.clip(py_melspec, -128, 127).astype(np.int8)
        print(py_melspec.shape)
        if end_trim > 0:
            py_melspec = py_melspec[:, :-end_trim]
        if top_trim > 0:
            py_melspec = py_melspec[:, top_trim:]
        py_melspecs.append(py_melspec)

        print(1)
        c_input = ffi.new(f"int16_t[{n_samples}]", data_block.tolist())
        c_output = ffi.new(f"int8_t[{top_dim * frame_dim * n_mel * low_dim}]")
        c_output_trim_top = ffi.NULL
        c_output_trim_end = ffi.NULL
        print(2)
        if option == MEL_SPEC_OPTION_T["MEL_SPEC_LARGE"]:
            c_output_trim_top = ffi.new(
                f"int8_t[{top_dim * frame_dim * top_trim * low_dim}]"
            )
            c_output_trim_end = ffi.new(
                f"int8_t[{top_dim * frame_dim * end_trim * low_dim}]"
            )
        print(3)
        print(test(6031769))
        c_func(c_output, c_output_trim_top, c_output_trim_end, c_input, option, quantise, db, sm)
        print(4)
        c_melspec = np.frombuffer(
            ffi.buffer(c_output, top_dim * frame_dim * n_mel * low_dim), dtype=np.int8
        ).reshape(top_dim, n_mel, frame_dim, low_dim)
        c_melspecs.append(c_melspec[0, :, :, 0])
        print(5)

    num_sp = 2
    _, axs = plt.subplots(
        num_sp, data_blocks, squeeze=False, sharex="col", sharey="all"
    )

    with open(opy, "w") as f:
        f.write("\n")
    with open(oc, "w") as f:
        f.write("\n")

    for c_idx in range(data_blocks):
        targets = {0: py_melspecs[c_idx], 1: c_melspecs[c_idx]}

        time_vals = np.array(
            [
                round((n_samples / fs) * ((x / (frame_dim - 1)) + c_idx), 3)
                for x in range(frame_dim)
            ]
        )

        for r_idx in range(num_sp):
            lrdp.specshow(
                targets[r_idx],
                sr=sr,
                y_axis="mel",
                x_axis="frames",
                # x_coords=time_vals,
                fmax=(sr / 2),
                hop_length=hop,
                # n_fft=n_fft,
                ax=axs[r_idx][c_idx],
            )
        with open(opy, "a") as f:
            for mel in targets[0][-1::-1]:
                f.write("".join([f"{x:4}, " for x in mel]))
                f.write("\n")
            f.write("\n")
        with open(oc, "a") as f:
            for mel in targets[1][-1::-1]:
                f.write("".join([f"{x:4}, " for x in mel]))
                f.write("\n")
            f.write("\n")
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare Py and C Mel specs")
    parser.add_argument("-i", "--input-file", action="store")
    parser.add_argument("-b", "--number-of-blocks", action="store", type=int)
    parser.add_argument("-s", "--size", action="store", choices=("small", "large"))
    parser.add_argument(
        "-op", "--output-file-python", action="store", default="out_python.txt"
    )
    parser.add_argument("-oc", "--output-file-c", action="store", default="out_c.txt")
    parser.add_argument("-q", "--quantise", action="store_true")
    parser.add_argument("-db", "--to-decibel", action="store_true")
    parser.add_argument("-submean", "--subtract-mean", action="store_true")

    args = parser.parse_args()

    test_mel_spec(
        args.input_file,
        args.number_of_blocks,
        args.size,
        args.output_file_python,
        args.output_file_c,
        args.quantise,
        args.to_decibel,
        args.subtract_mean,
    )
