// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "vnr_inference_priv.h"
#include "vnr_quant_spec_defines.h"

void vnr_priv_init_ie_config(vnr_ie_config_t *ie_config)
{
    ie_config->input_scale_inv = double_to_float_s32(VNR_INPUT_SCALE_INV); //from interpreter_tflite.get_input_details()[0] call in python 
    ie_config->input_zero_point = double_to_float_s32(VNR_INPUT_ZERO_POINT);

    ie_config->output_scale = double_to_float_s32(VNR_OUTPUT_SCALE); //from interpreter_tflite.get_output_details()[0] call in python 
    ie_config->output_zero_point = double_to_float_s32(VNR_OUTPUT_ZERO_POINT);
}

void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_ie_config_t *ie_config) {
    // this_patch = this_patch / input_scale + input_zero_point        
    bfp_s32_scale(normalised_patch, normalised_patch, ie_config->input_scale_inv);
    bfp_s32_add_scalar(normalised_patch, normalised_patch, ie_config->input_zero_point);
    // this_patch = np.round(this_patch)
    bfp_s32_use_exponent(normalised_patch, -24);

    /*printf("framenum %d\n",framenum);
    printf("Before rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/
    //Test a few values
    /*normalised_patch->data[0] = (int)((2.3278371)*(1<<24));
    normalised_patch->data[1] = (int)((-2.3278371)*(1<<24));
    normalised_patch->data[2] = (int)((2.5809)*(1<<24));
    normalised_patch->data[3] = (int)((-2.5809)*(1<<24));
    normalised_patch->data[4] = (int)((2.50000)*(1<<24));
    normalised_patch->data[5] = (int)((-2.50000)*(1<<24));
    normalised_patch->data[6] = (int)((4.5)*(1<<24));
    normalised_patch->data[7] = (int)((-4.5)*(1<<24));
    normalised_patch->data[8] = (int)((5.5)*(1<<24));
    normalised_patch->data[9] = (int)((-5.5)*(1<<24));
    normalised_patch->data[10] = (int)((5.51)*(1<<24));
    normalised_patch->data[11] = (int)((-5.51)*(1<<24));*/
    float_s32_t round_lsb = {(1<<23), -24};
    bfp_s32_add_scalar(normalised_patch, normalised_patch, round_lsb);
    bfp_s32_use_exponent(normalised_patch, -24);
    left_shift_t ls = -24;
    bfp_s32_shl(normalised_patch, normalised_patch, ls);
    normalised_patch->exp -= -24; 

    /*printf("In rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/
    
    for(int i=0; i<96; i++) {
        quantised_patch[i] = (int8_t)normalised_patch->data[i]; 
    }

    /*printf("quant_cycles = %ld\n", (int32_t)(end-start));
    printf("After rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f, final_val = %d\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp), quantised_patch[i]);
    }*/
}

void vnr_priv_output_dequantise(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_ie_config_t *ie_config) {
    // output_data_float = (output_data_float - output_zero_point)*output_scale
    dequant_output->mant = ((int32_t)*quant_output) << 24; //8.24
    dequant_output->exp = -24;
    *dequant_output = float_s32_sub(*dequant_output, ie_config->output_zero_point);
    *dequant_output = float_s32_mul(*dequant_output, ie_config->output_scale);
}
