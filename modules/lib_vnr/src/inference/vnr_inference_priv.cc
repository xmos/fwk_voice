// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "vnr_defines.h"
#include "vnr_inference_priv.h"
#include "model/vnr_quant_spec_defines.h"
#include "xmath/xmath.h"

void vnr_priv_init_quant_spec(vnr_model_quant_spec_t *quant_spec)
{
    quant_spec->input_scale_inv = f64_to_float_s32(VNR_INPUT_SCALE_INV); //from interpreter_tflite.get_input_details()[0] call in python 
    quant_spec->input_zero_point = f64_to_float_s32(VNR_INPUT_ZERO_POINT);

    quant_spec->output_scale = f64_to_float_s32(VNR_OUTPUT_SCALE); //from interpreter_tflite.get_output_details()[0] call in python 
    quant_spec->output_zero_point = f64_to_float_s32(VNR_OUTPUT_ZERO_POINT);
}

#define Q24_EXP (-24)
void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_model_quant_spec_t *quant_spec) {
    // this_patch = this_patch / input_scale + input_zero_point        
    bfp_s32_scale(normalised_patch, normalised_patch, quant_spec->input_scale_inv); // normalised_patch gets overwritten here!
    bfp_s32_add_scalar(normalised_patch, normalised_patch, quant_spec->input_zero_point);
    // this_patch = np.round(this_patch)
    bfp_s32_use_exponent(normalised_patch, Q24_EXP);
    
    float_s32_t round_lsb = {1<< (-(Q24_EXP+1)) , Q24_EXP}; //0.5 in Q24 format
    bfp_s32_add_scalar(normalised_patch, normalised_patch, round_lsb);
    bfp_s32_use_exponent(normalised_patch, Q24_EXP);
    left_shift_t ls = Q24_EXP;
    bfp_s32_shl(normalised_patch, normalised_patch, ls); // Get the integer part of the 8.24
    normalised_patch->exp -= Q24_EXP; 
 
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        quantised_patch[i] = (int8_t)normalised_patch->data[i]; // Copy quantised features
    }
}

void vnr_priv_output_dequantise(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_model_quant_spec_t *quant_spec) {
    // output_data_float = (output_data_float - output_zero_point)*output_scale
    dequant_output->mant = ((int32_t)*quant_output) << (-Q24_EXP); //8.24
    dequant_output->exp = Q24_EXP;
    *dequant_output = float_s32_sub(*dequant_output, quant_spec->output_zero_point);
    *dequant_output = float_s32_mul(*dequant_output, quant_spec->output_scale);
}
