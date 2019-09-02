/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/
/**
\file
\ingroup sink_app
\brief
    This file contains the Authentication functionality for the Sink 
    Application
*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_audio_prompts.h"
#include "sink_statemanager.h"
#include "sink_auth.h"

#include "sink_devicemanager.h"
#include "sink_debug.h"
#include "sink_ble_gap.h"
#include "sink_audio_indication.h"
#include "sink_sc.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <ps.h>
#include <bdaddr.h>
#include <stdlib.h>
#include <sink.h>

#ifdef DEBUG_AUTH
    #define AUTH_DEBUG(x) DEBUG(x)    
#else
    #define AUTH_DEBUG(x) 
#endif   

/* In case of RRA enabled devices  the key distribution is as follows
    when we are initiator we require the responder to distribute its LTK, EDIV, RAND and IRK
    when we are responder  we require the initiator to distribute its LTK, EDIV, RAND and IRK.
*/
#define KEY_DISTRIBUTION_RANDOM (KEY_DIST_RESPONDER_ENC_CENTRAL | KEY_DIST_RESPONDER_ID | KEY_DIST_INITIATOR_ENC_CENTRAL | KEY_DIST_INITIATOR_ID)
#define KEY_DISTRIBUTION_PUBLIC  (KEY_DIST_RESPONDER_ENC_CENTRAL | KEY_DIST_INITIATOR_ENC_CENTRAL)

#ifdef ENABLE_PEER

    /* PERSISTENT STORE KEY
     *
     * Persistent store keys are held in terms of uint16's
     *
     * sizeof() works on the "byte" type of the processor, which was
     * also a uint16 on the CSR XAP processor.
     *
     * Other processors will have a different allocation size
     */
#define BD_ADDR_SIZE (PS_SIZE_ADJ(sizeof(bdaddr)))
#define LINK_KEY_SIZE 8
#define ATTRIBUTES_SIZE (PS_SIZE_ADJ(sizeof(sink_attributes)))
#define STATUS_LOC 0
#define BD_ADDR_LOC 1
#define LINK_KEY_LOC (BD_ADDR_LOC+BD_ADDR_SIZE)
#define ATTRIBUTES_LOC (LINK_KEY_LOC+LINK_KEY_SIZE)
#define PERMPAIR_PS_SIZE (BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1)


static void readPsPermanentPairing (bdaddr *bd_addr, uint16 *link_key, uint16 *link_key_status, sink_attributes *attributes)
{
    uint16 * ps_key;

    /* Allocate and zero buffer to hold PS key */
    ps_key = (uint16*)callocDebugPanic(PERMPAIR_PS_SIZE,sizeof(uint16));

    /* Attempt to obtain current pairing data */
    PsRetrieve(CONFIG_PERMANENT_PAIRING, ps_key, PERMPAIR_PS_SIZE);

    /* Return any requested fields */
    if (link_key_status)
    {
        *link_key_status = ps_key[STATUS_LOC];
    }

    if (bd_addr)
    {
        memcpy(bd_addr, &ps_key[BD_ADDR_LOC], sizeof(*bd_addr));
    }

    if (link_key)
    {
        memcpy(link_key, &ps_key[LINK_KEY_LOC], LINK_KEY_SIZE * sizeof(uint16));
    }

    if (attributes)
    {
        memcpy(attributes, &ps_key[ATTRIBUTES_LOC], sizeof(*attributes));
    }
    
#ifdef DEBUG_DEV
    {
        bdaddr *perm_addr = (bdaddr *) &ps_key[BD_ADDR_LOC];
        sink_attributes *perm_attributes = (sink_attributes *) &ps_key[ATTRIBUTES_LOC];

        DEBUG(("DEV: perm read %04X %02X %06lX prof:0x%02X route:%u,%u\n",
               perm_addr->nap,
               perm_addr->uap,
               perm_addr->lap,
               perm_attributes->profiles,
               perm_attributes->master_routing_mode,
               perm_attributes->slave_routing_mode));
    }
#endif

    free(ps_key);
}

