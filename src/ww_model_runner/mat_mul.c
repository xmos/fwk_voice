// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1
#include <stdint.h>
#include <string.h>

#include "xcore_device_memory.h"
#include "xs3_math.h"

#define MAX_N_COLS (540)
#define MAX_MATRIX_SIZE_BYTES (46980)

#define MAX_SCRATCH_SIZE_BYTES ((MAX_N_COLS + 16) & ~31)

__attribute__((
    aligned(4))) static int8_t scratch_buffer[MAX_SCRATCH_SIZE_BYTES] = {0};

__attribute__((
    aligned(4))) static int8_t matrix_buffer[MAX_MATRIX_SIZE_BYTES] = {0};

inline size_t load(void **dest, const void *src, size_t size) {
#if (USE_SWMEM == 1)
  if (IS_SWMEM(src)) {
    return swmem_load(*dest, src, size);
  } else
#endif /* USE_SWMEM */
  {
    if (IS_RAM(src)) {
      *dest = (void *)src;
      return 0;
    } else {
      memcpy(*dest, src, size); // TODO: if size >= 128, vpu_memcpy is faster
    }
    return size;
  }
}

// NOTE: The symbol has to be "mtx_asm" to match the name of the symbol in the
// original libpryon_lite-U.a
void mtx_asm(int32_t *output, const int8_t *matrix, const int16_t *input_vector,
             const signed int M_rows, const signed int N_cols) {
  size_t matrix_size_bytes = M_rows * N_cols;
  int8_t *matrix_buffer_ptr = &matrix_buffer[0];

  load((void **)&matrix_buffer_ptr, (void *)matrix, matrix_size_bytes);

  xs3_mat_mul_s8_x_s16_yield_s32(output, matrix_buffer, input_vector, M_rows,
                                 N_cols, &scratch_buffer[0]);
};