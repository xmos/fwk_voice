/*
 * usb_descriptors.h
 *
 *  Created on: May 12, 2021
 *      Author: mbruno
 */

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_


// Unit numbers are arbitrary selected
#define UAC2_ENTITY_CLOCK               0x01
// Speaker path
#define UAC2_ENTITY_SPK_INPUT_TERMINAL  0x11
#define UAC2_ENTITY_SPK_FEATURE_UNIT    0x12
#define UAC2_ENTITY_SPK_OUTPUT_TERMINAL 0x13

// Microphone path
#define UAC2_ENTITY_MIC_INPUT_TERMINAL  0x21
#define UAC2_ENTITY_MIC_FEATURE_UNIT    0x22
#define UAC2_ENTITY_MIC_OUTPUT_TERMINAL 0x23

#endif /* USB_DESCRIPTORS_H_ */
