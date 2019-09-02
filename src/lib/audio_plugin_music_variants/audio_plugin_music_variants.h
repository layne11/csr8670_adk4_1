/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    audio_plugin_music_variants.h
 
DESCRIPTION
    Definitions of music plugin variants.
*/

#ifndef AUDIO_PLUGIN_MUSIC_VARIANTS_H_
#define AUDIO_PLUGIN_MUSIC_VARIANTS_H_

typedef enum
{
     SBC_DECODER                = 1
    ,MP3_DECODER                = 2
    ,AAC_DECODER                = 3
    ,FASTSTREAM_SINK            = 4
    ,USB_DECODER                = 5
    ,APTX_DECODER               = 6
    ,APTX_ACL_SPRINT_DECODER    = 7
    ,ANALOG_DECODER             = 8
    ,SPDIF_DECODER              = 9
    ,TWS_SBC_DECODER            = 10
    ,TWS_MP3_DECODER            = 11
    ,TWS_AAC_DECODER            = 12
    ,TWS_APTX_DECODER           = 13
    ,FM_DECODER                 = 14
    ,I2S_DECODER                = 15
    ,APTXHD_DECODER             = 16
    ,NUM_DECODER_PLUGINS
}A2DP_DECODER_PLUGIN_TYPE_T;

typedef struct
{
    TaskData                    data;
    A2DP_DECODER_PLUGIN_TYPE_T  a2dp_plugin_variant:8 ; /* Selects the A2DP plugin variant */
    unsigned                    reserved:8 ;           /* Set the reserved bits to zero */
}A2dpPluginTaskdata;

extern const A2dpPluginTaskdata csr_sbc_decoder_plugin;
extern const A2dpPluginTaskdata csr_mp3_decoder_plugin;
extern const A2dpPluginTaskdata csr_aac_decoder_plugin;
extern const A2dpPluginTaskdata csr_faststream_sink_plugin;
extern const A2dpPluginTaskdata csr_aptx_decoder_plugin;
extern const A2dpPluginTaskdata csr_aptx_acl_sprint_decoder_plugin;
extern const A2dpPluginTaskdata csr_aptxhd_decoder_plugin;
extern const A2dpPluginTaskdata csr_tws_sbc_decoder_plugin;
extern const A2dpPluginTaskdata csr_tws_mp3_decoder_plugin;
extern const A2dpPluginTaskdata csr_tws_aac_decoder_plugin;
extern const A2dpPluginTaskdata csr_tws_aptx_decoder_plugin;
extern const A2dpPluginTaskdata csr_fm_decoder_plugin;
extern const A2dpPluginTaskdata csr_analogue_decoder_plugin;
extern const A2dpPluginTaskdata csr_i2s_decoder_plugin;
extern const A2dpPluginTaskdata csr_spdif_decoder_plugin;

#endif /* AUDIO_PLUGIN_MUSIC_VARIANTS_H_ */
