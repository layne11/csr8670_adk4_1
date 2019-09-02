/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_dsp_if.c

DESCRIPTION
    Interface to DSP operations

NOTES
*/
#include "csr_cvc_common_ctx.h"
#include "csr_cvc_common_dsp_if.h"
#include "csr_cvc_common_io_if.h"
#include "csr_cvc_common_if.h"
#include "csr_cvc_common.h"


#include <audio_output.h>
#include <csr_i2s_audio_plugin.h>
#include <audio.h>
#include <gain_utils.h>
#include <vmal.h>
#include <print.h>
#include <string.h>
#include <file.h>
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <kalimba_if.h>
#include <ps.h>

/* SCO rate is specified in KHz * 8 */
#define SCO_RATE_SCALING_FACTOR 8
#define convertScoRate(rate_khz) ((uint16)(rate_khz * SCO_RATE_SCALING_FACTOR))

/* Enumeration for audio i/f selection */
#define CVC_INTERFACE_ANALOG          0
#define CVC_INTERFACE_I2S             1

/* Rate match enable masks */
#define RATE_MATCH_HARDWARE_MASK      (1 << 0)
#define RATE_MATCH_SOFTWARE_MASK      (1 << 1)

/* DAC and ADC rate is specified in Hz / 10 */
#define DAC_ADC_RATE_SCALING_FACTOR  10
#define convertDacRate(rate_hz) (uint16)(rate_hz / DAC_ADC_RATE_SCALING_FACTOR)
#define convertAdcRate(rate_hz) (uint16)(rate_hz / DAC_ADC_RATE_SCALING_FACTOR)

/* Bit set in DSP volume message to indicate digital volume is to be used */
#define DSP_DIG_VOL_FLAG (1 << 4)

#define CONVERT_ERROR(type) Panic(); return (type)-1;

/*******************************************************************************
DESCRIPTION
    Convert link_encoding_t to DSP format understood by the CVC KAP file
*/
static LINK_ENCODING_TYPE_T convertTransportEncoding(link_encoding_t encoding)
{
    switch(encoding)
    {
        case link_encoding_cvsd:
            return LINK_ENCODING_CVSD;
        case link_encoding_msbc:
            return LINK_ENCODING_SBC;
        case link_encoding_usb_pcm:
            return LINK_ENCODING_USB;
        default:
            break;
    }
    CONVERT_ERROR(LINK_ENCODING_TYPE_T);
}

/*******************************************************************************
DESCRIPTION
    Convert bandwidth_t to DSP format understood by the CVC KAP file
*/
static CVC_SR_TYPE_T convertBandwidth(bandwidth_t bandwidth)
{
    switch(bandwidth)
    {
        case bandwidth_narrow:
            return CVC_SR_NB;
        case bandwidth_wide:
            return CVC_SR_WB;
        case bandwidth_expanded:
            return CVC_SR_BEX;
        default:
            break;
    }

    CONVERT_ERROR(CVC_SR_TYPE_T);
}

/*******************************************************************************
DESCRIPTION
    Convert hardware_type_t to DSP format understood by the CVC KAP file
*/
static uint16 convertHardwareType(hardware_type_t hardware_type)
{
    PRINT(("CVC: Hardware "));
    if(hardware_type == hardware_type_i2s)
    {
        PRINT(("I2S\n"));
        return CVC_INTERFACE_I2S;
    }
    PRINT(("DAC\n"));
    return CVC_INTERFACE_ANALOG;
}

/*******************************************************************************
DESCRIPTION
    Convert rate_matching_t to DSP format understood by the CVC KAP file
*/
static uint16 convertRateMatchType(rate_matching_t rate_matching)
{
    PRINT(("CVC: Rate Matching "));
    if(rate_matching == rate_matching_software)
    {
        PRINT(("software\n"));
        return RATE_MATCH_SOFTWARE_MASK;
    }
    PRINT(("hardware\n"));
    return RATE_MATCH_HARDWARE_MASK;
}

