// Copyright 2022 XMOS LIMITED.
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
        adec_config_t adec_conf;
        adec_conf.bypass = 1; // Bypass automatic DE correction
        adec_conf.force_de_cycle_trigger = 1; // Force a delay correction cycle, so that delay correction happens once after initialisation
        adec_init(&adec_state, &adec_conf);
        // Application needs to ensure that adec_state->adec_config.force_de_cycle_trigger is set to 0 after ADEC has requested a transition to delay estimation mode once in order to ensure that delay is corrected only at startup.  
 * @endcode
 *
 * @par Example with ADEC configured for automatic delay estimation and correction
 * @code{.c}
 *      adec_state_t adec_state;
        adec_conf.bypass = 0;
        adec_conf.force_de_cycle_trigger = 0;
        adec_init(&adec_state, &adec_conf);
 * @endcode
 *
 * @ingroup adec_func
 */
void adec_init(adec_state_t *state, adec_config_t *config);

/** @brief Perform ADEC processing on an input frame of data
 * 
 * This function takes information about the latest AEC processed frame and the latest measured delay estimate as input, and decides if a delay correction between input microphone and reference signals is required.
 * If a correction is needed, it outputs a new requested input delay, optionally accompanied with a request for AEC restart in a different configuration.
 * It updates the internal ADEC state structure to reflect the current state of the ADEC process.
 *
 * @param[inout] state ADEC internal state structure
 * @param[out] adec_output ADEC output structure
 * @param[in] adec_in ADEC input structure
 * 
 * @ingroup adec_func
 */
void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const adec_input_t *adec_in);

/** @brief Estimate microphone delay
 * 
 * This function measures the microphone signal delay wrt the reference signal. It does so by
 * looking for the phase with the peak energy among all AEC filter phases and uses the peak energy phase index
 * as the estimate of the microphone delay. Along with the measured delay, it also outputs information
 * about the peak phase energy that can then be used to gauge the AEC filter convergence and the reliability of the
 * measured delay.
 *
 * @param[out] de_state Delay estimator output structure
 * @param[in] H_hat bfp_complex_s32_t array storing the AEC filter spectrum
 * @param[in] Number of phases in the AEC filter
 * 
 * @ingroup adec_func
 */
void adec_estimate_delay (
        de_output_t *de_output,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases);
