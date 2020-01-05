/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
*/
/**
\file

\ingroup sink_app

\brief  This is main file for the application software for a sink device

*/
/** 
 \addtogroup sink_app 
 \{
*/
/****************************************************************************
    Header files
*/

#include <message.h>
#include <hfp.h>
#include "sink_private.h"
#include "sink_init.h"
#include "sink_auth.h"
#include "sink_scan.h"
#include "sink_slc.h"
#include "sink_inquiry.h"
#include "sink_devicemanager.h"
#include "sink_link_policy.h"
#include "sink_indicators.h"
#include "sink_hf_indicators.h"
#ifndef HYDRACORE_TODO
#include "sink_dut.h"
#endif
#include "sink_pio.h"
#include "sink_multipoint.h"
#include "sink_led_manager.h"
#include "sink_buttonmanager.h"
#include "sink_configmanager.h"
#include "sink_events.h"
#include "sink_statemanager.h"
#include "sink_states.h"
#include "sink_powermanager.h"
#include "sink_callmanager.h"
#include "sink_csr_features.h"
#include "sink_usb.h"
#include "sink_display.h"
#include "sink_speech_recognition.h"
#include "sink_a2dp.h"
#include "sink_config.h"
#include "sink_audio_routing.h"
#include "sink_partymode.h"
#include "sink_leds.h"
#include "sink_upgrade.h"
#include "sink_fm.h"
#include "sink_anc.h"
#ifdef HYDRACORE_TODO
#include <os.h>
#endif
/* BLE related headers */
#ifndef HYDRACORE_TODO
#include "sink_ble.h"
#include "sink_ble_gap.h"
#include "sink_ble_advertising.h"
#include "sink_ble_scanning.h"

#include "sink_gatt_server_ias.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_server_lls.h"
#include "sink_gatt_client_ias.h"
#include "sink_gatt_server_hrs.h"
#include "sink_gatt_client.h"
#endif /*!HYDRACORE_TODO*/

#include "sink_ble_sc.h"

#ifdef ENABLE_IR_REMOTE
#include "sink_ir_remote_control.h"
#endif
#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#include "gaia.h"
#endif
#ifdef ENABLE_PBAP
#include "sink_pbap.h"
#endif
#ifdef ENABLE_MAPC
#include "sink_mapc.h"
#endif
#ifdef ENABLE_AVRCP
#include "sink_avrcp.h"
#endif
#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif
#ifdef ENABLE_PEER
#include "sink_peer.h"
#endif
#include "sink_linkloss.h"
#include "sink_sc.h"

#include "sink_avrcp_qualification.h"
#include "sink_peer_qualification.h"

#include "sink_accessory.h"

#include "sink_volume.h"
#include "sink_tones.h"
#include "sink_audio_prompts.h"
#include "sink_audio_indication.h"

#include "sink_audio.h"
#include "sink_at_commands.h"

#include "sink_watchdog.h"
#include "vm.h"

#include "sink_nfc.h"

#ifdef TEST_HARNESS
#include "test_sink.h"
#include "vm2host_connection.h"
#include "vm2host_hfp.h"
#include "vm2host_a2dp.h"
#include "vm2host_avrcp.h"
#endif

#include <library.h>
#include <bdaddr.h>
#include <connection.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <boot.h>
#include <string.h>
#include <audio.h>
#include <sink.h>
#include <kalimba_standard_messages.h>
#include <audio_plugin_if.h>
#include <print.h>
#include <loader.h>
#include <pio_common.h>

#ifdef HYDRACORE_TODO
/*#define WAIT_FOR_HERACLES_ON_START*/
#ifdef WAIT_FOR_HERACLES_ON_START
static void wait_for_heracles(void);
#endif /* WAIT_FOR_HERACLES_ON_START */
#endif /* HYDRACORE_TODO */

#include <display.h>
#include <display_plugin_if.h>
#include "sink_custom.h"
#include "sink_custom_uart.h"

#ifdef DEBUG_MAIN
	#define MAIN_DEBUG(x) DEBUG(x)
    #define TRUE_OR_FALSE(x)  ((x) ? 'T':'F')
#else
    #define MAIN_DEBUG(x)
#endif

#if defined ENABLE_PEER && defined PEER_TWS
static const uint16 tws_audio_routing[4] =
{
    (PEER_TWS_ROUTING_STEREO << 2) | (PEER_TWS_ROUTING_STEREO),  /* Master stereo, Slave stereo */
    (  PEER_TWS_ROUTING_LEFT << 2) | ( PEER_TWS_ROUTING_RIGHT),
    ( PEER_TWS_ROUTING_RIGHT << 2) | (  PEER_TWS_ROUTING_LEFT),
    (  PEER_TWS_ROUTING_DMIX << 2) | (  PEER_TWS_ROUTING_DMIX)

};
#endif

#define IS_SOURCE_CONNECTED ((!usbIsAttached()) && (!analogAudioConnected()) && \
                             (!spdifAudioConnected()) && (!sinkFmIsFmRxOn()) && (!i2sAudioConnected()))


/* Single instance of the device state */
hsTaskData theSink;

void app_handler(Task task, MessageId id, Message message);

static void handleHFPStatusCFM ( hfp_lib_status pStatus ) ;
static void sinkConnectionInit(void);


#ifdef HOSTED_TEST_ENVIRONMENT
extern void _sink_init(void);
#else
extern void _init(void);
#endif
static void IndicateEvent(MessageId id);


/*************************************************************************
NAME
    sinkSendLater
    
DESCRIPTION
    Send an event to the application main task after a delay, cancelling
    any already queued

RETURNS
    void
*/
void sinkSendLater(sinkEvents_t event, uint32 delay)
{
    MessageCancelAll(&theSink.task, event);
    MessageSendLater(&theSink.task, event, NULL, delay);
}


/*************************************************************************
NAME    
    handleCLMessage

DESCRIPTION
    Function to handle the CL Lib messages - these are independent of state

RETURNS

*/
static void handleCLMessage ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CL = [%x]\n", id)) ;

    UNUSED(task); /* Can be used depending on compile-time definitions */

    switch(id)
    {
        case CL_INIT_CFM:
            MAIN_DEBUG(("CL_INIT_CFM [%d]\n" , ((CL_INIT_CFM_T*)message)->status ));
            if(((const CL_INIT_CFM_T*)message)->status == success)
            {
                /* Codec deprecated - init hfp instead - srf debug comment*/
                sinkHfpInit();

#ifdef ENABLE_PEER
                if (VmGetResetSource() != UNEXPECTED_RESET)
                {
                    peerPurgeTemporaryPairing();
                }
#endif

#ifdef ENABLE_GAIA
                /* Initialise Gaia with a concurrent connection limit of 1 */
                GaiaInit(task, 1);
#endif
                /*  Initialise Accessory */
                sinkAccessoryInit();
            }
            else
            {
                Panic();
            }
            NFC_CL_TAG_INIT(&theSink.task);
        break;
        case CL_DM_WRITE_INQUIRY_MODE_CFM:
            /* Read the local name to put in our EIR data */
            ConnectionReadInquiryTx(&theSink.task);
        break;
        case CL_DM_READ_INQUIRY_TX_CFM:
            theSink.inquiry_tx = ((const CL_DM_READ_INQUIRY_TX_CFM_T*)message)->tx_power;
            ConnectionReadLocalName(&theSink.task);
        break;
        case CL_DM_LOCAL_NAME_COMPLETE:
            MAIN_DEBUG(("CL_DM_LOCAL_NAME_COMPLETE\n"));
            NFC_CL_HDL_LOCAL_NAME(message);
            /* Write EIR data and initialise the codec task */
            sinkWriteEirData((const CL_DM_LOCAL_NAME_COMPLETE_T*)message);
            break;

        case CL_SM_SEC_MODE_CONFIG_CFM:
            MAIN_DEBUG(("CL_SM_SEC_MODE_CONFIG_CFM\n"));
            /* Remember if debug keys are on or off */
            theSink.debug_keys_enabled = ((const CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
        break;
        case CL_SM_PIN_CODE_IND:
            MAIN_DEBUG(("CL_SM_PIN_IND\n"));
            sinkHandlePinCodeInd((const CL_SM_PIN_CODE_IND_T*) message);
        break;
        case CL_SM_USER_CONFIRMATION_REQ_IND:
            MAIN_DEBUG(("CL_SM_USER_CONFIRMATION_REQ_IND\n"));
            sinkHandleUserConfirmationInd((const CL_SM_USER_CONFIRMATION_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_REQ_IND:
            MAIN_DEBUG(("CL_SM_USER_PASSKEY_REQ_IND\n"));
            sinkHandleUserPasskeyInd((const CL_SM_USER_PASSKEY_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
            MAIN_DEBUG(("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n"));
            sinkHandleUserPasskeyNotificationInd((const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*) message);
        break;
        case CL_SM_KEYPRESS_NOTIFICATION_IND:
        break;
        case CL_SM_REMOTE_IO_CAPABILITY_IND:
            MAIN_DEBUG(("CL_SM_IO_CAPABILITY_IND\n"));
            sinkHandleRemoteIoCapabilityInd((const CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
        break;
        case CL_SM_IO_CAPABILITY_REQ_IND:
            MAIN_DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
            sinkHandleIoCapabilityInd((const CL_SM_IO_CAPABILITY_REQ_IND_T*) message);
        break;
        case CL_SM_AUTHORISE_IND:
            MAIN_DEBUG(("CL_SM_AUTHORISE_IND\n"));
            sinkHandleAuthoriseInd((const CL_SM_AUTHORISE_IND_T*) message);
        break;
        case CL_SM_AUTHENTICATE_CFM:
            MAIN_DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
            sinkHandleAuthenticateCfm((const CL_SM_AUTHENTICATE_CFM_T*) message);
        break;
#if defined ENABLE_PEER && defined ENABLE_SUBWOOFER
#error ENABLE_PEER and ENABLE_SUBWOOFER are mutually exclusive due to their use of CL_SM_GET_AUTH_DEVICE_CFM
#endif
#ifdef ENABLE_SUBWOOFER
        case CL_SM_GET_AUTH_DEVICE_CFM: /* This message should only be sent for subwoofer devices */
            MAIN_DEBUG(("CL_SM_GET_AUTH_DEVICE_CFM\n"));
            handleSubwooferGetAuthDevice((const CL_SM_GET_AUTH_DEVICE_CFM_T*) message);
        break;
#endif
#ifdef ENABLE_PEER
        case CL_SM_GET_AUTH_DEVICE_CFM:
            MAIN_DEBUG(("CL_SM_GET_AUTH_DEVICE_CFM\n"));
            handleGetAuthDeviceCfm((const CL_SM_GET_AUTH_DEVICE_CFM_T *)message);
        break;
        case CL_SM_ADD_AUTH_DEVICE_CFM:
            MAIN_DEBUG(("CL_SM_ADD_AUTH_DEVICE_CFM\n"));
            handleAddAuthDeviceCfm((const CL_SM_ADD_AUTH_DEVICE_CFM_T *)message);
        break;
#endif

        case CL_DM_REMOTE_FEATURES_CFM:
            MAIN_DEBUG(("HS : Supported Features\n")) ;
        break ;
        case CL_DM_REMOTE_EXTENDED_FEATURES_CFM:
            MAIN_DEBUG(("HS : Supported Extended Features\n")) ;
        break;
        case CL_DM_INQUIRE_RESULT:
            MAIN_DEBUG(("HS : Inquiry Result\n"));
            inquiryHandleResult((CL_DM_INQUIRE_RESULT_T*)message);
			inquiryCustomHandleResult((CL_DM_INQUIRE_RESULT_T*)message);
        break;
        case CL_SM_GET_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS : CL_SM_GET_ATTRIBUTE_CFM Vol:%d \n",
                            ((const CL_SM_GET_ATTRIBUTE_CFM_T *)(message))->psdata[0]));
        break;
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS: CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , 
                            ((const CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T*)message)->status)) ;
        break ;

        case CL_DM_LOCAL_BD_ADDR_CFM:
            MAIN_DEBUG(("CL_DM_LOCAL_BD_ADDR_CFM\n"));
#ifndef HYDRACORE_TODO
            DutHandleLocalAddr((const CL_DM_LOCAL_BD_ADDR_CFM_T *)message);
#endif /* !HYDRACORE_TODO */
            NFC_CL_HDL_LOCAL_BDADDR(message);
        break ;
        
        case CL_DM_ROLE_IND:
            linkPolicyHandleRoleInd((const CL_DM_ROLE_IND_T *)message);
        break;
        case CL_DM_ROLE_CFM:
            linkPolicyHandleRoleCfm((const CL_DM_ROLE_CFM_T *)message);
        break;
        case CL_SM_SET_TRUST_LEVEL_CFM:
            MAIN_DEBUG(("HS : CL_SM_SET_TRUST_LEVEL_CFM status %x\n",
                            ((const CL_SM_SET_TRUST_LEVEL_CFM_T*)message)->status));
        break;
        case CL_DM_ACL_OPENED_IND:
            MAIN_DEBUG(("HS : ACL Opened\n"));
            if(((CL_DM_ACL_OPENED_IND_T*)message)->status == hci_success)
            {
                sinkStartAutoPowerOffTimer();
            }
        break;
        case CL_DM_ACL_CLOSED_IND:
            MAIN_DEBUG(("HS : ACL Closed\n"));
#ifdef ENABLE_AVRCP
            if(theSink.features.avrcp_enabled)
            {
                sinkAvrcpAclClosed(((const CL_DM_ACL_CLOSED_IND_T *)message)->taddr.addr);
            }
#endif
        break;

#ifndef HYDRACORE_TODO
/* BLE Messages */
        case CL_DM_BLE_ADVERTISING_REPORT_IND:
        {
            MAIN_DEBUG(("CL_DM_BLE_ADVERTISING_REPORT_IND\n"));
            bleHandleScanResponse((const CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message);
        }
        break;
        case CL_DM_BLE_SET_ADVERTISING_DATA_CFM:
        {
            bleHandleSetAdvertisingData( (const CL_DM_BLE_SET_ADVERTISING_DATA_CFM_T*)message );
        }
        break;
        case CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM\n"));
        }
        break;
        case CL_DM_BLE_SECURITY_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_SECURITY_CFM [%x]\n", 
                            ((const CL_DM_BLE_SECURITY_CFM_T*)message)->status));
        }
        break;
        case CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM [%x]\n", 
                            ((const CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM_T*)message)->status));
        }
        break;
        case CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM [%x]\n", 
                            ((const CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM_T*)message)->status));
        }
        break;
        case CL_DM_BLE_SET_SCAN_PARAMETERS_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_SET_SCAN_PARAMETERS_CFM [%x]\n", 
                            ((const CL_DM_BLE_SET_SCAN_PARAMETERS_CFM_T*)message)->status));
        }
        break;
        case CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM\n"));
        }
        break;
        case CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM num[%d]\n", 
                            ((const CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM_T*)message)->white_list_size));
        }
        break;
        case CL_DM_BLE_CLEAR_WHITE_LIST_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_CLEAR_WHITE_LIST_CFM\n"));
        }
        break;
        case CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM status[%u]\n", 
                            ((const CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T*)message)->status));
            sinkBleGapAddDeviceWhiteListCfm((const CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T*)message);
        }
        break;
        case CL_DM_BLE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM\n"));
        }
        break;
        case CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM:
        {
            MAIN_DEBUG(("CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM\n"));
        }
        break;
        case CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND:
        {
            const CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T * req = (const CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T*)message;
            bool accept = TRUE;

            /* Based on the allowed range, accept or reject the connection parameter updates. */
            if(SinkBleConnectionParameterIsOutOfRange(req))
            {
                accept = FALSE;
            }

            ConnectionDmBleAcceptConnectionParUpdateResponse(accept, &req->taddr, req->id, 
                                                req->conn_interval_min, req->conn_interval_max, 
                                                req->conn_latency, req->supervision_timeout);
        }
        break;
        case CL_SM_BLE_LINK_SECURITY_IND:
        {
            MAIN_DEBUG(("CL_SM_BLE_LINK_SECURITY_IND\n"));
            
            sinkBleLinkSecurityInd((const CL_SM_BLE_LINK_SECURITY_IND_T*)message);
        }
        break;
        case CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND:
        {
            MAIN_DEBUG(("CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND [%x]\n", 
                            ((const CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T*)message)->status));
            
            sinkBleSimplePairingCompleteInd((const CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T*)message);
        }
        break;
        case CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND:
        {
            MAIN_DEBUG(("CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND\n"));
        }
        break;

#endif /* !HYDRACORE_TODO */
        case CL_DM_APT_IND:
        {
            MAIN_DEBUG(("CL_DM_APT_IND - APT expire indication \n"));
        }
        break;
        case CL_DM_READ_APT_CFM:
        {
            MAIN_DEBUG(("CL_DM_READ_APT_CFM [%x]\n", ((CL_DM_READ_APT_CFM_T*)message)->apt));			
        }
        break;
        case CL_SM_ENCRYPT_CFM:
        {
            MAIN_DEBUG(("CL_SM_ENCRYPT_CFM \n"));
            sinkHandleBrEdrEncryptionCfm((const CL_SM_ENCRYPT_CFM_T*)message);
        }
        break;

        case CL_SM_ENCRYPTION_CHANGE_IND:
        {
            MAIN_DEBUG(("CL_SM_ENCRYPTION_CHANGE_IND \n"));
            sinkHandleBrEdrEncryptionChangeInd((const CL_SM_ENCRYPTION_CHANGE_IND_T*)message);
            sinkHandleBleEncryptionChangeInd((const CL_SM_ENCRYPTION_CHANGE_IND_T*)message);
        }
        break;
        
            /* filter connection library messages */
        case CL_SDP_REGISTER_CFM:
        case CL_DM_WRITE_INQUIRY_ACCESS_CODE_CFM:
        case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        break;
        
        /*all unhandled connection lib messages end up here*/
        default :
            MAIN_DEBUG(("Sink - Unhandled CL msg[%x]\n", id));
        break ;
    }

}


