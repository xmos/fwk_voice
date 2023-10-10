// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <xmath/xmath.h>
#include <assert.h>

#include "xmath/mel_spectrogram/coeffs/mel_filter_512_64_compressed.h"
#include "xmath/mel_spectrogram/coeffs/mel_filter_1024_128_compressed.h"
#include "xmath/mel_spectrogram/coeffs/hann_512.h"
#include "xmath/mel_spectrogram/coeffs/hann_1024.h"

#ifndef __XS3A__
#define isnanf isnan
#define isinff isinf
#endif

typedef enum
{
  MEL_SPEC_PAD_MODE_CONSTANT,
  MEL_SPEC_PAD_MODE_REFLECT,
  MEL_SPEC_PAD_MODE_SYMMETRIC
} mel_spec_pad_mode_t;

typedef struct
{
  uint32_t n_fft;
  uint32_t n_samples;
  uint32_t n_mels;
  uint32_t n_frames;
  uint32_t hop;
  bool centre;
  mel_spec_pad_mode_t pad_mode;
} mel_spec_settings_t;

typedef struct
{
  uint32_t frame_dim;
  uint32_t top_dim;
  uint32_t low_dim;
} mel_spec_output_shape_t;

typedef struct
{
  int16_t (*filter_indices)[2];
  int32_t *filter;
  int32_t *window;
} mel_spec_filters_t;

typedef struct
{
  bool quantise;
  double scale;
  int32_t zero_point;
} mel_spec_quantisation_t;

typedef struct
{
  bool to_db;
  float minimum_db_input;
  int32_t dynamic_range;
  float pre_db_offset;
} mel_spec_db_conversion_t;

typedef struct
{
  uint32_t trim_top_len;
  uint32_t trim_end_len;
} mel_spec_trim_t;

typedef struct
{
  bool sub_mean;
} mel_spec_normalisation_t;

typedef struct
{
  float *trim_top_buf;
  uint32_t trim_top_sz;
  uint32_t trim_top_frm;
  float *trim_end_buf;
  uint32_t trim_end_sz;
  uint32_t trim_end_frm;
  float *quantise_buf;
  uint32_t quantise_sz;
  uint32_t quantise_frm;
} mel_spec_buffers_t;

int test(int val){
    printf("val: %d\n", val);
    return 42;
}

static void mel_compute(float_s32_t filter_output[],
                        const int n_mel,
                        const bfp_complex_s32_t *X,
                        const int n_fft,
                        const int16_t filter_indices[][2],
                        int32_t filter[])
{
  bfp_s32_t squared_mag;
  int32_t DWORD_ALIGNED squared_mag_data[(n_fft / 2) + 1];
  bfp_s32_init(&squared_mag, squared_mag_data, 0, X->length, 0);

  bfp_complex_s32_squared_mag(&squared_mag, X);

  // Mel filtering
  unsigned filter_head = 0;

  for (int i = 0; i < n_mel; i++)
  {
    // Get start and end bins for the filter
    unsigned filter_start = filter_indices[i][0];
    unsigned filter_length = filter_indices[i][1];
    // Create input spectrum subset BFP structure
    bfp_s32_t spect_subset;
    bfp_s32_init(&spect_subset,
                 &squared_mag.data[filter_start],
                 squared_mag.exp,
                 filter_length,
                 1);

    // Create MEL filter BFP structure
    bfp_s32_t filter_subset;
    bfp_s32_init(&filter_subset, &filter[filter_head], -31, filter_length, 0);
    filter_head += filter_length;

    // Dot product
    filter_output[i] = float_s64_to_float_s32(bfp_s32_dot(&spect_subset,
                                                          &filter_subset));
  }
}

