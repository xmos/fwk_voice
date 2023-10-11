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
    XCORE_MATH = "../../../build/fwk_voice_deps/lib_xcore_math/lib_xcore_math/"
    MEL_SPEC_PATH = "../../../modules/lib_melspectrogram/"

    FLAGS = ["-std=c99", "-fPIC", "-D_GNU_SOURCE", "-w"]

    # Source and include files
    lib_mel_spec_path = pathlib.Path(MEL_SPEC_PATH).resolve()
    lib_mel_spec_sources = (lib_mel_spec_path / "src").rglob("*.c")
    lib_xcore_math_path = pathlib.Path(XCORE_MATH).resolve()

    SRCS = [p for p in lib_mel_spec_sources]
    SRCS.extend((lib_xcore_math_path / "src").rglob("*.c"))

    INCLUDES = [
        lib_mel_spec_path / "api",
        lib_xcore_math_path / "api",
        lib_xcore_math_path / "src",
        lib_xcore_math_path / "src" / "arch" / "ref",
        lib_xcore_math_path / "src" / "etc" / "xmath_fft_lut",

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
        "melspectrogram",
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

    output_file = pathlib.Path("build/melspectrogram.dylib")
    if not output_file.exists():
        ffibuilder.compile(tmpdir=build_dir, target="melspectrogram.*", verbose=True)

    # Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    if sys.platform == "darwin":
        copyfile(output_file, output_file.with_suffix('.so'))

    open(build_dir + "/__init__.py", 'w').close() # Needed to load module from subdir

    print("Build complete!")


def clean_ffi():
    rmtree(f"./{build_dir}")


def build_uut():
    build_ffi()

    import build.melspectrogram as ms
    return ms.ffi, ms.lib.x_melspectrogram, ms.lib.test

if __name__ == "__main__":
    build_ffi()