static bool eventToBeIndicatedBeforeProcessing(const MessageId id)
{
    if(id == EventUsrMainOutMuteOn)
    {
        return TRUE;
    }
    return FALSE;
}

/*************************************************************************
NAME
    handleUEMessage

DESCRIPTION
    handles messages from the User Events

RETURNS

*/
static void handleUEMessage  ( Task task, MessageId id, Message message )
{
    /* Event state control is done by the config - we will be in the right state for the message
    therefore messages need only be passed to the relative handlers unless configurable */
    sinkState lState = stateManagerGetState() ;

    /*if we do not want the event received to be indicated then set this to FALSE*/
    bool lIndicateEvent = TRUE ;
    bool lResetAutoSwitchOff = FALSE;

    /* Reset the auto switch off timer when either BT device disconnects. */
    if((id == EventSysPrimaryDeviceDisconnected) || (id == EventSysSecondaryDeviceDisconnected))
    {
        /* postpone auto switch-off */
        lResetAutoSwitchOff = TRUE;       
    }

    /* Deal with user generated Event specific actions*/
    if (id < EVENTS_SYS_MESSAGE_BASE)
    {
        /*cancel any missed call indicator on a user event (button press)*/
        MessageCancelAll(task , EventSysMissedCall ) ;

        /* postpone auto switch-off */
        lResetAutoSwitchOff = TRUE;
            
        /* check for led timeout/re-enable */
        LEDManagerCheckTimeoutState();
            
#ifdef ENABLE_GAIA
        gaiaReportUserEvent(id);
#endif
    }

    if(eventToBeIndicatedBeforeProcessing(id))
    {    
        IndicateEvent(id);
    }

    MAIN_DEBUG (( "HS : UE[%x]\n", id ));

    /* The configurable Events*/
    switch ( id )
    {
        case (EventSysResetWatchdog):
		    watchdogReset();
	    break;
        case (EventUsrDebugKeysToggle):
            MAIN_DEBUG(("HS: Toggle Debug Keys\n"));
            /* If the device has debug keys enabled then toggle on/off */
            ConnectionSmSecModeConfig(&theSink.task, cl_sm_wae_acl_none, !theSink.debug_keys_enabled, TRUE);
        break;
        case (EventUsrPowerOn):
        case (EventSysPowerOnPanic):
            MAIN_DEBUG(("HS: Power On\n" )) ;

            /* if this init occurs and in limbo wait for the display init */
            if (stateManagerGetState() == deviceLimbo)
            {
#ifdef ENABLE_SUBWOOFER
                updateSwatVolume((theSink.features.DefaultVolume * sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main))/VOLUME_NUM_VOICE_STEPS);
#endif
            }
                        
            /* Power on ANC */
            sinkAncHandlePowerOn();

            /*we have received the power on event- we have fully powered on*/
            stateManagerPowerOn();
            
            /* Indicate now "Power On" voice prompt before audio
               plugins kick in, derisking playing audio with low volume.*/
            IndicateEvent(id);
            /* Clear flag since events have been indicated.*/
            lIndicateEvent = FALSE;

            /* Read and configure the volume orientation, LED Disable state, and tts_language */
            configManagerReadSessionData();
#ifndef HYDRACORE_TODO
            /* Don't route audio when the DUT PIO is active. Device will go
               into DUT mode at next state change */
            if(!isDUTPIOActive())
#endif /* HYDRACORE_TODO */
            {
                if(theSink.conf2->audio_routing_data.PluginFeatures.manual_source_selection)
                {
                    if(!theSink.features.defaultSource)
                    {
                        /* set the active routed source to the last used source stored in session data */
                        audioSwitchToAudioSource(theSink.rundata->requested_audio_source);
                    }
                    else
                    {
                        audioSwitchToAudioSource((audio_sources)theSink.features.defaultSource);
                    }
                }
                else
                    audioUpdateAudioRouting();                    
            }

            sinkFmSetFmRxOn(FALSE);

            /* set flag to indicate just powered up and may use different led pattern for connectable state
               if configured, flag is cleared on first successful connection */
            theSink.powerup_no_connection = TRUE;

            /* If critical temperature immediately power off */
            if(powerManagerIsVthmCritical())
                MessageSend(&theSink.task, EventUsrPowerOff, 0);

            /* Take an initial battery reading (and power off if critical) */
            powerManagerReadVbat(battery_level_initial_reading);

            if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                MessageSendLater(&theSink.task, EventSysRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));

            if ( theSink.features.DisablePowerOffAfterPowerOn )
            {
                theSink.PowerOffIsEnabled = FALSE ;
                MAIN_DEBUG(("DIS[%x]\n" , theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s  )) ;
                MessageSendLater ( &theSink.task , EventSysEnablePowerOff , 0 , D_SEC ( theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s ) ) ;
            }
            else
            {
                theSink.PowerOffIsEnabled = TRUE ;
            }

#ifdef ENABLE_SUBWOOFER
            /* check to see if there is a paired subwoofer device, if not kick off an inquiry scan */
            MessageSend(&theSink.task, EventSysSubwooferCheckPairing, 0);
#endif

            /* kick off a timer to check the PS store fragmentation status */
            if(theSink.conf1->timeouts.DefragCheckTimer_s)
            {
                MessageSendLater(&theSink.task, EventSysCheckDefrag, 0, D_SEC(theSink.conf1->timeouts.DefragCheckTimer_s));
            }

            /* Power on BLE */
#ifndef HYDRACORE_TODO
            sinkBlePowerOnEvent();    
#endif /*!HYDRACORE_TODO*/

            /*enable PA*/
            PioSetDir32(1<<11, 1<<11);
            PioSetDir32(1<<12, 1<<12);
            PioSet32(1<<11, 1<<11);
            PioSet32(1<<12, 1<<12);
    
        break ;
        case (EventUsrPowerOff):
            /* Reset the silence detected flag in case if the audio silence detected (i.e., set to 1) and before 
              * the timer expiry if user pressed the EventUsrPowerOff eventt.*/
            if(theSink.silence_detected)
                theSink.silence_detected = 0;

            MAIN_DEBUG(("HS: PowerOff - En[%c]\n" , TRUE_OR_FALSE(theSink.PowerOffIsEnabled))) ;
#ifdef ENABLE_PEER
            if(theSink.PowerOffIsEnabled)
            {
                uint16 peerIndex = 0;
                /* If  a TWS peer device is connected, the TWS single device operation is enabled and the power off flag is not set,
                    send the power off command to the peer */
                if(a2dpGetPeerIndex(&peerIndex) &&(theSink.a2dp_link_data->peer_features[peerIndex] & (remote_features_tws_a2dp_sink|remote_features_tws_a2dp_source))
                    && theSink.features.TwsSingleDeviceOperation && !(theSink.a2dp_link_data->local_peer_status[peerIndex] & PEER_STATUS_POWER_OFF) )
                {
                    theSink.a2dp_link_data->local_peer_status[peerIndex] |= PEER_STATUS_POWER_OFF;
                    sinkAvrcpPowerOff();
                    lIndicateEvent = FALSE ;
                    break;
                }
            }
