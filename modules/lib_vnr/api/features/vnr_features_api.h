#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

/**
 * @page page_vnr_features_api_h vnr_features_api.h
 * 
 * This header contains lib_vnr features extraction API functions.
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_features_api   VNR feature extraction API functions
 */ 

/**
 * @brief Initialise previous frame samples buffer that is used when creating an input frame for processing through the VNR estimator.
 *
 * This function should be called once at device startup.
 *
 * @param[inout] input_state pointer to the VNR input state structure
 *
 * @ingroup vnr_features_api
 */
void vnr_input_state_init(vnr_input_state_t *input_state);

/**
 * @brief Create the input frame for processing through the VNR estimator.
 * 
 * This function takes in VNR_FRAME_ADVANCE new samples, combines them with previous frame's samples to form a
 * VNR_PROC_FRAME_LENGTH samples input frame of time domain data, and outputs the DFT spectrum of the input frame.
 * The DFT spectrum is output in the BFP structure and data memory provided by the user.
 *
 * The frequency spectrum output from this function is processed through the VNR feature extraction stage.
 *
 * If sharing the DFT spectrum calculated in some other module, vnr_form_input_frame() is not needed.
 *
 * @param[inout] input_state pointer to the VNR input state structure
 * @param[out] X pointer to a variable of type bfp_complex_s32_t that the user allocates. The user doesn't need to initialise this bfp variable. After this function,
 *             X is updated to point to the DFT output spectrum and can be passed as input to the feature extraction stage.
 * @param[out] X_data pointer to VNR_FD_FRAME_LENGTH values of type complex_s32_t that the user allocates. After this function, the DFT spectrum values are
 *             written to this array, and ``X->data`` points to X_data memory.
 * @param[in] new_x_frame Pointer to VNR_FRAME_ADVANCE new time domain samples
 *
 * @par Example
 * @code{.c}
 *      #include "vnr_features_api.h"
        complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_data);
 * @endcode
 * @ingroup vnr_features_api
 */
void vnr_form_input_frame(vnr_input_state_t *input_state,
        bfp_complex_s32_t *X,
        complex_s32_t X_data[VNR_FD_FRAME_LENGTH],
        const int32_t new_x_frame[VNR_FRAME_ADVANCE]);

/**
 * @brief Initialise the state structure for the VNR feature extraction stage.
 *
 * This function is called once at device startup.
 *
 * @param[inout] feature_state pointer to the VNR feature extraction state structure
 *
 * @ingroup vnr_features_api
 */
void vnr_feature_state_init(vnr_feature_state_t *feature_state);

/**
 * @brief Extract features.
 *
 * This function takes in DFT spectrum of the VNR input frame and does the feature extraction. The features are written to the feature_patch BFP
 * structure and feature_patch_data memory provided by the user. The feature output from this function are passed as input to the VNR inference engine.
 *
 * @param[inout] vnr_feature_state Pointer to the VNR feature extraction state structure
 * @param[out] feature_patch Pointer to the bfp_s32_t structure allocated by the user. The user doesn't need to initialise this BFP structure
 *             before passing it to this function. After this function call feature_patch will be updated and will point to the extracted features.
 *             It can then be passed to the inference stage.
 * @param[out] feature_patch_data Pointer to the VNR_PATCH_WIDTH * VNR_MEL_FILTERS int32_t values allocated by the user. The extracted features will be written
 *             to the feature_patch_data array and the BFP structure's ``feature_patch->data`` will point to this array.
 *
 * @ingroup vnr_features_api
 */
void vnr_extract_features(vnr_feature_state_t *vnr_feature_state,
        bfp_s32_t *feature_patch,
        int32_t feature_patch_data[VNR_PATCH_WIDTH * VNR_MEL_FILTERS],
        const bfp_complex_s32_t *X);

#endif
