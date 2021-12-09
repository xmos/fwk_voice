

#include <suppression_state.h>
#include <xs3_math_types.h>

/*
 * Y is a complex array of length length-1
 * new_mag is a complex array of length length
 * original_mag is a complex array of length length
 */

//void sup_rescale(dsp_complex_t &Y, uint32_t new_mag, uint32_t original_mag);
void sup_rescale_vector(bfp_complex_s32_t * Y,
                        bfp_s32_t * new_mag,
                        bfp_s32_t * orig_mag);

void sup_pack_input(bfp_s32_t * current,
                    const int32_t * input,
                    bfp_s32_t * prev);
/*
 * window is an array of q1_31 of length (window_length/2) it is applied symmetrically to the frame
 * from sample 0 (oldest) to (window_length/2 - 1) then zeros are applied to the end of the frame.
 */
void sup_apply_window(bfp_s32_t * input, 
                    bfp_s32_t * window, 
                    bfp_s32_t * rev_window, 
                    const unsigned frame_length, 
                    const unsigned window_length);

void sup_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap);

void ns_update_S(bfp_s32_t * abs_Y,
        suppression_state_t * state);

void ns_minimum(bfp_s32_t * dst,
                bfp_s32_t * src1, 
                bfp_s32_t * src2);

void ns_update_p(suppression_state_t * state);

void ns_update_alpha_d_tilde(suppression_state_t * state);

void ns_update_lambda_hat(bfp_s32_t * abs_Y, 
                        suppression_state_t * state);

int ns_update_and_test_reset(suppression_state_t * state);

void ns_subtract_lambda_from_frame(bfp_s32_t * abs_Y, 
                                suppression_state_t * state);