#endif
            /* don't indicate event if already in limbo state */
            if(lState == deviceLimbo) lIndicateEvent = FALSE ;

            /* only power off if timer has expired or battery is low and the charger isn't connected or temperature high */
            if ( theSink.PowerOffIsEnabled || ((!powerManagerIsChargerConnected() || 
			     theSink.features.AllowAutomaticPowerOffWhenCharging) && powerManagerIsVbatCritical()) || powerManagerIsVthmCritical())
            {
                lResetAutoSwitchOff = FALSE;
                
                /* store current volume levels for non bluetooth volumes */
                configManagerWriteSessionData () ;

                /* Store DSP data */
                configManagerWriteDspData();

                /* set the active routed source back to none */
                audioSwitchToAudioSource(audio_source_none);

                stateManagerEnterLimboState();
                AuthResetConfirmationFlags();

                VolumeUpdateMuteStatusAllOutputs(FALSE);
                VolumeSetHfpMicrophoneGain(hfp_invalid_link, MICROPHONE_MUTE_OFF);

                sinkClearQueueudEvent();

                /* Cancel the auto power off timer. */
                sinkStopAutoPowerOffTimer();

                if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                    MessageCancelAll ( &theSink.task, EventSysRefreshEncryption) ;

                MessageCancelAll (&theSink.task, EventSysCheckDefrag);
                MessageCancelAll (&theSink.task, EventSysDefrag);

                MessageCancelAll (&theSink.task, EventSysPairingFail);
#ifdef ENABLE_PEER
                MessageCancelAll(&theSink.task , EventSysA2DPPeerLinkLossTimeout);
                theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE;
#endif

#ifdef ENABLE_AVRCP
                /* cancel any queued ff or rw requests */
                MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
#endif
#ifdef ENABLE_SPEECH_RECOGNITION
                /* if speech recognition is in tuning mode stop it */
                if(theSink.csr_speech_recognition_tuning_active)
                {
                    speechRecognitionStop();
                    theSink.csr_speech_recognition_tuning_active = FALSE;
                }
#endif
                if(sinkFmIsFmRxOn())
                {
                    MessageSend(&theSink.task, EventUsrFmRxOff, 0);
                }


#ifdef ENABLE_GAIA
                if (!theSink.features.GaiaRemainConnected)
                    gaiaDisconnect();
#endif

                /* Power off BLE */
#ifndef HYDRACORE_TODO
                sinkBlePowerOffEvent();
#endif /* !HYDRACORE_TODO */
                /* Power off ANC */
                sinkAncHandlePowerOff();
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
            /*
            MessageSendLater(&theSink.task, EventUsrPowerOn, 0, 1000);
            */
        break ;
        case (EventUsrInitateVoiceDial):
            MAIN_DEBUG(("HS: InitVoiceDial [%d]\n", theSink.VoiceRecognitionIsActive));
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {

                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_primary_link) ;
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */
                    audioUpdateAudioRouting();
                }
                else
                {
                    sinkInitiateVoiceDial (hfp_primary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrInitateVoiceDial_AG2):
            MAIN_DEBUG(("HS: InitVoiceDial AG2[%d]\n", theSink.VoiceRecognitionIsActive));
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {

                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_secondary_link) ;
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */
                    audioUpdateAudioRouting();
                }
                else
                {
                    sinkInitiateVoiceDial(hfp_secondary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrLastNumberRedial):
            MAIN_DEBUG(("HS: LNR\n" )) ;

            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 1 */
                        sinkInitiateLNR(hfp_primary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 1 */
                    sinkInitiateLNR(hfp_primary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrLastNumberRedial_AG2):
            MAIN_DEBUG(("HS: LNR AG2\n" )) ;
            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 2 */
                        sinkInitiateLNR(hfp_secondary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 2 */
                   sinkInitiateLNR(hfp_secondary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrAnswer):
            MAIN_DEBUG(("HS: Answer\n" )) ;
            /* don't indicate event if not in incoming call state as answer event is used
               for some of the multipoint three way calling operations which generate unwanted
               tones */
            if(stateManagerGetState() != deviceIncomingCallEstablish) lIndicateEvent = FALSE ;

            /* Call the HFP lib function, this will determine the AT cmd to send
               depending on whether the profile instance is HSP or HFP compliant. */
            sinkAnswerOrRejectCall( TRUE );
        break ;
        case (EventUsrReject):
            MAIN_DEBUG(("HS: Reject\n" )) ;
            /* Reject incoming call - only valid for instances of HFP */
            sinkAnswerOrRejectCall( FALSE );
        break ;
        case (EventUsrCancelEnd):
            MAIN_DEBUG(("HS: CancelEnd\n" )) ;
            /* Terminate the current ongoing call process */
            sinkHangUpCall();

        break ;
        case (EventUsrTransferToggle):
            MAIN_DEBUG(("HS: Transfer\n" )) ;
            sinkTransferToggle();
        break ;
        case EventSysCheckForAudioTransfer :
            MAIN_DEBUG(("HS: Check Aud Tx\n")) ;
            sinkCheckForAudioTransfer(((AUDIO_TRANSFER_MESSAGE_T *)message)->priority);
            break ;

        case EventUsrMicrophoneMuteToggle:
        case EventUsrMicrophoneMuteOn:
        case EventUsrMicrophoneMuteOff:
        case EventUsrVolumeOrientationNormal:
        case EventUsrVolumeOrientationInvert:
        case EventUsrVolumeOrientationToggle:
        case EventUsrMainOutVolumeUp:
        case EventUsrMainOutVolumeDown:
        case EventUsrAuxOutVolumeUp:
        case EventUsrAuxOutVolumeDown:
        case EventUsrMainOutMuteOn:
        case EventUsrMainOutMuteOff:
        case EventUsrMainOutMuteToggle:
        case EventUsrAuxOutMuteOn:
        case EventUsrAuxOutMuteOff:
        case EventUsrAuxOutMuteToggle:
        case EventSysVolumeMax:
        case EventSysVolumeMin:
        case EventSysVolumeAndSourceChangeTimer:
            MAIN_DEBUG(("Sys/Usr Volume Event\n"));
            id = sinkVolumeModifyEventAccordingToVolumeOrientation(id);
            lIndicateEvent = sinkVolumeProcessEventVolume(id);
            break;

        case (EventSysEnterPairingEmptyPDL):
        case (EventUsrEnterPairing):
            MAIN_DEBUG(("HS: EnterPair [%d]\n" , lState )) ;

            /*go into pairing mode*/
            if (( lState != deviceLimbo) && (lState != deviceConnDiscoverable ))
            {
                theSink.inquiry.session = inquiry_session_normal;
                stateManagerEnterConnDiscoverableState( TRUE );
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventSysPairingFail):
            /*we have failed to pair in the alloted time - return to the connectable state*/
            MAIN_DEBUG(("HS: Pairing Fail\n")) ;
            if (lState != deviceTestMode)
            {
                switch (theSink.features.PowerDownOnDiscoTimeout)
                {
                    case PAIRTIMEOUT_POWER_OFF:
                    {
                        if (!sinkFmIsFmRxOn())
                        {
                            MessageSend ( task , EventUsrPowerOff , 0) ;
                        }
                    }
                        break;
                    case PAIRTIMEOUT_POWER_OFF_IF_NO_PDL:
                        /* Power off if no entries in PDL list */
                        if (ConnectionTrustedDeviceListSize() == 0)
                        {
                            if (!sinkFmIsFmRxOn())
                            {
                                MessageSend ( task , EventUsrPowerOff , 0) ;
                            }
                        }
                        else
                        {
                            /* when not configured to stay disconverable at all times */
                            if(!theSink.features.RemainDiscoverableAtAllTimes)
                            {
                                /* enter connectable mode */
                                stateManagerEnterConnectableState(TRUE);
                            }
#ifdef ENABLE_PEER
                                /* Attempt to establish connection with Peer */
                            peerConnectPeer();
#endif
                        }
                        break;
                    case PAIRTIMEOUT_CONNECTABLE:
                    default:
                        /* when not configured to stay disconverable at all times */
                        if(!theSink.features.RemainDiscoverableAtAllTimes)
                        {
							/* Check if we were connected before*/
                            if(deviceManagerNumConnectedDevs() == 0)
                            {
                            /* enter connectable state */
                            stateManagerEnterConnectableState(TRUE);
                        }
                            else
                            {
                                /* return to connected mode */
                                stateManagerEnterConnectedState();
                            }      
                        }
#ifdef ENABLE_PEER
                            /* Attempt to establish connection with Peer */
                        peerConnectPeer();
#endif
                        break;
                }
            }
            /* have attempted to connect following a power on and failed so clear the power up connection flag */
            theSink.powerup_no_connection = FALSE;

        break ;
        case ( EventSysPairingSuccessful):
            MAIN_DEBUG(("HS: Pairing Successful\n")) ;
            if (lState == deviceConnDiscoverable)
            {
                stateManagerEnterConnectableState(FALSE);
            }
        break ;

        case EventUsrEstablishPeerConnection:
            MAIN_DEBUG(("HS: Establish peer Connection\n"));
#ifdef ENABLE_PEER
                /* Attempt to establish connection with Peer */
                peerConnectPeer();
#endif
        break ;

        case ( EventUsrConfirmationAccept ):
            MAIN_DEBUG(("HS: Pairing Correct Res\n" )) ;
            sinkPairingAcceptRes();
        break;
        case ( EventUsrConfirmationReject ):
            MAIN_DEBUG(("HS: Pairing Reject Res\n" )) ;
            sinkPairingRejectRes();
        break;
        case ( EventUsrEstablishSLC ) :
                /* Make sure we're not using the Panic action */
                theSink.panic_reconnect = FALSE;
                /* Fall through */
        case ( EventSysEstablishSLCOnPanic ):

#ifdef ENABLE_SUBWOOFER
            /* if performing a subwoofer inquiry scan then cancel the SLC connect request
               this will resume after the inquiry scan has completed */
            if(theSink.inquiry.action == rssi_subwoofer)
            {
                lIndicateEvent = FALSE;
                break;
            }
#endif
            /* check we are not already connecting before starting */
            {
                MAIN_DEBUG(("EventUsrEstablishSLC\n")) ;

                slcEstablishSLCRequest() ;

                /* don't indicate the event at first power up if the use different event at power on
                   feature bit is enabled, this enables the establish slc event to be used for the second manual
                   connection request */
                if(stateManagerGetState() == deviceConnectable)
                {
                    /* send message to do indicate a start of paging process when in connectable state */
                    MessageSend(&theSink.task, EventSysStartPagingInConnState ,0);
                }
            }
        break ;
        case ( EventUsrRssiPair ):
            MAIN_DEBUG(("HS: RSSI Pair\n"));
            lIndicateEvent = inquiryPair( inquiry_session_normal, TRUE );
        break;
        case ( EventSysRssiResume ):
            MAIN_DEBUG(("HS: RSSI Resume\n"));
            inquiryResume();
        break;
        case ( EventSysRssiPairReminder ):
            MAIN_DEBUG(("HS: RSSI Pair Reminder\n"));
            if (stateManagerGetState() != deviceLimbo )
                MessageSendLater(&theSink.task, EventSysRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
            else
                lIndicateEvent = FALSE;

        break;
        case ( EventSysRssiPairTimeout ):
            MAIN_DEBUG(("HS: RSSI Pair Timeout\n"));
            inquiryTimeout();
        break;
        case ( EventSysRefreshEncryption ):
            MAIN_DEBUG(("HS: Refresh Encryption\n"));
            {
                uint8 k;
                Sink sink;
                Sink audioSink;
                /* For each profile */
                for(k=0;k<MAX_PROFILES;k++)
                {
                    MAIN_DEBUG(("Profile %d: ",k));
                    /* If profile is connected */
                    if((HfpLinkGetSlcSink((hfp_link_priority)(k + 1), &sink)) && sink)
                    {
                        /* If profile has no valid SCO sink associated with it */
                        HfpLinkGetAudioSink((hfp_link_priority)(k + hfp_primary_link), &audioSink);
                        if(!SinkIsValid(audioSink))
                        {
                            MAIN_DEBUG(("Key Refreshed\n"));
                            /* Refresh the encryption key */
                            ConnectionSmEncryptionKeyRefreshSink(sink);
                        }
#ifdef DEBUG_MAIN
                        else
                        {
                            MAIN_DEBUG(("Key Not Refreshed, SCO Active\n"));
                        }
                    }
                    else
                    {
                        MAIN_DEBUG(("Key Not Refreshed, SLC Not Active\n"));
#endif
                    }
                }
                MessageSendLater(&theSink.task, EventSysRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));
            }
        break;

        /* 60 second timer has triggered to disable connectable mode in multipoint
            connection mode */
        case ( EventSysConnectableTimeout ) :
#ifdef ENABLE_SUBWOOFER
            if(!SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
            {
                MAIN_DEBUG(("SM: disable Connectable Cancelled due to lack of subwoofer\n" ));
                break;
            }
#endif
#ifdef ENABLE_PARTYMODE
            /* leave headset connectable when using party mode */
            if(!(theSink.PartyModeEnabled))
#endif
            {
                /* only disable connectable mode if at least one hfp instance is connected */
                if(deviceManagerNumConnectedDevs())
                {
                    MAIN_DEBUG(("SM: disable Connectable \n" ));
                    /* disable connectability */
                    sinkDisableConnectable();
                }
            }
        break;

        case ( EventSysLEDEventComplete ) :
            /*the message is a ptr to the event we have completed*/
            MAIN_DEBUG(("HS : LEDEvCmp[%x]\n" ,  (( LMEndMessage_t *)message)->Event  )) ;

            switch ( (( const LMEndMessage_t *)message)->Event )
            {
                case (EventUsrResetPairedDeviceList) :
                {      /*then the reset has been completed*/
                    MessageSend(&theSink.task , EventSysResetComplete , 0 ) ;

                        /*power cycle if required*/
                    if ((theSink.features.PowerOffAfterPDLReset )&&
                        (stateManagerGetState() > deviceLimbo ))
                    {
                        MAIN_DEBUG(("HS: Reboot After Reset\n")) ;
                        if (!sinkFmIsFmRxOn())
                        {
                            MessageSend ( &theSink.task , EventUsrPowerOff , 0 ) ;
                        }
                    }
                }
                break ;

                case EventUsrPowerOff:
                {
                    /* Determine if a reset is required because the PS needs defragmentation */
                    configManagerDefragIfRequired();

                    /*allows a reset of the device for those designs which keep the chip permanently powered on*/
                    if (theSink.features.ResetAfterPowerOffComplete )
                    {
                        MAIN_DEBUG(("Reset\n"));
                        /* Reboot always - set the same boot mode; this triggers the target to reboot.*/
                        BootSetMode(BootGetMode());
                    }
                                       
                    sinkEventQueueDelete();

                    if(powerManagerIsVthmCritical())
                        stateManagerUpdateLimboState();
                }
                break ;

                default:
                break ;
            }

            if (theSink.features.QueueLEDEvents )
            {
                    /*if there is a queueud event*/
                if (LedManagerQueuedEvent())
                {
                    MAIN_DEBUG(("HS : Play Q'd Ev [%x]\n", LedManagerQueuedEvent()));
                    LedManagerIndicateQueuedEvent();
                }
                else
                {
                    /* restart state indication */
                    LEDManagerIndicateState ( stateManagerGetState () ) ;
                }
            }
            else
                LEDManagerIndicateState ( stateManagerGetState () ) ;

        break ;
        case (EventSysAutoSwitchOff):
            lResetAutoSwitchOff = sinkHandleAutoPowerOff(task, lState);
            break;

        case (EventUsrChargerConnected):
        {
            MAIN_DEBUG(("HS: Charger Connected\n"));
            powerManagerChargerConnected();
            if ( lState == deviceLimbo )
            {
                stateManagerUpdateLimboState();
            }

            /* indicate battery charging on the display */
            displayUpdateBatteryLevel(TRUE);
        }
        break;
        case (EventUsrChargerDisconnected):
        {
            MAIN_DEBUG(("HS: Charger Disconnected\n"));
            powerManagerChargerDisconnected();

            /* if in limbo state, schedule a power off event */
            if (lState == deviceLimbo )
            {
                /* cancel existing limbo timeout and rescheduled another limbo timeout */
                sinkSendLater(EventSysLimboTimeout, D_SEC(theSink.conf1->timeouts.LimboTimeout_s));

                /* turn off the display if in limbo and no longer charging */
                displaySetState(FALSE);
            }
            else
            {
                /* update battery display */
                displayUpdateBatteryLevel(FALSE);
            }
        }
        break;
        case (EventUsrResetPairedDeviceList):
            {
			/* NOTE: For Soundbar application,
			 * devices considered as "protected" (i.e BLE
			 * remote or subwoofer) will not be deleted in this
			 * event.*/

                MAIN_DEBUG(("HS: --Reset PDL--")) ;
                if ( stateManagerIsConnected () )
                {
                   /*disconnect any connected HFP profiles*/
                   sinkDisconnectAllSlc();
                   /*disconnect any connected A2DP/AVRCP profiles*/
                   disconnectAllA2dpAvrcp(TRUE);
                }

                deviceManagerRemoveAllDevices();

#ifdef ENABLE_PEER
                /* Ensure permanently paired Peer device is placed back into PDL */
                AuthInitPermanentPairing();
#endif

                if(INQUIRY_ON_PDL_RESET)
                    MessageSend(&theSink.task, EventUsrRssiPair, 0);
            }
        break ;
        case ( EventSysLimboTimeout ):
            {
                /*we have received a power on timeout - shutdown*/
                MAIN_DEBUG(("HS: EvLimbo TIMEOUT\n")) ;
                if (lState != deviceTestMode)
                {
                    stateManagerUpdateLimboState();
                }
            }
        break ;
        case EventSysSLCDisconnected:
                MAIN_DEBUG(("HS: EvSLCDisconnect\n")) ;
            {
                theSink.VoiceRecognitionIsActive = FALSE ;
                MessageCancelAll ( &theSink.task , EventSysNetworkOrServiceNotPresent ) ;
            }
        break ;
        case (EventSysLinkLoss):
            MAIN_DEBUG(("HS: Link Loss\n")) ;
            {
                conn_mask mask;

                /* should the device have been powered off prior to a linkloss event being
                   generated, this can happen if a link loss has occurred within 5 seconds
                   of power off, ensure the device does not attempt to reconnet from limbo mode */
                if(stateManagerGetState()== deviceLimbo)
                    lIndicateEvent = FALSE;

                /* Only get the profiles for the device which has a link loss.
                   If it is not specified fallback to all connected profiles. */
                if (theSink.linkloss_bd_addr && !BdaddrIsZero(theSink.linkloss_bd_addr))
                {
                    mask = deviceManagerProfilesConnected(theSink.linkloss_bd_addr);
                }
                else
                {
                    mask = deviceManagerGetProfilesConnected();
                }

                MAIN_DEBUG(("MAIN:    mask 0x%x linkLossReminderTime %u protection %u routed_audio 0x%p\n", 
                    mask, theSink.linkLossReminderTime,
                    theSink.stream_protection_state == linkloss_stream_protection_on,
                    (void *)theSink.routed_audio));

                /* The hfp library will generate repeat link loss events but
                   the a2dp library doesn't, so only repeat it here if:
                   a2dp only is in use, peer link loss stream protection is
                   not on (or there is no routed audio), or the link loss interval is > 0. */
                if (((!theSink.hfp_profiles) || (!(mask & conn_hfp) && (mask & conn_a2dp)))
                    && (theSink.linkloss_bd_addr 
                        && !(linklossIsStreamProtected(theSink.linkloss_bd_addr) && (theSink.routed_audio != 0)))
                    && (theSink.linkLossReminderTime != 0))
                {
                    linklossSendLinkLossTone(theSink.linkloss_bd_addr, D_SEC(theSink.linkLossReminderTime));
                }
                if(theSink.conf1->timeouts.AutoSwitchOffTime_s !=0)
                {
                    MAIN_DEBUG(("APD restart\n"));                    
                    sinkStartAutoPowerOffTimer();
                }
            }
        break ;
        case (EventSysMuteReminder) :
            MAIN_DEBUG(("HS: Mute Remind\n")) ;
            /*arrange the next mute reminder tone*/
            MessageSendLater( &theSink.task , EventSysMuteReminder , 0 ,D_SEC(theSink.conf1->timeouts.MuteRemindTime_s ) )  ;

            /* only play the mute reminder tone if AG currently having its audio routed is in mute state */
            if(!VolumePlayMuteToneQuery())
                lIndicateEvent = FALSE;
        break;

        case EventUsrBatteryLevelRequest:
          MAIN_DEBUG(("EventUsrBatteryLevelRequest\n")) ;
          powerManagerReadVbat(battery_level_user);
        break;

        case EventSysBatteryCritical:
            MAIN_DEBUG(("HS: EventSysBatteryCritical\n")) ;
	    /* Test code for HF indicator Battery Level */
#ifdef TEST_HF_INDICATORS
            hfIndicatorNotify(hf_battery_level, 1);
#endif /* TEST_HF_INDICATORS*/			
        break;

        case EventSysBatteryLow:
            MAIN_DEBUG(("HS: EventSysBatteryLow\n")) ;
	    /* Test code for HF indicator Battery Level */
#ifdef TEST_HF_INDICATORS
            hfIndicatorNotify(hf_battery_level, 10);
#endif /* TEST_HF_INDICATORS*/			
        break;

        case EventSysGasGauge0 :
        case EventSysGasGauge1 :
        case EventSysGasGauge2 :
        case EventSysGasGauge3 :
            MAIN_DEBUG(("HS: EventSysGasGauge%d\n", id - EventSysGasGauge0)) ;
        break ;

        case EventSysBatteryOk:
            MAIN_DEBUG(("HS: EventSysBatteryOk\n")) ;
	    /* Test code for HF indicator Battery Level */
#ifdef TEST_HF_INDICATORS
            hfIndicatorNotify(hf_battery_level, 100);
#endif /* TEST_HF_INDICATORS*/
        break;

        case EventSysChargeInProgress:
            MAIN_DEBUG(("HS: EventSysChargeInProgress\n")) ;
        break;

        case EventSysChargeComplete:
            MAIN_DEBUG(("HS: EventSysChargeComplete\n")) ;
        break;

        case EventSysChargeDisabled:
            MAIN_DEBUG(("HS: EventSysChargeDisabled\n")) ;
        break;

        case EventUsrEnterDUTState :
        {
            MAIN_DEBUG(("EnterDUTState \n")) ;
            stateManagerEnterTestModeState();
        }
        break;
#ifndef HYDRACORE_TODO
        case EventUsrEnterDutMode :
        {
            MAIN_DEBUG(("Enter DUT Mode \n")) ;
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventUsrEnterDUTState, 0 ) ;
            }
            enterDutMode () ;
        }
        break;

        case EventUsrEnterTXContTestMode :
        {
            MAIN_DEBUG(("Enter TX Cont \n")) ;
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventUsrEnterDUTState , 0 ) ;
            }
            enterTxContinuousTestMode() ;
        }
        break ;
#endif /* !HYDRACORE_TODO */

        case EventSysNetworkOrServiceNotPresent:
            {       /*only bother to repeat this indication if it is not 0*/
                if ( theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s )
                {       /*make sure only ever one in the system*/
                    sinkSendLater(EventSysNetworkOrServiceNotPresent,
                                        D_SEC(theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s) ) ;
                }
                MAIN_DEBUG(("HS: NO NETWORK [%d]\n", theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s )) ;
            }
        break ;
        case EventSysNetworkOrServicePresent:
            {
                MessageCancelAll ( task , EventSysNetworkOrServiceNotPresent ) ;
                MAIN_DEBUG(("HS: YES NETWORK\n")) ;
            }
        break ;
        case EventUsrLedsOnOffToggle  :
            MAIN_DEBUG(("HS: Toggle EN_DIS LEDS ")) ;
            MAIN_DEBUG(("HS: Tog Was[%c]\n" , TRUE_OR_FALSE(theSink.theLEDTask->gLEDSEnabled)));

            LedManagerToggleLEDS();
            MAIN_DEBUG(("HS: Tog Now[%c]\n" , TRUE_OR_FALSE(theSink.theLEDTask->gLEDSEnabled)));

            break ;
        case EventUsrLedsOn:
            MAIN_DEBUG(("HS: Enable LEDS\n")) ;
            LedManagerEnableLEDS ( ) ;
                /* also include the led disable state as well as orientation, write this to the PSKEY*/
            configManagerWriteSessionData ( ) ;
#ifdef ENABLE_PEER                
            sinkAvrcpUpdateLedIndicationOnOff(TRUE);
#endif
            break ;
        case EventUsrLedsOff:
            MAIN_DEBUG(("HS: Disable LEDS\n")) ;
            LedManagerDisableLEDS ( ) ;

                /* also include the led disable state as well as orientation, write this to the PSKEY*/
            configManagerWriteSessionData ( ) ;
#ifdef ENABLE_PEER                
            sinkAvrcpUpdateLedIndicationOnOff(FALSE);
#endif
            break ;
        case EventSysCancelLedIndication:
            MAIN_DEBUG(("HS: Disable LED indication\n")) ;
            LedManagerResetLEDIndications ( ) ;
            break ;
        case EventSysCallAnswered:
            MAIN_DEBUG(("HS: EventSysCallAnswered\n")) ;
        break;
        
        case EventSysSLCConnected:
        case EventSysSLCConnectedAfterPowerOn:
            
            MAIN_DEBUG(("HS: EventSysSLCConnected\n")) ;
            /*if there is a queued event - we might want to know*/
            sinkRecallQueuedEvent();
            
            /* postpone auto switch-off */
            lResetAutoSwitchOff = TRUE;
        break;
        
        case EventSysPrimaryDeviceConnected:
        case EventSysSecondaryDeviceConnected:
            /*used for indication purposes only*/
            MAIN_DEBUG(("HS:Device Connected [%c]\n " , (id - EventSysPrimaryDeviceConnected)? 'S' : 'P'  ));
            sinkHandleAccessoryConnect(id - EventSysPrimaryDeviceConnected);
            break;
        case EventSysPrimaryDeviceDisconnected:
        case EventSysSecondaryDeviceDisconnected:
            /*used for indication purposes only*/
            MAIN_DEBUG(("HS:Device Disconnected [%c]\n " , (id - EventSysPrimaryDeviceDisconnected)? 'S' : 'P'  ));
            sinkHandleAccessoryDisconnect(id - EventSysPrimaryDeviceDisconnected);
            break;
        case EventSysVLongTimer:
        case EventSysLongTimer:
           if (lState == deviceLimbo)
           {
               lIndicateEvent = FALSE ;
           }
        break ;
            /*these events have no required action directly associated with them  */
             /*they are received here so that LED patterns and Tones can be assigned*/
        case EventSysSCOLinkOpen :
            MAIN_DEBUG(("EventSysSCOLinkOpen\n")) ;
        break ;
        case EventSysSCOLinkClose:
            MAIN_DEBUG(("EventSysSCOLinkClose\n")) ;
        break ;
        case EventSysEndOfCall :
            MAIN_DEBUG(("EventSysEndOfCall\n")) ;
            displayShowSimpleText(DISPLAYSTR_CLEAR,1);
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
        break;
        case EventSysResetComplete:
            MAIN_DEBUG(("EventSysResetComplete\n")) ;
        break ;
        case EventSysError:
            MAIN_DEBUG(("EventSysError\n")) ;
        break;
        case EventSysReconnectFailed:
            MAIN_DEBUG(("EventSysReconnectFailed\n")) ;
        break;

