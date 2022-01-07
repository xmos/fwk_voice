// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <xs3_math.h>
#include <bfp_init.h>
#include <bfp_s32.h>
#include <bfp_fft.h>
#include <bfp_complex_s32.h>

#include <suppression.h>
#include "suppression_ns.h"
#include "suppression_testing.h"

#define SUP_SQRT_HANN_LUT           sqrt_hanning_480

int32_t sqrt_hanning_480[SUPPRESSION_WINDOW_LENGTH / 2] = {
	13996827, 27993059, 41988102, 55981362, 69972243, 83960152, 97944494, 111924676, 
	125900102, 139870180, 153834316, 167791917, 181742389, 195685141, 209619580, 223545114, 
	237461151, 251367100, 265262371, 279146373, 293018516, 306878212, 320724870, 334557904, 
	348376724, 362180746, 375969381, 389742044, 403498150, 417237114, 430958354, 444661286, 
	458345328, 472009898, 485654416, 499278303, 512880980, 526461868, 540020391, 553555974, 
	567068040, 580556016, 594019328, 607457406, 620869678, 634255574, 647614526, 660945965, 
	674249327, 687524045, 700769556, 713985296, 727170706, 740325223, 753448290, 766539350, 
	779597845, 792623221, 805614925, 818572406, 831495111, 844382493, 857234004, 870049099, 
	882827231, 895567860, 908270443, 920934441, 933559316, 946144532, 958689554, 971193848, 
	983656885, 996078134, 1008457067, 1020793159, 1033085887, 1045334726, 1057539158, 1069698663, 
	1081812726, 1093880831, 1105902466, 1117877120, 1129804285, 1141683453, 1153514121, 1165295785, 
	1177027945, 1188710102, 1200341761, 1211922427, 1223451608, 1234928815, 1246353559, 1257725356, 
	1269043723, 1280308178, 1291518243, 1302673442, 1313773300, 1324817348, 1335805114, 1346736133, 
	1357609940, 1368426073, 1379184073, 1389883482, 1400523846, 1411104714, 1421625635, 1432086162, 
	1442485852, 1452824262, 1463100954, 1473315490, 1483467437, 1493556363, 1503581841, 1513543443, 
	1523440747, 1533273332, 1543040781, 1552742678, 1562378612, 1571948173, 1581450955, 1590886554, 
	1600254569, 1609554602, 1618786258, 1627949145, 1637042873, 1646067057, 1655021312, 1663905259, 
	1672718521, 1681460722, 1690131491, 1698730460, 1707257264, 1715711540, 1724092930, 1732401077, 
	1740635628, 1748796234, 1756882547, 1764894225, 1772830926, 1780692314, 1788478055, 1796187818, 
	1803821276, 1811378104, 1818857981, 1826260589, 1833585614, 1840832745, 1848001674, 1855092097, 
	1862103711, 1869036220, 1875889328, 1882662745, 1889356183, 1895969358, 1902501988, 1908953796, 
	1915324508, 1921613853, 1927821565, 1933947379, 1939991035, 1945952277, 1951830851, 1957626507, 
	1963339000, 1968968086, 1974513527, 1979975086, 1985352533, 1990645637, 1995854176, 2000977926, 
	2006016672, 2010970198, 2015838294, 2020620753, 2025317372, 2029927952, 2034452297, 2038890215, 
	2043241516, 2047506017, 2051683535, 2055773894, 2059776920, 2063692443, 2067520296, 2071260317, 
	2074912347, 2078476230, 2081951816, 2085338956, 2088637508, 2091847329, 2094968286, 2098000244, 
	2100943075, 2103796653, 2106560859, 2109235574, 2111820684, 2114316080, 2116721656, 2119037310, 
	2121262942, 2123398460, 2125443771, 2127398790, 2129263432, 2131037620, 2132721276, 2134314331, 
	2135816716, 2137228367, 2138549225, 2139779233, 2140918339, 2141966495, 2142923656, 2143789781, 
	2144564834, 2145248782, 2145841596, 2146343250, 2146753723, 2147072999, 2147301062, 2147437904, 
	};

void sup_fill_rev_wind (int32_t * rev_wind, int32_t * wind, const unsigned length){

	for (int v = 0; v < length / 2; v++){
		rev_wind[v] = wind[length - 1 - v];
		rev_wind[length - 1 - v] = wind[v];
	}
}

void sup_set_noise_floor(sup_state_t * state, float_s32_t noise_floor){

    state->noise_floor = noise_floor;

}

