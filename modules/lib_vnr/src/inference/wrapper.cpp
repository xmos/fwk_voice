#include "model/trained_model_xcore.cpp.h"
#include "wrapper.h"
#include <string.h>

// Flag to make sure a given model is initilised only once. This is needed because the model/trained_model_xcore.cpp file have one time initialised non-const global
// values which will not be reset to their original values in subsequent calls to model_init() causing the initialisation to go wrong. 
static int32_t model_initialised = 0;

int32_t vnr_init() {
    if(!model_initialised)
    {
        model_initialised = 1;
        int32_t ret = model_init(NULL);
        return ret;
    }
    else
    {
        return 0;
    }
}

int8_t* vnr_get_input() {
    return model_input(0)->data.int8;
}

int8_t* vnr_get_output() {
    return model_output(0)->data.int8;
}

void vnr_inference_invoke() {
    static int8_t state_h[32];
    static int8_t state_c[32];

    // set the LSTM states
    memcpy(model_input(1), state_h, 32);
    memcpy(model_input(2), state_c, 32);

    model_invoke();

    // save the LSTM states
    memcpy(state_h, model_output(1), 32);
    memcpy(state_c, model_output(2), 32);

}