static void writePsPermanentPairing (const bdaddr *bd_addr, const uint16 *link_key, uint16 link_key_status, const sink_attributes *attributes)
{
    uint16 * ps_key;

    /* Allocate and zero buffer to hold PS key */
    ps_key = (uint16*)callocDebugPanic(PERMPAIR_PS_SIZE,sizeof(uint16));

    /* Attempt to obtain current pairing data */
    PsRetrieve(CONFIG_PERMANENT_PAIRING, ps_key, PERMPAIR_PS_SIZE);
    
    /* Update supplied fields */
    if (link_key_status)
    {
        ps_key[STATUS_LOC] = link_key_status;
    }

    if (bd_addr)
    {
        memcpy(&ps_key[BD_ADDR_LOC], bd_addr, sizeof(*bd_addr));
    }

    if (link_key)
    {
        memcpy(&ps_key[LINK_KEY_LOC], link_key, LINK_KEY_SIZE * sizeof(uint16));
    }

    if (attributes)
    {
        memcpy(&ps_key[ATTRIBUTES_LOC], attributes, sizeof(*attributes));
    }

#ifdef DEBUG_DEV
    {
        bdaddr *perm_addr = (bdaddr *) &ps_key[BD_ADDR_LOC];
        sink_attributes *perm_attributes = (sink_attributes *) &ps_key[ATTRIBUTES_LOC];

        DEBUG(("DEV: perm write %04X %02X %06lX prof:0x%02X route:%u,%u\n",
               perm_addr->nap,
               perm_addr->uap,
               perm_addr->lap,
               perm_attributes->profiles,
               perm_attributes->master_routing_mode,
               perm_attributes->slave_routing_mode));
    }
#endif

    /* Store updated pairing data */
    PsStore(CONFIG_PERMANENT_PAIRING, ps_key, PERMPAIR_PS_SIZE);

    free(ps_key);
}

/****************************************************************************
NAME    
    AuthInitPermanentPairing
    
DESCRIPTION
    Add devices stored in CONFIG_PERMANENT_PAIRING to Connection library's PDL
    
RETURNS
    void
*/
void AuthInitPermanentPairing (void)
{
    bdaddr ps_bd_addr;
    
    AUTH_DEBUG(("AuthInitPermanentPairing\n"));
    
    /* Obtain just the bluetooth address of a permanently paired device */
    readPsPermanentPairing(&ps_bd_addr, 0, 0, 0);
    AUTH_DEBUG(("   ps bd_addr = [%x:%x:%lx]\n", ps_bd_addr.uap, ps_bd_addr.nap, ps_bd_addr.lap));
    
    if ( !BdaddrIsZero(&ps_bd_addr) )
    {   /* A valid address has been obtained, ask CL for info on it */
        ConnectionSmGetAuthDevice(&theSink.task, (const bdaddr *)&ps_bd_addr);
    }
}

/****************************************************************************
NAME    
    AuthRemovePermanentPairing
    
DESCRIPTION
    Removes permanent paired device from Connection library's PDL.
    Will also erase CONFIG_PERMANENT_PAIRING if erase_ps_key set TRUE.
    
RETURNS
    void
*/
void AuthRemovePermanentPairing (bool erase_ps_key)
{
    bdaddr ps_bd_addr;
    
    AUTH_DEBUG(("AuthRemovePermanentPairing  erase_ps_key = %u\n", erase_ps_key));
    
    readPsPermanentPairing(&ps_bd_addr, 0, 0, 0);
    AUTH_DEBUG(("   ps bd_addr = [%x:%x:%lx]\n", ps_bd_addr.uap, ps_bd_addr.nap, ps_bd_addr.lap));
    
    if ( !BdaddrIsZero(&ps_bd_addr) )
    {
        ConnectionSmDeleteAuthDeviceReq(TYPED_BDADDR_PUBLIC, (const bdaddr *)&ps_bd_addr);
    }
    
    if ( erase_ps_key )
    {
        PsStore(CONFIG_PERMANENT_PAIRING, 0, 0);
    }
}

/****************************************************************************
NAME    
    AuthUpdatePermanentPairing
    
DESCRIPTION
    Use supplied BDADDR to obtain linkkey from Connection library and update
    CONFIG_PERMANENT_PAIRING to retain this as the permanently paired device
    
RETURNS
    void
*/
void AuthUpdatePermanentPairing (const bdaddr *bd_addr, const sink_attributes *attributes)
{
    bdaddr ps_bdaddr;
    AUTH_DEBUG(("AuthUpdatePermanentPairing\n"));
    
    readPsPermanentPairing(&ps_bdaddr, 0, 0, 0);

    if(!BdaddrIsZero(&ps_bdaddr) && !BdaddrIsSame(&ps_bdaddr, bd_addr))
    {
        AuthRemovePermanentPairing(FALSE);
    }
    
    /* Update permanent pairing info */
    writePsPermanentPairing(0, 0, 0, attributes);
    
    ConnectionSmGetAuthDevice(&theSink.task, bd_addr);
}

