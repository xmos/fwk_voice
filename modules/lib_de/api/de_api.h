#ifndef DE_API_H
#define DE_API_H

#include <stdio.h>
#include <string.h>
#include "de_state.h"

/// Estimate delay
void estimate_delay (
        de_output_t *de_state,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases);
#endif
