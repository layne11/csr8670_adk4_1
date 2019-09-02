/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_ag_audio.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_AG_AUDIO_H_
#define _CSR_AG_AUDIO_H_

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
void CsrAgAudioPluginConnect( CsrAgAudioPluginTaskData *task, Sink audio_sink ,AUDIO_SINK_T sink_type , uint16 volume , uint32 rate , bool stereo , AUDIO_MODE_T mode , const void * params , Task app_task) ;
void CsrAgAudioPluginDisconnect(CsrAgAudioPluginTaskData *task) ;
void CsrAgAudioPluginSetVolume( CsrAgAudioPluginTaskData *task, uint16 volume ) ;
void CsrAgAudioPluginSetMode ( CsrAgAudioPluginTaskData *task, AUDIO_MODE_T mode , const void * params ) ;
void CsrAgAudioPluginPlayTone (CsrAgAudioPluginTaskData *task, const ringtone_note * tone , uint16 tone_volume) ;
void CsrAgAudioPluginStopTone (  void) ;

/*internal plugin message functions*/
void CsrAgAudioPluginInternalMessage( CsrAgAudioPluginTaskData *task ,uint16 id , Message message ) ;

void CsrAgAudioPluginToneComplete (CsrAgAudioPluginTaskData *task) ;
#endif

