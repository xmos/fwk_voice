#include "model/outFile.cpp.h"
#include "wrapper.h"

int32_t vnr_init() {
    int32_t ret = model_init(NULL);
    return ret;
}
int8_t* vnr_get_input() {
    return model_input(0)->data.int8;
}

int8_t* vnr_get_output() {
    return model_output(0)->data.int8;
}

void vnr_inference_invoke() {
    model_invoke();
}