void sup_set_noise_delta(sup_state_t * state, float_s32_t delta){

    state->delta = delta;

}

void sup_set_noise_alpha_p(sup_state_t * state, float_s32_t alpha_p){

    state->alpha_p = alpha_p;

}

void sup_set_noise_alpha_d(sup_state_t * state, float_s32_t alpha_d){

    state->alpha_d = alpha_d;

}

void sup_set_noise_alpha_s(sup_state_t * state, float_s32_t alpha_s){

    state->alpha_s = alpha_s;

}

void sup_reset_noise_suppression(sup_state_t * sup) {
    
    sup->reset_counter = 0;

    bfp_s32_set(&sup->S, INT_MAX, INT_EXP);
    bfp_s32_set(&sup->S_tmp, INT_MAX, INT_EXP);
    bfp_s32_set(&sup->S_min, INT_MAX, INT_EXP);
    bfp_s32_set(&sup->p, 0, INT_EXP);
    bfp_s32_set(&sup->lambda_hat, 0, INT_EXP);
}

void sup_bfp_init(bfp_s32_t * a, int32_t * data, unsigned length, int32_t value){

    bfp_s32_init(a, data, INT_EXP, length, 0);
    bfp_s32_set(a, value, INT_EXP);
}

void sup_pack_input(bfp_s32_t * current, const int32_t * input, bfp_s32_t * prev){

    memcpy(current->data, prev->data, (SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE) * sizeof(int32_t));
    memcpy(&current->data[SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE], input, SUP_FRAME_ADVANCE * sizeof(int32_t));
    current->exp = INT_EXP;
    bfp_s32_headroom(current);

    memcpy(prev->data, &prev->data[SUP_FRAME_ADVANCE], (SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)) * sizeof(int32_t));
    memcpy(&prev->data[SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)], input, SUP_FRAME_ADVANCE * sizeof(int32_t));
    prev->exp = INT_EXP;
    bfp_s32_headroom(prev);
}

void sup_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap){
    bfp_s32_t in_half, output;
    
    bfp_s32_init(&output, out, INT_EXP, SUP_FRAME_ADVANCE, 0);
    bfp_s32_init(&in_half, in->data, in->exp, SUP_FRAME_ADVANCE, 1);

    bfp_s32_add(&output, &in_half, overlap);

    memcpy(overlap->data, &in->data[SUP_FRAME_ADVANCE], SUP_FRAME_ADVANCE * sizeof(int32_t));
    overlap->exp = in->exp;
    bfp_s32_headroom(overlap);

    bfp_s32_use_exponent(&output, INT_EXP);
    bfp_s32_use_exponent(overlap, INT_EXP);
}

void sup_init_state(sup_state_t * state){
    memset(state, 0, sizeof(sup_state_t));

    sup_bfp_init(&state->S, state->data_S,SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->S_min, state->data_S_min, SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->S_tmp, state->data_S_tmp, SUP_PROC_FRAME_BINS, INT_MAX);
    sup_bfp_init(&state->p, state->data_p, SUP_PROC_FRAME_BINS, 0);
    sup_bfp_init(&state->alpha_d_tilde, state->data_adt, SUP_PROC_FRAME_BINS, 0);
    sup_bfp_init(&state->lambda_hat, state->data_lambda_hat, SUP_PROC_FRAME_BINS, 0);

    sup_bfp_init(&state->prev_frame, state->data_prev_frame, SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE, 0);
    sup_bfp_init(&state->overlap, state->data_ovelap, SUP_FRAME_ADVANCE, 0);

    sup_fill_rev_wind(state->data_rev_wind, SUP_SQRT_HANN_LUT, SUPPRESSION_WINDOW_LENGTH / 4);

    bfp_s32_init(&state->wind, SUP_SQRT_HANN_LUT, INT_EXP, SUPPRESSION_WINDOW_LENGTH / 2, 1);
    bfp_s32_init(&state->rev_wind, state->data_rev_wind, INT_EXP, SUPPRESSION_WINDOW_LENGTH / 2, 1);
    
    state->reset_period = (unsigned)(16000.0 * 0.15);
    state->alpha_d = float_to_float_s32(0.95);
    state->alpha_s = float_to_float_s32(0.8);
    state->alpha_p = float_to_float_s32(0.2);
    state->noise_floor = float_to_float_s32(0.1258925412); // -18dB
    state->delta = float_to_float_s32(1.5);
}

