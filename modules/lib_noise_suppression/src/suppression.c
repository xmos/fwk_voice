#include <stdint.h>
#include <string.h>
#include <suppression.h>
#include <limits.h>
#include <bfp_init.h>
#include <bfp_fft.h>
#include <xs3_math_types.h>
#include <bfp_complex_s32.h>
#include <scalar_float.h>
#include <suppression_window.h>


void sup_bfp_init(bfp_s32_t * a, int32_t * data, unsigned length, int32_t value){

    bfp_s32_init(a, data, INT_EXP, length, 0);
    bfp_s32_set(a, value, INT_EXP);
}

void fft(bfp_complex_s32_t *output,
        bfp_s32_t *input)
{
    //Input bfp_s32_t structure will get overwritten since FFT is computed in-place. Keep a copy of input->length and assign it back after fft call.
    //This is done to avoid having to call bfp_s32_init() on the input every frame
    int32_t len = input->length; 
    bfp_complex_s32_t *temp = bfp_fft_forward_mono(input);
    
    memcpy(output, temp, sizeof(bfp_complex_s32_t));
    bfp_fft_unpack_mono(output);
    input->length = len;
    return;
}

void ifft(bfp_s32_t *output,
        bfp_complex_s32_t *input)
{
    //Input bfp_complex_s32_t structure will get overwritten since IFFT is computed in-place. Keep a copy of input->length and assign it back after ifft call.
    //This is done to avoid having to call bfp_complex_s32_init() on the input every frame
    int32_t len = input->length;
    bfp_fft_pack_mono(input);
    bfp_s32_t *temp = bfp_fft_inverse_mono(input);
    memcpy(output, temp, sizeof(bfp_s32_t));

    input->length = len;
    return;
}

void sup_pack_input(bfp_s32_t * current, int32_t * input, bfp_s32_t * prev){

    memcpy(current->data, prev->data, (SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE) * sizeof(int32_t));
    memcpy(current->data[SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE], input, SUP_FRAME_ADVANCE * sizeof(int32_t));
    current->exp = INT_EXP;
    bfp_s32_headroom(current);

    memcpy(prev->data, prev->data[SUP_FRAME_ADVANCE], (SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)) * sizeof(int32_t));
    memcpy(prev->data[SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)], input, SUP_FRAME_ADVANCE * sizeof(int32_t));
    prev->exp = INT_EXP;
    bfp_s32_headroom(prev);
}

void sup_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap){
    bfp_s32_t in_half, output;

    bfp_s32_init(&output, out, INT_EXP, SUP_FRAME_ADVANCE, 0);
    bfp_s32_init(&in_half, in->data, in->exp, SUP_FRAME_ADVANCE, 0);
    bfp_s32_add(&output, &in_half, overlap);
    bfp_s32_init(overlap, in->data[SUP_FRAME_ADVANCE], in->exp, SUP_FRAME_ADVANCE, 0);

    bfp_s32_use_exponent(&output, INT_EXP);
    bfp_s32_use_exponent(overlap, INT_EXP);
    if (output.hr != 0){bfp_s32_shl(&output, &output, output.hr);}
    if (overlap->hr != 0){bfp_s32_shl(overlap, overlap, overlap->hr);}
}

void sup_init_state(suppression_state_t * state){
    memset(state, 0, sizeof(suppression_state_t));

    sup_bfp_init(&state->S, &state->data_S,SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->S_min, &state->data_S_min, SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->S_tmp, &state->data_S_tmp, SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->p, &state->data_p, SUP_PROC_FRAME_BINS, 0);
    sup_bfp_init(&state->alpha_d_title, &state->data_adt, SUP_PROC_FRAME_BINS, 0);
    sup_bfp_init(&state->lambda_hat, &state->data_lambda_hat, SUP_PROC_FRAME_BINS, 0);
    sup_bfp_init(&state->bin_gain, &state->data_bin_gain, SUP_PROC_FRAME_BINS, INT_MAX);

    sup_bfp_init(&state->prev_frame, &state->data_prev_frame, SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE, 0);
    sup_bfp_init(&state->overlap, &state->data_ovelap, SUP_FRAME_ADVANCE, 0);
    
    ////////////////////////////////////////////////todo: recalc
    state->reset_period = (unsigned)(16000.0 * 0.15);
    state->alpha_d = double_to_float_s32((double)INT_MAX * 0.95);
    state->alpha_s = double_to_float_s32((double)INT_MAX * 0.8);
    state->alpha_p = double_to_float_s32((double)INT_MAX * 0.2);
    state->noise_floor = double_to_float_s32((double)INT_MAX * 0.1258925412); // -18dB
    state->delta = double_to_float_s32(1.5);/////////////////////////////////q24
    state->alpha_gain = double_to_float_s32((double)INT_MAX);


    fill_rev_wind(&rev_wind, &sqrt_hanning_480, 120);
}