/*******************************************************************************
DESCRIPTION
    Convert cvc_call_state_t to DSP format understood by the CVC KAP file
*/
static CVC_CALL_STATES_T convertCallState(cvc_call_state_t call_state)
{
    switch(call_state)
    {
        case cvc_call_none:
            return CALLST_NONE;
        case cvc_call_active:
            return CALLST_CONNECTED;
        default:
            break;
    }
    CONVERT_ERROR(CVC_CALL_STATES_T);
}

/*******************************************************************************
DESCRIPTION
    Convert cvc_mode_t to DSP format understood by the CVC KAP file
*/
static CVC_MODES_T convertCvcMode(cvc_mode_t mode)
{
    switch(mode)
    {
        case cvc_mode_hands_free_kit:
            return SYSMODE_HFK;
        case cvc_mode_speech_recognition:
            return SYSMODE_ASR;
        case cvc_mode_pass_through:
            return SYSMODE_PSTHRGH;
        case cvc_mode_stand_by:
            return SYSMODE_STANDBY;
        default:
            break;
    }
    CONVERT_ERROR(CVC_MODES_T);
}

/*******************************************************************************
DESCRIPTION
    Work out the kap file to load for a given plug-in variant
*/
static const char* csrCvcPluginGetKapFile(cvc_plugin_type_t variant)
{
    switch(variant)
    {
        case cvc_1_mic_headset_cvsd:
        case cvc_1_mic_headset_cvsd_bex:
        case cvc_1_mic_headset_msbc:
        case cvc_1_mic_headset_cvsd_asr:
            return "cvc_headset/cvc_headset.kap";

        case cvc_2_mic_headset_cvsd:
        case cvc_2_mic_headset_cvsd_bex:
        case cvc_2_mic_headset_msbc:
        case cvc_2_mic_headset_cvsd_asr:
            return "cvc_headset_2mic/cvc_headset_2mic.kap";

        case cvc_1_mic_handsfree_cvsd:
        case cvc_1_mic_handsfree_cvsd_bex:
        case cvc_1_mic_handsfree_msbc:
        case cvc_1_mic_handsfree_cvsd_asr:
            return "cvc_handsfree/cvc_handsfree.kap";

        case cvc_2_mic_handsfree_cvsd:
        case cvc_2_mic_handsfree_cvsd_bex:
        case cvc_2_mic_handsfree_msbc:
        case cvc_2_mic_handsfree_cvsd_asr:
            return "cvc_handsfree_2mic/cvc_handsfree_2mic.kap";

        /* special case to engage re-sampler for I2S outputs */
        case cvc_disabled:
            return "cvc_headset/cvc_headset.kap";

        default:
            Panic() ;
            return NULL;
    }
}

/******************************************************************************/
void csrCvcCommonDspPowerOn(void)
{
    /* KalimbaLoad will power on DSP for us, nothing to do here */
}

/******************************************************************************/
void csrCvcCommonDspLoadDsp(CvcPluginTaskdata *task)
{
    FILE_INDEX index = FILE_NONE;
    const char* kap_file = NULL ;

    /*ensure that the messages received are from the correct kap file*/
    (void) MessageCancelAll((Task)task, MESSAGE_FROM_KALIMBA);
    MessageKalimbaTask((Task)task);

    /* Select which Kap file to be loaded based on the plugin selected */
    kap_file = csrCvcPluginGetKapFile(task->cvc_plugin_variant);
    index = FileFind(FILE_ROOT, kap_file, (uint16)strlen(kap_file));
    PRINT(("CVC: App File [0x%X] %s\n", index, kap_file));
    PanicFalse(index != FILE_NONE);

    /* Load the cvc algorithm into Kalimba*/
    if(!KalimbaLoad(index))
    {
        PRINT(("CVC: Kalimba load fail\n"));
        Panic();
    }
}

