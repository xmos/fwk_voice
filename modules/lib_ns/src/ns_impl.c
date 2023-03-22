// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <limits.h>
#include "xmath/xmath.h"
#include "ns_priv.h"
#include <ns_api.h>

#define NS_SQRT_HANN_LUT           sqrt_hanning_480

// array which stores the first half of the window
int32_t sqrt_hanning_480[NS_WINDOW_LENGTH / 2] = {
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

//reverse window
void ns_priv_fill_rev_wind (int32_t * rev_wind, const int32_t * wind, const unsigned length){

	for (int v = 0; v < length / 2; v++){
		rev_wind[v] = wind[length - 1 - v];
		rev_wind[length - 1 - v] = wind[v];
	}
}

void ns_priv_bfp_init(bfp_s32_t * a, int32_t * data, unsigned length, int32_t value){

    bfp_s32_init(a, data, NS_INT_EXP, length, 0);
    bfp_s32_set(a, value, NS_INT_EXP);
}

// pack 512 frame using the input and previous frames
void ns_priv_pack_input(bfp_s32_t * current, const int32_t * input, bfp_s32_t * prev){

    memcpy(current->data, prev->data, (NS_PROC_FRAME_LENGTH - NS_FRAME_ADVANCE) * sizeof(int32_t));
    memcpy(&current->data[NS_PROC_FRAME_LENGTH - NS_FRAME_ADVANCE], input, NS_FRAME_ADVANCE * sizeof(int32_t));
    current->exp = NS_INT_EXP;
    bfp_s32_headroom(current);

    memcpy(prev->data, &prev->data[NS_FRAME_ADVANCE], (NS_PROC_FRAME_LENGTH - (2 * NS_FRAME_ADVANCE)) * sizeof(int32_t));
    memcpy(&prev->data[NS_PROC_FRAME_LENGTH - (2 * NS_FRAME_ADVANCE)], input, NS_FRAME_ADVANCE * sizeof(int32_t));
    prev->exp = NS_INT_EXP;
    bfp_s32_headroom(prev);
}

// apply overlap to the frame to get output and updated overlap
void ns_priv_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap){
    bfp_s32_t in_half, output;
    
    bfp_s32_init(&output, out, NS_INT_EXP, NS_FRAME_ADVANCE, 0);
    bfp_s32_init(&in_half, in->data, in->exp, NS_FRAME_ADVANCE, 1);

    bfp_s32_add(&output, &in_half, overlap);

    memcpy(overlap->data, &in->data[NS_FRAME_ADVANCE], NS_FRAME_ADVANCE * sizeof(int32_t));
    overlap->exp = in->exp;
    bfp_s32_headroom(overlap);

    bfp_s32_use_exponent(&output, NS_INT_EXP);
    bfp_s32_use_exponent(overlap, NS_INT_EXP);
}

// initialise state
void ns_init(ns_state_t * ns){
    memset(ns, 0, sizeof(ns_state_t));

    ns_priv_bfp_init(&ns->S, ns->data_S, NS_PROC_FRAME_BINS, INT_MAX);
    ns_priv_bfp_init(&ns->S_min, ns->data_S_min, NS_PROC_FRAME_BINS, INT_MAX);
    ns_priv_bfp_init(&ns->S_tmp, ns->data_S_tmp, NS_PROC_FRAME_BINS, INT_MAX);
    ns_priv_bfp_init(&ns->p, ns->data_p, NS_PROC_FRAME_BINS, 0);
    ns_priv_bfp_init(&ns->alpha_d_tilde, ns->data_adt, NS_PROC_FRAME_BINS, 0);
    ns_priv_bfp_init(&ns->lambda_hat, ns->data_lambda_hat, NS_PROC_FRAME_BINS, 0);

    ns_priv_bfp_init(&ns->prev_frame, ns->data_prev_frame, NS_PROC_FRAME_LENGTH - NS_FRAME_ADVANCE, 0);
    ns_priv_bfp_init(&ns->overlap, ns->data_ovelap, NS_FRAME_ADVANCE, 0);

    ns_priv_fill_rev_wind(ns->data_rev_wind, NS_SQRT_HANN_LUT, NS_WINDOW_LENGTH / 2);

    bfp_s32_init(&ns->wind, NS_SQRT_HANN_LUT, NS_INT_EXP, NS_WINDOW_LENGTH / 2, 1);
    bfp_s32_init(&ns->rev_wind, ns->data_rev_wind, NS_INT_EXP, NS_WINDOW_LENGTH / 2, 1);

    // delta = 1.5 exactly
    ns->delta.mant = 1610612736;
    ns->delta.exp = -30;

    // If modifying any of these parameters, modify one_minus parameters as well!!
    // alpha_d = 0.95 to 9 d.p.
    ns->alpha_d.mant = 2040109466;
    ns->alpha_d.exp = -31;
    // alpha_s = 0.8 to 9 d.p.
    ns->alpha_s.mant = 1717986919;
    ns->alpha_s.exp = -31;
    // alpha_p = 0.2 to 10 d.p.
    ns->alpha_p.mant = 1717986919;
    ns->alpha_p.exp = -33;

    // 1 - 0.95 to 9 d.p
    ns->one_minus_aplha_d.mant = 26843546;
    ns->one_minus_aplha_d.exp = -29;
    // since alpha_s = 0.8 and 1 - 0.8 = 0.2, we can use alpha_p value here
    ns->one_minus_alpha_s = ns->alpha_p;
    // since alpha_p = 0.2 and 1 - 0.2 = 0.8, we can use alpha_s value here
    ns->one_minus_alpha_p = ns->alpha_s;

    ns->reset_counter = 0;
    // we sample at 16 kHz and want to reset every 150 ms (every 10 frames)
    ns->reset_period = (unsigned)(16000.0 * 0.15);
}

