/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_dut_audio.h

DESCRIPTION
    Audio plugin for DUT mode

*/

#ifndef _CSR_DUT_AUDIO_H_
#define _CSR_DUT_AUDIO_H_


void CsrDutAudioPluginConnect(const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_message);

void CsrDutAudioPluginDisconnect(void);

void CsrDutAudioPluginSetMode(const AUDIO_MODE_T mode);

void CsrDutAudioPluginSetVolume(const uint16 volume);

void CsrDutAudioPluginPlayTone(const AUDIO_PLUGIN_PLAY_TONE_MSG_T * const tone_message);

void CsrDutAudioPluginStopTone(void);

bool CsrDutAudioPluginToneComplete(void);

void CsrDutAudioPluginRepeatTone(Task task);


#endif