/****************************************************************************
NAME    
    handleGetAuthDeviceCfm
    
DESCRIPTION
    Called in response to CL_SM_GET_AUTH_DEVICE_CFM message, which is generated
    due to calling updatePermanentPairing.
    Both the BDADDR and linkkey contained in CL_SM_GET_AUTH_DEVICE_CFM are used to
    update CONFIG_PERMANENT_PAIRING to retain this as the permanently paired device
    
RETURNS
    void
*/
void handleGetAuthDeviceCfm (const CL_SM_GET_AUTH_DEVICE_CFM_T *cfm)
{
    AUTH_DEBUG(("handleGetAuthDeviceCfm\n"));
    AUTH_DEBUG(("   status = %u\n",cfm->status));
    AUTH_DEBUG(("   ps bd_addr = [%x:%x:%lx]\n", cfm->bd_addr.uap, cfm->bd_addr.nap, cfm->bd_addr.lap));
    AUTH_DEBUG(("   trusted = %u\n",cfm->trusted));
    AUTH_DEBUG(("   link key type = %u",cfm->link_key_type));
    AUTH_DEBUG(("   link key size = %u\n",cfm->size_link_key));
    
    if ( cfm->status == success )
    {   /* Device exists in CL PDL */
        sink_attributes attributes;
        uint16 link_key_status = ((cfm->trusted & 0xF)<<8) | ((cfm->link_key_type & 0xF)<<4) | (cfm->size_link_key & 0xF);

        if (!deviceManagerGetAttributes(&attributes, &cfm->bd_addr))
        {
            bdaddr perm_bdaddr;

            /* No attributes in PDL, so check if attributes for this bdaddr
               are stored in the permanent pairing data.
               If not, revert to defaults. */
            readPsPermanentPairing(&perm_bdaddr, 0, 0, &attributes);
            if (BdaddrIsZero(&perm_bdaddr) || !BdaddrIsSame(&perm_bdaddr, &cfm->bd_addr))
            {
                deviceManagerGetDefaultAttributes(&attributes, dev_type_none);
            }
        }

        /* Update permanent pairing info */
        writePsPermanentPairing(&cfm->bd_addr, cfm->link_key, link_key_status, &attributes);
        
        /* Update attributes */
        deviceManagerStoreAttributes(&attributes, (const bdaddr *)&cfm->bd_addr);
        
        /* Mark the device as trusted and push it to the top of the PDL */
        ConnectionSmUpdateMruDevice((const bdaddr *)&cfm->bd_addr); 
        
        deviceManagerUpdatePriorityDevices();
    }
    else
    {   /* Device *does not* exist in CL PDL */ 
        bdaddr ps_bd_addr;
        uint16 ps_link_key_status;
        uint16 ps_link_key[LINK_KEY_SIZE];
    
        readPsPermanentPairing(&ps_bd_addr, ps_link_key, &ps_link_key_status, 0);
    
        if ( !BdaddrIsZero(&ps_bd_addr) )
        {   /* We have permanently paired device, add it to CL PDL */
            uint16 trusted = ((ps_link_key_status>>8) & 0xF);
            cl_sm_link_key_type key_type = (cl_sm_link_key_type)((ps_link_key_status>>4) & 0xF);
            uint16 size_link_key = ps_link_key_status & 0xF;
        
            ConnectionSmAddAuthDevice(&theSink.task, (const bdaddr *)&ps_bd_addr, trusted, TRUE, key_type, size_link_key, (const uint16 *)ps_link_key);
        }
    }
 }

/****************************************************************************
NAME    
    handleAddAuthDeviceCfm
    
DESCRIPTION
    Called in response to CL_SM_ADD_AUTH_DEVICE_CFM message, which is generated
    due to calling ConnectionSmAddAuthDevice.
    
RETURNS
    void
*/
void handleAddAuthDeviceCfm (const CL_SM_ADD_AUTH_DEVICE_CFM_T *cfm)
{
    if ( cfm->status == success )
    {   /* Ask for device info again to allow write of attribute data */
        ConnectionSmGetAuthDevice(&theSink.task, &cfm->bd_addr);  
    }
}
#endif  /* ENABLE_PEER */