static inline void _to_float(mel_spec_buffers_t *const bufs,
                             float_s32_t const *const input,
                             uint32_t const in_sz,
                             uint32_t const frame)
{
  float *target;
  uint32_t target_frm;
  int32_t frame_adjust;

  if (frame < bufs->trim_top_frm)
  {
    // We're in the top trimmed part
    target = bufs->trim_top_buf;
    target_frm = bufs->trim_top_frm;
    frame_adjust = 0;
  }
  else if (frame >= bufs->trim_top_frm + bufs->quantise_frm)
  {
    // We're in the end trimmed part.
    target = bufs->trim_end_buf;
    target_frm = bufs->trim_end_frm;
    frame_adjust = -(bufs->trim_top_frm + bufs->quantise_frm);
  }
  else
  {
    // We're in the middle part
    target = bufs->quantise_buf;
    target_frm = bufs->quantise_frm;
    frame_adjust = -bufs->trim_top_frm;
  }

  for (uint32_t i = 0; i < in_sz; i++)
  {
    // Intentional matrix transpose
    target[(i * target_frm) + frame + frame_adjust] = float_s32_to_float(input[i]);
  }
}

static inline void _to_float_and_db(mel_spec_buffers_t *const bufs,
                                    float_s32_t const *const input,
                                    uint32_t const in_sz,
                                    uint32_t const frame,
                                    float min_val,
                                    float offset)
{
  float *target;
  uint32_t target_frm;
  int32_t frame_adjust;

  if (frame < bufs->trim_top_frm)
  {
    // We're in the top trimmed part
    target = bufs->trim_top_buf;
    target_frm = bufs->trim_top_frm;
    frame_adjust = 0;
  }
  else if (frame >= bufs->trim_top_frm + bufs->quantise_frm)
  {
    // We're in the end trimmed part.
    target = bufs->trim_end_buf;
    target_frm = bufs->trim_end_frm;
    frame_adjust = -(bufs->trim_top_frm + bufs->quantise_frm);
  }
  else
  {
    // We're in the middle part
    target = bufs->quantise_buf;
    target_frm = bufs->quantise_frm;
    frame_adjust = -bufs->trim_top_frm;
  }

  for (uint32_t i = 0; i < in_sz; i++)
  {
    float temp = float_s32_to_float(input[i]) + offset;
    if (input[i].exp == 0 || input[i].mant == 0 || temp < min_val)
    {
      temp = min_val;
    }
    // Intentional matrix transpose
    target[(i * target_frm) + frame + frame_adjust] = 10 * log10f(temp);
  }
}

static inline void _subtract_mean_in_place(mel_spec_buffers_t *const bufs)
{
  // Find sum
  float acc = 0;
  uint32_t elems = bufs->quantise_sz + bufs->trim_end_sz + bufs->trim_top_sz;

  // Accumulate all three arrays
  for (uint32_t i = 0; i < bufs->trim_top_sz; i++)
  {
    if (!isinff(bufs->trim_top_buf[i]) && !isnanf(bufs->trim_top_buf[i]))
    {
      acc += bufs->trim_top_buf[i];
    }
    else
    {
      elems--;
    }
  }
  for (uint32_t i = 0; i < bufs->quantise_sz; i++)
  {
    if (!isinff(bufs->quantise_buf[i]) && !isnanf(bufs->quantise_buf[i]))
    {
      acc += bufs->quantise_buf[i];
    }
    else
    {
      elems--;
    }
  }
  for (uint32_t i = 0; i < bufs->trim_end_sz; i++)
  {
    if (!isinff(bufs->trim_end_buf[i]) && !isnanf(bufs->trim_end_buf[i]))
    {
      acc += bufs->trim_end_buf[i];
    }
    else
    {
      elems--;
    }
  }

  // Divide by num_elem
  acc /= elems;

  // Subtract from arrays
  for (uint32_t i = 0; i < bufs->trim_top_sz; i++)
  {
    bufs->trim_top_buf[i] -= acc;
  }
  for (uint32_t i = 0; i < bufs->quantise_sz; i++)
  {
    bufs->quantise_buf[i] -= acc;
  }
  for (uint32_t i = 0; i < bufs->trim_end_sz; i++)
  {
    bufs->trim_end_buf[i] -= acc;
  }
}

