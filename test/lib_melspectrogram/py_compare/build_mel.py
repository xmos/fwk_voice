# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from shutil import rmtree, copyfile
import pathlib
import sys

build_dir = "build"

def build_ffi():
    print("Building application...")
    # One more ../ than necessary - builds in the 'build' subdirectory in this folder
    XCORE_MATH = "../../.."

    FLAGS = ["-std=c99", "-fPIC", "-D_GNU_SOURCE", "-w"]

    # Source file
    lib_xcore_math = pathlib.Path(XCORE_MATH).resolve()
    lib_xcore_math_sources = (lib_xcore_math / "lib_xcore_math" / "src").rglob("*.c")

    SRCS = [p for p in lib_xcore_math_sources]
    INCLUDES = [
        lib_xcore_math / "lib_xcore_math" / "api",
        lib_xcore_math / "lib_xcore_math" / "src" / "etc" / "xmath_fft_lut",
    ]

    # Units under test
    ffibuilder = FFI()
    ffibuilder.cdef(
        """
        typedef enum
        {
            MEL_SPEC_SMALL,
            MEL_SPEC_LARGE
        } mel_spec_option_t;
        int test(int);
        void x_melspectrogram(int8_t *out,
                      int8_t *out_trim_top,
                      int8_t *out_trim_end,
                      int16_t *in,
                      mel_spec_option_t mel_spec_option,
                      bool quantise,
                      bool convert_to_db,
                      bool subtract_mean);
        """
    )

    ffibuilder.set_source(
        "mel_spectrogram_api",
        """
        #include <stdint.h>     
        #include <stdbool.h>   
        typedef enum
        {
            MEL_SPEC_SMALL,
            MEL_SPEC_LARGE
        } mel_spec_option_t;
        int test(int);
        void x_melspectrogram(int8_t *out,
                      int8_t *out_trim_top,
                      int8_t *out_trim_end,
                      int16_t *in,
                      mel_spec_option_t mel_spec_option,
                      bool quantise,
                      bool convert_to_db,
                      bool subtract_mean);
    """,
        sources=SRCS,
        include_dirs=INCLUDES,
        libraries=["m"],
        extra_compile_args=FLAGS,
    )

    ffibuilder.compile(tmpdir=build_dir, target="mel_spectrogram_api.*", verbose=False)

    # Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    if sys.platform == "darwin":
        copyfile("build/mel_spectrogram_api.dylib", "build/mel_spectrogram_api.so")

    print("Build complete!")


def clean_ffi():
    rmtree(f"./{build_dir}")


def build_uut():
    build_ffi()

    from build import mel_spectrogram_api
    from mel_spectrogram_api import ffi
    import mel_spectrogram_api.lib as mel_spectrogram_api_lib

    return ffi, mel_spectrogram_api_lib.x_melspectrogram, mel_spectrogram_api_lib.test


if __name__ == "__main__":
    build_ffi()