void sup_apply_window(bfp_s32_t * input, const int32_t * window, const int32_t * rev_window, const unsigned frame_length, const unsigned window_length){

    bfp_s32_t wind, rev_wind, input_chunk[3];
    bfp_s32_init(&wind, window, INT_EXP, window_length / 2, 0);
    bfp_s32_init(&rev_wind, rev_window, INT_EXP, window_length / 2, 0);

    bfp_s32_init(&input_chunk[0], input->data[0], input->exp, window_length / 2, 0);
    bfp_s32_init(&input_chunk[1], input->data[window_length / 2], input->exp, window_length / 2, 0);
    bfp_s32_init(&input_chunk[2], input->data[window_length], input->exp, frame_length - window_length, 0);

    bfp_s32_mul(&input_chunk[0], &input_chunk[0], &wind); 
    bfp_s32_mul(&input_chunk[1], &input_chunk[1], &rev_wind);
    bfp_s32_set(&input_chunk[2], 0, input->exp);
    
    adjust_exp(&input_chunk[0], &input_chunk[1], input);
    /*
    bfp_s32_use_exponent(&input_chunk[0], input->exp);
    bfp_s32_use_exponent(&input_chunk[1], input->exp);
    int min_hr = (input_chunk[0].hr < input_chunk[1].hr) ? input_chunk[0].hr : input_chunk[1].hr;
    min_hr = (min_hr < input->hr) ? min_hr : input->hr;
    input->hr = min_hr;
    */
}

void sup_rescale_vector(bfp_complex_s32_t * Y, bfp_s32_t * new_mag, bfp_s32_t * orig_mag){

    bfp_s32_inverse(orig_mag, orig_mag);
    bfp_s32_mul(orig_mag, orig_mag, new_mag);
    
    bfp_complex_s32_real_mul(Y, Y, orig_mag);
    Y->data->im = 0;
}

void sup_process_frame(suppression_state_t * state, int32_t (*output)[SUP_FRAME_ADVANCE], const int32_t (*input)[SUP_FRAME_ADVANCE]){

    int32_t current[SUP_PROC_FRAME_LENGTH] = {0};
    bfp_s32_t curr;
    bfp_s32_init(&curr, &current, INT_EXP, SUP_PROC_FRAME_LENGTH, 0);

    sup_pack_input(&curr, input, &state->prev_frame);

    sup_apply_window(&curr, &sqrt_hanning_480, &rev_wind, SUP_PROC_FRAME_LENGTH, SUPPRESSION_WINDOW_LENGTH);

    bfp_complex_s32_t curr_fft, *tmp1 = bfp_fft_forward_mono(&curr);
    memcpy(&curr_fft, tmp1, sizeof(bfp_complex_s32_t));
    //fft(&curr_fft, &curr);


    int32_t scratch1[SUP_PROC_FRAME_BINS] = {0};
    int32_t scratch2[SUP_PROC_FRAME_BINS] = {0};
    bfp_s32_t abs_Y_suppressed, abs_Y_original;
    bfp_s32_init(&abs_Y_suppressed, &scratch1, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&abs_Y_original, &scratch2, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    
    bfp_complex_s32_mag(&abs_Y_suppressed, &curr_fft);
    memcpy(&abs_Y_original, &abs_Y_suppressed, sizeof(abs_Y_suppressed));

    ns_process_frame(&abs_Y_suppressed, state);

    sup_rescale_vector(&curr_fft, &abs_Y_suppressed, &abs_Y_original);
    ////////////////////////////don't use abs_Y_orig after this point 

    bfp_s32_t *tmp2 = bfp_fft_inverse_mono(&curr_fft);
    memcpy(&curr, tmp2, sizeof(bfp_s32_t));
    //ifft(&curr, &curr_fft);

    sup_apply_window(&curr, &sqrt_hanning_480, &rev_wind, SUP_PROC_FRAME_LENGTH, SUPPRESSION_WINDOW_LENGTH);

    sup_form_output(output, &curr, &state->overlap);
}