static inline void _min_thresh(mel_spec_buffers_t *const bufs,
                               int32_t const dynamic_range)
{
  // Find max
  float max_val = -INFINITY;

  // We need to do this for all three arrays so the mean ends up correct
  for (uint32_t i = 0; i < bufs->trim_top_sz; i++)
  {
    max_val = MAX(max_val, bufs->trim_top_buf[i]);
  }
  for (uint32_t i = 0; i < bufs->quantise_sz; i++)
  {
    max_val = MAX(max_val, bufs->quantise_buf[i]);
  }
  for (uint32_t i = 0; i < bufs->trim_end_sz; i++)
  {
    max_val = MAX(max_val, bufs->trim_end_buf[i]);
  }

  // Find const min value
  const float min_val = max_val - dynamic_range;

  // Replace if less than.
  for (uint32_t i = 0; i < bufs->trim_top_sz; i++)
  {
    bufs->trim_top_buf[i] = MAX(min_val, bufs->trim_top_buf[i]);
  }

  for (uint32_t i = 0; i < bufs->quantise_sz; i++)
  {
    bufs->quantise_buf[i] = MAX(min_val, bufs->quantise_buf[i]);
  }

  for (uint32_t i = 0; i < bufs->trim_end_sz; i++)
  {
    bufs->trim_end_buf[i] = MAX(min_val, bufs->trim_end_buf[i]);
  }
}

static inline void _quantise(float *const in_out,
                             uint32_t const x_width,
                             uint32_t const y_width,
                             mel_spec_quantisation_t const *const opts)
{
  // Quantise a 2d array of floats to int8s in place.
  // This _only works_ because low_dim is 4. This code will need to
  // change for any other value!
  // This uses undefined behaviour that is both arch and compiler specific!
  for (uint32_t x = 0; x < x_width; x++)
  {
    for (uint32_t y = 0; y < y_width; y++)
    {
      uint32_t const idx = (x * y_width) + y;
      union
      {
        float f;
        int8_t i[4];
        uint32_t z;
      } temp = {.z = 0};
      // We use .f to avoid weird casting/aliasing issues,
      // .i[] to hold the int8_t we actually want,
      // and .z just to initialise to all 0s
      if (opts->quantise)
      {
        in_out[idx] += opts->zero_point;
        in_out[idx] *= opts->scale;
      }
      if (in_out[idx] > 127.0)
      {
        temp.i[0] = 127;
      }
      else if (in_out[idx] < -128.0)
      {
        temp.i[0] = -128;
      }
      else
      {
        temp.i[0] = (int8_t)in_out[idx]; // We know that temp.i fits into 8b
      }
      in_out[idx] = temp.f;
    }
  }
}

static inline void _get_slice(int16_t *const dst,
                              int16_t const *const src,
                              uint32_t slice,
                              mel_spec_settings_t const *const opts)
{
  uint32_t const src_idx = slice * opts->hop;
  uint32_t const end_idx = (src_idx + opts->n_fft) - 1;
  uint32_t n_copy = opts->n_fft;
  if (end_idx >= opts->n_samples)
  {
    /* Because slice is initialised to 0, this implicitly pads slice
        with zeros at the end. */
    n_copy = opts->n_fft - (end_idx - opts->n_samples + 1);
  }
  if (n_copy > 0)
  {
    memcpy(&dst[0], &src[src_idx], n_copy * sizeof(int16_t));
  }
}

