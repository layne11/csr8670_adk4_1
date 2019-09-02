/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    audioDecodeSourceConn.h
 
DESCRIPTION
    Write a short description about what the sub module does and how it 
    should be used.
*/

#ifndef VM_LIBS_CSR_CVC_COMMON_PLUGIN_AUDIODECODESOURCECONN_H_
#define VM_LIBS_CSR_CVC_COMMON_PLUGIN_AUDIODECODESOURCECONN_H_

void audioDecoderSetRelayMode(void);
sources_t audioDecoderGetSources(AUDIO_SINK_T sink_type);
void audioDecoderConnectInputSources(AUDIO_SINK_T sink_type, local_config_t* localConfig);
void audioDecoderStartTransformCheckScms(Transform rtp_transform, bool content_protection);

#endif /* VM_LIBS_CSR_CVC_COMMON_PLUGIN_AUDIODECODESOURCECONN_H_ */
