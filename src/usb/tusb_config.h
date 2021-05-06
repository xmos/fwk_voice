/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUSB_RHPORT0_MODE      (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#define CFG_TUSB_OS                OPT_OS_CUSTOM

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG             0
#endif

#define CFG_TUSB_MEM_ALIGN         __attribute__ ((aligned(4)))

#define CFG_TUSB_DEBUG_PRINTF     rtos_printf

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#define CFG_TUD_EP_MAX            12
#define CFG_TUD_TASK_QUEUE_SZ     8
#define CFG_TUD_ENDPOINT0_SIZE    64

#define CFG_TUD_XCORE_INTERRUPT_CORE 1
#define CFG_TUD_XCORE_IO_CORE_MASK (1 << 4)

//------------- CLASS -------------//
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_AUDIO             1
#define CFG_TUD_VENDOR            0

//--------------------------------------------------------------------
// AUDIO CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                                 TUD_AUDIO_MIC_ONE_CH_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT                                 1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ                              64

// Audio format type I specifications
#define CFG_TUD_AUDIO_FORMAT_TYPE_I_TX 				AUDIO_DATA_FORMAT_TYPE_I_PCM
#define CFG_TUD_AUDIO_N_CHANNELS_TX 				2
#define CFG_TUD_AUDIO_N_BYTES_PER_SAMPLE_TX			2

#define CFG_TUD_AUDIO_ENABLE_EP_IN                                    1
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX                    2                                       // Driver gets this info from the descriptors - we define it here to use it to setup the descriptors and to do calculations with it below
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX                            2                                       // Driver gets this info from the descriptors - we define it here to use it to setup the descriptors and to do calculations with it below - be aware: for different number of channels you need another descriptor!


// EP and buffer sizes
#define SAMPLES_PER_FRAME_NOMINAL                   (16000 / 1000)
#define SAMPLES_PER_FRAME_EXTRA                     (SAMPLES_PER_FRAME_NOMINAL + 2)

#define BYTES_PER_FRAME_NOMINAL                     (SAMPLES_PER_FRAME_NOMINAL * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)
#define BYTES_PER_FRAME_EXTRA                       (SAMPLES_PER_FRAME_EXTRA   * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)

#define CFG_TUD_AUDIO_EPSIZE_IN                     BYTES_PER_FRAME_EXTRA
#define CFG_TUD_AUDIO_TX_FIFO_SIZE                  BYTES_PER_FRAME_EXTRA

#define CFG_TUD_AUDIO_EP_SZ_IN                      CFG_TUD_AUDIO_EPSIZE_IN
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX           CFG_TUD_AUDIO_EP_SZ_IN                  // Maximum EP IN size for all AS alternate settings used
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ        CFG_TUD_AUDIO_TX_FIFO_SIZE


#endif /* _TUSB_CONFIG_H_ */