static inline void _get_slice_centre(int16_t *const dst,
                                     int16_t const *const src,
                                     uint32_t slice_no,
                                     mel_spec_settings_t const *const opts)
{
  uint32_t key = slice_no * opts->hop;
  uint32_t wing = opts->n_fft / 2;
  uint32_t arr_size = opts->n_fft - 1;

  uint32_t src_idx = key - wing;
  uint32_t end_idx = MIN(key + wing, opts->n_samples);
  uint32_t span = end_idx - src_idx;

printf("memcpy pre\n");
    printf("wing: %d slice_no: %d\n", wing, slice_no);

    printf("%d\n", &dst[slice_no ? 0 : wing]);
    dst[slice_no ? 0 : wing] = 1;
    printf("%d\n", &src[src_idx]);
  // memcpy(&dst[slice_no ? 0 : wing], &src[src_idx], span * sizeof(int16_t));
  memcpy(&dst[slice_no ? 0 : wing], &src[src_idx], 1 * sizeof(int16_t));
printf("memcpy post\n");

  if (span < opts->n_fft) // This frame needs padding
  {
    if (opts->pad_mode == MEL_SPEC_PAD_MODE_REFLECT)
    {
      // If at end (slice is nonzero), [X Y Z 0 0 0] -> [|X Y Z| Y X W]
      // If at start (slice is zero), [0 0 0 A B C] -> [|D C B| A B C]
      for (int top = wing - 1, end = wing; top > -1; top--, end++)
      {
        if (slice_no) // if n then pad right. Pos n_fft/2 - 1 is pivot.
        {
          dst[end - 1] = dst[top];
        }
        else // if not n then pad left. Pos n_fft/2 is pivot.
        {
          dst[top + 1] = dst[end];
        }
      }
      // Because we're reflecting, the edge sample will be reflected out
      // of the set of values we have here. Find it and insert it.
      if (slice_no) // Last sample will be (n_fft/2 + 1) from end of block.
      {
        dst[arr_size] = src[opts->n_samples - (wing + 1)];
      }
      else // First sample will be (n_fft/2 + 1) from start of block
      {
        dst[0] = src[wing + 1];
      }
    }
    else if (opts->pad_mode == MEL_SPEC_PAD_MODE_SYMMETRIC)
    {
      // Symmetric pad
      for (uint32_t top = 0, end = arr_size; top < wing; top++, end--)
      {
        if (slice_no) // if n then pad right
        {
          dst[end] = dst[top];
        }
        else // if not n then pad left
        {
          dst[top] = dst[end];
        }
      }
    } // else we assume 0-pad, and dst is initialised to 0
  }   // else the frame doesn't need padding and we're done
}