/******************************************************************************/
void csrCvcCommonDspConnectScoSource(Source sco_source)
{
    Sink sink = StreamKalimbaSink(CVC_SCO_IN_PORT);
    /* Need to test both sink and source are valid at point of connect as it is possible
     * for firmware to have invalidated them since they were passed in.
     * */
    if (SourceIsValid(sco_source))
        PanicNull(StreamConnect(sco_source, sink));
}

/******************************************************************************/
void csrCvcCommonDspConnectScoSink(Sink sco_sink)
{
    uint16 kal_port = CsrCvcIsUsbAudio() ? CVC_USB_OUT_PORT : CVC_SCO_OUT_PORT;
    Source source = StreamKalimbaSource(kal_port);
    /* Need to test both sink and source are valid at point of connect as it is possible
     * for firmware to have invalidated them since they were passed in.
     * */
    if (SinkIsValid(sco_sink))
        PanicNull(StreamConnect(source, sco_sink));
}

/******************************************************************************/
void csrCvcCommonDspConnectSpeakers(audio_output_params_t* params)
{
    /* CVC only supports primary outputs, connect only those ports */
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_LEFT), audio_output_primary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_RIGHT), audio_output_primary_right);
    AudioOutputConnectOrPanic(params);
}

/******************************************************************************/
void csrCvcCommonDspDisconnectSpeakers(void)
{
    PanicFalse(AudioOutputDisconnect());
}

/******************************************************************************/
void csrCvcCommonDspConnectMonoSpeaker(audio_output_t output, audio_output_params_t* params)
{
    Source source = StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_LEFT);
    PanicFalse(AudioOutputConnectSource(source, output, params));
}

/******************************************************************************/
void csrCvcCommonDspConnectMicrophones(Source mic_source_a, Source mic_source_b)
{
    if(mic_source_a != NULL)
    {
        PRINT(("CVC: Connect Mic A\n"));
        /* Connect mic A to DSP ports */
        PanicNull(StreamConnect(mic_source_a, StreamKalimbaSink(CVC_LEFT_IN_PORT)));
    }

    if(mic_source_b != NULL)
    {
        PRINT(("CVC: Connect Mic B\n"));
        /* Connect mic B to DSP ports */
        PanicNull(StreamConnect(mic_source_b, StreamKalimbaSink(CVC_RIGHT_IN_PORT)));
    }
}

/******************************************************************************/
void csrCvcCommonDspConnectI2sMicrophones(bool two_mic, uint32 i2s_rate)
{
    Sink kal_left_sink = StreamKalimbaSink(CVC_LEFT_IN_PORT);
    Sink kal_right_sink = two_mic ? StreamKalimbaSink(CVC_RIGHT_IN_PORT) : NULL;

    CsrI2SAudioInputConnect(i2s_rate, two_mic, kal_left_sink, kal_right_sink, RESOLUTION_MODE_16BIT );
}

/******************************************************************************/
void csrCvcCommonDspConfigureTransport(link_encoding_t encoding, uint16 rate_khz)
{
    PRINT(("CVC: AUDIO_CONFIG_MSG 0x%x %ukHz\n", encoding, rate_khz));
    PanicFalse(KalimbaSendMessage(CVC_AUDIO_CONFIG_MSG,
                                  convertTransportEncoding(encoding),
                                  AUDIO_CODEC_CVSD,
                                  convertScoRate(rate_khz), 0));
}

