#ifndef __VNR_FEATURES_STATE_H__
#define __VNR_FEATURES_STATE_H__

#include "vnr_defines.h"
#include "xmath/xmath.h"

/**
 * @page page_vnr_features_state_h vnr_features_state.h
 * 
 * This header contains lib_vnr feature extraction related public #defines and data structure definitions 
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_features_state   VNR #define constants and data structure definitions
 */ 

/** @brief Time domain samples block length used internally in VNR DFT computation. 
 * NOT USER MODIFIABLE.
 *
 * @ingroup vnr_features_state   
 */
#define VNR_PROC_FRAME_LENGTH (512)

/** @brief VNR new samples frame size
 * This is the number of samples of new data that the VNR processes every frame. 240 samples at 16kHz is 15msec.
 * NOT USER MODIFIABLE.
 *
 * @ingroup vnr_features_state
 */
#define VNR_FRAME_ADVANCE (240)

/** Number of bins of spectrum data computed when doing a DFT of a VNR_PROC_FRAME_LENGTH length time domain vector. The
 * VNR_FD_FRAME_LENGTH spectrum values represent the bins from DC to Nyquist. NOT USER MODIFIABLE.
 *
 * @ingroup vnr_features_state
 */   
#define VNR_FD_FRAME_LENGTH ((VNR_PROC_FRAME_LENGTH/2)+1)

/**
 * @brief VNR form_input state structure
 *
 * @ingroup vnr_features_state
 */
typedef struct {
    /** Previous frame time domain input samples which are combined with VNR_FRAME_ADVANCE new samples to form the VNR input frame. */
    int32_t DWORD_ALIGNED prev_input_samples[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE];
}vnr_input_state_t;

/**
 * @brief VNR feature extraction config structure
 *
 * @ingroup vnr_features_state
 */
typedef struct {
    /** Enable highpass filtering of VNR MEL filter output. Disabled by default*/
    int32_t enable_highpass;
}vnr_feature_config_t;

/**
 * @brief State structure used in VNR feature extraction
 *
 * @ingroup vnr_features_state
 */
typedef struct {
    /** Feature buffer containing the most recent VNR_MEL_FILTERS frames' MEL frequency spectrum. */
    int32_t DWORD_ALIGNED feature_buffers[VNR_PATCH_WIDTH][VNR_MEL_FILTERS];
    vnr_feature_config_t config;
}vnr_feature_state_t;
#endif