#ifdef THREE_WAY_CALLING
        case EventUsrThreeWayReleaseAllHeld:
            MAIN_DEBUG(("HS3 : RELEASE ALL\n"));
            /* release the held call */
            MpReleaseAllHeld();
        break;
        case EventUsrThreeWayAcceptWaitingReleaseActive:
            MAIN_DEBUG(("HS3 : ACCEPT & RELEASE\n"));
            MpAcceptWaitingReleaseActive();
        break ;
        case EventUsrThreeWayAcceptWaitingHoldActive  :
            MAIN_DEBUG(("HS3 : ACCEPT & HOLD\n"));
            /* three way calling not available in multipoint usage */
            MpAcceptWaitingHoldActive();
        break ;
        case EventUsrThreeWayAddHeldTo3Way  :
            MAIN_DEBUG(("HS3 : ADD HELD to 3WAY\n"));
            /* check to see if a conference call can be created, more than one call must be on the same AG */
            MpHandleConferenceCall(TRUE);
        break ;
        case EventUsrThreeWayConnect2Disconnect:
            MAIN_DEBUG(("HS3 : EXPLICIT TRANSFER\n"));
            /* check to see if a conference call can be created, more than one call must be on the same AG */
            MpHandleConferenceCall(FALSE);
        break ;
#endif
        case (EventSysEnablePowerOff):
        {
            MAIN_DEBUG(("HS: EventSysEnablePowerOff \n")) ;
            theSink.PowerOffIsEnabled = TRUE ;
        }
        break;
        case EventUsrPlaceIncomingCallOnHold:
            sinkPlaceIncomingCallOnHold();
        break ;

        case EventUsrAcceptHeldIncomingCall:
            sinkAcceptHeldIncomingCall();
        break ;
        case EventUsrRejectHeldIncomingCall:
            sinkRejectHeldIncomingCall();
        break;

        case EventUsrEnterDFUMode:
        {
            MAIN_DEBUG(("EventUsrEnterDFUMode\n")) ;
            BootSetMode(BOOTMODE_DFU);
        }
        break;
#ifndef HYDRACORE_TODO
        case EventUsrEnterServiceMode:
        {
            MAIN_DEBUG(("Enter Service Mode \n")) ;

            enterServiceMode();
        }
        break ;
