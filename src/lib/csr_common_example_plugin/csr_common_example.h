/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_common_example.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_COMMON_EXAMPLE_H_
#define _CSR_COMMON_EXAMPLE_H_

#ifdef DEBUG_PRINT_ENABLED
#define KALIMBA_SEND_MESSAGE(id, a, b, c, d) \
if(!KalimbaSendMessage(id, a, b, c, d)) \
{\
    PRINT(("KalimbaSendMessageFailed %d\n", id)); \
    Panic(); \
}
#else
#define KALIMBA_SEND_MESSAGE(id, a, b, c, d) \
PanicFalse(KalimbaSendMessage(id, a, b, c, d));
#endif

/*plugin functions*/
void CsrExamplePluginConnect(const ExamplePluginTaskdata * const task, const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_msg);
void CsrExamplePluginDisconnect(const ExamplePluginTaskdata * const task);
void CsrExamplePluginSetVolume(const uint16 volume);
void CsrExamplePluginSetMode(const AUDIO_MODE_T mode);
void CsrExamplePluginSetSoftMute(const AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T * const message);
void CsrExamplePluginPlayTone(const ExamplePluginTaskdata * const task, const AUDIO_PLUGIN_PLAY_TONE_MSG_T * const tone_message);
void CsrExamplePluginStopTone(void);

/*internal plugin message functions*/
void CsrExamplePluginInternalMessage(const ExamplePluginTaskdata * const task, const uint16 id, const Message message);

void CsrExamplePluginToneComplete(const ExamplePluginTaskdata * const task);
#endif