void sup_apply_window(bfp_s32_t * input, bfp_s32_t * window, bfp_s32_t * rev_window, const unsigned frame_length, const unsigned window_length){

    bfp_s32_t input_chunk[3];

    bfp_s32_init(&input_chunk[0], &input->data[0], input->exp, window_length / 2, 1);
    bfp_s32_init(&input_chunk[1], &input->data[window_length / 2], input->exp, window_length / 2, 1);
    bfp_s32_init(&input_chunk[2], &input->data[window_length], input->exp, frame_length - window_length, 1);

    bfp_s32_mul(&input_chunk[0], &input_chunk[0], window); 
    bfp_s32_mul(&input_chunk[1], &input_chunk[1], rev_window);
    bfp_s32_set(&input_chunk[2], 0, input->exp);
    
    ns_adjust_exp(&input_chunk[0], &input_chunk[1], input);
    bfp_s32_headroom(input);
}

void sup_rescale_vector(bfp_complex_s32_t * Y, bfp_s32_t * new_mag, bfp_s32_t * orig_mag){

    int max_exp = INT_MIN;
    float_s32_t t, t1, t2[SUP_PROC_FRAME_BINS];
    //bfp_s32_inverse(orig_mag, orig_mag);
    //bfp_s32_mul(orig_mag, orig_mag, new_mag);

    for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
        t.mant = orig_mag->data[v];
        t.exp = orig_mag->exp;
        t1.mant = new_mag->data[v];
        t1.exp = new_mag->exp;

        if(t1.mant != 0)t2[v] = float_s32_div(t1, t);
        else {t2[v].mant = 0;t2[v].exp = t1.exp;}
        orig_mag->data[v] = t2[v].mant;
        if(t2[v].exp > max_exp)max_exp = t2[v].exp;
    }
    orig_mag->exp = max_exp;

    for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
        if(t2[v].exp != max_exp){
            orig_mag->data[v] >>= (max_exp - t2[v].exp);
        }
    }

    bfp_s32_headroom(orig_mag);
    
    bfp_complex_s32_real_mul(Y, Y, orig_mag);

    Y->data[0].im = 0;
}

void sup_process_frame(sup_state_t * state,
                        int32_t output [SUP_FRAME_ADVANCE],
                        const int32_t input[SUP_FRAME_ADVANCE]){

    bfp_s32_t curr_frame, abs_Y_suppressed, abs_Y_original;
    int32_t curr_frame_data[SUP_PROC_FRAME_LENGTH];
    int32_t scratch1[SUP_PROC_FRAME_BINS];
    int32_t scratch2[SUP_PROC_FRAME_BINS];
    bfp_s32_init(&curr_frame, curr_frame_data, INT_EXP, SUP_PROC_FRAME_LENGTH, 0);
    bfp_s32_init(&abs_Y_suppressed, scratch1, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&abs_Y_original, scratch2, INT_EXP, SUP_PROC_FRAME_BINS, 0);

    sup_pack_input(&curr_frame, input, &state->prev_frame);
    
    sup_apply_window(&curr_frame, &state->wind, &state->rev_wind, SUP_PROC_FRAME_LENGTH, SUPPRESSION_WINDOW_LENGTH);

    bfp_complex_s32_t *curr_fft = bfp_fft_forward_mono(&curr_frame);
    
    bfp_complex_s32_mag(&abs_Y_suppressed, curr_fft);

    memcpy(&abs_Y_original.data, &abs_Y_suppressed.data, sizeof(abs_Y_suppressed.data));
    abs_Y_original.exp = abs_Y_suppressed.exp;
    abs_Y_original.hr = abs_Y_suppressed.hr;

    ns_process_frame(&abs_Y_suppressed, state);

    sup_rescale_vector(curr_fft, &abs_Y_suppressed, &abs_Y_original);
    ////////////////////////////don't use abs_Y_orig after this point 

    bfp_fft_inverse_mono(curr_fft);

    sup_apply_window(&curr_frame, &state->wind, &state->rev_wind, SUP_PROC_FRAME_LENGTH, SUPPRESSION_WINDOW_LENGTH);

    sup_form_output(output, &curr_frame, &state->overlap);
    memcpy(state->prev_frame.data, curr_frame.data, SUP_PROC_FRAME_LENGTH * sizeof(int32_t));
    state->prev_frame.exp = curr_frame.exp;
    state->prev_frame.hr = curr_frame.hr;
}