#endif /* !HYDRACORE_TODO */
        case EventSysServiceModeEntered:
        {
            MAIN_DEBUG(("Service Mode!!!\n")) ;
        }
        break;

        case EventSysAudioMessage1:
        case EventSysAudioMessage2:
        case EventSysAudioMessage3:
        case EventSysAudioMessage4:
        {
            if (theSink.routed_audio)
            {
                uint16 * lParam = (uint16*)mallocPanic(sizeof(uint16)) ;

                *lParam = (id -  EventSysAudioMessage1) ; /*0,1,2,3*/
                if(!AudioSetMode ( AUDIO_MODE_CONNECTED , (void *) lParam) )
                    freePanic(lParam);
            }
        }
        break ;

        case EventUsrUpdateStoredNumber:
            sinkUpdateStoredNumber();
        break;

        case EventUsrDialStoredNumber:
            MAIN_DEBUG(("EventUsrDialStoredNumber\n"));
            sinkDialStoredNumber();

        break;
        case EventUsrRestoreDefaults:
            MAIN_DEBUG(("EventUsrRestoreDefaults\n"));
            configManagerRestoreDefaults();

        break;

        case EventSysTone1:
        case EventSysTone2:
            MAIN_DEBUG(("HS: EventTone[%d]\n" , (id - EventSysTone1 + 1) )) ;
        break;

        case EventUsrSelectAudioPromptLanguageMode:
            if(theSink.audio_prompts_enabled)
            {
                MAIN_DEBUG(("EventUsrSelectAudioPromptLanguageMode"));
                AudioPromptSelectLanguage();
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break;

        case EventSysStoreAudioPromptLanguage:
            if(theSink.audio_prompts_enabled)
            {
                /* Store Prompt language in PS */
                configManagerWriteSessionData () ;
            }
        break;

        
        
        /* disabled leds have been re-enabled by means of a button press or a specific event */
        case EventSysResetLEDTimeout:
            MAIN_DEBUG(("EventSysResetLEDTimeout\n"));
            LEDManagerIndicateState ( lState ) ;
            theSink.theLEDTask->gLEDSStateTimeout = FALSE ;
        break;
        /* starting paging whilst in connectable state */
        case EventSysStartPagingInConnState:
            MAIN_DEBUG(("EventSysStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theSink.paging_in_progress = TRUE;
        break;

        /* paging stopped whilst in connectable state */
        case EventSysStopPagingInConnState:
            MAIN_DEBUG(("EventSysStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theSink.paging_in_progress = FALSE;
        break;

        /* continue the slc connection procedure, will attempt connection
           to next available device */
        case EventSysContinueSlcConnectRequest:
            /* don't continue connecting if in pairing mode */
            if(stateManagerGetState() != deviceConnDiscoverable)
            {
                MAIN_DEBUG(("EventSysContinueSlcConnectRequest\n"));
                /* attempt next connection */
                slcContinueEstablishSLCRequest();
            }
        break;

        /* indication of call waiting when using two AG's in multipoint mode */
        case EventSysMultipointCallWaiting:
            MAIN_DEBUG(("EventSysMultipointCallWaiting\n"));
        break;

        /* kick off a check the role of the device and make changes if appropriate by requesting a role indication */
        case EventSysCheckRole:
            linkPolicyCheckRoles();
        break;

        case EventSysMissedCall:
        {
            if(theSink.conf1->timeouts.MissedCallIndicateTime_s != 0)
            {
                MessageCancelAll(task , EventSysMissedCall ) ;

                theSink.MissedCallIndicated -= 1;
                if(theSink.MissedCallIndicated != 0)
                {
                    MessageSendLater( &theSink.task , EventSysMissedCall , 0 , D_SEC(theSink.conf1->timeouts.MissedCallIndicateTime_s) ) ;
                }
            }
        }
        break;

        case EventUsrHfpEnhancedSafetyIndicator:
        { 
#ifdef TEST_HF_INDICATORS        
            hfIndicatorNotify(hf_enhanced_safety, 0);
#endif /*  TEST_HF_INDICATORS */
        }
        break;
        
#ifdef ENABLE_PBAP
        case EventUsrPbapDialMch:
        {
            /* pbap dial from missed call history */
            MAIN_DEBUG(("EventUsrPbapDialMch\n"));

            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   &&
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {
                    pbapDialPhoneBook(pbap_mch);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;

        case EventUsrPbapDialIch:
        {
            /* pbap dial from incoming call history */
            MAIN_DEBUG(("EventUsrPbapDialIch\n"));

            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   &&
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {
                    pbapDialPhoneBook(pbap_ich);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;

        case EventSysEstablishPbap:
        {
            MAIN_DEBUG(("EventSysEstablishPbap\n"));

            /* Connect to the primary and secondary hfp link devices */
            theSink.pbapc_data.pbap_command = pbapc_action_idle;

            pbapConnect( hfp_primary_link );
            pbapConnect( hfp_secondary_link );
        }
        break;

        case EventUsrPbapSetPhonebook:
        {
            MAIN_DEBUG(("EventUsrPbapSetPhonebook, active pb is [%d]\n", theSink.pbapc_data.pbap_active_pb));

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_command = pbapc_setting_phonebook;

            if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
            {
                pbapConnect( hfp_primary_link );
            }
            else
            {
                /* Set the link to active state */
                linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseEntry:
        {
            MAIN_DEBUG(("EventUsrPbapBrowseEntry\n"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                /* If Pbap profile does not connected, connect it first */
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                    theSink.pbapc_data.pbap_browsing_start_flag = 1;
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    if(theSink.pbapc_data.pbap_browsing_start_flag == 0)
                    {
                        theSink.pbapc_data.pbap_browsing_start_flag = 1;
                        PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                    }
                    else
                    {
                        MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_ENTRY , 0 ) ;
                    }
                }

                theSink.pbapc_data.pbap_command = pbapc_browsing_entry;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseList:
        /* EventUsrPbapBrowseListByName event is added for PTS qualification */
        case EventUsrPbapBrowseListByName:
        {
            MAIN_DEBUG(("EventUsrPbapBrowseList%s\n",(id == EventUsrPbapBrowseListByName) ? "ByName" : "" ));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                }

                theSink.pbapc_data.pbap_command = pbapc_browsing_list;

                if (id == EventUsrPbapBrowseListByName)
                {
                    theSink.pbapc_data.pbap_srch_attr = pbap_search_name;
                    /* for PTS qualification, search string expected is "PTS" */
                    theSink.pbapc_data.pbap_srch_val = (const uint8*) "PTS";
                }
                else
                {
                    theSink.pbapc_data.pbap_srch_attr = pbap_search_number;
                    theSink.pbapc_data.pbap_srch_val = NULL;
                }
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapDownloadPhonebook:
        {
            MAIN_DEBUG(("EventUsrPbapDownloadPhonebook\n"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PULL_PHONE_BOOK , 0 ) ;
                }

                theSink.pbapc_data.pbap_command = pbapc_downloading;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapGetPhonebookSize:
        {
            MAIN_DEBUG(("EventUsrPbapGetPhonebookSize"));

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PHONE_BOOK_SIZE , 0 ) ;
                }

                theSink.pbapc_data.pbap_command = pbapc_phonebooksize;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapSelectPhonebookObject:
        {
            MAIN_DEBUG(("EventUsrPbapSelectPhonebookObject\n"));

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                theSink.pbapc_data.pbap_active_pb += 1;

                if(theSink.pbapc_data.pbap_active_pb > pbap_cch)
                {
                    theSink.pbapc_data.pbap_active_pb = pbap_pb;
                }
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseComplete:
        {
            MAIN_DEBUG(("EventUsrPbapBrowseComplete\n"));

            /* Set the link policy based on the HFP or A2DP state */
            linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;
            lIndicateEvent = FALSE ;

        }
        break;


#endif

#ifdef WBS_TEST
        /* TEST EVENTS for WBS testing */
        case EventUsrWbsTestSetCodecs:
            if(theSink.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), FALSE);
            }
            else
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
                theSink.RenegotiateSco = 1;
                HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , FALSE);
            }

        break;

        case EventUsrWbsTestSetCodecsSendBAC:
            if(theSink.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), TRUE);
            }
           else
           {
               MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
               theSink.RenegotiateSco = 1;
               HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , TRUE);
           }
           break;

         case EventUsrWbsTestOverrideResponse:

           if(theSink.FailAudioNegotiation)
           {
               MAIN_DEBUG(("HS : Fail Neg = off\n")) ;
               theSink.FailAudioNegotiation = 0;
           }
           else
           {
               MAIN_DEBUG(("HS : Fail Neg = on\n")) ;
               theSink.FailAudioNegotiation = 1;
           }
       break;

#endif

       case EventUsrCreateAudioConnection:
           MAIN_DEBUG(("HS : Create Audio Connection\n")) ;

           CreateAudioConnection();
       break;

#ifdef ENABLE_MAPC
        case EventSysMapcMsgNotification:
            /* Generate a tone or audio prompt */
            MAIN_DEBUG(("HS : EventSysMapcMsgNotification\n")) ;
		break;
        case EventSysMapcMnsSuccess:
            /* Generate a tone to indicate the mns service success */
            MAIN_DEBUG(("HS : EventSysMapcMnsSuccess\n")) ;
        break;
        case EventSysMapcMnsFailed:
            /* Generate a tone to indicate the mns service failed */
            MAIN_DEBUG(("HS : EventSysMapcMnsFailed\n")) ;
        break;
#endif

       case EventUsrIntelligentPowerManagementOn:
           MAIN_DEBUG(("HS : Enable LBIPM\n")) ;
            /* enable LBIPM operation */
           theSink.lbipmEnable = 1;
           /* send plugin current power level */
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ;
       break;

       case EventUsrIntelligentPowerManagementOff:
           MAIN_DEBUG(("HS : Disable LBIPM\n")) ;
            /* disable LBIPM operation */
           theSink.lbipmEnable = 0;
           /* notify the plugin Low power mode is no longer required */
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ;
       break;

       case EventUsrIntelligentPowerManagementToggle:
           MAIN_DEBUG(("HS : Toggle LBIPM\n")) ;
           if(theSink.lbipmEnable)
           {
               MessageSend( &theSink.task , EventUsrIntelligentPowerManagementOff , 0 ) ;
           }
           else
           {
               MessageSend( &theSink.task , EventUsrIntelligentPowerManagementOn , 0 ) ;
           }

       break;
#ifndef HYDRACORE_TODO
        case EventUsrUsbPlayPause:
        case EventUsrUsbStop:
        case EventUsrUsbFwd:
        case EventUsrUsbBack:
        case EventUsrUsbMute:
        case EventUsrUsbLowPowerMode:
        case EventSysUsbDeadBatteryTimeout:
        case EventSysAllowUSBVolEvents:
            lIndicateEvent = sinkUsbProcessEventUsb(id);
            break;
#endif /* !HYDRACORE_TODO */
        case EventUsrAnalogAudioConnected:
            /* Start the timer here to turn off the device if this feature is enabled by the user*/
            if(theSink.features.PowerOffOnWiredAudioConnected)
            {
                /* cancel existing limbo timeout and reschedule another limbo timeout */
                sinkSendLater(EventSysLimboTimeout, D_SEC(theSink.conf1->timeouts.WiredAudioConnectedPowerOffTimeout_s));
            }
            else
            {
#ifdef ENABLE_PEER
                /*If the Analog Audio has connected then notify this to the peer device */
                sinkAvrcpUpdatePeerWiredSourceConnected(ANALOG_AUDIO);
                peerClaimRelay(TRUE);
                PEER_UPDATE_REQUIRED_RELAY_STATE("ANALOG AUDIO CONNECTED");
#endif
                /* Update audio routing */
                audioUpdateAudioRouting();
            }
            break;

        case EventUsrSpdifAudioConnected:
            /* Update audio routing */
        	audioUpdateAudioRouting();
        break;
        case EventUsrAnalogAudioDisconnected:
#ifdef ENABLE_PEER
            {
                /*If the Analog Audio has disconnected then notify this to the peer device */
                sinkAvrcpUpdatePeerSourceDisconnected(ANALOG_AUDIO);
                peerClaimRelay(FALSE);
                PEER_UPDATE_REQUIRED_RELAY_STATE("ANALOG AUDIO DISCONNECTED");
            }
#endif
            /* Update audio routing */
            audioUpdateAudioRouting();
        break;

        case EventUsrSpdifAudioDisconnected:
            /* Update audio routing */
        	audioUpdateAudioRouting();
        break;


#ifdef ENABLE_AVRCP

       case EventUsrAvrcpPlayPause:
            MAIN_DEBUG(("HS : EventUsrAvrcpPlayPause\n")) ;
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPlayPause();
       break;

      case EventUsrAvrcpPlay:
            MAIN_DEBUG(("HS : EventUsrAvrcpPlay\n")) ;
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPlay();
       break;

       case EventUsrAvrcpPause:
            MAIN_DEBUG(("HS : EventUsrAvrcpPause\n")) ;
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPause();
       break;

       case EventUsrAvrcpStop:
            MAIN_DEBUG(("HS : EventUsrAvrcpStop\n")) ;
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpStop();
       break;

       case EventUsrAvrcpSkipForward:
           MAIN_DEBUG(("HS : EventUsrAvrcpSkipForward\n")) ;
           sinkAvrcpSkipForward();
       break;

       case EventUsrEnterBootMode2:
            MAIN_DEBUG(("Reboot into different bootmode [2]\n")) ;
           BootSetMode(BOOTMODE_CUSTOM) ;
       break ;

       case EventUsrAvrcpSkipBackward:
           MAIN_DEBUG(("HS : EventUsrAvrcpSkipBackward\n")) ;
           sinkAvrcpSkipBackward();
       break;

       case EventUsrAvrcpFastForwardPress:
           MAIN_DEBUG(("HS : EventUsrAvrcpFastForwardPress\n")) ;
           sinkAvrcpFastForwardPress();
           /* rescehdule a repeat of this message every 1.5 seconds */
           MessageSendLater( &theSink.task , EventUsrAvrcpFastForwardPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
       break;

       case EventUsrAvrcpFastForwardRelease:
           MAIN_DEBUG(("HS : EventUsrAvrcpFastForwardRelease\n")) ;
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
           sinkAvrcpFastForwardRelease();
       break;

       case EventUsrAvrcpRewindPress:
           MAIN_DEBUG(("HS : EventUsrAvrcpRewindPress\n")) ;
           /* rescehdule a repeat of this message every 1.8 seconds */
           MessageSendLater( &theSink.task , EventUsrAvrcpRewindPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
           sinkAvrcpRewindPress();
       break;

       case EventUsrAvrcpRewindRelease:
           MAIN_DEBUG(("HS : EventUsrAvrcpRewindRelease\n")) ;
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
           sinkAvrcpRewindRelease();
       break;

       case EventUsrAvrcpToggleActive:
           MAIN_DEBUG(("HS : EventUsrAvrcpToggleActive\n"));
           if (sinkAvrcpGetNumberConnections() > 1)
               sinkAvrcpToggleActiveConnection();
           else
               lIndicateEvent = FALSE;
       break;

       case EventUsrAvrcpNextGroupPress:
           MAIN_DEBUG(("HS : EventUsrAvrcpNextGroupPress\n"));
           sinkAvrcpNextGroupPress();
       break;

        case EventUsrAvrcpNextGroupRelease:
           MAIN_DEBUG(("HS : EventUsrAvrcpNextGroupRelease\n"));
           sinkAvrcpNextGroupRelease();
       break;

       case EventUsrAvrcpPreviousGroupPress:
           MAIN_DEBUG(("HS : EventUsrAvrcpPreviousGroupPress\n"));
           sinkAvrcpPreviousGroupPress();
       break;

       case EventUsrAvrcpPreviousGroupRelease:
           MAIN_DEBUG(("HS : EventUsrAvrcpPreviousGroupRelease\n"));
           sinkAvrcpPreviousGroupRelease();
       break;

       case EventUsrAvrcpShuffleOff:
           MAIN_DEBUG(("HS : EventUsrAvrcpShuffleOff\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_OFF);
        break;

       case EventUsrAvrcpShuffleAllTrack:
           MAIN_DEBUG(("HS : EventUsrAvrcpShuffleAllTrack\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_ALL_TRACK);
        break;

       case EventUsrAvrcpShuffleGroup:
           MAIN_DEBUG(("HS : EventUsrAvrcpShuffleGroup\n"));
           sinkAvrcpShuffle(AVRCP_SHUFFLE_GROUP);
        break;

       case EventUsrAvrcpRepeatOff:
           MAIN_DEBUG(("HS : EventUsrAvrcpRepeatOff\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_OFF);
        break;

       case EventUsrAvrcpRepeatSingleTrack:
           MAIN_DEBUG(("HS : EventUsrAvrcpRepeatSingleTrack\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_SINGLE_TRACK);
        break;

       case EventUsrAvrcpRepeatAllTrack:
           MAIN_DEBUG(("HS : EventUsrAvrcpRepeatAllTrack\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_ALL_TRACK);
        break;

       case EventUsrAvrcpRepeatGroup:
           MAIN_DEBUG(("HS : EventUsrAvrcpRepeatGroup\n"));
           sinkAvrcpRepeat(AVRCP_REPEAT_GROUP);
        break;
        case EventSysSetActiveAvrcpConnection:
        {
            sinkAvrcpSetActiveConnectionFromBdaddr(&((const UpdateAvrpcMessage_t *)message)->bd_addr);
        }
        break;
       case EventSysResetAvrcpMode:
        {
            uint16 index = *(const uint16 *) message;
            lIndicateEvent = FALSE ;
            theSink.avrcp_link_data->link_active[index] =  FALSE;
        }
        break;

        case EventUsrTwsQualificationVolUp:
            MAIN_DEBUG(( "TWS Qualification Volume Up\n" ));
            handleAvrcpQualificationVolumeUp();
            break;

        case EventUsrTwsQualificationVolDown:
            MAIN_DEBUG(( "TWS Qualification Volume Down\n" ));
            handleAvrcpQualificationVolumeDown();
            break;

        case EventUsrTwsQualificationSetAbsVolume:
            MAIN_DEBUG(( "TWS SetAbsoluteVolume\n" ));
            handleAvrcpQualificationSetAbsoluteVolume();
            break;
			
        case EventUsrTwsQualificationPlayTrack:
            MAIN_DEBUG(( "TWS Qualification Play Track\n" ));
            handleAvrcpQualificationPlayTrack();
            break;

        case EventUsrTwsQualificationAVRCPConfigureDataSize:
            MAIN_DEBUG(("TWS Qualification AVRCP Configure Data Size\n" ));
            handleAvrcpQualificationConfigureDataSize();
            break;

        case EventUsrQualificationSecondAvctpConnectReq:
            MAIN_DEBUG(("AVCTP:Qualification Send Second Request\n" ));
            sinkSendAvctpSecondConnect();
            break;

#endif

#ifdef ENABLE_PEER
        case EventUsrSrcQualificationSendReconfigurationReq:
            MAIN_DEBUG(("TWS: SRC Send Reconfiguration Request\n" ));
            a2dpIssuePeerReconfigureRequest();
            break;
#endif

        case EventUsrSinkQualificationSendReconfigurationReq:
            MAIN_DEBUG(("HS: Sink Send Reconfiguration Request\n" ));
            a2dpIssueSinkReconfigureRequest();
            break;

        case EventUsrSinkQualificationSendSuspendReq:
            MAIN_DEBUG(("HS: Sink Send Suspend Request\n" ));

            if(theSink.a2dp_link_data)
            {
                if(theSink.a2dp_link_data->qual_disable_stream_resume == FALSE)
                {
                    theSink.a2dp_link_data->qual_disable_stream_resume = TRUE;
                    a2dpIssueSuspendRequest();
                }
                else
                {
                    /* Reset the stream suspend flag. */
                    theSink.a2dp_link_data->qual_disable_stream_resume = FALSE;
                }
            }
            break;

        case EventUsrSwitchAudioMode:
        {
            AUDIO_MODE_T mode = AUDIO_MODE_CONNECTED;
            /* If USB in use set current USB mode */
#ifndef HYDRACORE_TODO
            usbAudioGetMode(&mode);
#endif /* !HYDRACORE_TODO */
            /* cycle through EQ modes */
            theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = A2DP_MUSIC_PROCESSING_FULL_NEXT_EQ_BANK;
            MAIN_DEBUG(("HS : EventUsrSwitchAudioMode %x\n", theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing ));
            AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
        break;

       case EventUsrButtonLockingToggle:
            MAIN_DEBUG(("HS : EventUsrButtonLockingToggle (%d)\n",theSink.buttons_locked));
            if (theSink.buttons_locked)
            {
                MessageSend( &theSink.task , EventUsrButtonLockingOff , 0 ) ;
            }
            else
            {
                MessageSend( &theSink.task , EventUsrButtonLockingOn , 0 ) ;
            }
        break;

        case EventUsrButtonLockingOn:
            MAIN_DEBUG(("HS : EventUsrButtonLockingOn\n"));
            theSink.buttons_locked = TRUE;
        break;

        case EventUsrButtonLockingOff:
            MAIN_DEBUG(("HS : EventUsrButtonLockingOff\n"));
            theSink.buttons_locked = FALSE;
        break;

        case EventUsrAudioPromptsOff:
            MAIN_DEBUG(("HS : EventUsrAudioPromptsOff"));
            /* disable audio prompts */

            /* Play the disable audio prompts prompt before actually disabling them */
            if(theSink.audio_prompts_enabled == TRUE) /* Check if audio prompts are already enabled */
            {
                sinkAudioIndicationPlayEvent(id);
            }

            theSink.audio_prompts_enabled = FALSE;
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;
        break;

        case EventUsrAudioPromptsOn:
            MAIN_DEBUG(("HS : EventUsrAudioPromptsOn"));
            /* enable audio prompts */
            theSink.audio_prompts_enabled = TRUE;
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;
        break;
#ifndef HYDRACORE_TODO
        case EventUsrTestModeAudio:
            MAIN_DEBUG(("HS : EventUsrTestModeAudio\n"));
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterAudioTestMode();
        break;

        case EventUsrTestModeTone:
            MAIN_DEBUG(("HS : EventUsrTestModeTone\n"));
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterToneTestMode();
        break;

        case EventUsrTestModeKey:
            MAIN_DEBUG(("HS : EventUsrTestModeKey\n"));
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterKeyTestMode();
        break;
#endif /* !HYDRACORE_TODO */
#ifdef ENABLE_SPEECH_RECOGNITION
        case EventSysSpeechRecognitionStart:
        {

            if ( speechRecognitionIsEnabled() )
                speechRecognitionStart() ;
            else
                lIndicateEvent = FALSE;
        }
        break ;
        case EventSysSpeechRecognitionStop:
        {
            if(speechRecognitionIsEnabled() )
                speechRecognitionStop() ;
            else
                lIndicateEvent = FALSE;
        }
        break ;
        /* to tune the Speech Recognition using the UFE generate this event */
        case EventUsrSpeechRecognitionTuningStart:
        {
            /* ensure speech recognition is enabled */
            if ( speechRecognitionIsEnabled() )
            {
                /* ensure not already in tuning mode */
                if(!theSink.csr_speech_recognition_tuning_active)
                {
                    theSink.csr_speech_recognition_tuning_active = TRUE;
                    speechRecognitionStart() ;
                }
            }
        }
        break;

        case EventSysSpeechRecognitionTuningYes:
        break;

        case EventSysSpeechRecognitionTuningNo:
        break;

        case EventSysSpeechRecognitionFailed:
        break;
#endif

        case EventUsrTestDefrag:
            MAIN_DEBUG(("HS : EventUsrTestDefrag\n"));
            configManagerFillPs();
        break;

        case EventSysStreamEstablish:
            MAIN_DEBUG(("HS : EventSysStreamEstablish[%u]\n", ((const EVENT_STREAM_ESTABLISH_T *)message)->priority));
            connectA2dpStream( ((const EVENT_STREAM_ESTABLISH_T *)message)->priority, 0 );
        break;

        case EventSysA2dpConnected:
            MAIN_DEBUG(("HS : EventSysA2dpConnected\n"));
        break;

        case EventSysA2dpDisconnected:
            MAIN_DEBUG(("HS : EventSysA2dpDisconnected\n"));
        break;

        case EventSysUpdateAttributes:
            deviceManagerDelayedUpdateAttributes((const EVENT_UPDATE_ATTRIBUTES_T*)message);
        break;

        case EventUsrPeerSessionConnDisc:
            MAIN_DEBUG(("HS: PeerSessionConnDisc [%d]\n" , lState )) ;
            /*go into pairing mode*/
            if ( lState != deviceLimbo)
            {
                /* ensure there is only one device connected to allow peer dev to connect */
                if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
                {
#ifdef ENABLE_PEER
                    uint16 index;
                    uint16 srcIndex;
                    uint16 avrcpIndex;
                    /* check whether the a2dp connection is present and streaming data and that the audio is routed, if thats true then pause the stream */
                    if(theSink.routed_audio && getA2dpIndexFromSink(theSink.routed_audio, &index)
                        && (A2dpMediaGetState(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index]) == a2dp_stream_streaming)
                        && a2dpGetSourceIndex(&srcIndex) && (srcIndex == index)
                        && sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[index], &avrcpIndex, TRUE))
                    {
                        /* cancel any queued ff or rw requests and then pause the streaming*/
                        MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                        MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
                        sinkAvrcpPlayPauseRequest(avrcpIndex,AVRCP_PAUSE);
                    }
#endif

                    theSink.inquiry.session = inquiry_session_peer;
                    stateManagerEnterConnDiscoverableState( FALSE );
                }
                /* no free connections, indicate an error condition */
                else
                {
                    lIndicateEvent = FALSE;
                    MessageSend ( &theSink.task , EventSysError , 0 ) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;

        case ( EventUsrPeerSessionInquire ):
            MAIN_DEBUG(("HS: PeerSessionInquire\n"));

            /* ensure there is only one device connected to allow peer dev to connect */
            if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
            {
#ifdef ENABLE_PEER
                uint16 index;
                uint16 srcIndex;
                uint16 avrcpIndex;
                /* check whether the a2dp connection is present and streaming data and that the audio is routed, if thats true then pause the stream */
                if(theSink.routed_audio && getA2dpIndexFromSink(theSink.routed_audio, &index)
                    && (A2dpMediaGetState(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index]) == a2dp_stream_streaming)
                    && a2dpGetSourceIndex(&srcIndex) && (srcIndex == index)
                    && sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[index], &avrcpIndex, TRUE))
                {
                    /* cancel any queued ff or rw requests and then pause the streaming*/
                    MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                    MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
                    sinkAvrcpPlayPauseRequest(avrcpIndex,AVRCP_PAUSE);
                }
#endif
                lIndicateEvent = inquiryPair( inquiry_session_peer, FALSE );
            }
            /* no free connections, indicate an error condition */
            else
            {
                lIndicateEvent = FALSE;
                MessageSend ( &theSink.task , EventSysError , 0 ) ;
            }

        break;

#ifdef ENABLE_PEER
        case EventUsrPeerSessionPair:
            MAIN_DEBUG(("HS: EventUsrPeerSessionPair [%d]\n" , lState )) ;
            /*go into pairing mode*/ 
            if ( lState != deviceLimbo)
            {
                /* ensure there is only one device connected to allow peer dev to connect */
                if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
                {
                    uint16 index;  
                    uint16 srcIndex;
                    uint16 avrcpIndex;
                    /* check whether the a2dp connection is present and streaming data and that the audio is routed, 
                       if so cancel any fast forward/rewind actions and do not inquire for any slave
                       devices since this will disrupt the A2DP audio stream */
                    if(theSink.routed_audio && getA2dpIndexFromSink(theSink.routed_audio, &index)  
                        && (A2dpMediaGetState(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index]) == a2dp_stream_streaming)
                        && a2dpGetSourceIndex(&srcIndex) && (srcIndex == index)
                        && sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[index], &avrcpIndex, TRUE))
                    {
                        /* cancel any queued ff or rw requests and then pause the streaming*/
                        MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                        MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
                    }                   
                    /* if not currently receiving an A2DP stream from an AG,
                       attempt to discover a peer device, this will protect the 
                       audio quality of the A2DP link */
                    else
                    {
                        lIndicateEvent = inquiryPair( inquiry_session_peer, FALSE );
                    }
                    /* go into discoverable mode */
                    theSink.inquiry.session = inquiry_session_peer;
                    stateManagerEnterConnDiscoverableState( FALSE );    
                }
                /* no free connections, indicate an error condition */
                else
                {
                    lIndicateEvent = FALSE;
                    MessageSend ( &theSink.task , EventSysError , 0 ) ;               
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break;
#endif /* def ENABLE_PEER */
        
        case EventUsrPeerSessionEnd:
        {
#ifdef PEER_SCATTERNET_DEBUG   /* Scatternet debugging only */
            uint16 i;
            for_all_a2dp(i)
            {
                if (theSink.a2dp_link_data)
                {
                    theSink.a2dp_link_data->invert_ag_role[i] = !theSink.a2dp_link_data->invert_ag_role[i];
                    MAIN_DEBUG(("HS: invert_ag_role[%u] = %u\n",i,theSink.a2dp_link_data->invert_ag_role[i]));

                    if (theSink.a2dp_link_data->connected[i] && (theSink.a2dp_link_data->peer_device[i] != remote_device_peer))
                    {
                        linkPolicyUseA2dpSettings( theSink.a2dp_link_data->device_id[i],
                                                   theSink.a2dp_link_data->stream_id[i],
                                                   A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[i]) );
                    }
                }
            }
#else   /* Normal operation */
            MAIN_DEBUG(("HS: EventUsrPeerSessionEnd\n"));
            lIndicateEvent = disconnectAllA2dpPeerDevices();
#endif
        }
        break;

        case EventUsrPeerReserveLinkOn:
#ifdef ENABLE_PEER
            peerReserveLink(TRUE);  
#endif              
        break;

        case EventUsrPeerReserveLinkOff:
#ifdef ENABLE_PEER
            peerReserveLink(FALSE);
#endif
        break;        
        
        case EventUsrSwapA2dpMediaChannel:
            /* attempt to swap media channels, don't indicate event if not successful */
            if(!audioSwapMediaChannel())
                lIndicateEvent = FALSE;
        break;

        /* bassboost/bassplus  enable disable toggle */
        case EventUsrBassEnhanceEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_BASS_ENHANCE_BYPASS)
            {
                /* disable bassboost/bassplus */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_ENHANCE_BYPASS,FALSE);
            }
            else
            {
                /* enable bassboost/bassplus */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_ENHANCE_BYPASS,TRUE);
            }
        break;

        /* bassboost/bassplus  enable indication */
        case EventUsrBassEnhanceOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable bassboost/bassplus  */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_ENHANCE_BYPASS,TRUE);
        break;

        /* bassboost/bassplus disable indication */
        case EventUsrBassEnhanceOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable bassboost/bassplus  */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_ENHANCE_BYPASS,FALSE);
        break;

        /* 3D/3DV enhancement enable disable toggle */
        case EventUsr3DEnhancementEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS)
            {
                /* disable 3D/3DV */
                sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS,FALSE);
            }
            else
            {
                /* enable 3D/3DV */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS,TRUE);
            }
        break;

        /* 3D/3DV enhancement enable indication */
        case EventUsr3DEnhancementOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable 3D/3DV */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS,TRUE);
        break;

        /* 3D/3DV enhancement disable indication */
        case EventUsr3DEnhancementOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable 3D/3DV */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS,FALSE);
        break;

         /* User EQ enable disable toggle indication */
        case EventUsrUserEqOnOffToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_USER_EQ_BYPASS)
            {
                /* disable User EQ */
                sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, FALSE);
            }
            else
            {
                /* enable User EQ */
                sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, TRUE);
            }
        break;

       /* User EQ enable indication */
        case EventUsrUserEqOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, TRUE);
        break;

        /* User EQ disable indication */
        case EventUsrUserEqOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, FALSE);
        break;

        /* check whether the Audio Amplifier drive can be turned off after playing
           a tone or voice prompt */
        case EventSysCheckAudioAmpDrive:
            /* cancel any pending messages */
            MessageCancelAll( &theSink.task , EventSysCheckAudioAmpDrive);
            /* when the device is no longer routing audio tot he speaker then
               turn off the audio amplifier */
            if((!IsAudioBusy()) && (!theSink.routed_audio))
            {
                MAIN_DEBUG (( "HS : EventSysCheckAudioAmpDrive turn off amp\n" ));
                PioDrivePio(PIO_AUDIO_ACTIVE, FALSE);
            }
            /* audio is still busy, check again later */
            else
            {
#ifndef HYDRACORE_TODO
                MAIN_DEBUG (( "HS : EventSysCheckAudioAmpDrive still busy, reschedule\n" ));
#endif /* HYDRACORE_TODO */
                MessageSendLater(&theSink.task , EventSysCheckAudioAmpDrive, 0, CHECK_AUDIO_AMP_PIO_DRIVE_DELAY);
            }
        break;

        /* external microphone has been connected */
        case EventUsrExternalMicConnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        break;

        /* external microphone has been disconnected */
        case EventUsrExternalMicDisconnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_NOT_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
       break;

       /* event to enable the simple speech recognition functionality, persistant over power cycle */
       case EventUsrSSROn:
            theSink.ssr_enabled = TRUE;
       break;

       /* event to disable the simple speech recognition functionality, persistant over power cycle */
       case EventUsrSSROff:
            theSink.ssr_enabled = FALSE;
       break;

       /* NFC tag detected, determine action based on current connection state */
       case EventUsrNFCTagDetected:
           /** The code below should be replaced something similar to
            *  handleNfcClTagReadStarted(task) to enable fast connection.
            *  This would be triggered on the detection of NFC Field from
            *  the phone. */
            /* ADK 4.x code */
            /* if not connected to an AG, go straight into pairing mode */
            if(stateManagerGetState() < deviceConnected)
            {        
                theSink.inquiry.session = inquiry_session_normal;
                stateManagerEnterConnDiscoverableState( FALSE ); 
            }
       break;

       /* check whether the current routed audio is still the correct one and
          change sources if appropriate */
       case EventSysCheckAudioRouting:
            /* check audio routing */
    	   audioUpdateAudioRouting();
            /* don't indicate event as may be generated by USB prior to configuration
               being loaded */
            lIndicateEvent = FALSE;
       break;
       

            
	   /* Audio amplifier is to be shut down by PIO for power saving purposes */
       case EventSysAmpPowerDown:
            MAIN_DEBUG (( "HS : EventSysAmpPowerDown\n"));
            stateManagerAmpPowerControl(POWER_DOWN);
       break;
               
       case EventSysAmpPowerUp:
            MAIN_DEBUG (( "HS : EventSysAmpPowerUp\n"));
            stateManagerAmpPowerControl(POWER_UP);
            break;

       case EventUsrFmRxOn:
       case EventUsrFmRxOff:
       case EventUsrFmRxTuneUp:
       case EventUsrFmRxTuneDown:
       case EventUsrFmRxStore:
       case EventUsrFmRxTuneToStore:
       case EventUsrFmRxErase:
           lIndicateEvent = sinkFmProcessEventUsrFmRx(id);
           break;

       case EventUsrSelectAudioSourceAnalog:
       case EventUsrSelectAudioSourceSpdif:
       case EventUsrSelectAudioSourceI2S:
       case EventUsrSelectAudioSourceUSB:
       case EventUsrSelectAudioSourceAG1:
       case EventUsrSelectAudioSourceAG2:
       case EventUsrSelectAudioSourceFM:
       case EventUsrSelectAudioSourceNone:
       case EventUsrSelectAudioSourceNext:
       case EventUsrSelectAudioSourceNextRoutable:
           lIndicateEvent = processEventUsrSelectAudioSource(id);
       break;



#ifdef ENABLE_SUBWOOFER
       case EventUsrSubwooferStartInquiry:
            handleEventUsrSubwooferStartInquiry();
       break;

       case EventSysSubwooferCheckPairing:
            handleEventSysSubwooferCheckPairing();
       break;

       case EventSysSubwooferOpenLLMedia:
            /* open a Low Latency media connection */
            handleEventSysSubwooferOpenLLMedia();
       break;

       case EventSysSubwooferOpenStdMedia:
            /* open a standard latency media connection */
            handleEventSysSubwooferOpenStdMedia();
       break;

       case EventUsrSubwooferVolumeUp:
            handleEventUsrSubwooferVolumeUp();
       break;

       case EventUsrSubwooferVolumeDown:
            handleEventUsrSubwooferVolumeDown();
       break;

       case EventSysSubwooferCloseMedia:
            handleEventSysSubwooferCloseMedia();
       break;

       case EventSysSubwooferStartStreaming:
            handleEventSysSubwooferStartStreaming();
       break;

       case EventSysSubwooferSuspendStreaming:
            handleEventSysSubwooferSuspendStreaming();
       break;

       case EventUsrSubwooferDisconnect:
            handleEventUsrSubwooferDisconnect();
       break;

       case EventUsrSubwooferDeletePairing:
            handleEventUsrSubwooferDeletePairing();
       break;

       /* set subwoofer volume level by message to maintain synchronisation with
          audio plugins */
       case EventSysSubwooferSetVolume:
            /* send volume level change to subwoofer */
            updateSwatVolumeNow(((SWAT_VOLUME_CHANGE_MSG_T*)message)->new_volume);
       break;
#endif
#ifndef HYDRACORE_TODO
       case EventUsrEnterDriverlessDFUMode:
            LoaderModeEnter();
       break;
#endif /* !HYDRACORE_TODO */
#ifdef ENABLE_PARTYMODE
       /* enabling the runtime control of the party mode feature */
       case EventUsrPartyModeOn:
       {
            /* ensure a party mode operating type has been selected in configuration 
               before enabling the feature */
            if(theSink.features.PartyMode)   
            {    
               /* turn party mode on */
               theSink.PartyModeEnabled = TRUE;

			   /* if HFP channels are connected, disconnect them, it's 
		        * more code efficient to just call to disconnect both 
		        * links and let it fail if they're not already connected 
		        */

               HfpSlcDisconnectRequest(hfp_primary_link);
		       HfpSlcDisconnectRequest(hfp_secondary_link);

               /* if more than one device connected, drop the second device */
               if( deviceManagerNumConnectedDevs() > 1 )
               {
                   /* but allow a second one to be connected */
                   sinkPartyModeDisconnectDevice(a2dp_secondary);
               }
               else
               {
                   /* ensure headset is discoverable
                      and connectable once enabled */
                   sinkEnableConnectable();
               }
               /* ensure pairable */
               sinkEnableDiscoverable();
           }
       }
       break;

       /* disabling the runtime control of the party mode feature */
       case EventUsrPartyModeOff:
       {
           /* no need to ensure a party mode operating type has been selected
              in configuration before disabling the feature */
           if(theSink.PartyModeEnabled)
           {
               /* turn party mode off */
               theSink.PartyModeEnabled = FALSE;
			   
			   /* Shutting down all connections as it's impossible to re-connect
			    * after party mode ... because the connection and pairing tables
				* are not accurately maintained after so many connections and 
				* pairings while in Party Mode. */

			   sinkDisconnectAllSlc();
               disconnectAllA2dpAvrcp(TRUE);

               
               /* ensure pairable */
               sinkEnableDiscoverable();
           }
       }
       break;
	   
       /* connected device hasn't played music before timeout, disconnect it to allow
          another device to connect */
       case EventSysPartyModeTimeoutDevice1:
            sinkPartyModeDisconnectDevice(a2dp_primary);
       break;

       /* connected device hasn't played music before timeout, disconnect it to allow
          another device to connect */
       case EventSysPartyModeTimeoutDevice2:
            sinkPartyModeDisconnectDevice(a2dp_secondary);
       break;
#endif

#ifdef ENABLE_GAIA
        case EventUsrGaiaDFURequest:
        /*  GAIA DFU requires that audio not be busy, so disallow any tone  */
            lIndicateEvent = FALSE;
            gaiaDfuRequest();
        break;
#endif

        case EventSysRemoteControlCodeReceived:
             /* Display a led pattern*/
        break;

#ifdef ENABLE_IR_REMOTE
        case EventSysIRCodeLearnSuccess:
        {
            /* TODO : Play a tone or something */
        }
        break;
        case EventSysIRCodeLearnFail:
        {
            /* TODO : Play a tone or something */
        }
        break;
        case EventSysIRLearningModeTimeout:
        {
            irStopLearningMode();
        }
        break;
        case EventSysIRLearningModeReminder:
        {
            handleIrLearningModeReminder();
        }
        break;
        case EventUsrStartIRLearningMode:
        {
            irStartLearningMode();
        }
        break;
        case EventUsrStopIRLearningMode:
        {
            irStopLearningMode();
        }
        break;
        case EventUsrClearIRCodes:
        {
            irClearLearntCodes();
        }
        break;
#endif

#if defined ENABLE_PEER && defined PEER_TWS
        case EventUsrMasterDeviceTrimVolumeUp:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(increase_volume, tws_master);
            break;

        case EventUsrMasterDeviceTrimVolumeDown:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(decrease_volume, tws_master);
            break;

        case EventUsrSlaveDeviceTrimVolumeUp:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(increase_volume, tws_slave);
            break;

        case EventUsrSlaveDeviceTrimVolumeDown:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(decrease_volume, tws_slave);
            break;

        case EventUsrChangeAudioRouting:
        {
            if(theSink.features.PeerPermittedRouting != 0)
            {
                uint16 current_routing = ((theSink.a2dp_link_data->a2dp_audio_mode_params.master_routing_mode & 0x3) << 2) | (theSink.a2dp_link_data->a2dp_audio_mode_params.slave_routing_mode & 0x3);
                uint16 distance = 16;
                uint16 index = 0;
                uint16 i;
            
                /* Find entry in tws_audio_routing table which is closest to current routing mode */
                for(i = 0; i < sizeof(tws_audio_routing); i++)
                {
                    if (tws_audio_routing[i] < current_routing)
                    {
                        if ((current_routing - tws_audio_routing[i]) < distance)
                        {
                            distance = current_routing - tws_audio_routing[i];
                            index = i;
                        }
                    }
                    else
                    {
                        if ((tws_audio_routing[i] - current_routing) < distance)
                        {
                            distance = tws_audio_routing[i] - current_routing;
                            index = i;
                        }
                    }
                }

                do{
                    /* Select next routing mode in table */
                    index = (index + 1) % sizeof(tws_audio_routing);
                }while(!((1 << index) & theSink.features.PeerPermittedRouting));
            
                sinkA2dpSetPeerAudioRouting((PeerTwsAudioRouting)((tws_audio_routing[index] >> 2) & 0x3), 
                                            (PeerTwsAudioRouting)(tws_audio_routing[index] & 0x3));
            }
            break;
        }
#endif

#ifdef ENABLE_GAIA_PERSISTENT_USER_EQ_BANK

        /* When timeout occurs, session data needs to be updated for new EQ settings */
        case EventSysGaiaEQChangesStoreTimeout:
            configManagerWriteSessionData ();
            break;
#endif

        case EventSysCheckDefrag:
        case EventSysDefrag:
            configManagerProcessEventSysDefrag(id);
            break;

        case EventSysToneDigit0:
        case EventSysToneDigit1:
        case EventSysToneDigit2:
        case EventSysToneDigit3:
        case EventSysToneDigit4:
        case EventSysToneDigit5:
        case EventSysToneDigit6:
        case EventSysToneDigit7:
        case EventSysToneDigit8:
        case EventSysToneDigit9:
            break;

        /* event to start an LED state indication, called by
           message to reduce maximum stack usage */
        case EventSysLEDIndicateState:
            {
               /* use LEDS to indicate current state */
               LEDManagerIndicateState ( ((const LED_INDICATE_STATE_T*)message)->state ) ;
            }
            break;
#ifndef HYDRACORE_TODO
        case EventUsrBleStartBonding:
            {
                MAIN_DEBUG(("HS : BLE Bondable\n"));
                sinkBleBondableEvent();
            }
            break;

        case EventUsrBleDeleteDevice:
            {
                MAIN_DEBUG(( "HS : Delete Marked LE device \n" ));
                sinkBleDeleteMarkedDevices();
            }
            break;

        case EventSysBleBondablePairingTimeout:
            {
                MAIN_DEBUG(("HS : BLE Bondable Pairing Timeout\n"));
                sinkBleBondablePairingTimeoutEvent();
            }
            break;

        case EventSysBleBondableConnectionTimeout:
            {
                MAIN_DEBUG(("HS : BLE Bondable Connection Timeout\n"));
                sinkBleBondableConnectionTimeoutEvent();
            }
            break;

        case EventSysBleHrSensorInContact:
            {
                MAIN_DEBUG(("HS : HR Sensor in contact, start sending Heart Rate measurements \n"));
                sinkBleHRSensorInContact();
            }
            break;
                
        case EventSysBleHrSensorNotInContact:
            {
                MAIN_DEBUG(("HS : HR sensor not in contact, Stop sending Heart Rate measurements \n"));
                sinkBleHRSensorNotInContact();
            }
            break;
            

       case EventSysHeartRateThresholdReached:
            {
                MAIN_DEBUG(( "User heart rate threshold reached\n" ));
            }
            break;
            
       case EventUsrFindMyRemoteImmAlertMild:
            {
                MAIN_DEBUG(( "Find my remote Imm Alert Mild\n" ));
                sinkGattIasClientSetAlert(gatt_imm_alert_level_mild, sink_gatt_ias_alert_remote);
            }
            break;
            
        case EventUsrFindMyRemoteImmAlertHigh:
            {
                MAIN_DEBUG(( "Find my remote Imm Alert High\n" ));
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_remote);
            }
            break;

       case EventUsrFindMyPhoneImmAlertMild:
            {
                MAIN_DEBUG(( "Find my phone Imm Alert Mild\n" ));
                sinkGattIasClientSetAlert(gatt_imm_alert_level_mild, sink_gatt_ias_alert_phone);
            }
            break;

        case EventUsrFindMyPhoneImmAlertHigh:
            {
                MAIN_DEBUG(( "Find my phone Imm Alert High \n" ));
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_phone);
            }
            break;

        /*Alert the phone from remote control using HID. This event needs to be mapped to the HID code for alert*/
        case EventUsrFindMyPhoneRemoteImmAlertHigh:
            {
                MAIN_DEBUG(( "Find my phone Imm Alert High - Triggered from HID Remote\n" ));
                sinkGattImmAlertLocalAlert(gatt_imm_alert_level_high);
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_phone);
            }
            break;

        case EventUsrImmAlertStop:
        case EventSysImmAlertTimeout:
            {
                MAIN_DEBUG(( "IAS : Stop Alert/Timeout\n" ));

                /*Local Alert*/
                sinkGattServerImmAlertStopAlert();

                /*Remote Alert*/
                if (sinkGattIasClientEnabled())
                {
                    sinkGattIasClientSetAlert(gatt_imm_alert_level_no, sink_gatt_ias_alert_none);
                }
            }
            break;

        case EventSysImmAlertMild:
            {
                MAIN_DEBUG(( "HS : Mild Alert \n" ));
                sinkGattServerImmAlertMild(theSink.conf1->timeouts.ImmediateAlertTimer_s);
            }
            break;

        case EventSysImmAlertHigh:
            {
                MAIN_DEBUG(( "HS : High Alert \n" ));
                sinkGattServerImmAlertHigh(theSink.conf1->timeouts.ImmediateAlertTimer_s);
            }
        break;
        case EventSysImmAlertErrorBeep:
            MAIN_DEBUG(( "HS : Recieved Immediate Alert Error Beep\n" ));
        break;

        case EventSysLlsAlertTimeout:
        case EventUsrLlsAlertStop:
        {
            MAIN_DEBUG(( "HS : Link Loss Stop Alert \n" ));
            sinkGattLinkLossAlertStop();
        }
        break;
        case EventSysLlsAlertMild:
        {
            MAIN_DEBUG(( "HS : Link Loss Mild Alert \n" ));
            sinkGattLinkLossAlertMild(theSink.conf1->timeouts.LinkLossTimer_s);
        }
        break;
        case EventSysLlsAlertHigh:
        {
            MAIN_DEBUG(( "HS : Link Loss High Alert \n" ));
            sinkGattLinkLossAlertHigh(theSink.conf1->timeouts.LinkLossTimer_s);
        }
        break;

        case EventSysAncsOtherAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Other Notifications Alert\n" ));
        break;

        case EventSysAncsIncomingCallAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Incoming Call Alert\n" ));
        break;

        case EventSysAncsMissedCallAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Missed Call Alert\n" ));
        break;

        case EventSysAncsVoiceMailAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Voice Mail Alert\n" ));
        break;

        case EventSysAncsSocialAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Social Alert\n" ));
        break;

        case EventSysAncsScheduleAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Schedule Alert\n" ));
        break;

        case EventSysAncsEmailAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Email Alert\n" ));
        break;

        case EventSysAncsNewsAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS News Alert\n" ));
        break;

        case EventSysAncsHealthNFittnessAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Health N Fittness Alert\n" ));
        break;

        case EventSysAncsBusinessNFinanceAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Business N Finance Alert\n" ));
        break;

        case EventSysAncsLocationAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Location Alert\n" ));
        break;

        case EventSysAncsEntertainmentAlert:
            MAIN_DEBUG(( "HS : Recieved ANCS Entertainmen Alert\n" ));
        break;

        case EventUsrBleHidExtraConfig:
            MAIN_DEBUG(( "HS : BLE HID Extra configuration for qualification\n" ));
            sinkGattHIDClientExtraConfig();
        break;

        case EventUsrBleSimulateHrNotifications:
            MAIN_DEBUG(( "HS : BLE notify simulated Heart rate measurements\n" ));
            MessageSend(&theSink.task, EventSysBleHrSensorInContact, 0);
        break;