/****************************************************************************
NAME    
    AuthCanSinkConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr );

/****************************************************************************
NAME    
    AuthCanSinkPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void ) ;

/*************************************************************************
NAME    
     sinkHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The Sink devices default pin code is sent back.

RETURNS
     
*/
void sinkHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind)
{
    uint16  pin_length = 0;
    uint16  pin[16];
    void   *pin_to_play = pin;
#ifdef HYDRACORE
    uint8   packed_pin[16];
#endif

    if ( AuthCanSinkPair() )
    {
	    
		AUTH_DEBUG(("auth: Can Pin\n")) ;
		
   		/* Do we have a fixed pin in PS, if not reject pairing */
    	if ((pin_length = PsFullRetrieve(PSKEY_FIXED_PIN, pin, 16)) == 0 || pin_length > 16)
   		{
   	    	/* Set length to 0 indicating we're rejecting the PIN request */
        	AUTH_DEBUG(("auth : failed to get pin\n")) ;
       		pin_length = 0; 
   		}	
        else if(theSink.features.VoicePromptPairing)
        {
#ifdef HYDRACORE
            /* The PSSTORE entry holds uint8's in a uint16. We need to pack */
            unsigned    i;

            for (i = 0; i < pin_length; i++)
            {
                packed_pin[i] = (uint8)pin[i];
            }
            pin_to_play = packed_pin;
#endif
            sinkAudioIndicationPlayEvent(EventSysPinCodeRequest);
            AudioPromptPlayNumString(pin_length, (uint8 *)pin_to_play);
        }
    } 

    /* Respond to the PIN code request */
    ConnectionSmPinCodeResponse(&ind->taddr, (uint8)pin_length, (uint8*)pin_to_play); 
}

/*************************************************************************
NAME    
     sinkHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void sinkHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind)
{
    bool can_pair = FALSE;
    if(ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
    {
        can_pair = AuthCanSinkPair();
    }
    else if(ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        can_pair = sinkBleGapIsBondable();
    }
	/* Can we pair? */
	if ( can_pair && theSink.features.ManInTheMiddle)
    {
        theSink.confirmation = TRUE;
        AUTH_DEBUG(("auth: Can Confirm %ld\n",ind->numeric_value)) ;
        /* Should use text to speech here */
        theSink.confirmation_addr  = newDebugPanic(tp_bdaddr);
        *theSink.confirmation_addr = ind->tpaddr;
        if(theSink.features.VoicePromptPairing)
        {
            sinkAudioIndicationPlayEvent(EventSysConfirmationRequest);
            AudioPromptPlayNumber(ind->numeric_value);
        }
	}
	else
    {
		/* Reject the Confirmation request */
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(&ind->tpaddr, FALSE);
    }
}

/*************************************************************************
NAME    
     sinkHandleUserPasskeyInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS
     
*/
void sinkHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind)
{
	/* Reject the Passkey request */
	AUTH_DEBUG(("auth: Rejected Passkey Req\n")) ;
	ConnectionSmUserPasskeyResponse(&ind->tpaddr, TRUE, 0);
}


/*************************************************************************
NAME    
     sinkHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     
*/
void sinkHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind)
{
	AUTH_DEBUG(("Passkey: %ld \n", ind->passkey));
    if(theSink.features.ManInTheMiddle && theSink.features.VoicePromptPairing)
    {
        sinkAudioIndicationPlayEvent(EventSysPasskeyDisplay);
        AudioPromptPlayNumber(ind->passkey);
    }
	/* Should use text to speech here */
}