// Caveat: the compiler claims of not knowing how to calculate the stack for
// this function, even though I'm not using any function pointers anywhere.
// Naively calculated by looking at the three arrays I'm creating in this func
// then rounding up for luck.
// Worth keeping an eye on.
#ifdef __XS3A__
#pragma stackfunction 1024
#endif
static void x_mel_spec(int8_t *const output,
                       int8_t *const out_trim_top,
                       int8_t *const out_trim_end,
                       int16_t *const input,
                       mel_spec_output_shape_t const *const out_shape,
                       mel_spec_settings_t const *const mel_opts,
                       mel_spec_filters_t const *const filters,
                       mel_spec_quantisation_t const *const quant_opts,
                       mel_spec_db_conversion_t const *const db_opts,
                       mel_spec_normalisation_t const *const norm_opts,
                       mel_spec_trim_t const *const trim_opts)
{
  printf("x_mel_spec pre\n");

  memset(&output[0], 0, out_shape->top_dim * out_shape->low_dim * out_shape->frame_dim * mel_opts->n_mels);

  printf("x_mel_spec post\n");


  /* Trimming values from the start and end means we need more space for Mel
   * output than we have in the output buffer. We really, really don't want to
   * realloc that space, so use temporary buffers for the trimmed top and
   * end sections.
   * To operate over the whole array (subtract mean, threshold) we need to
   * calculate the whole array. This means we need somewhere to store the
   * output of calculating the whole array, before we quantise it. Let's use
   * the fact we have an output buffer that is 4 bytes per element. */
  mel_spec_buffers_t buffers = {.trim_top_buf = (float *)out_trim_top,
                                .trim_top_sz = trim_opts->trim_top_len * mel_opts->n_mels,
                                .trim_top_frm = trim_opts->trim_top_len,
                                .trim_end_buf = (float *)out_trim_end,
                                .trim_end_sz = trim_opts->trim_end_len * mel_opts->n_mels,
                                .trim_end_frm = trim_opts->trim_end_len,
                                .quantise_buf = (float *)output,
                                .quantise_sz = out_shape->frame_dim * mel_opts->n_mels,
                                .quantise_frm = out_shape->frame_dim};

  if (buffers.trim_top_buf)
  {
    memset(buffers.trim_top_buf, 0, buffers.trim_top_sz * sizeof(float));
  }
  printf("x_mel_spec post\n");

  if (buffers.trim_end_buf)
  {
    memset(buffers.trim_end_buf, 0, buffers.trim_end_sz * sizeof(float));
  }
  printf("x_mel_spec post\n");

  for (uint32_t n = 0; n < mel_opts->n_frames; n++)
  {
    printf("n=%d\n", n);
    int16_t WORD_ALIGNED slice[mel_opts->n_fft];
    memset(slice, 0, mel_opts->n_fft * sizeof(int16_t));
    printf("n=%d\n", n);

    if (mel_opts->centre == false)
    {
        printf("_get_slice\n");

      _get_slice(slice, input, n, mel_opts);
    }
    else
    {
        printf("_get_slice_centre\n");

      _get_slice_centre(slice, input, n, mel_opts);
    }
    printf("n=%d\n", n);

    /* Shuffle some data around. Highlight is doing 16b -> 32b vectorwise.*/
    bfp_s16_t slice_bfp;
    bfp_s16_init(&slice_bfp, slice, -15, mel_opts->n_fft, 0);

    bfp_s32_t to_fft;
    int32_t DWORD_ALIGNED to_fft_buf[mel_opts->n_fft + 2];
    bfp_s32_init(&to_fft, to_fft_buf, -31, mel_opts->n_fft, 0);
    bfp_s16_to_bfp_s32(&to_fft, &slice_bfp);

    /* Apply the window, again vector-accelerated */
    bfp_s32_t window_bfp;
    bfp_s32_init(&window_bfp, filters->window, -31, mel_opts->n_fft, 0);
    bfp_s32_mul(&to_fft, &to_fft, &window_bfp);

    /* Do the FFT. Unpack the result. */
    bfp_complex_s32_t *slice_fft = bfp_fft_forward_mono(&to_fft);
    bfp_fft_unpack_mono(slice_fft);

    printf("mel_compute pre\n");

    /* Transform into Mel domain */
    float_s32_t DWORD_ALIGNED mel_out[mel_opts->n_mels];
    mel_compute(mel_out,
                mel_opts->n_mels,
                slice_fft,
                mel_opts->n_fft,
                filters->filter_indices,
                filters->filter);

    if (db_opts->to_db)
    {
      _to_float_and_db(&buffers,
                       mel_out,
                       mel_opts->n_mels,
                       n,
                       db_opts->minimum_db_input,
                       db_opts->pre_db_offset);
    }
    else
    {
      _to_float(&buffers,
                mel_out,
                mel_opts->n_mels,
                n);
    }
  }

  // /* Okay, we've now calculated all frames. Three things to do on the tensor:
  //  * - Set minimum threshold to max(out_f) - top_db if we're in db-scale
  //  * - Find the mean and subtract
  //  * - Quantise */
  if (db_opts->to_db)
  {
    _min_thresh(&buffers, db_opts->dynamic_range);
  }

  if (norm_opts->sub_mean)
  {
    _subtract_mean_in_place(&buffers);
  }

  _quantise(buffers.trim_top_buf, mel_opts->n_mels, buffers.trim_top_frm, quant_opts);
  _quantise(buffers.quantise_buf, mel_opts->n_mels, out_shape->frame_dim, quant_opts);
  _quantise(buffers.trim_end_buf, mel_opts->n_mels, buffers.trim_end_frm, quant_opts);
}

