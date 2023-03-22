#ifndef __VNR_INFERENCE_API_H__
#define __VNR_INFERENCE_API_H__

/**
 * @page page_vnr_inference_api_h vnr_inference_api.h
 * 
 * This header contains lib_vnr inference engine API functions.
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_inference_api   VNR inference API functions
 */ 

#ifdef __cplusplus
extern "C" {
#endif
    #include "xmath/xmath.h"

    /**
     * @brief Initialise the inference_engine object and load the VNR model into the inference engine.
     *
     * This function calls lib_tflite_micro functions to initialise the inference engine and load the VNR model into it.
     * It is called once at startup. The memory required for the inference engine object as well as the tensor arena size required for inference 
     * is statically allocated as global buffers in the VNR module. The VNR model is compiled as part of the VNR module.
     *
     * @ingroup vnr_inference_api
     */
    int32_t vnr_inference_init();

    /**
     * @brief Run model prediction on a feature patch
     *
     * This function invokes the inference engine. It takes in a set of features corresponding to an input frame of data and outputs
     * the VNR prediction value. The VNR output is a single value ranging between 0 and 1 returned in float_s32_t format, with 0 being the lowest SNR
     * and 1 being the strongest possible SNR in speech compared to noise. 
     *
     * @param[out] vnr_output VNR prediction value.
     * @param[in] features Input feature vector. Note that this is not passed as a const pointer and the feature memory is overwritten as part of the inference computation. 
     * @ingroup vnr_inference_api
     */
    void vnr_inference(float_s32_t *vnr_output, bfp_s32_t *features);
#ifdef __cplusplus
}
#endif

#endif
