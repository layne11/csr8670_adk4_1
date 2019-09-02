// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// *****************************************************************************


#ifndef two_mic_example_LIB_H
#define two_mic_example_LIB_H


.CONST $two_mic_example.REINITIALIZE                    1;
.CONST $two_mic_example.VMMSG.SETMODE                   4;
.CONST $two_mic_example.VMMSG.READY                     5;
.CONST $two_mic_example.VM_SET_TONE_RATE_MESSAGE_ID     0x1072;         // Set the tone/prompt sampling rate from the VM
.CONST $two_mic_example.PLAY_BACK_FINISHED_MSG          0x1080;         // Indicate tone/prompt finished to VM
.CONST $two_mic_example.MESSAGE_REM_BT_ADDRESS          0x2001;

.CONST $two_mic_example.$PCM_END_DETECTION_TIME_OUT     40;

// System Modes
.CONST $two_mic_example.SYSMODE.PASSTHRU       0;

// Data block size
#if uses_16kHz
.CONST $two_mic_example.NUM_SAMPLES_PER_FRAME  120;
.CONST $two_mic_example.JITTER                 16;
#else
.CONST $two_mic_example.NUM_SAMPLES_PER_FRAME  60;
.CONST $two_mic_example.JITTER                 8;
#endif


#endif