void x_melspectrogram(int8_t *output,
                      int8_t *out_trim_top,
                      int8_t *out_trim_end,
                      int16_t *input,
                      mel_spec_option_t mel_spec_option,
                      bool quantise,
                      bool convert_to_db,
                      bool subtract_mean)
{
  printf("x_melspectrogram: %d %d %d\n", quantise, convert_to_db, subtract_mean);

  switch (mel_spec_option)
  {
  case MEL_SPEC_SMALL:
  {
    mel_spec_settings_t const mel_opts =
        {
            .n_fft = MEL_SPEC_SMALL_N_FFT,
            .n_samples = MEL_SPEC_SMALL_N_SAMPLES,
            .n_mels = MEL_SPEC_SMALL_N_MEL,
            .n_frames = MEL_SPEC_SMALL_N_FRAMES,
            .hop = MEL_SPEC_SMALL_HOP,
            .centre = MEL_SPEC_SMALL_CENTRE,
            .pad_mode = MEL_SPEC_SMALL_PAD_MODE};
    mel_spec_output_shape_t const out_shape =
        {
            .frame_dim = MEL_SPEC_SMALL_FRAME_DIM_SZ,
            .top_dim = MEL_SPEC_SMALL_TOP_DIM_SZ,
            .low_dim = MEL_SPEC_SMALL_LOW_DIM_SZ};
    mel_spec_filters_t const filters =
        {
            .filter_indices = mel_filter_512_64_compressed_indicies,
            .filter = mel_filter_512_64_compressed,
            .window = (int32_t *)hanning_512};
    mel_spec_quantisation_t const quant_opts =
        {
            .quantise = quantise,
            .scale = MEL_SPEC_SMALL_SCALE,
            .zero_point = MEL_SPEC_SMALL_ZERO_POINT};
    mel_spec_db_conversion_t const db_opts =
        {
            .to_db = convert_to_db,
            .minimum_db_input = MEL_SPEC_SMALL_MIN_LOG10_IN,
            .dynamic_range = MEL_SPEC_SMALL_DB_DYNAMIC_RANGE,
            .pre_db_offset = MEL_SPEC_SMALL_PRE_DB_OFFSET};
    mel_spec_normalisation_t const norm_opts =
        {
            .sub_mean = subtract_mean};
    mel_spec_trim_t trim_opts =
        {
            .trim_top_len = MEL_SPEC_SMALL_TRIM_START,
            .trim_end_len = MEL_SPEC_SMALL_TRIM_END};

    printf("x_mel_spec MEL_SPEC_SMALL\n");

    x_mel_spec(output,
               out_trim_top,
               out_trim_end,
               input,
               &out_shape,
               &mel_opts,
               &filters,
               &quant_opts,
               &db_opts,
               &norm_opts,
               &trim_opts);

    break;
  }

  case MEL_SPEC_LARGE:
  {
    mel_spec_settings_t const mel_opts =
        {
            .n_fft = MEL_SPEC_LARGE_N_FFT,
            .n_samples = MEL_SPEC_LARGE_N_SAMPLES,
            .n_mels = MEL_SPEC_LARGE_N_MEL,
            .n_frames = MEL_SPEC_LARGE_N_FRAMES,
            .hop = MEL_SPEC_LARGE_HOP,
            .centre = MEL_SPEC_LARGE_CENTRE,
            .pad_mode = MEL_SPEC_LARGE_PAD_MODE};
    mel_spec_output_shape_t const out_shape =
        {
            .frame_dim = MEL_SPEC_LARGE_FRAME_DIM_SZ,
            .top_dim = MEL_SPEC_LARGE_TOP_DIM_SZ,
            .low_dim = MEL_SPEC_LARGE_LOW_DIM_SZ,
        };
    mel_spec_filters_t const filters =
        {
            .filter_indices = mel_filter_1024_128_compressed_indicies,
            .filter = mel_filter_1024_128_compressed,
            .window = (int32_t *)hanning_1024};
    mel_spec_quantisation_t const quant_opts =
        {
            .quantise = quantise,
            .scale = MEL_SPEC_LARGE_SCALE,
            .zero_point = MEL_SPEC_LARGE_ZERO_POINT};
    mel_spec_db_conversion_t const db_opts =
        {
            .to_db = convert_to_db,
            .minimum_db_input = MEL_SPEC_LARGE_MIN_LOG10_IN,
            .dynamic_range = MEL_SPEC_LARGE_DB_DYNAMIC_RANGE,
            .pre_db_offset = MEL_SPEC_LARGE_PRE_DB_OFFSET};
    mel_spec_normalisation_t const norm_opts =
        {
            .sub_mean = subtract_mean};
    mel_spec_trim_t trim_opts =
        {
            .trim_top_len = MEL_SPEC_LARGE_TRIM_START,
            .trim_end_len = MEL_SPEC_LARGE_TRIM_END};

    printf("x_mel_spec MEL_SPEC_LARGE\n");

    x_mel_spec(output,
               out_trim_top,
               out_trim_end,
               input,
               &out_shape,
               &mel_opts,
               &filters,
               &quant_opts,
               &db_opts,
               &norm_opts,
               &trim_opts);
    break;
  }
  default:
  {
    break;
  }
  }
}
