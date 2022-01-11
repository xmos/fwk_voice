#ifndef DE_STATE_H
#define DE_STATE_H

#include "de_defines.h"
/**
 * @ingroup de_types
 */
typedef struct {
    int32_t measured_delay; ///< Estimated delay in samples
    int32_t peak_power_phase_index; ///< H_hat phase index with the maximum energy
    float_s32_t peak_phase_power; ///< Maximum energy across all H_hat phases
    float_s32_t sum_phase_powers; ///< Sum of filter energy across all filter phases. Used in peak_to_average_ratio calculation. 
    float_s32_t peak_to_average_ratio; ///< peak to average ratio of H_hat phase energy.
    float_s32_t phase_power[DE_LIB_MAX_PHASES]; ///< Energy for every H_hat phase
}de_output_t;

#endif