/*************************************************************************
NAME    
     sinkHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     
*/
void sinkHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind)
{	
    uint16 key_dist = KEY_DIST_NONE;
    bool can_pair = FALSE;
    const tp_bdaddr* remote_bdaddr = &ind->tpaddr;

    /* Check if this is for a BR/EDR device */
    if((remote_bdaddr->transport == TRANSPORT_BREDR_ACL) && (AuthCanSinkPair()))
    {
        can_pair = TRUE;
    }

    /* Check if this is for a LE device */
    if ((remote_bdaddr->transport == TRANSPORT_BLE_ACL) && ((sinkBleGapIsBondable() || ind->sm_over_bredr)))
    {
        can_pair = TRUE;
        if(ind->sm_over_bredr)
        {
            /* If the LE pairing request is sent over BR/EDR link then we need to add  KEY_DIST_RESPONDER_ID & KEY_DIST_INITIATOR_ID 
            * for the Bluestack to generate IRK and register remote bd address with the Bluestack Security Manager. This is required as later
            * when gatt connection over LE link is created bluestack will be able to resolve the public address from the random address.
            */
            key_dist = KEY_DISTRIBUTION_RANDOM;
        }
        else
        {
           key_dist = (remote_bdaddr->taddr.type == TYPED_BDADDR_RANDOM) ? 
                        KEY_DISTRIBUTION_RANDOM : KEY_DISTRIBUTION_PUBLIC;
	    }
    }
    
    /* If not pairable should reject */
    if(can_pair)
    {
        cl_sm_io_capability local_io_capability = theSink.features.ManInTheMiddle ? cl_sm_io_cap_display_yes_no : cl_sm_io_cap_no_input_no_output;
        mitm_setting sink_mitm_setting = theSink.features.ManInTheMiddle ? mitm_required : mitm_not_required;

        AUTH_DEBUG(("auth: Sending IO Capability \n"));

        /* Send Response and request to bond with device */
        ConnectionSmIoCapabilityResponse(&ind->tpaddr, local_io_capability, sink_mitm_setting, TRUE, key_dist, oob_data_none, NULL, NULL);
    }
    else
    {
        AUTH_DEBUG(("auth: Rejecting IO Capability Req \n"));
        ConnectionSmIoCapabilityResponse(&ind->tpaddr, cl_sm_reject_request, mitm_not_required, FALSE, key_dist, oob_data_none, NULL, NULL);
    }
}

/*************************************************************************
NAME    
     sinkHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS
     
*/
void sinkHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind)
{
    UNUSED(ind);

    AUTH_DEBUG(("auth: Incoming Authentication Request\n"));
}

/****************************************************************************
NAME    
    sinkHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void sinkHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind)
{
	
	bool lAuthorised = FALSE ;
	
	if ( AuthCanSinkConnect(&ind->bd_addr) )
	{
		lAuthorised = TRUE ;
	}
	
	AUTH_DEBUG(("auth: Authorised [%d]\n" , lAuthorised)) ;
	    
	/*complete the authentication with the authorised or not flag*/
    ConnectionSmAuthoriseResponse(&ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming, lAuthorised);
}


/****************************************************************************
NAME    
    sinkHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void sinkHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm)
{
#ifdef ENABLE_SUBWOOFER
    if (theSink.inquiry.action == rssi_subwoofer)
    {
        if ((cfm->status == auth_status_success) && (cfm->bonded))
        {           
            /* Mark the subwoofer as a trusted device */
            deviceManagerMarkTrusted(&cfm->bd_addr);

            /* Store the subwoofers BDADDR to PS */
            configManagerWriteSubwooferBdaddr(&cfm->bd_addr);

            /* Setup some default attributes for the subwoofer */
            deviceManagerStoreDefaultAttributes(&cfm->bd_addr, dev_type_sub);

            /* mark the subwoofer device as DO NOT DELETE in PDL */
            ConnectionAuthSetPriorityDevice(&cfm->bd_addr, TRUE);
        }
        return;
    }
#endif
	/* Leave bondable mode if successful unless we got a debug key */
	if (cfm->status == auth_status_success && cfm->key_type != cl_sm_link_key_debug)
    {
        if ((theSink.inquiry.action != rssi_pairing) || (theSink.inquiry.session != inquiry_session_normal))
        {
            /* Mark the device as trusted */
            deviceManagerMarkTrusted(&cfm->bd_addr);
            MessageSend (&theSink.task , EventSysPairingSuccessful , 0 );
        }
    }
    
	/* Set up some default params and shuffle PDL */
	if(cfm->bonded)
	{
        sink_attributes attributes;
        
        deviceManagerClearAttributes(&attributes);
        if(!deviceManagerGetAttributes(&attributes, &cfm->bd_addr))
        {
            deviceManagerGetDefaultAttributes(&attributes, dev_type_ag);
        }
        else
        {
            deviceManagerUpdateAttributesWithDeviceDefaults(&attributes, dev_type_ag);
        }

        /* Check if the key_type generated is p256. If yes then set the
        * attribute.mode to sink_mode_unknown. Once the encryption type is known in
        * CL_SM_ENCRYPTION_CHANGE_IND or  CL_SM_ENCRYPT_CFM message,device
        * attributes will be updated accordingly with proper mode.
        * Update the device attributes with this information to be reused later.
        */
        if((theSink.features.SecureConnectionMode > 0)  && 
               ((cfm->key_type == cl_sm_link_key_unauthenticated_p256) ||
               (cfm->key_type == cl_sm_link_key_authenticated_p256)))
        {
            attributes.mode = sink_mode_unknown;
        }
        else
        {
            attributes.mode = sink_no_secure_connection;
        }

        deviceManagerStoreAttributes(&attributes, &cfm->bd_addr);

        ConnectionAuthSetPriorityDevice((const bdaddr *)&cfm->bd_addr, FALSE);
	}
	
	/* Reset pairing info if we timed out on confirmation */
	AuthResetConfirmationFlags();
}