void ns_priv_apply_window(bfp_s32_t * input, bfp_s32_t * window, bfp_s32_t * rev_window, const unsigned frame_length, const unsigned window_length){

    bfp_s32_t input_chunk[3];

    bfp_s32_init(&input_chunk[0], &input->data[0], input->exp, window_length / 2, 1);
    bfp_s32_init(&input_chunk[1], &input->data[window_length / 2], input->exp, window_length / 2, 1);
    bfp_s32_init(&input_chunk[2], &input->data[window_length], input->exp, frame_length - window_length, 1);

    bfp_s32_mul(&input_chunk[0], &input_chunk[0], window); 
    bfp_s32_mul(&input_chunk[1], &input_chunk[1], rev_window);
    bfp_s32_set(&input_chunk[2], 0, input->exp);
    
    bfp_s32_use_exponent(&input_chunk[0], input->exp);
    bfp_s32_use_exponent(&input_chunk[1], input->exp);
    bfp_s32_headroom(input);
}

// apply suppression
// does not work well in some cases and takes a lot of cycles
void ns_priv_rescale_vector_old(bfp_complex_s32_t * Y, bfp_s32_t * new_mag, bfp_s32_t * orig_mag){

    // leads to a poor precision
    //bfp_s32_inverse(orig_mag, orig_mag);
    //bfp_s32_mul(orig_mag, orig_mag, new_mag);

    vect_s32_shl(new_mag->data, new_mag->data, NS_PROC_FRAME_BINS, new_mag->hr);
    new_mag->exp -= new_mag->hr; new_mag->hr = 0;

    vect_s32_shl(orig_mag->data, orig_mag->data, NS_PROC_FRAME_BINS, orig_mag->hr);
    orig_mag->exp -= orig_mag->hr; orig_mag->hr = 0;

    int max_exp = INT_MIN;
    float_s32_t t, t1, t2[NS_PROC_FRAME_BINS];
    for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
        t.mant = orig_mag->data[v];
        t.exp = orig_mag->exp;
        t1.mant = new_mag->data[v];
        t1.exp = new_mag->exp;

        if(t1.mant != 0){
            t2[v] = float_s32_div(t1, t);
        }
        else {
            t2[v].mant = 0;
            t2[v].exp = NS_INT_EXP;
        }
        orig_mag->data[v] = t2[v].mant;
        if(t2[v].exp > max_exp){
            max_exp = t2[v].exp;
        }
    }
    orig_mag->exp = max_exp;

    for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
        if(t2[v].exp != max_exp){
            orig_mag->data[v] >>= (max_exp - t2[v].exp);
        }
    }

    // setting a headroom to be 1 to get the maximum precision and avoid overflow
    left_shift_t shl = bfp_s32_headroom(orig_mag) - 1;
    vect_s32_shl(orig_mag->data, orig_mag->data, NS_PROC_FRAME_BINS, shl);
    orig_mag->exp -= shl; orig_mag->hr = 1;
    
    bfp_complex_s32_real_mul(Y, Y, orig_mag);
}

