// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved

#ifndef AGC_CONF_H_
#define AGC_CONF_H_

#define AGC_FRAME_ADVANCE           (240)
#define AGC_PROC_FRAME_LENGTH       (AGC_FRAME_ADVANCE)

#define AGC_INPUT_CHANNELS          (2)
#define AGC_CHANNEL_PAIRS           ((AGC_INPUT_CHANNELS+1)/2)

#define AGC_CH0_ADAPT               (1)
#define AGC_CH0_GAIN                (500)
#define AGC_CH0_MAX_GAIN            (1000)
#define AGC_CH0_UPPER_THRESHOLD     (0.7079)
#define AGC_CH0_LOWER_THRESHOLD     (0.1905)
#define AGC_CH0_GAIN_INC            (1.197)
#define AGC_CH0_GAIN_DEC            (0.87)

#define AGC_CH1_ADAPT               (1)
#define AGC_CH1_GAIN                (500)
#define AGC_CH1_MAX_GAIN            (1000)
#define AGC_CH1_UPPER_THRESHOLD     (0.4)
#define AGC_CH1_LOWER_THRESHOLD     (0.4)
#define AGC_CH1_GAIN_INC            (1.0121)
#define AGC_CH1_GAIN_DEC            (0.98804)
#define AGC_VAD_THRESHOLD           (205)

#define AGC_FILT_NUM_SECTIONS 2
#define AGC_FILT_NUM_COEFFS (AGC_FILT_NUM_SECTIONS * DSP_NUM_COEFFS_PER_BIQUAD)
#define AGC_FILT_STATE (AGC_FILT_NUM_SECTIONS * DSP_NUM_STATES_PER_BIQUAD)
#define AGC_FILT_Q_FORMAT (28)


extern int32_t AGC_FILT_COEFFS[AGC_FILT_NUM_COEFFS];

#endif /* AGC_CONF_H_ */
