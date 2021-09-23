// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1
#include "xcore_device_memory.h"
#include "xs3_math.h"
#include <stdint.h>
#include <string.h>

#define ROUND_UPTO_NEAREST_MULTIPLE(VAL, MULT) ((VAL + MULT) & ~31)

#define STRIP_ROWS (256)
#define MAX_N_COLS (540)

//#define MAX_MATRIX_SIZE_BYTES (87*540 /*=46980*/)  // for loading entire
//matrix at once
#define MAX_MATRIX_SIZE_BYTES                                                  \
  (MAX_N_COLS * STRIP_ROWS) // for loading matrix in strips of STRIP_ROWS

#define MAX_SCRATCH_SIZE_BYTES ROUND_UPTO_NEAREST_MULTIPLE(MAX_N_COLS, 16)

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

// This version loads the matrix coeffs in strips, limiting the amount of SRAM
// required
//   NOTE: Be sure to use the correct MAX_MATRIX_SIZE_BYTES at the beginning of
//   this source file
void mtx_asm(int32_t *output, const int8_t *matrix, const int16_t *input_vector,
             const signed int M_rows, const signed int N_cols) {

  int8_t *matrix_buffer_ptr = &matrix_buffer[0];

  // rtos_printf("M_rows=%d   N_cols=%d\n", M_rows, N_cols);

  int start_row = 0;
  int end_row = MIN(STRIP_ROWS, M_rows) - 1;

  do {
    int matrix_offset = start_row * N_cols;
    int S_rows = (end_row - start_row) + 1;

    size_t strip_size_bytes = S_rows * N_cols;

    // rtos_printf(
    //     "start_row=%d    end_row=%d    S_rows=%d   offset=%d   bytes=%d\n",
    //     start_row, end_row, S_rows, matrix_offset, strip_size_bytes);

    load((void **)&matrix_buffer_ptr, (void *)&matrix[matrix_offset],
         strip_size_bytes);

    xs3_mat_mul_s8_x_s16_yield_s32(&output[start_row], matrix_buffer_ptr,
                                   input_vector, S_rows, N_cols,
                                   &scratch_buffer[0]);

    start_row += STRIP_ROWS;
    end_row = MIN(start_row + STRIP_ROWS, M_rows) - 1;

  } while (start_row < M_rows);
};

// This version loads all the matrix coeffs at once and requires the most SRAM
//   NOTE: Be sure to use the correct MAX_MATRIX_SIZE_BYTES at the beginning of
//   this source file
// void mtx_asm(int32_t *output, const int8_t *matrix, const int16_t
// *input_vector,
//              const signed int M_rows, const signed int N_cols) {

//   int8_t *matrix_buffer_ptr = &matrix_buffer[0];
//   size_t matrix_size_bytes = M_rows * N_cols;

//   load((void **)&matrix_buffer_ptr, (void *)matrix, matrix_size_bytes);

//   xs3_mat_mul_s8_x_s16_yield_s32(output, matrix_buffer, input_vector, M_rows,
//                                  N_cols, &scratch_buffer[0]);
// };