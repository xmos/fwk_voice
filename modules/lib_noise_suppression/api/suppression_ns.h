#include <suppression_state.h>
#include <xs3_math_types.h>

void ns_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main);

void ns_process_frame(bfp_s32_t * abs_Y, suppression_state_t * state);