void ns_priv_rescale(complex_s32_t * Y, int32_t new_mag, int32_t orig_mag){
    if(orig_mag){
        int64_t S = ((int64_t)new_mag)<<31;
        S /= orig_mag;
        // shift to be sure that S is 32 bit wide, and has a bit of headroom
        // tests have shown that sometimes S can be more then 32 bit wide
        // so taking the lower 32 bits of S will lead to the wrong answer
        // moreover when Y is also large S needs to have a bit of the headroom so the Y won't overflow
        // 4 is been tested to result the best precision in critical situations
        // you can think of it as 2 + 2 for the accurate int32_t cast and Y overflow protection
        // 3 will also works for most of test cases
        right_shift_t rsh = 4;
        S >>= rsh;
        Y->re =((int64_t)Y->re * (int64_t)S)>>(31 - rsh);
        Y->im =((int64_t)Y->im * (int64_t)S)>>(31 - rsh);
    }
}

// apply suppression
// new implementation
void ns_priv_rescale_vector(bfp_complex_s32_t * Y, bfp_s32_t * new_mag, bfp_s32_t * orig_mag){

    // preparing input data
    left_shift_t lsh = new_mag->hr;
    vect_s32_shl(new_mag->data, new_mag->data, NS_PROC_FRAME_BINS, lsh);
    new_mag->exp -= lsh;

    lsh = orig_mag->hr;
    vect_s32_shl(orig_mag->data, orig_mag->data, NS_PROC_FRAME_BINS, lsh);
    orig_mag->exp -= lsh;

    lsh = Y->hr - 2;
    vect_complex_s32_shl(Y->data, Y->data, NS_PROC_FRAME_BINS, lsh);
    Y->exp -= lsh;

    // modify the exponent
    right_shift_t delta_exp = orig_mag->exp - new_mag->exp;
    Y->exp -= delta_exp;

    // apply supression
    for(unsigned v = 0; v < NS_PROC_FRAME_BINS; v++){
        ns_priv_rescale(&Y->data[v], new_mag->data[v], orig_mag->data[v]);
    }

    bfp_complex_s32_headroom(Y);
}

void ns_process_frame(ns_state_t * ns,
                        int32_t output [NS_FRAME_ADVANCE],
                        const int32_t input[NS_FRAME_ADVANCE]){

    bfp_s32_t curr_frame, abs_Y_suppressed, abs_Y_original;
    int32_t DWORD_ALIGNED curr_frame_data[NS_PROC_FRAME_LENGTH + 2];
    int32_t scratch1[NS_PROC_FRAME_BINS];
    int32_t scratch2[NS_PROC_FRAME_BINS];
    bfp_s32_init(&curr_frame, curr_frame_data, NS_INT_EXP, NS_PROC_FRAME_LENGTH, 0);
    bfp_s32_init(&abs_Y_suppressed, scratch1, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);
    bfp_s32_init(&abs_Y_original, scratch2, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    ns_priv_pack_input(&curr_frame, input, &ns->prev_frame);
    
    ns_priv_apply_window(&curr_frame, &ns->wind, &ns->rev_wind, NS_PROC_FRAME_LENGTH, NS_WINDOW_LENGTH);

    bfp_complex_s32_t *curr_fft = bfp_fft_forward_mono(&curr_frame);
    curr_fft->hr = bfp_complex_s32_headroom(curr_fft); // TODO Workaround till https://github.com/xmos/lib_xcore_math/issues/96 is fixed
    bfp_fft_unpack_mono(curr_fft);

    bfp_complex_s32_mag(&abs_Y_suppressed, curr_fft);

    memcpy(abs_Y_original.data, abs_Y_suppressed.data, sizeof(int32_t) * NS_PROC_FRAME_BINS);
    abs_Y_original.exp = abs_Y_suppressed.exp;
    abs_Y_original.hr = abs_Y_suppressed.hr;

    ns_priv_process_frame(&abs_Y_suppressed, ns);

    ns_priv_rescale_vector(curr_fft, &abs_Y_suppressed, &abs_Y_original);
    // if using old implementation don't use abs_Y_orig after this point 

    bfp_fft_pack_mono(curr_fft);
    bfp_fft_inverse_mono(curr_fft);

    ns_priv_apply_window(&curr_frame, &ns->wind, &ns->rev_wind, NS_PROC_FRAME_LENGTH, NS_WINDOW_LENGTH);

    ns_priv_form_output(output, &curr_frame, &ns->overlap);
}