/*******************************************************************************
DESCRIPTION
    Work out the DSP PS key which a plug-in variant uses to store permanent data
*/
static uint16 GetPsBaseFromVariant(uint16 variant, bool mic_attached)
{
#ifndef REMOVABLE_MIC
    UNUSED(mic_attached);
#endif

    switch(variant)
    {
        case cvc_1_mic_headset_cvsd_bex:
        case cvc_1_mic_headset_cvsd_asr:
            return CVC_1MIC_PS_BASE;

#ifndef REMOVABLE_MIC
        case cvc_1_mic_headset_cvsd:
            return CVC_1MIC_PS_BASE;

        case cvc_1_mic_headset_msbc:
            return CVC_1MIC_PS_BASE_WBS;
#else
        case cvc_1_mic_headset_cvsd:
            if(mic_attached)
                return CVC_1MIC_PS_BASE_EXT_MIC_FITTED;
            else
                return CVC_1MIC_PS_BASE_EXT_MIC_NOT_FITTED;

        case cvc_1_mic_headset_msbc:
            if(mic_attached)
                return CVC_1MIC_PS_BASE_WBS_EXT_MIC_FITTED;
            else
                return CVC_1MIC_PS_BASE_WBS_EXT_MIC_NOT_FITTED;
#endif

        case cvc_2_mic_headset_cvsd_bex:
        case cvc_1_mic_handsfree_cvsd_bex:
        case cvc_2_mic_handsfree_cvsd_bex:
        case cvc_2_mic_headset_cvsd_asr:
        case cvc_1_mic_handsfree_cvsd_asr:
        case cvc_2_mic_handsfree_cvsd_asr:
        case cvc_2_mic_headset_cvsd:
        case cvc_1_mic_handsfree_cvsd:
        case cvc_2_mic_handsfree_cvsd:
            return CVC_2MIC_PS_BASE;

        case cvc_2_mic_headset_msbc:
        case cvc_1_mic_handsfree_msbc:
        case cvc_2_mic_handsfree_msbc:
            return CVC_2MIC_PS_BASE_WBS;

        case cvc_disabled:
        default:
            break;
    }

    Panic();
    return 0;
}

/******************************************************************************/
void csrCvcCommonDspConfigureBandwidth(CvcPluginTaskdata *task)
{
    /* Assume microphone detached when configuring bandwidth */
    uint16 ps_base = GetPsBaseFromVariant(task->cvc_plugin_variant, FALSE);
    CVC_SR_TYPE_T dsp_bandwidth = convertBandwidth(csrCvcGetBandwidth(task));
    KalimbaSendMessage(CVC_LOADPARAMS_MSG, ps_base, dsp_bandwidth, 0, 0);
}

/******************************************************************************/
#ifdef REMOVABLE_MIC
void csrCvcCommonDspSendExternalMicrophoneUpdate(CvcPluginTaskdata *task, bool mic_attached)
{
    CVC_SR_TYPE_T dsp_bandwidth = convertBandwidth(csrCvcGetBandwidth(task));
    uint16 ps_base = GetPsBaseFromVariant(task->cvc_plugin_variant, mic_attached);

    switch(task->cvc_plugin_variant)
    {
        case cvc_1_mic_headset_cvsd:
        case cvc_1_mic_headset_msbc:
            KalimbaSendMessage(CVC_LOADPARAMS_MSG, ps_base, dsp_bandwidth, 0, 0);
        break;

        default:
            /* This can safely be ignored, unsupported in other plugins */
        break;
    }
}
#endif

/******************************************************************************/
void csrCvcCommonDspConfigureHardware(uint32 dac_rate_hz, uint32 adc_rate_hz, rate_matching_t rate_matching, hardware_type_t hardware_type)
{
    PRINT(("CVC: SET_SAMPLE_RATE "));
    PRINT(("DAC rate %luHz ", dac_rate_hz));
    PRINT(("ADC rate %luHz ", adc_rate_hz));
    PRINT(("\n"));

    PanicFalse(KalimbaSendMessage(MESSAGE_SET_ADCDAC_SAMPLE_RATE_MESSAGE_ID,
                                  convertDacRate(dac_rate_hz),
                                  convertRateMatchType(rate_matching),
                                  convertHardwareType(hardware_type),
                                  convertAdcRate(adc_rate_hz)));
}