#endif /* !HYDRACORE_TODO */           
        

        case EventSysReboot:
            BootSetMode(BootGetMode());
            break;

        case EventUsrAncOn:
            sinkAncEnable();
            break;

        case EventUsrAncOff:
            sinkAncDisable();
            break;

        case EventUsrAncToggleOnOff:
            sinkAncToggleEnable();
            break;

        case EventUsrAncLeakthroughMode:
            sinkAncSetLeakthroughMode();
            break;

        case EventUsrAncActiveMode:
            sinkAncSetActiveMode();
            break;

        case EventUsrAncNextMode:
            sinkAncSetNextMode();
            break;

        case EventUsrAncVolumeDown:
            sinkAncVolumeDown();
            break;

        case EventUsrAncVolumeUp:
            sinkAncVolumeUp();
            break;

        case EventUsrAncCycleGain:
            sinkAncCycleAdcDigitalGain();
            break;
            
        case EventSysAncDisabled:
            MAIN_DEBUG(( "ANC Disabled\n" ));
            break;

        case EventSysAncActiveModeEnabled:
            MAIN_DEBUG(( "ANC Enabled in Active Mode\n" ));
            break;

        case EventSysAncLeakthroughModeEnabled:
            MAIN_DEBUG(( "ANC Enabled in Leakthrough Mode\n" ));
            break;

        case EventUsrStartA2DPStream:
            MAIN_DEBUG(( "A2DP Start streaming media\n" ));
            audioA2dpStartStream();
            break;

        case EventSysPromptsTonesQueueCheck:
            if(!IsAudioBusy())
            {
                MAIN_DEBUG(("Audio Free ,Fetch Event\n" ));
                id = sinkEventQueueFetch();
            }
            break;

