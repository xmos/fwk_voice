// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "adec_state.h"

/**
 * @page page_adec_api_h adec_api.h
 * 
 * lib_adec public functions API.
 *
 * @ingroup adec_header_file
 */

/**
 * @defgroup adec_func     ADEC API Functions
 */

/** @brief Initialise ADEC data structures
 * 
 * This function initialises ADEC state for a given configuration. It must be called at startup to initialise the ADEC
 * data structures before processing any frames, and can be called at any time after that to reset the ADEC instance, returning
 * the internal ADEC state to its defaults.
 * 
 * @param[out] state Pointer to ADEC state structure
 * @param[in] config Pointer to ADEC configuration structure.
 * 
 * @par Example with ADEC configured for delay estimation only at startup
 * @code{.c}
 *      adec_state_t adec_state;
        adec_config_t adec_conf = ADEC_CONFIG_DE_ONLY_AT_STARTUP;
        adec_init(&adec_state, &adec_conf);
 * @endcode
 *
 * @par Example with ADEC configured for automatic delay estimation and control
 * @code{.c}
 *      adec_state_t adec_state;
        adec_config_t adec_conf = ADEC_CONFIG_AUTOMATIC_DE_CONTROL;
        adec_init(&adec_state, &adec_conf);
 * @endcode
 *
 * @ingroup adec_func
 */
void adec_init(adec_state_t *state, adec_config_t *config);

/** Function that takes statistics about the signals and decides if a delay
 *  estimation and/or a delay change and AEC reset is required
 */
void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const adec_input_t *adec_in);

/// Estimate delay
void adec_estimate_delay (
        de_output_t *de_state,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases);