/*******************************************************************************
DESCRIPTION
    Send CVC_SETMODE_MSG to the DSP
*/
static void sendModeMessage(CVC_MODES_T dsp_mode, CVC_CALL_STATES_T dsp_call_state)
{
    /* Digital flags are also part of this message, get them from cvc_common */
    const audio_mic_params* mic_a = CsrCvcGetMicParamsFromMicId(microphone_input_id_voice_a);
    const audio_mic_params* mic_b = CsrCvcGetMicParamsFromMicId(microphone_input_id_voice_b);

    uint16 mic_flags = (uint16)((mic_a->digital << 1) | mic_b->digital);

    KalimbaSendMessage(CVC_SETMODE_MSG, dsp_mode, 0, dsp_call_state, mic_flags);
}

/******************************************************************************/
void csrCvcCommonDspConfigureProcessing(cvc_mode_t mode, cvc_call_state_t call_state)
{
    sendModeMessage(convertCvcMode(mode), convertCallState(call_state));
}

/******************************************************************************/
void csrCvcCommonDspConfigureMute(bool mute_microphone, bool mute_speaker)
{
    CVC_CALL_STATES_T dsp_call_state = CALLST_CONNECTED;
    CVC_MODES_T dsp_mode = CsrCvcIsHandsFreeKitEnabled() ? SYSMODE_HFK : SYSMODE_PSTHRGH;
    CVC_t* CVC = CsrCvcGetCtx();

    bool speaker_mute_changed = (mute_speaker != CVC->speaker_muted);
    bool mic_mute_changed = (mute_microphone != CVC->mic_muted);
    
    if(mute_microphone)
    {
        /* Set call state to mute the microphone */
        dsp_call_state = CALLST_MUTE;

        /* Stand-by if mic and speaker are muted */
        if(mute_speaker)
            dsp_mode = SYSMODE_STANDBY;
    }

    if(speaker_mute_changed)
        csrCvcCommonDspConfigureVolume(mute_speaker? DAC_MUTE : CVC->volume);
    
    /* Mute of microphone is always done via mode change. If speaker and 
       microphone are muted we enter standby, so if mic is muted we must be
       sure a mode change is sent to wake the DSP and un-mute the speaker */
    if(mic_mute_changed || (speaker_mute_changed && mute_microphone))
        sendModeMessage(dsp_mode, dsp_call_state);
}

/******************************************************************************/
void csrCvcCommonDspConfigureVolume(const int16 volume)
{
    uint16 digital_dsp = 0;
    int16 tone_volume = CsrCvcGetToneVolume();

    if (AudioOutputGainGetType(audio_output_group_main) == audio_output_gain_digital)
    {
        audio_output_gain_t gain;

        /* CVC needs to know the fixed system gain, no need to pass master/tone gain
           so set them to zero and call function to get the fixed system gain. */
        AudioOutputGainGetDigital(audio_output_group_main, 0, 0, &gain);

        digital_dsp = (uint16)(DSP_DIG_VOL_FLAG | (gain.common.system & 0xF));
    }

    PRINT(("CVC: Send vol->DSP; vol[0x%x] tone[0x%x] dig[0x%x]\n", volume, tone_volume, digital_dsp));
    
    KalimbaSendMessage(CVC_VOLUME_MSG, 0, digital_dsp, (uint16)volume, (uint16)tone_volume);
}

/******************************************************************************/
/* Instance 1 */
#define SILENCE_CLIP_DETECT_INSTANCE (1)

/* Silence detection limit 0x7FFF (32.767s) */
#define SILENCE_TIMEOUT (D_SEC(32) + 767)

/* Clip limit 1000 */
#define CLIP_LIMIT (1000)

/* PKSEY DSP 10 used to store confidence threshold for ASR... */
#define PSKEY_DSP10 (60)

/* ...and it is 4 words long */
#define SIZE_CONFIDENCE_THRESHOLD (4)