#ifdef ENABLE_PEER
        case EventUsrTwsQualificationEnablePeerOpen:
            MAIN_DEBUG(( "TWS Qualification Enable Opening of Peer Media Channel\n" ));
            handlePeerQualificationEnablePeerOpen();
            break;
        case EventSysA2DPPeerLinkLossTimeout:
            theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE; 
            MAIN_DEBUG(("EventSysA2DPPeerLinkLossTimeout\n"));
            break;         
        case EventSysRemovePeerTempPairing:
            HandlePeerRemoveAuthDevice((const bdaddr*)message); 
            break; 
        case EventSysA2dpPauseSuspendTimeoutDevice1:
            /* Connected device hasn't suspended its a2dp media channel before timeout,try to force it. */
            a2dpSuspendNonRoutedStream(a2dp_primary);
            break;

        case EventSysA2dpPauseSuspendTimeoutDevice2:
            /* Connected device hasn't suspended its a2dp media channel before timeout, try to force it. */
            a2dpSuspendNonRoutedStream(a2dp_secondary);      
            break;
            
        case EventSysTwsAudioRoutingChanged:
            MAIN_DEBUG(("HS : EventSysTwsAudioRoutingChanged\n"));
            break;
#endif /* def ENABLE_PEER */
            
        case EventUsrSensorUp:
        case EventUsrSensorDown:
            sinkHandleAccessoryReportUserEvent(id);
            break;

       /*these events have no required action directly associated with them  */
       /*they are received here so that LED patterns and Tones can be assigned*/
       case EventSysHfpSecureLink:
           MAIN_DEBUG(( "HFP link is Secure\n" ));
           break;
      case EventSysSCOSecureLinkOpen :
            MAIN_DEBUG(("EventSysSCOSecureLinkOpen\n")) ;
            break ;
      case EventSysSCOSecureLinkClose:
            MAIN_DEBUG(("EventSysSCOSecureLinkClose\n")) ;
            break;
      case EventUsrTestUserConfirmationYes:
            MAIN_DEBUG(("EventUsrTestUserConfirmationYes\n")) ;
            test_sinkHandleUserConfirmation(TRUE);
           break;
      case EventUsrTestUserConfirmationNo:
            MAIN_DEBUG(("EventUsrTestUserConfirmationNo\n")) ;
            test_sinkHandleUserConfirmation(FALSE);
           break;
      case EventSysLESecureLink:
            MAIN_DEBUG(("EventSysLESecureLink\n"));
            break;
        default :
            MAIN_DEBUG (( "HS : UE unhandled!! [%x]\n", id ));
        break ;

    }

    if((TRUE ==lResetAutoSwitchOff) && (id != EventUsrPowerOff))
    {
         sinkStartAutoPowerOffTimer();
    }
	
        /* Inform the event indications that we have received a user event*/
        /* For all events except the end event notification as this will just end up here again calling itself...*/
    if ( lIndicateEvent && (!eventToBeIndicatedBeforeProcessing(id)))
    {
    	IndicateEvent(id);
    }

#ifdef ENABLE_GAIA
    gaiaReportEvent(id);
#endif

#ifdef TEST_HARNESS
    vm2host_send_event(id);
#endif

#ifdef DEBUG_MALLOC
    printf("MAIN: Event [%x] Available SLOTS:[%d]\n" ,id, VmGetAvailableAllocations() ) ;
#endif

}


/*************************************************************************
NAME
    handleHFPMessage

DESCRIPTION
    handles the messages from the user events

RETURNS

*/
static void handleHFPMessage  ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("HFP = [%x]\n", id)) ;

    UNUSED(task);

    switch(id)
    {
        /* -- Handsfree Profile Library Messages -- */
    case HFP_INIT_CFM:
        {
            /* Init configuration that is required now */


            InitEarlyUserFeatures();
            MAIN_DEBUG(("HFP_INIT_CFM - enable streaming[%x]\n", theSink.features.EnableA2dpStreaming)) ;

            sinkInitConfigureDeviceClass();

            if  ( stateManagerGetState() == deviceLimbo )
            {
                if ( ((const HFP_INIT_CFM_T*)message)->status == hfp_success )
                {
                    sinkInitComplete( (const HFP_INIT_CFM_T*)message );
                    UartSendStr("+HFINIT\r\n");
                }
                else
                {
                    Panic();
            }
        }
        }
    break;

    case HFP_SLC_CONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_CONNECT_IND\n"));
        if (stateManagerGetState() != deviceLimbo)
        {
            sinkHandleSlcConnectInd((const HFP_SLC_CONNECT_IND_T *) message);
        }
    break;

    case HFP_SLC_CONNECT_CFM:
        {
            const HFP_SLC_CONNECT_CFM_T *conncfm = (const HFP_SLC_CONNECT_CFM_T *)message;

            MAIN_DEBUG(("HFP_SLC_CONNECT_CFM [%x]\n", conncfm->status ));
        if (stateManagerGetState() == deviceLimbo)
        {
                if ( conncfm->status == hfp_success )
            {
                /*A connection has been made and we are now  logically off*/
                sinkDisconnectAllSlc();
            }
        }
        else
        {
                sinkHandleSlcConnectCfm(conncfm);
#ifdef ENABLE_PEER            
                if(!peerLinkReservedCanDeviceConnect(&conncfm->bd_addr))
            {  /* Another link is reserved for a peer device to connect, disconnect the second AG.*/ 
                    sinkDisconnectSlcFromDevice(&conncfm->bd_addr);
            }
#endif
#ifdef ENABLE_PARTYMODE
            if( theSink.PartyModeEnabled )
			{
			    /* if an HFP channel is connected, disconnect it 
				 * immediately, the identity of the conenction is 
				 * found in the message confirming the connection */
                    HfpSlcDisconnectRequest( conncfm->priority );
            }
#endif
        }
        }
        break;

    case HFP_SLC_LINK_LOSS_IND:
        MAIN_DEBUG(("HFP_SLC_LINK_LOSS_IND\n"));
        slcHandleLinkLossInd((const HFP_SLC_LINK_LOSS_IND_T*)message);
    break;

    case HFP_SLC_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_DISCONNECT_IND\n"));
        MAIN_DEBUG(("Handle Disconnect\n"));
        sinkHandleSlcDisconnectInd((const HFP_SLC_DISCONNECT_IND_T *) message);
    break;
    case HFP_SERVICE_IND:
        MAIN_DEBUG(("HFP_SERVICE_IND [%x]\n" , ((const HFP_SERVICE_IND_T*)message)->service  ));
        indicatorsHandleServiceInd ( (const HFP_SERVICE_IND_T*)message ) ;
    break;
    case HFP_HF_INDICATORS_REPORT_IND:
        MAIN_DEBUG(("HFP_HF_INDICATORS_REPORT_IND  Number of HF Indicators:%x BitMask:[%x]\n" , 
              ((HFP_HF_INDICATORS_REPORT_IND_T*)message)->num_hf_indicators,
              ((HFP_HF_INDICATORS_REPORT_IND_T*)message)->hf_indicators_mask));
    break;
    case HFP_HF_INDICATORS_IND:
        /* To do- App devleopers should take action accordingly based on HF indication*/
        MAIN_DEBUG(("HFP_HF_INDICATORS_IND Assigned Num:[%x] Status:[%x]\n" , ((HFP_HF_INDICATORS_IND_T*)message)->hf_indicator_assigned_num,
              ((HFP_HF_INDICATORS_IND_T*)message)->hf_indicator_status));
    break;
    /* indication of call status information, sent whenever a change in call status
       occurs within the hfp lib */
    case HFP_CALL_STATE_IND:
        /* the Call Handler will perform device state changes and be
           used to determine multipoint functionality */
        /* don't process call indications if in limbo mode */
        if(stateManagerGetState()!= deviceLimbo)
            sinkHandleCallInd((const HFP_CALL_STATE_IND_T*)message);
    break;

    case HFP_RING_IND:
        MAIN_DEBUG(("HFP_RING_IND\n"));
        sinkHandleRingInd((const HFP_RING_IND_T *)message);
    break;
    case HFP_VOICE_TAG_NUMBER_IND:
        MAIN_DEBUG(("HFP_VOICE_TAG_NUMBER_IND\n"));
        sinkWriteStoredNumber((const HFP_VOICE_TAG_NUMBER_IND_T*)message);
    break;
    case HFP_DIAL_LAST_NUMBER_CFM:
        MAIN_DEBUG(("HFP_LAST_NUMBER_REDIAL_CFM\n"));
        handleHFPStatusCFM (((const HFP_DIAL_LAST_NUMBER_CFM_T*)message)->status ) ;
    break;
    case HFP_DIAL_NUMBER_CFM:
        MAIN_DEBUG(("HFP_DIAL_NUMBER_CFM %d %d\n", stateManagerGetState(), 
                            ((const HFP_DIAL_NUMBER_CFM_T *) message)->status));
        handleHFPStatusCFM (((const HFP_DIAL_NUMBER_CFM_T*)message)->status ) ;
    break;
    case HFP_DIAL_MEMORY_CFM:
        MAIN_DEBUG(("HFP_DIAL_MEMORY_CFM %d %d\n", stateManagerGetState(), 
                            ((const HFP_DIAL_MEMORY_CFM_T *) message)->status));
    break ;
    case HFP_CALL_ANSWER_CFM:
        MAIN_DEBUG(("HFP_ANSWER_CALL_CFM\n"));
    break;
    case HFP_CALL_TERMINATE_CFM:
        MAIN_DEBUG(("HFP_TERMINATE_CALL_CFM %d\n", stateManagerGetState()));
    break;
    case HFP_VOICE_RECOGNITION_IND:
        MAIN_DEBUG(("HS: HFP_VOICE_RECOGNITION_IND_T [%c]\n" ,
                            TRUE_OR_FALSE( ((const HFP_VOICE_RECOGNITION_IND_T* )message)->enable) )) ;

             
    /*update the state of the voice dialling on the back of the indication*/
        theSink.VoiceRecognitionIsActive = ((const HFP_VOICE_RECOGNITION_IND_T* ) message)->enable ;
    break;
    case HFP_VOICE_RECOGNITION_ENABLE_CFM:
        MAIN_DEBUG(("HFP_VOICE_RECOGNITION_ENABLE_CFM s[%d] w[%d]i", 
                            (((const HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) , 
                            theSink.VoiceRecognitionIsActive));

            /*if the cfm is in error then we did not succeed - toggle */
        if  ( (((const HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) )
            theSink.VoiceRecognitionIsActive = 0 ;

        MAIN_DEBUG(("[%d]\n", theSink.VoiceRecognitionIsActive));

        handleHFPStatusCFM (((const HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) ;
    break;
    case HFP_CALLER_ID_ENABLE_CFM:
        MAIN_DEBUG(("HFP_CALLER_ID_ENABLE_CFM\n"));
    break;
    case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
    {
        const HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind = (const HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *) message;

        MAIN_DEBUG(("HFP_VOLUME_SYNC_SPEAKER_GAIN_IND %d\n", ind->volume_gain));

        VolumeHandleSpeakerGainInd(ind);
    }
    break;
    case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
    {
        const HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind = (const HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T*)message;

        MAIN_DEBUG(("HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND %d\n", ind->mic_gain));
        if(theSink.features.EnableSyncMuteMicrophones)
        {
            VolumeSetHfpMicrophoneGainCheckMute(ind->priority, (uint8)ind->mic_gain);
        }
    }

    break;

    case HFP_CALLER_ID_IND:
        {
            const HFP_CALLER_ID_IND_T *ind = (const HFP_CALLER_ID_IND_T *) message;

            /* ensure this is not a HSP profile */
            MAIN_DEBUG(("HFP_CALLER_ID_IND number %s", ind->caller_info + ind->offset_number));
            MAIN_DEBUG((" name %s\n", ind->caller_info + ind->offset_name));

            /* Show name or number on display */
            if (ind->size_name)
            {
                displayShowSimpleText((char *) ind->caller_info + ind->offset_name, 1);
            }
            else
            {
                displayShowSimpleText((char *) ind->caller_info + ind->offset_number, 1);
            }

            /* Attempt to play caller name */
            if(!AudioPromptPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
            {
                /* Caller name not present or not supported, try to play number */
                AudioPromptPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
            }
        }

    break;

    case HFP_UNRECOGNISED_AT_CMD_IND:
    {
        sinkHandleUnrecognisedATCmd( (HFP_UNRECOGNISED_AT_CMD_IND_T*)message ) ;
    }
    break ;

    case HFP_HS_BUTTON_PRESS_CFM:
        {
            MAIN_DEBUG(("HFP_HS_BUTTON_PRESS_CFM\n")) ;
        }
    break ;
     /*****************************************************************/

#ifdef THREE_WAY_CALLING
    case HFP_CALL_WAITING_ENABLE_CFM :
            MAIN_DEBUG(("HS3 : HFP_CALL_WAITING_ENABLE_CFM_T [%c]\n", 
                            TRUE_OR_FALSE(((const HFP_CALL_WAITING_ENABLE_CFM_T * )message)->status == hfp_success)));
    break ;
    case HFP_CALL_WAITING_IND:
        {
            /* pass the indication to the multipoint handler which will determine if the call waiting tone needs
               to be played, this will depend upon whether the indication has come from the AG with
               the currently routed audio */
            mpHandleCallWaitingInd((HFP_CALL_WAITING_IND_T *)message);
        }
    break;

#endif
    case HFP_SUBSCRIBER_NUMBERS_CFM:
        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBERS_CFM [%c]\n" , 
                            TRUE_OR_FALSE(((const HFP_SUBSCRIBER_NUMBERS_CFM_T*)message)->status == hfp_success)));
    break ;
#ifdef DEBUG_MAIN
    case HFP_SUBSCRIBER_NUMBER_IND:
    {
        uint16 i=0;
        const HFP_SUBSCRIBER_NUMBER_IND_T *subs_ind = (const HFP_SUBSCRIBER_NUMBER_IND_T*)message;

        MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBER_IND [%d]\n" , subs_ind->service ));

        for (i=0;i < subs_ind->size_number ; i++)
        {
            MAIN_DEBUG(("%c", subs_ind->number[i]));
        }
        MAIN_DEBUG(("\n")) ;
    }
    break ;
#endif
    case HFP_CURRENT_CALLS_CFM:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_CFM [%c]\n", 
                        TRUE_OR_FALSE(((const HFP_CURRENT_CALLS_CFM_T*)message)->status == hfp_success)));
    break ;
    case HFP_CURRENT_CALLS_IND:
        MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_IND id[%d] mult[%d] status[%d]\n" ,
                                        ((const HFP_CURRENT_CALLS_IND_T*)message)->call_idx ,
                                        ((const HFP_CURRENT_CALLS_IND_T*)message)->multiparty  ,
                                        ((const HFP_CURRENT_CALLS_IND_T*)message)->status)) ;
    break;
    case HFP_AUDIO_CONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_IND\n")) ;
        audioHandleSyncConnectInd( (const HFP_AUDIO_CONNECT_IND_T *)message ) ;
    break ;
    case HFP_AUDIO_CONNECT_CFM:
        {
            const HFP_AUDIO_CONNECT_CFM_T *conn_cfm = (const HFP_AUDIO_CONNECT_CFM_T *)message;

            MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM[%x][%x][%s%s%s] r[%d]t[%d]\n", conn_cfm->status ,
                                                          (int)conn_cfm->audio_sink ,
                                                          ((conn_cfm->link_type == sync_link_sco) ? "SCO" : "" )      ,
                                                          ((conn_cfm->link_type == sync_link_esco) ? "eSCO" : "" )    ,
                                                          ((conn_cfm->link_type == sync_link_unknown) ? "unk?" : "" ) ,
                                                          (uint16)conn_cfm->rx_bandwidth ,
                                                          (uint16)conn_cfm->tx_bandwidth)) ;

        /* should the device receive a sco connect cfm in limbo state */
        if (stateManagerGetState() == deviceLimbo)
        {
            /* confirm that it connected successfully before disconnecting it */
                if (conn_cfm->status == hfp_audio_connect_no_hfp_link)
            {
                MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM in limbo state, disconnect it\n" ));
                    ConnectionSyncDisconnect(conn_cfm->audio_sink, hci_error_oetc_user);
            }
        }
        /* not in limbo state, process sco connect indication */
        else
        {
                audioHandleSyncConnectCfm(conn_cfm);
            }
        }
    break ;
    case HFP_AUDIO_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_DISCONNECT_IND [%x]\n", ((const HFP_AUDIO_DISCONNECT_IND_T *)message)->status)) ;
        audioHandleSyncDisconnectInd ((const HFP_AUDIO_DISCONNECT_IND_T *)message) ;
    break ;
    case HFP_SIGNAL_IND:
        MAIN_DEBUG(("HS: HFP_SIGNAL_IND [%d]\n", ((const HFP_SIGNAL_IND_T* )message)->signal )) ;
    break ;
    case HFP_ROAM_IND:
        MAIN_DEBUG(("HS: HFP_ROAM_IND [%d]\n", ((const HFP_ROAM_IND_T* )message)->roam )) ;
    break;
    case HFP_BATTCHG_IND:
        MAIN_DEBUG(("HS: HFP_BATTCHG_IND [%d]\n", ((const HFP_BATTCHG_IND_T* )message)->battchg )) ;
    break;

