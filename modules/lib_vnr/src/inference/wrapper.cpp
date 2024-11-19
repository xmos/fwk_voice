#include "model/trained_model_xcore.tflite.h"
#include "wrapper.h"

// Flag to make sure a given model is initilised only once. This is needed because the model/trained_model_xcore.cpp file have one time initialised non-const global
// values which will not be reset to their original values in subsequent calls to model_init() causing the initialisation to go wrong. 
static int32_t model_initialised = 0;

void vnr_priv_init_quant_spec(vnr_model_quant_spec_t * quant_spec) {
    quant_spec->input_scale_inv = f64_to_float_s32(1.0 / model_input_scale(0));
    quant_spec->input_zero_point = (float_s32_t){model_input_zeropoint(0), 0};

    quant_spec->output_scale = f32_to_float_s32(model_output_scale(0));
    quant_spec->output_zero_point = (float_s32_t){model_output_zeropoint(0), 0};
}

int32_t vnr_init(vnr_model_quant_spec_t * quant_spec) {
    if(!model_initialised)
    {
        model_initialised = 1;
        int32_t ret = model_init(NULL);

        vnr_priv_init_quant_spec(quant_spec);

        return ret;
    }
    else
    {
        return 0;
    }
}

int8_t* vnr_get_input() {
    return (int8_t*)model_input_ptr(0);
}

int8_t* vnr_get_output() {
    return (int8_t*)model_output_ptr(0);
}

void vnr_inference_invoke() {
    model_invoke();
}

