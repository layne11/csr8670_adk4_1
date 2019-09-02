/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_dsp_message_handler.c

DESCRIPTION
    Interface header to DSP message handler
    
NOTES
*/
#include "csr_cvc_common_dsp_message_handler.h"
#include "csr_cvc_common.h"
#include "csr_cvc_common_if.h"
#include "csr_cvc_common_dsp_if.h"
#include "csr_cvc_common_state_machine.h"

#include <stddef.h>
#include <print.h>
#include <pblock.h>
#include <audio.h>
#include <kalimba_standard_messages.h>
#include <kalimba.h>

/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

typedef struct
{
    uint16 id;
    uint16 size;
    char   buf[1];
} DSP_LONG_REGISTER_T;

/****************************************************************************
DESCRIPTION
    Handles messages from the DSP
*/
void CsrCvcCommonDspMessageHandler( CvcPluginTaskdata *task ,uint16 id, Message message )
{
    switch(id)
    {
        case MESSAGE_FROM_KALIMBA:
        {
            const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
            PRINT(("CVC: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
        
            switch ( m->id ) 
            {
                case CVC_READY_MSG:
                {
                    PRINT(("CVC: CVC_READY, SysId[%x] BuildVersion[%x] \n",m->a, m->b));
                    csrCvcCommonDspConfigureBandwidth(task);
                    
                    /* ASR is a separate operating mode of CVC */
                    CvcConfigureSpeechRecognitionIfSupported(task);
                    
                    csrCvcCommonStateMachineHandleEvent(cvc_ready_for_configuration, NULL);
                }
                break;
            
                case CVC_CODEC_MSG:
                {
                    int16 lOutput_gain;
                    T_mic_gain lInput_gain_l;
                    T_mic_gain lInput_gain_r;
                  
                    lOutput_gain = (int16)m->a;
                    lInput_gain_l = *(T_mic_gain*)&m->b;
                    lInput_gain_r = *(T_mic_gain*)&m->c;

                    csrCvcCodecMessage(task, lInput_gain_l,lInput_gain_r, lOutput_gain);

                    csrCvcCommonStateMachineHandleEvent(cvc_configuration_complete, NULL);
                 }
                break;

                case MESSAGE_TONE_COMPLETE:
                    csrCvcCommonToneCompleteMessage(task);
                    break;
            
                case CVC_LOADPERSIST_MSG:
                {
                    /* a=sysid, b=len */
                    const pblock_entry* entry = PblockGet(m->a);
                    KalimbaSendLongMessage(CVC_LOADPERSIST_RESP, entry->size, entry->data);
                }
                break;

                case CVC_SECPASSED_MSG:
                PRINT(("CVC:  Sec passed.\n"));
                    /*cvc is now loaded, signal that tones etc can be scheduled*/
                    csrCvcCommonStateMachineHandleEvent(cvc_security_complete, NULL);
                break;
 
                case CVC_SECFAILED_MSG:
                PRINT(("CVC: Security has failed.\n"));
                    /*cvc is now loaded, signal that tones etc can be scheduled*/
                    csrCvcCommonStateMachineHandleEvent(cvc_security_complete, NULL);
                break;
               
                case CVC_ASR_YES:
                {
                    PRINT(("ASR: YES\n")) ;

                    /*Send Message to VM app if VM app has been specified*/
                    CsrCvcSendApplicationMessage(CSR_SR_WORD_RESP_YES, NULL);
                }
                break;

                case CVC_ASR_NO:
                {
                    PRINT(("ASR: NO\n")) ;              

                    /*Send Message to VM app if VM app has been specified*/
                    CsrCvcSendApplicationMessage(CSR_SR_WORD_RESP_NO, NULL);
                }
                break;

                case CVC_ASR_FAILED_YES:
                {
                    PRINT(("ASR: FAILED YES\n")) ;

                    /*Send Message to VM app if VM app has been specified*/
                    CsrCvcSendApplicationMessage(CSR_SR_WORD_RESP_FAILED_YES, NULL);
                }
                break;

                case CVC_ASR_FAILED_NO:
                {
                    PRINT(("ASR: FAILED NO\n")) ;              

                    /*Send Message to VM app if VM app has been specified*/
                    CsrCvcSendApplicationMessage(CSR_SR_WORD_RESP_FAILED_NO, NULL);
                }
                break;

                case CVC_ASR_UNKNOWN:
                {
                    PRINT(("ASR: UNKNOWN\n")) ;
                    /*Send Message to VM app if VM app has been specified*/
                    CsrCvcSendApplicationMessage(CSR_SR_WORD_RESP_UNKNOWN, NULL);
                }
                break; 
    
                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_CLIP_DETECTED_ID:
                {
                   PRINT(("\n\n\n\n\nInput level clipped.\n"));
                }
                break;                      
            
    
                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_SILENCE_DETECTED_ID:
                {
                   PRINT(("\n\n\n\n\nInput level silence.\n"));
                }
                break;   

                default:
                break;
            }
        }
        break;

        case MESSAGE_FROM_KALIMBA_LONG:
        {
            const DSP_LONG_REGISTER_T *m = (const DSP_LONG_REGISTER_T*) message;
/*            PRINT(("CVC: LONG_MESSAGE_FROM_KALIMBA id[0x%x] l[0x%x] \n", m->id, m->size));*/
            switch (m->id)
            {
                case CVC_STOREPERSIST_MSG:
                    /* Set the DSP app's pblock */
/*                   PRINT(("CVC: StorePersist key[0x%x], data[0x%x] \n", m->buf[0], m->buf[1]));*/
                   PblockSet((uint16)(m->buf[0]), (uint16)(m->size-1), (uint16*)m->buf+1);
                break;
                
                default:
                break;
            }
        }
        break;
        
        default:
        break ;
    }
}