/*******************************************************************/

    case HFP_CSR_FEATURES_TEXT_IND:
        csr2csrHandleTxtInd () ;
    break ;

    case HFP_CSR_FEATURES_NEW_SMS_IND:
       csr2csrHandleSmsInd () ;
    break ;

    case HFP_CSR_FEATURES_GET_SMS_CFM:
       csr2csrHandleSmsCfm() ;
    break ;

    case HFP_CSR_FEATURES_BATTERY_LEVEL_REQUEST_IND:
       csr2csrHandleAgBatteryRequestInd() ;
    break ;

/*******************************************************************/

/*******************************************************************/

    /*******************************/

    default :
        MAIN_DEBUG(("HS :  HFP ? [%x]\n",id)) ;
    break ;
    }
}
/* Handle any audio plugin messages */
static void handleAudioPluginMessage( Task task, MessageId id, Message message )
{

    UNUSED(task);

    switch (id)
    {
        case AUDIO_PLUGIN_DSP_IND:
        {
            const AUDIO_PLUGIN_DSP_IND_T *dspind = (const AUDIO_PLUGIN_DSP_IND_T*)message;
            /* Clock mismatch rate, sent from the DSP via the a2dp decoder common plugin? */
            if (dspind->id == KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE)
            {
                handleA2DPStoreClockMismatchRate(dspind->value[0]);
            }
            /* Current EQ bank, sent from the DSP via the a2dp decoder common plugin? */
            else if (dspind->id == A2DP_MUSIC_MSG_CUR_EQ_BANK)
            {
                handleA2DPStoreCurrentEqBank(dspind->value[0]);
            }
            /* Current enhancements, sent from the DSP via the a2dp decoder common plugin? */
            else if (dspind->id == A2DP_MUSIC_MSG_ENHANCEMENTS)
            {
                handleA2DPStoreEnhancements(dspind->value[1]);
            }
            }
        break;

        /* indication that the DSP is ready to accept data ensuring no audio samples are disposed of */
        case AUDIO_PLUGIN_DSP_READY_FOR_DATA:
            /* ensure dsp is up and running */

#if defined ENABLE_GAIA && defined ENABLE_GAIA_PERSISTENT_USER_EQ_BANK
            handleA2DPUserEqBankUpdate();
#endif

            if(((const AUDIO_PLUGIN_DSP_READY_FOR_DATA_T*)message)->dsp_status == DSP_RUNNING)
            {
                MAIN_DEBUG(("HS :  DSP ready for data\n")) ;
#ifdef ENABLE_PEER
                /*Request the connected peer device to send its current user EQ settings across if its a peer source.*/
                peerRequestUserEqSetings();
#endif
            }

#ifdef ENABLE_SUBWOOFER
            /* configure the subwoofer type when the dsp is up and running */
            if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_STANDARD)
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_L2CAP, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
            else if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_LOW_LATENCY)
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_ESCO, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
#endif
        break;

#ifdef ENABLE_GAIA

        case AUDIO_PLUGIN_DSP_GAIA_EQ_MSG:
        {
            uint8 payload[4];
            payload[0] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[0]) >> 8;
            payload[1] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[0]) & 0x00ff;
            payload[2] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[1]) >> 8;
            payload[3] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[1]) & 0x00ff;
            gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_USER_EQ_PARAMETER, GAIA_STATUS_SUCCESS, 4, payload);
        }
        break;

        case AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG:
        {
            AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG_T *gaiaMsg = (AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG_T *)message;

                gaia_send_response_16(GAIA_COMMAND_GET_USER_EQ_GROUP_PARAMETER,
                                  GAIA_STATUS_SUCCESS,
                                  gaiaMsg->size_value,
                                  gaiaMsg->value);
        }
        break;
#endif    /* ENABLE_GAIA */

		case AUDIO_PLUGIN_LATENCY_REPORT:
        {
            const AUDIO_PLUGIN_LATENCY_REPORT_T *report = (const AUDIO_PLUGIN_LATENCY_REPORT_T *)message;

            handleA2DPLatencyReport(report->audio_plugin, report->estimated, report->latency);
        }
		break;

        case AUDIO_PLUGIN_REFRESH_VOLUME:
        {
            MAIN_DEBUG(("HS :  AUDIO Refresh volume\n")) ;
            /* Refresh the volume and mute status for the routed audio */
            VolumeUpdateRoutedAudioMainAndAuxVolume();
            VolumeApplySoftMuteStates();
        }
        break;

        case AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG:
        {
            bool stream_detected;

            if (stateManagerGetState() != deviceLimbo)
            {
                stream_detected = ((AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG_T*)message)->signal_detected;

                if(stream_detected)
                {
                    /* Music started streaming so reset the silence detection flag. */
                    theSink.silence_detected = 0;
                }
                else
                {
                    /* Music is not streaming so set the silence detection flag. */
                    theSink.silence_detected = 1;
                }
                PioDrivePio(PIO_AUDIO_ACTIVE, stream_detected);
            }
        }
        break;

        case AUDIO_PLUGIN_AUDIO_PROMPT_DONE:
            MAIN_DEBUG(("HS : Audio prompt done\n")) ;
            break;
            
        default:
            MAIN_DEBUG(("HS :  AUDIO ? [%x]\n",id)) ;
        break ;
    }
}

#ifdef ENABLE_DISPLAY
/* Handle any display plugin messages */
static void handleDisplayPluginMessage( Task task, MessageId id, Message message )
{
    UNUSED(task);
    switch (id)
    {
    case DISPLAY_PLUGIN_INIT_IND:
        {
            DISPLAY_PLUGIN_INIT_IND_T *m = (DISPLAY_PLUGIN_INIT_IND_T *) message;
            MAIN_DEBUG(("HS :  DISPLAY INIT: %u\n", m->result));
            if (m->result)
            {
                if (powerManagerIsChargerConnected() && (stateManagerGetState() == deviceLimbo) )
                {
                    /* indicate charging if in limbo */
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_CHARGING,  strlen(DISPLAYSTR_CHARGING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
                    displayUpdateVolume(0);
                    displayUpdateBatteryLevel(TRUE);
                }
                else if (stateManagerGetState() != deviceLimbo)
                {
                    /* if this init occurs and not in limbo, turn the display on */
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_HELLO,  strlen(DISPLAYSTR_HELLO), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 10);
                    displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * theSink.features.DefaultVolume)/sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main));
                    /* update battery display */
                    displayUpdateBatteryLevel(FALSE);
                }
            }
        }
        break;

    default:
        MAIN_DEBUG(("HS :  DISPLAY ? [%x]\n",id)) ;
        break ;
    }
}
#endif /* ENABLE_DISPLAY */

/*************************************************************************
NAME
    app_handler

DESCRIPTION
    This is the main message handler for the Sink Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
void app_handler(Task task, MessageId id, Message message)
{
/*    MAIN_DEBUG(("MSG [%x][%x][%x]\n", (int)task , (int)id , (int)&message)) ;*/

    /* determine the message type based on base and offset */
    if ( ( id >= EVENTS_MESSAGE_BASE ) && ( id < EVENTS_LAST_EVENT ) )
    {
        handleUEMessage(task, id,  message);
    }
#ifdef MESSAGE_EXE_FS_VALIDATION_STATUS
    else if (id == MESSAGE_EXE_FS_VALIDATION_STATUS)
    {
        sinkUpgradeMsgHandler(task, id, message);
    }
#endif
    else  if ( (id >= CL_MESSAGE_BASE) && (id < CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_connection(task, id, message);
    #endif
    }
#if defined(ENABLE_ADK_NFC)
    else if (( id >=NFC_MESSAGE_BASE) && (id < NFC_MESSAGE_TOP) )
    {
        handleNfcClMessage(task, id,  message);
    }
#endif /* ENABLE_ADK_NFC*/
    else if ( (id >= HFP_MESSAGE_BASE ) && (id < HFP_MESSAGE_TOP) )
    {
        handleHFPMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_hfp(task, id, message);
    #endif
    }
    else if ( (id >= POWER_MESSAGE_BASE ) && (id < POWER_MESSAGE_TOP) )
    {
        handlePowerMessage (task, id, message) ;
        sinkUpgradePowerEventHandler();
    }
#ifdef ENABLE_PBAP
    else if ( ((id >= PBAPC_MESSAGE_BASE ) && (id < PBAPC_MESSAGE_TOP)) ||
              ((id >= PBAPC_APP_MSG_BASE ) && (id < PBAPC_APP_MSG_TOP)) )
    {
        handlePbapMessages (task, id, message) ;
    }
#endif
#ifdef ENABLE_MAPC
    else if ( ((id >= MAPC_MESSAGE_BASE )    && (id < MAPC_API_MESSAGE_END)) ||
              ((id >= MAPC_APP_MESSAGE_BASE) && (id < MAPC_APP_MESSAGE_TOP)) )
    {
        handleMapcMessages (task, id, message) ;
    }
#endif
#ifdef ENABLE_AVRCP
    else if ( (id >= AVRCP_INIT_CFM ) && (id < SINK_AVRCP_MESSAGE_TOP) )
    {
        sinkAvrcpHandleMessage (task, id, message) ;
    #ifdef TEST_HARNESS
        vm2host_avrcp(task, id, message);
    #endif
    }
#endif

#ifdef CVC_PRODTEST
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        cvcProductionTestKalimbaMessage (task, id, message);
    }
#endif
    else if ( (id >= A2DP_MESSAGE_BASE ) && (id < A2DP_MESSAGE_TOP) )
    {
        handleA2DPMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_a2dp(task, id, message);
    #endif
        return;
    }
    else if ( (id >= AUDIO_UPSTREAM_MESSAGE_BASE ) && (id < AUDIO_UPSTREAM_MESSAGE_TOP) )
    {
        handleAudioPluginMessage(task, id,  message);
    }
#ifdef ENABLE_USB
    else if( ((id >= MESSAGE_USB_ENUMERATED) && (id <= MESSAGE_USB_SUSPENDED)) ||
             ((id >= MESSAGE_USB_DECONFIGURED) && (id <= MESSAGE_USB_DETACHED)) ||
             ((id >= USB_DEVICE_CLASS_MSG_BASE) && (id < USB_DEVICE_CLASS_MSG_TOP)) )
    {
        handleUsbMessage(task, id, message);
        return;
    }
#endif /* ENABLE_USB */
#ifdef ENABLE_GAIA
    else if ((id >= GAIA_MESSAGE_BASE) && (id < GAIA_MESSAGE_TOP))
    {
        handleGaiaMessage(task, id, message);
    }
#endif
    else if (id == MESSAGE_DFU_SQIF_STATUS)
    {
        /* tell interested modules the status of a DFU from SQIF operation */
        sinkUpgradeMsgHandler(task, id, message);
    }
#ifdef ENABLE_DISPLAY
    else if ( (id >= DISPLAY_UPSTREAM_MESSAGE_BASE ) && (id < DISPLAY_UPSTREAM_MESSAGE_TOP) )
    {
        handleDisplayPluginMessage(task, id,  message);
    }
#endif   /* ENABLE_DISPLAY */
#ifdef ENABLE_SUBWOOFER
    else if ( (id >= SWAT_MESSAGE_BASE) && (id < SWAT_MESSAGE_TOP) )
    {
        handleSwatMessage(task, id, message);
    }
#endif /* ENABLE_SUBWOOFER */
#ifdef ENABLE_FM
    else if ( (id >= FM_UPSTREAM_MESSAGE_BASE ) && (id < FM_UPSTREAM_MESSAGE_TOP) )
    {
        sinkFmHandleFmPluginMessage(id, message);
    }
#endif /* ENABLE_FM*/
    else if (sinkUpgradeIsUpgradeMsg(id))
    {
        sinkUpgradeMsgHandler(task, id, message);
    }
    else
    {
        MAIN_DEBUG(("MSGTYPE ? [%x]\n", id)) ;
    }
}

/* Time critical initialisation */
#ifdef HOSTED_TEST_ENVIRONMENT
void _sink_init(void)
#else
void _init(void)
#endif
{
    /* Set the application task */
    theSink.task.handler = app_handler;

    /* set flag to indicate that configuration is being read, use to prevent use of variables
       prior to completion of initialisation */
    theSink.SinkInitialising = TRUE;
    
    /* Read in any PIOs required */
    configManagerPioMap();
    
    /* Assert audio amplifier mute if confugured */
    PioDrivePio(PIO_AMP_MUTE, TRUE);

    _custom_init();
    _custom_deinit();
    custom_uart_init(&theSink.task);
    
    /* Time critical USB setup */
#ifdef ENABLE_USB
    usbTimeCriticalInit();
#endif /* ENABLE_USB */
}

/*************************************************************************
NAME
    sinkConnectionInit

DESCRIPTION
    Initialise the Connection library
*/
static void sinkConnectionInit(void)
{
    /* read the lengths key into a temporary malloc to get pdl length */
    lengths_config_type * lengths_key = (lengths_config_type *)PanicUnlessMalloc(sizeof(lengths_config_type));

    /* The number of paired devices can be restricted using pskey user 40,  a number between 1 and 8 is allowed */
    ConfigRetrieve(CONFIG_LENGTHS, lengths_key , sizeof(lengths_config_type) );
    DEBUG (("PDLSize[%d]\n" , lengths_key->pdl_size ));

    /* Need to know if SC is enabled so configManagerInitFeatures() is moved in sinkConnectionInit */
    configManagerInitFeatures();

    /* Initialise the Connection library */
    ConnectionInitEx3(&theSink.task, NULL, lengths_key->pdl_size, SC_CONNECTION_LIB_OPTIONS);

    /* free the malloc'd memory */
    free(lengths_key);
}


/* The Sink Application starts here...*/
#ifdef HOSTED_TEST_ENVIRONMENT
int sink_main(void)
#else
int main(void)
#endif
{
#ifdef HYDRACORE_TODO
    OsInit();

#ifdef WAIT_FOR_HERACLES_ON_START
    wait_for_heracles();
#endif

    _init();

#else

    DEBUG (("Main [%s]\n",__TIME__));

#endif /* HYDRACORE_TODO */

    /* Certain chips require a HUB attach to enumerate as a USB device. */
    usbAttachToHub();

    /* Initialise the Upgrade lib */
    sinkUpgradeInit(&theSink.task);

    /* check and update as necessary the software version pskey, this is used
       for ensuring maximum compatibility with the sink configuration tool */
    configManagerSetVersionNo();

    /* Initialise memory required early */
    configManagerInitMemory();

    /* initialise memory for the led manager */
    LedManagerMemoryInit();

    /* Initialise device state */
    AuthResetConfirmationFlags();

    /*the internal regs must be latched on (smps and LDO)*/
    PioSetPowerPin ( TRUE ) ;

    switch (BootGetMode() )
    {
#ifdef CVC_PRODTEST
        case BOOTMODE_CVC_PRODTEST:
            /*run the cvc prod test code and dont start the applicaiton */
            cvcProductionTestEnter() ;
        break ;
#endif
        case BOOTMODE_DFU:
            /*do nothing special for the DFU boot mode,
            This mode expects to have the appropriate host interfface enabled
            Don't start the application */

            /* Initializing only the system components required for flashing the led pattern in the DFU mode*/
            configManagerInit(FALSE);
            LEDManagerIndicateEvent(EventUsrEnterDFUMode);
        break ;

        case BOOTMODE_DEFAULT:
        case BOOTMODE_CUSTOM:
        case BOOTMODE_USB_LOW_POWER:
        case BOOTMODE_ALT_FSTAB:
        default:
        {
            /* Initialise the Connection lib */
            sinkConnectionInit();

            #ifdef TEST_HARNESS
                test_init();
            #endif
        }
        break ;
    }

    /* Make sure the mute states are correctly set up */
    VolumeSetInitialMuteState();

    /* Start protection mechanism for buffer defences */
    watchdogReset();

    /* Start the message scheduler loop */
    MessageLoop();

    /* Never get here...*/
    return 0;
}


/*************************************************************************

NAME
    handleHFPStatusCFM

DESCRIPTION
    Handles a status response from the HFP and sends an error message if one was received

RETURNS

*/
static void handleHFPStatusCFM ( hfp_lib_status pStatus )
{
    if (pStatus != hfp_success )
    {
        MAIN_DEBUG(("HS: HFP CFM Err [%d]\n" , pStatus)) ;
        MessageSend ( &theSink.task , EventSysError , 0 ) ;
#ifdef ENABLE_PBAP
        if(theSink.pbapc_data.pbap_command == pbapc_dialling)
        {
            MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
        }
#endif
    }
    else
    {
         MAIN_DEBUG(("HS: HFP CFM Success [%d]\n" , pStatus)) ;
    }

#ifdef ENABLE_PBAP
    theSink.pbapc_data.pbap_command = pbapc_action_idle;
#endif
}

/*************************************************************************
NAME
    IndicateEvent

DESCRIPTION
    Passes the msg Id to the relevant indication informers.

RETURNS None

*/
static void IndicateEvent(MessageId id)
{        
    if (id != EventSysLEDEventComplete)
    {
        LEDManagerIndicateEvent(id);
    }

    sinkAudioIndicationPlayEvent(id);
    ATCommandPlayEvent(id) ;
}



#ifdef HYDRACORE_TODO
#ifdef WAIT_FOR_HERACLES_ON_START
/* Bodge : temporary function to delay the start of P1 code.*/
unsigned volatile rambleon=0x8000000;
static void wait_for_heracles(void)
    {
        while (--rambleon)
            ;
        rambleon=0x10000000;
    }
#endif /* WAIT_FOR_HERACLES_ON_START */

#endif /*HYDRACORE_TODO*/

/**  \} */ /* End sink_app group */

