// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "adec_state.h"

/** Function that initialises ADEC state
 */
void adec_init(adec_state_t *state);

/** Function that takes statistics about the signals and decides if a delay
 *  estimation and/or a delay change and AEC reset is required
 */
void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const adec_input_t *adec_in);

/// Estimate delay
void estimate_delay (
        de_output_t *de_state,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases);