/****************************************************************************
NAME    
    AuthCanSinkPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void )
{
	bool lCanPair = FALSE ;
	
    if (theSink.features.SecurePairing)
    {
	    	/*if we are in pairing mode*/
		if ((stateManagerGetState() == deviceConnDiscoverable)||(theSink.inquiry.action == rssi_subwoofer))
		{
			lCanPair = TRUE ;
			AUTH_DEBUG(("auth: is ConnDisco\n")) ;
		}
#ifdef ENABLE_PARTYMODE
        else if(theSink.PartyModeEnabled)
        {
			lCanPair = TRUE ;
			AUTH_DEBUG(("auth: allow PartyMode pairing\n")) ;
        }
#endif        
    }
    else
    {
	    lCanPair = TRUE ;
    }
    
    return lCanPair ;
}



/****************************************************************************
NAME    
    AuthCanSinkConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr )
{
	bool lCanConnect = FALSE ;
    uint8 NoOfDevices = deviceManagerNumConnectedDevs();
    
    /* if device is already connected via a different profile allow this next profile to connect */
    if(deviceManagerProfilesConnected(bd_addr))
    {
    	AUTH_DEBUG(("auth: already connected, CanConnect = TRUE\n")) ;
        lCanConnect = TRUE;
    }
    /* this bdaddr is not already connected, therefore this is a new device, ensure it is allowed 
       to connect, if not reject it */
    else
    {
        /* when multipoint is turned off, only allow one device to connect */
        if(((!theSink.MultipointEnable)&&(!NoOfDevices))||
           ((theSink.MultipointEnable)&&(NoOfDevices < MAX_MULTIPOINT_CONNECTIONS)))
        {
            /* is secure pairing enabled? */
            if (theSink.features.SecurePairing)
            {
    	        /* If page scan is enabled (i.e. we are either connectable/discoverable or 
    	    	connected in multi point) */
    	    	if ( theSink.page_scan_enabled )
    	    	{
    	    		lCanConnect = TRUE ;
    	    		AUTH_DEBUG(("auth: is connectable\n")) ;
    	    	}		
            }
            /* no secure pairing */
            else
            {
            	AUTH_DEBUG(("auth: MP CanConnect = TRUE\n")) ;
    	        lCanConnect = TRUE ;
            }
        }
    }
  
    AUTH_DEBUG(("auth:  CanConnect = %d\n",lCanConnect)) ;
  
    return lCanConnect ;
}

/****************************************************************************
NAME    
    sinkPairingAcceptRes 
    
DESCRIPTION
    Respond correctly to a pairing info request ind

RETURNS
    void
*/
void sinkPairingAcceptRes( void )
{		
    if(AuthCanSinkPair() && theSink.confirmation)
	{
		AUTH_DEBUG(("auth: Accepted Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, TRUE);
     }
	else
     {
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
     }
}

/****************************************************************************
NAME    
    sinkPairingRejectRes 
    
DESCRIPTION
    Respond reject to a pairing info request ind

RETURNS
    void
*/
void sinkPairingRejectRes( void )
{
	if(AuthCanSinkPair() && theSink.confirmation)
	{	
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, FALSE);
	}
	else
	{
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
	}
}

/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS
     
*/

void AuthResetConfirmationFlags ( void )
{
	AUTH_DEBUG(("auth: Reset Confirmation Flags\n"));
	if(theSink.confirmation)
	{
		AUTH_DEBUG(("auth: Free Confirmation Addr\n"));
		freePanic(theSink.confirmation_addr);
	}
	theSink.confirmation_addr = NULL;
	theSink.confirmation = FALSE;
}

