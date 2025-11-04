
#include "vnr.h"

void vnr_init(vnr_ctx_t *ctx){
    // vnr input state init
    vnr_input_state_init(&ctx->vnr_input_state);
    // vnr feature state init
    vnr_feature_state_init(&ctx->vnr_feature_state);
    // vnr inference init
    int32_t err = vnr_inference_init();
    assert(err == 0);
}

void vnr_add_frame(vnr_ctx_t *ctx, int32_t new_frame[VNR_FRAME_ADVANCE]){
    // Store the new frame pointer
    ctx->input_frame = new_frame;
}

void vnr_compute(vnr_ctx_t *ctx){
    static complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    static bfp_complex_s32_t X;
    static bfp_s32_t feature_patch;
    static float_s32_t vnr_output_s32;
    static float vnr_output_float = 0.0f;

    vnr_form_input_frame(&ctx->vnr_input_state, &X, input_frame, ctx->input_frame);
    vnr_extract_features(&ctx->vnr_feature_state, &feature_patch, ctx->feature_patch_data, &X);
    vnr_inference(&vnr_output_s32, &feature_patch);
    vnr_output_float = ldexpf((float)vnr_output_s32.mant, vnr_output_s32.exp);
    ctx->vnr_output_s32 = vnr_output_s32;
    ctx->vnr_output_float32 = vnr_output_float;
}