/******************************************************************************/
bool csrCvcCommonDspConfigureSpeechRecognition(void)
{
    uint16 ret_len = 0;
    uint16 psdata[SIZE_CONFIDENCE_THRESHOLD] ;

    /* Initialize clip detector */
    if(!KalimbaSendMessage(KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_INITIALISE_ID, SILENCE_CLIP_DETECT_INSTANCE, SILENCE_TIMEOUT, CLIP_LIMIT, 1))
        return FALSE;

    /* Read confidence threshold from from PS if it exists */
    /*needs to be stored somewhere else and not read every time*/
    ret_len = PsRetrieve(PSKEY_DSP10, psdata, SIZE_CONFIDENCE_THRESHOLD);

    /* Tell DSP to load Confidence Threshold */
    if(ret_len)
    {
        PRINT(("Confidence Threshold from PS: len=0x%x, MSB=0x%x, LSB=0x%x, 3rd=0x%x\n\n", ret_len, psdata[0], psdata[1], psdata[2]));
        if(!KalimbaSendMessage(SET_CONFI_THRESH_MSG, psdata[0], psdata[1], psdata[2], psdata[3]))
            return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
bool csrCvcCommonDspStartSpeechRecognition(void)
{
    KalimbaSendMessage(ASR_START,0,0,0,0);
    return TRUE;
}

/******************************************************************************/
void csrCvcCommonDspRegisterToneNotifications(Task task)
{
    Sink kal_sink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);

    /*request an indication that the tone has completed / been disconnected*/
    VmalMessageSinkTask ( kal_sink, task );
}

/******************************************************************************/
void csrCvcCommonDspConnectTone(AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message)
{
    Sink kal_sink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);
    Source tone_source = StreamRingtoneSource(tone_message->tone);

    /* Configure prompt playback mono 8KHz */
    KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, 8000, 0, 0, 0);

    /* connect the tone (mix the tone if this is CVC) */
    PanicFalse(StreamConnectAndDispose(tone_source, kal_sink));
}

/******************************************************************************/
void csrCvcCommonDspDisconnectTone(void)
{
    StreamDisconnect(0, StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT));
}

/******************************************************************************/
void csrCvcCommonDspStop(void)
{
}

/******************************************************************************/
void csrCvcCommonDspPowerOff(Task task)
{
    /* Turn off DSP and de-register the Kalimba Task */
    KalimbaPowerOff();
    MessageKalimbaTask(NULL);

    /* Cancel any outstanding cvc messages */
    MessageCancelAll(task, MESSAGE_FROM_KALIMBA);
}

/******************************************************************************/
Sink csrCvcCommonDspGetSink(dsp_sink_type_t type)
{
    switch(type)
    {
#ifdef ANC
        case dsp_sink_anc_mic_a:
            return StreamKalimbaSink(ANC_MIC_A_IN_PORT);
        case dsp_sink_anc_mic_b:
            return StreamKalimbaSink(ANC_MIC_B_IN_PORT);
#endif
        case dsp_sink_none:
        case dsp_sink_top:
        default:
            break;
    }
    return NULL;
}

/******************************************************************************/
bool csrCvcCommonDspSupportsNoDspMode(CvcPluginTaskdata *task)
{
    CVC_t* CVC = CsrCvcGetCtx();
    
    /* I2S always needs DSP to be running */
    if(AudioOutputConfigRequiresI2s())
        return FALSE;
    
    /* When using MSBC we need to make sure meta data is off or we must use DSP */
    if(task->encoder == link_encoding_msbc)
    {
        if(!SourceConfigure(CVC->audio_source, VM_SOURCE_SCO_METADATA_ENABLE, 0))
            return FALSE;
    }
    
    /* USB must have CVC running to work */
    if(CVC->link_type == AUDIO_SINK_USB)
        return FALSE;
    
    /* We can run with no DSP! */
    return TRUE;
}
