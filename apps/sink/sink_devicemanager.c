/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   
    Handles connections to devices over all profiles and storage of device
    related data in PS.

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_devicemanager.h"
#include "sink_configmanager.h"
#include "sink_a2dp.h"
#include "sink_slc.h"
#include "sink_swat.h"
#include "sink_ble.h"
#include "sink_accessory.h"

#include <ps.h>
#include <sink.h>
#include <bdaddr.h>
#include <string.h>

#ifdef DEBUG_DEV
    #define DEV_DEBUG(x) DEBUG(x)
#else
    #define DEV_DEBUG(x) 
#endif   

#define PDI_ENTRY(pdi, x) (((x >= 4) ? (pdi[1] >> ((x-4) * 4)) : (pdi[0] >> (x*4))) & 0xf)
#define PDI_ENTRY_UNUSED  (0xf)

  
/****************************************************************************
NAME    
    deviceManagerUpdateAttributesWithDeviceDefaults
    
DESCRIPTION
    Updates the attributes with the defaults for a specific device type.

RETURNS
    void
*/
void deviceManagerUpdateAttributesWithDeviceDefaults(sink_attributes* attributes, sink_remote_device_type device_type)
{
#ifdef ENABLE_PEER
    attributes->master_routing_mode = theSink.features.TwsMasterAudioRouting;
    attributes->slave_routing_mode = theSink.features.TwsSlaveAudioRouting;
    attributes->peer_device = remote_device_unknown;
    attributes->peer_features = remote_features_none;
    attributes->peer_version = 0;
#endif
    
    if (device_type == dev_type_sub)
    {
#ifdef ENABLE_SUBWOOFER
        attributes->profiles = sink_swat;
        attributes->hfp.volume = 0;
        attributes->a2dp.volume = 0;
        attributes->sub.sub_trim_idx = DEFAULT_SUB_TRIM_INDEX;
        DEV_DEBUG(("DEV: getDefaultAttrib prof:%d SWAT vol:%d\n",attributes->profiles, attributes->sub.sub_trim_idx));
        return;
#endif  /* ENABLE_SUBWOOFER */
    }

    attributes->mode = sink_mode_unknown;
    DEV_DEBUG(("DEV: getDefaultAttrib link_mode:%d \n",attributes->mode));

    attributes->profiles    = sink_none;
    attributes->hfp.volume  = theSink.features.DefaultVolume;
    attributes->a2dp.volume = ((sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main) * theSink.features.DefaultA2dpVolLevel)/ VOLUME_NUM_VOICE_STEPS);
#ifdef ENABLE_SUBWOOFER
    attributes->sub.sub_trim_idx = 0;
#endif
    
    DEV_DEBUG(("DEV: getDefaultAttrib - prof:0x%02X route:%u,%u, peer %d, features %x hfp_vol %d, a2dp_vol %d sub_trim %d gatt %d batt_l %d batt_r %d batt_p %d\n", 
               attributes->profiles, 
			   attributes->master_routing_mode,
               attributes->slave_routing_mode,
               attributes->peer_device, 
               attributes->peer_features, 
               attributes->hfp.volume,
               attributes->a2dp.volume,
               attributes->sub.sub_trim_idx,
               attributes->gatt_client.gatt,
               attributes->gatt_client.battery_local,
               attributes->gatt_client.battery_remote,
               attributes->gatt_client.battery_peer));
}


/****************************************************************************
NAME    
    deviceManagerClearAttributes
    
DESCRIPTION
    Clear sink_attributes struct

RETURNS
    void
*/
void deviceManagerClearAttributes(sink_attributes* attributes)
{
    memset(attributes, 0, sizeof(sink_attributes));
}

/****************************************************************************
NAME    
    deviceManagerGetDefaultAttributes
    
DESCRIPTION
    Get the default attributes for a specific device type. 
    All attributes not related to the device type will be cleared.

RETURNS
    void
*/
void deviceManagerGetDefaultAttributes(sink_attributes* attributes, sink_remote_device_type device_type)
{
    deviceManagerClearAttributes(attributes);
    
    deviceManagerUpdateAttributesWithDeviceDefaults(attributes, device_type);
}


/****************************************************************************
NAME    
    deviceManagerGetAttributes
    
DESCRIPTION
    Get the attributes for a given device, or check device is in PDL if 
    attributes is NULL.

RETURNS
    TRUE if device in PDL, otherwise FALSE
*/
bool deviceManagerGetAttributes(sink_attributes* attributes, const bdaddr* dev_addr)
{
    /* NULL is valid if we just want to check dev is in PDL */
    uint8 size = (attributes ? sizeof(sink_attributes) : 0);
    /* Attempt to retrieve attributes from PS */
    bool ok = ConnectionSmGetAttributeNow(0, dev_addr, size, (uint8*)attributes);

#ifdef DEBUG_DEV
    DEV_DEBUG(("DEV: GetAttribs %04X %02X %06lX ", 
               dev_addr->nap,
               dev_addr->uap,
               dev_addr->lap));
    if (attributes)
    {
        DEV_DEBUG(("prof:0x%02X route:%u,%u HfpVol:%d a2dpVol:%d link_mode:%x le_link_mode:%x\n",
               attributes->profiles, 
               attributes->master_routing_mode,
               attributes->slave_routing_mode,
               attributes->hfp.volume, 
               attributes->a2dp.volume,
               attributes->mode,
               attributes->le_mode));
    }
    else
    {
        DEV_DEBUG(("(none)\n"));
    }
#endif
    return ok;
}


/****************************************************************************
NAME    
    deviceManagerGetIndexedAttributes
    
DESCRIPTION
    Get the attributes for a given index of device

RETURNS
    TRUE if device is in PDL, otherwise FALSE
*/
bool deviceManagerGetIndexedAttributes(uint16 index, sink_attributes* attributes, typed_bdaddr* dev_addr)
{
    return ConnectionSmGetIndexedAttributeNowReq(0, index, sizeof(sink_attributes), (uint8*)attributes, dev_addr);
}

/****************************************************************************
NAME
    deviceManagerGetAddrFromProfile

DESCRIPTION
    Get the attributes for a specific device profile

RETURNS
    TRUE if device profile is in PDL, otherwise FALSE
*/
bool deviceManagerGetAddrFromProfile(sink_link_type profile, typed_bdaddr* dev_addr)
{
    sink_attributes attributes;
	uint16 pdlIndex = 0;
	bool validPdlIndex = TRUE;

    /* Scan through the PDL to find a matching device profile.
     * ConnectionSmGetIndexedAttributeNowReq() will fail when we reach the end of the PDL.*/
    while (validPdlIndex)
    {
    	validPdlIndex = ConnectionSmGetIndexedAttributeNowReq(0, pdlIndex, sizeof(sink_attributes), (uint8*)&attributes, dev_addr);

    	/* Make sure we return TRUE only when valid attributes have been found
    	 * and matching the requested profile.*/
    	if((attributes.profiles == profile) && (validPdlIndex))
    	{
    		return TRUE;
    	}
        /* There is still possibility that device in PDL doesn't have any attributes.*/
        else if(!validPdlIndex)
        {   
            /* If validPdlIndex is TRUE then it means there is another device in PDL but without attributes.
             * Continue looking for matching profile in the rest of the PDL.*/
            validPdlIndex = ConnectionSmGetIndexedAttributeNowReq(0, pdlIndex, 0, (uint8*)NULL, dev_addr);     
                        
        }

    	pdlIndex++;
    }

	/* TRUE if device profile is in PDL, otherwise FALSE */
	return FALSE;
}

/****************************************************************************
NAME
    deviceManagerStoreAttributes
    
DESCRIPTION
    Stores given attribute values against a given device in PS.

RETURNS
    void
*/
void deviceManagerStoreAttributes(sink_attributes* attributes, const bdaddr* dev_addr)
{
    DEV_DEBUG(("DEV: StoreAttribs - profiles %d, route %u,%u peer %d, features %x hfp_vol %d, a2dp_vol %d sub_trim %d gatt %d batt_l %d batt_r %d batt_p %d, hr %d link_mode %d le_link_mode %d\n", 
               attributes->profiles, 
               attributes->master_routing_mode, 
               attributes->slave_routing_mode,
               attributes->peer_device, 
               attributes->peer_features, 
               attributes->hfp.volume,
               attributes->a2dp.volume,
               attributes->sub.sub_trim_idx,
               attributes->gatt_client.gatt,
               attributes->gatt_client.battery_local,
               attributes->gatt_client.battery_remote,
               attributes->gatt_client.battery_peer,
               attributes->gatt_client.heart_rate,
			   attributes->mode,
			   attributes->le_mode
               ));

    ConnectionSmPutAttribute(0, dev_addr, sizeof(sink_attributes), (uint8*)attributes); 
}


/****************************************************************************
NAME
    deviceManagerStoreDefaultAttributes
    
DESCRIPTION
    Stores the default attributes against a given device in PS.

RETURNS
    void
*/
void deviceManagerStoreDefaultAttributes(const bdaddr* dev_addr, sink_remote_device_type device_type)
{
    sink_attributes attributes;
    deviceManagerGetDefaultAttributes(&attributes, device_type);
    DEV_DEBUG(("DEV: StoreDefaultAttribs - dev[%x] profiles %d\n", device_type, attributes.profiles));
    
    deviceManagerStoreAttributes(&attributes, dev_addr);
}

/****************************************************************************
NAME
    deviceManagerMarkTrusted
    
DESCRIPTION
     Sets a device as trusted

RETURNS
    void
*/
void deviceManagerMarkTrusted(const bdaddr* dev_addr)
{
    /* Mark the device as trusted */
    ConnectionSmSetTrustLevel(&theSink.task, dev_addr, TRUE);
}

/****************************************************************************
NAME
    deviceManagerCompareAttributes
    
DESCRIPTION
    Compare two sets of attributes

RETURNS
    TRUE if they match, FALSE otherwise
*/
static bool deviceManagerCompareAttributes(sink_attributes* attr1, sink_attributes* attr2)
{
    if(memcmp(attr1, attr2, sizeof(sink_attributes)) != 0)
        return FALSE;
    return TRUE;
}


/****************************************************************************
NAME
    deviceManagerUpdateAttributes
    
DESCRIPTION
    Stores the current attribute values for a given HFP/A2DP connection in
    PS.

RETURNS
    void
*/
void deviceManagerUpdateAttributes(const bdaddr* bd_addr, sink_link_type link_type, hfp_link_priority hfp_priority, a2dp_link_priority a2dp_priority)
{
    EVENT_UPDATE_ATTRIBUTES_T* update = PanicUnlessNew(EVENT_UPDATE_ATTRIBUTES_T);
    
    memset(update,0,sizeof(EVENT_UPDATE_ATTRIBUTES_T)); 
    
    update->bd_addr = *bd_addr;
    
    if(link_type == sink_hfp)
    {
        update->attributes.profiles = sink_hfp;
        update->attributes.hfp.volume = theSink.profile_data[PROFILE_INDEX(hfp_priority)].audio.gSMVolumeLevel;
        update->device_type = dev_type_ag;
    }
    else if(link_type == sink_a2dp)
    {
        update->attributes.profiles = sink_a2dp;
        /* Master Volume in volume_levels data structure is the A2DP volume level to be stored per paired device */
        update->attributes.a2dp.volume = theSink.volume_levels.a2dp_volume[a2dp_priority].main_volume;
        update->attributes.a2dp.clock_mismatch = theSink.a2dp_link_data->clockMismatchRate[a2dp_priority];
        update->device_type = dev_type_ag;
    }
#ifdef ENABLE_PEER
    else if(link_type == sink_tws)
    {
        update->attributes.profiles = sink_tws;
        update->attributes.master_routing_mode = theSink.a2dp_link_data->a2dp_audio_mode_params.master_routing_mode;
        update->attributes.slave_routing_mode = theSink.a2dp_link_data->a2dp_audio_mode_params.slave_routing_mode;
    }    
#endif
#ifdef ENABLE_SUBWOOFER
    else if(link_type == sink_swat)
    {
        update->attributes.profiles = sink_swat;
        update->attributes.sub.sub_trim_idx = theSink.rundata->subwoofer.sub_trim_idx;
        update->device_type = dev_type_sub;
        
        DEV_DEBUG(("DEV: DelayUpdateAttributes - type %d profiles %d, SUB Vol idx %d\n",link_type, update->attributes.profiles, update->attributes.sub.sub_trim_idx));
    }
#endif
    
    DEV_DEBUG(("DEV: DelayUpdateAttributes type:%d prof:0x%02X route:%u,%u hfp_vol:%d, a2dp_vol:%d clock_mismatch:%d ver:%x features:%x device:%x\n",
               link_type,
               update->attributes.profiles,
               update->attributes.master_routing_mode,
               update->attributes.slave_routing_mode,
               update->attributes.hfp.volume,
               update->attributes.a2dp.volume,
               update->attributes.a2dp.clock_mismatch,
               update->attributes.peer_version,
               update->attributes.peer_features,
               update->attributes.peer_device));

    /* Cancel the volume/source change timer event - 'EventSysVolumeAndSourceChangeTimer',  before updating the ps store*/
    MessageCancelAll( &theSink.task , EventSysVolumeAndSourceChangeTimer) ;

    MessageSendConditionallyOnTask(&theSink.task, EventSysUpdateAttributes, update, AudioBusyPtr());
}


/****************************************************************************
NAME
    deviceManagerDelayedUpdateAttributes
    
DESCRIPTION
    Store attributes contained in EVENT_UPDATE_ATTRIBUTES_T in PS

RETURNS
    void
*/
void deviceManagerDelayedUpdateAttributes(const EVENT_UPDATE_ATTRIBUTES_T* update)
{
    sink_attributes attributes;
    sink_attributes new_attributes;

    /* Get attributes from PS */
    deviceManagerGetDefaultAttributes(&attributes, update->device_type);
    deviceManagerGetAttributes(&attributes, &update->bd_addr);
    
    new_attributes = attributes;

    if(update->attributes.profiles == sink_hfp)
    {
        /* Update with current attribute values */
        new_attributes.profiles |= sink_hfp;
        new_attributes.hfp.volume = update->attributes.hfp.volume;
    }
    else if(update->attributes.profiles == sink_a2dp)
    {
        /* Update with current attribute values */
        new_attributes.profiles |= sink_a2dp;
        new_attributes.a2dp.volume = update->attributes.a2dp.volume;
        new_attributes.a2dp.clock_mismatch = update->attributes.a2dp.clock_mismatch;
    }
#ifdef ENABLE_PEER
    else if (update->attributes.profiles == sink_tws)
    {
        new_attributes.profiles |= sink_tws;
        new_attributes.master_routing_mode = update->attributes.master_routing_mode;
        new_attributes.slave_routing_mode = update->attributes.slave_routing_mode;
    }
#endif
#ifdef ENABLE_SUBWOOFER
    else if (update->attributes.profiles == sink_swat)
    {
        new_attributes.sub.sub_trim_idx = update->attributes.sub.sub_trim_idx;
        new_attributes.profiles = sink_swat;
        DEV_DEBUG(("DEV: UpdateAttributesOld - profiles %d, SWAT Vol Idx %d\n", attributes.profiles, attributes.sub.sub_trim_idx));
        DEV_DEBUG(("DEV: UpdateAttributesOld - profiles %d, SWAT Vol Idx %d\n", attributes.profiles, new_attributes.sub.sub_trim_idx));
    }
#endif
    
    DEV_DEBUG(("DEV: UpdateAttr old prof:0x%02X route:%u,%u hfp_vol:%d, a2dp_vol:%d clock_mismatch:%d ver:%x features:%x device:%x\n",
               attributes.profiles,
               attributes.master_routing_mode,
               attributes.slave_routing_mode,
               attributes.hfp.volume,
               attributes.a2dp.volume,
               attributes.a2dp.clock_mismatch,
               update->attributes.peer_version,
               update->attributes.peer_features,
               update->attributes.peer_device));
     
    DEV_DEBUG(("DEV: UpdateAttr new prof:0x%02X route:%u,%u hfp_vol:%d, a2dp_vol:%d clock_mismatch:%d ver:%x features:%x device:%x\n",
               new_attributes.profiles,
               new_attributes.master_routing_mode,
               new_attributes.slave_routing_mode,
               new_attributes.hfp.volume,
               new_attributes.a2dp.volume,
               new_attributes.a2dp.clock_mismatch,
               update->attributes.peer_version,
               update->attributes.peer_features,
               update->attributes.peer_device));

    /* Write updated attributes to PS */
    if(!deviceManagerCompareAttributes(&attributes, &new_attributes))
        deviceManagerStoreAttributes(&new_attributes, &update->bd_addr);
}


/****************************************************************************
NAME    
    deviceManagerGetProfileAddr
    
DESCRIPTION
    Get bluetooth address from connection mask

RETURNS
    TRUE if connection is valid, otherwise FALSE
*/
static bool deviceManagerGetProfileAddr(conn_mask mask, bdaddr* dev_addr)
{
    if(mask & conn_hfp)
    {
        /* Get bluetooth address for this profile if connected */
        hfp_link_priority hfp = ((mask & conn_hfp_pri) ? hfp_primary_link : hfp_secondary_link);
        /* Only valid if we have processed HFP connect */
        if(theSink.profile_data[PROFILE_INDEX(hfp)].status.connected)        
            return HfpLinkGetBdaddr(hfp, dev_addr);
    }
    else if(mask & conn_a2dp)
    {
        /* Get bluetooth address for this profile if connected */
        a2dp_link_priority a2dp = ((mask & conn_a2dp_pri) ? a2dp_primary : a2dp_secondary);
        /* Only valid if we have processed A2DP connect */
        if (theSink.a2dp_link_data->connected[a2dp])
        {
            *dev_addr = theSink.a2dp_link_data->bd_addr[a2dp];
            return TRUE;
        }
    }
    
    /* Couldn't find connection for this profile */
    return FALSE;
}

/****************************************************************************
NAME
    deviceManagerShufflePdl

DESCRIPTION
    Re-order devices in the PDL list. For soundbar
    application the subwoofer must live always on the top.

RETURNS
    void
*/
static void deviceManagerShufflePdl(const bdaddr* dev_addr)
{
	bdaddr tmp_addr;

    /* Move currently connected device to top of PDL.
     * This must be the secondary device. */
    ConnectionSmUpdateMruDevice(dev_addr);

    /* Check if we have a primary HFP/A2DP device connected and shuffle it to the top */
    if(deviceManagerGetProfileAddr(conn_hfp_pri, &tmp_addr))
    {
		ConnectionSmUpdateMruDevice(&tmp_addr);
    }
    else if(deviceManagerGetProfileAddr(conn_a2dp_pri, &tmp_addr))
    {
		ConnectionSmUpdateMruDevice(&tmp_addr);
    }

    /* Make sure that priority devices are looked after in the PDL.*/
    deviceManagerUpdatePriorityDevices();
}

/****************************************************************************
 NAME
 deviceManagerUpdatePriorityDevices

 DESCRIPTION
 Forces a type of device on top of the PDL

RETURNS
    void
*/
void deviceManagerUpdatePriorityDevices(void)
{    
#ifdef ENABLE_SUBWOOFER    
	typed_bdaddr dev_addr;

    if(deviceManagerGetAddrFromProfile(sink_swat, &dev_addr))
    {
    	ConnectionSmUpdateMruDevice(&dev_addr.addr);
    }
#endif /* ENABLE_SUBWOOFER */

}

/****************************************************************************
NAME
    deviceManagerSetPriority
    
DESCRIPTION
    Set a device's priority in the PDL

RETURNS
    new pdl listId of passed in src addr
*/
uint8 deviceManagerSetPriority(const bdaddr* dev_addr)
{
    conn_mask mask = deviceManagerProfilesConnected(dev_addr);
    uint8 ListId = 0;
    DEV_DEBUG(("DEV: Update PDL/MRU\n")) ;

    /* more than 1 connected device ? */
    if(deviceManagerNumConnectedDevs() > 1)
    {
        typed_bdaddr  typed_ag_addr;
        bdaddr ag_addr;
        sink_attributes attributes;

        /* is this a connection of a2dp or hfp to the already connected primary device ? */
        deviceManagerGetIndexedAttributes(0, &attributes, &typed_ag_addr);

#ifdef ENABLE_SUBWOOFER
        /* If there is a subwoofer in the top of the PDL,
           incease accordingly the index for primary device.*/
        if (attributes.profiles == sink_swat)
        {
            memset(&attributes,0,sizeof(sink_attributes));
            deviceManagerGetIndexedAttributes(1, &attributes, &typed_ag_addr);
        }
#endif /* ENABLE_SUBWOOFER */         

        DEV_DEBUG(("DEV: Update MRU - two devices connected\n")) ;
    
        /* extract bluetooth address from packed structure */
        ag_addr = typed_ag_addr.addr;
        /* check if this is the primary device? */
        if(BdaddrIsSame(&ag_addr,dev_addr))
        {
            DEV_DEBUG(("DEV: Update MRU - two devices two profiles connected - primary device\n")) ;
            ListId = 0;

            /* if this is the first profile for the device to be connected then send
               the primary device connected event */
            if(mask)
            	{
                   DEV_DEBUG(("deviceManagerSetPriority(PRI/2) : bdaddr %04x%02x%06lx : Profiles %x\n",
                            dev_addr->nap, dev_addr->uap, dev_addr->lap, mask));
                  if(sinkAccesoryIsEnabled() || !((mask & conn_hfp)&&(mask & conn_a2dp)))
                  {
                       MessageSend (&theSink.task , EventSysPrimaryDeviceConnected , NULL );
                  }
            	}
        }
        else
        {
            DEV_DEBUG(("DEV: Update MRU - two devices two profiles connected - secondary device\n")) ;

            /* Place the device at the correct position within the PDL.*/
            deviceManagerShufflePdl(dev_addr);

            /* this is the secondary device */
            ListId = 1;
            /* send connected event if not already done so */

            if(mask)
            	{
                   DEV_DEBUG(("deviceManagerSetPriority(SEC) : bdaddr %04x%02x%06lx : Profiles %x\n",
                            dev_addr->nap, dev_addr->uap, dev_addr->lap, mask));
                  if(sinkAccesoryIsEnabled() || !((mask & conn_hfp)&&(mask & conn_a2dp)))
                  {
                       MessageSend (&theSink.task , EventSysSecondaryDeviceConnected , NULL );
                  }
            	}
        }        
    }
    /* only 1 device so must be primary */
    else
    {
        /* Move device to top of the PDL but always below the protected devices.*/
        DEV_DEBUG(("DEV: Update MRU - primary device\n")) ;
        ConnectionSmUpdateMruDevice(dev_addr); 

        /* Make sure that priority devices are looked after in the PDL.*/
        deviceManagerUpdatePriorityDevices();
        
        /* if this is the first profile for the device to be connected then send
           the primary device connected event */
        if(mask)
        {
             DEV_DEBUG(("deviceManagerSetPriority(PRI) : bdaddr %04x%02x%06lx : Profiles %x\n",
                   dev_addr->nap, dev_addr->uap, dev_addr->lap, mask));
             if(sinkAccesoryIsEnabled() || !((mask & conn_hfp)&&(mask & conn_a2dp)))
             {
                  MessageSend (&theSink.task , EventSysPrimaryDeviceConnected , NULL );
             }
        }
    }
   
    /* return current pdl list position of this device which is 0, top of list */        
    DEV_DEBUG(("DEV: Update MRU - ListId = %x\n",ListId)) ;
    return ListId;
}

/****************************************************************************
NAME
    deviceManagerHandleLinkLoss
    
DESCRIPTION
   Handle link loss. Inform Accessory that reconnect is made after link loss.

RETURNS
    None
*/

void deviceManagerHandleLinkLoss(const bdaddr * dev_addr)
{
    if(sinkAccesoryIsEnabled())
    {
         /* more than 1 connected device ? */
         if(deviceManagerNumConnectedDevs() > 1)
         {
              typed_bdaddr  typed_ag_addr;
              bdaddr ag_addr;
              sink_attributes attributes;

              /* is this a connection of a2dp or hfp to the already connected primary device ? */
              deviceManagerGetIndexedAttributes(0, &attributes, &typed_ag_addr);
#ifdef ENABLE_SUBWOOFER
             /* If there is a subwoofer in the top of the PDL,
             increase accordingly the index for primary device.*/
             if (attributes.profiles == sink_swat)
             {
                  memset(&attributes,0,sizeof(sink_attributes));
                  deviceManagerGetIndexedAttributes(1, &attributes, &typed_ag_addr);
             }
#endif /* ENABLE_SUBWOOFER */

             DEV_DEBUG(("DEV: Update MRU - two devices connected\n")) ;

             /* extract bluetooth address from packed structure */
             ag_addr = typed_ag_addr.addr;
             /* check if this is the primary device? */
             if(BdaddrIsSame(&ag_addr,dev_addr))
            {
                 MessageSend (&theSink.task , EventSysPrimaryDeviceConnected , NULL );
            }
            else
            {
                 MessageSend (&theSink.task , EventSysSecondaryDeviceConnected , NULL );
            }
        }
        else
        {
            MessageSend (&theSink.task , EventSysPrimaryDeviceConnected , NULL ); 
        }
    }
}

/****************************************************************************
NAME    
    deviceManagerProfilesConnected
    
DESCRIPTION
    Compare bdaddr against those of the current connected devices

RETURNS
    conn_mask indicating which profiles the device is connected to
*/
conn_mask deviceManagerProfilesConnected(const bdaddr * bd_addr)
{
    bdaddr dev_addr;
    conn_mask mask;
    conn_mask result = 0;
    
    /* Go through all profiles */
    for(mask = conn_hfp_pri; mask <= conn_a2dp_sec; mask <<=1)
    {
        /* Get bluetooth address for profile if connected */
        if(deviceManagerGetProfileAddr(mask, &dev_addr))
        {
            /* If address matches device passed in... */
            if(BdaddrIsSame(bd_addr, &dev_addr))
                result |= mask;
        }
    }
    DEV_DEBUG(("DEV: connected %04X %02X %06lX profiles 0x%02X\n",
               bd_addr->nap, 
               bd_addr->uap,
               bd_addr->lap,
               result));
    return result;
}

/****************************************************************************
NAME    
    deviceManagerGetProfilesConnected
    
DESCRIPTION
    Get mask of all connected profiles.

RETURNS
    conn_mask indicating which profiles are connected.
*/
conn_mask deviceManagerGetProfilesConnected(void)
{
    bdaddr dev_addr;
    conn_mask conn;
    conn_mask result = 0;
    
    /* Go through all profiles */
    for(conn = conn_hfp_pri; conn <= conn_a2dp_sec; conn <<=1)
    {
        /* If connected we will be able to get the bluetooth addr. */
        if(deviceManagerGetProfileAddr(conn, &dev_addr))
        {
            result |= conn;
        }
    }
    DEV_DEBUG(("DEV: get connected profiles 0x%02X\n", result));

    return result;
}

/****************************************************************************
NAME    
    deviceManagerNumConnectedDevs
    
DESCRIPTION
   determines the number of different connected devices, a device may connected
   both hfp and a2dp or only one of each
RETURNS
    number of connected devices
*/
uint8 deviceManagerNumConnectedDevs(void)
{
    conn_mask mask;
    bdaddr dev_addr;
    uint8 no_devices = 0;
    /* Go through all profiles */
    for(mask = conn_hfp_pri; mask <= conn_a2dp_sec; mask <<=1)
    {
        /* Get bluetooth address for profile if connected */
        if(deviceManagerGetProfileAddr(mask, &dev_addr))
        {
            /* If HFP connection, or A2DP connection on device with no HFP... */
            if( (mask & conn_hfp) || (!(conn_hfp & deviceManagerProfilesConnected(&dev_addr))) )
                no_devices++;
        }
    }
    DEV_DEBUG(("DEV: Conn Count %d\n", no_devices));
    return no_devices;
}


/****************************************************************************
NAME    
    deviceManagerNumOtherConnectedDevs
    
DESCRIPTION
   determines the number of different connected devices other than the one provided, a device may connected
   both hfp and a2dp or only one of each
RETURNS
    number of connected devices
*/
uint16 deviceManagerNumOtherConnectedDevs(const bdaddr *bd_addr)
{
    conn_mask mask;
    bdaddr dev_addr;
    uint8 no_devices = 0;
    /* Go through all profiles */
    for(mask = conn_hfp_pri; mask <= conn_a2dp_sec; mask <<=1)
    {
        /* Get bluetooth address for profile if connected and it should be different to one provided*/
        if(deviceManagerGetProfileAddr(mask, &dev_addr) && !BdaddrIsSame(&dev_addr, bd_addr))
        {
            /* If HFP connection, or A2DP connection on device with no HFP... */
            if( (mask & conn_hfp) || (!(conn_hfp & deviceManagerProfilesConnected(&dev_addr))) )
                no_devices++;
        }
    }
    DEV_DEBUG(("DEV: Conn Count %d\n", no_devices));
    return no_devices;
}


/****************************************************************************
NAME    
    deviceManagerNumConnectedPeerDevs
    
DESCRIPTION
   determines the number of different connected peer devices, a device will only
   have connected a2dp
RETURNS
    number of connected peer devices
*/
uint8 deviceManagerNumConnectedPeerDevs(void)
{
    a2dp_link_priority priority;
    uint8 no_devices = 0;
    /* Go through all profiles */
    for(priority = a2dp_primary; priority <= a2dp_secondary; priority++)
    {
        /* Only valid if we have processed A2DP connect */
        if (theSink.a2dp_link_data && 
            (theSink.a2dp_link_data->device_id[priority] != INVALID_DEVICE_ID) && 
            (theSink.a2dp_link_data->peer_device[priority] == remote_device_peer))
        {
            no_devices++;
        }
    }
    DEV_DEBUG(("DEV: Peer Conn Count %d\n", no_devices));
    return no_devices;
}


/****************************************************************************
NAME    
    deviceManagerCanConnect
    
DESCRIPTION
   Determines if max number of possible connections have been made
RETURNS
   TRUE if connection can be made, FALSE otherwise
*/
bool deviceManagerCanConnect(void)
{
    bdaddr dev_addr1, dev_addr2;

    DEV_DEBUG(("DEV: %d of %d connected\n", deviceManagerNumConnectedDevs(), (theSink.MultipointEnable ? MAX_MULTIPOINT_CONNECTIONS : 1)));
    /* If no devices connected we're okay */
    if(deviceManagerNumConnectedDevs() == 0)
        return TRUE;
    /* If multipoint is not enabled check if this is manual connect request for A2DP 
        for an already connected HFP profile to the same device
    */
    if(!theSink.MultipointEnable)
    {
        if((deviceManagerGetProfileAddr(conn_hfp_pri, &dev_addr1)) && 
            !(deviceManagerGetProfileAddr(conn_a2dp_pri, &dev_addr2)))
            {
                if(BdaddrIsSame(&dev_addr1, &dev_addr2))
                {    
                    DEV_DEBUG(("DEV: No multipoint, allow the device to connect next profile\n"));
                    return TRUE;
                }
                else
                {
                    DEV_DEBUG(("DEV: No multipoint, next profile is a different device, reject\n"));
                    return FALSE;
                }
            }
    }
    /* If multipoint and not all devices connected then we're okay */
    if(theSink.MultipointEnable && (deviceManagerNumConnectedDevs() < MAX_MULTIPOINT_CONNECTIONS)
#ifdef ENABLE_PEER
        /* If the peer is already streaming stop any more connections.
           This is to avoid dropouts on the peer a2dp stream when paging
           the next device. */
        && (theSink.features.PeerLinkRecoveryWhileStreaming
            || (theSink.peer.current_state < RELAY_STATE_STARTING))
#endif
    )
    {
        DEV_DEBUG(("DEV: Multipoint and not at max, allow the device to connect\n"));        
        return TRUE;
    }
    /* We can't accept any more connections */
    DEV_DEBUG(("DEV: No more connections\n"));
    return FALSE;
}

/****************************************************************************
NAME    
    deviceManagerIsSameDevice
    
DESCRIPTION
    Determines if the supplied HF and AV devices are actually one and the same

RETURNS
    TRUE if the devices are the same, FALSE otherwise
*/
bool deviceManagerIsSameDevice(a2dp_link_priority a2dp_link, hfp_link_priority hfp_link)
{
    typed_bdaddr tbdaddr1, tbdaddr2;
    
    if (theSink.a2dp_link_data && theSink.a2dp_link_data->connected[a2dp_link])
    {   /* A2dp link data present and device marked as connected */
        if (HfpLinkGetBdaddr(hfp_link, &tbdaddr1.addr) && A2dpDeviceGetBdaddr(theSink.a2dp_link_data->device_id[a2dp_link], &tbdaddr2.addr))
        {   /* Successfully obtained bdaddrs */
            return BdaddrIsSame(&tbdaddr1.addr, &tbdaddr2.addr);
        }
    }

    return FALSE;
}

/****************************************************************************
NAME    
    deviceManagerDeviceDisconnectedInd
    
DESCRIPTION
    Finds which device has been disconnected(primary or secondary) and sends the appropriate event to notify the same

RETURNS
    void
*/
void deviceManagerDeviceDisconnectedInd(const bdaddr* dev_addr)
{
    conn_mask mask = deviceManagerProfilesConnected(dev_addr);

    typed_bdaddr  typed_ag_addr;
    bdaddr ag_addr;
    sink_attributes attributes;

    /* is this a connection of a2dp or hfp to the already connected primary device ? */
    deviceManagerGetIndexedAttributes(0, &attributes, &typed_ag_addr);

#ifdef ENABLE_SUBWOOFER
    /* If there is a subwoofer in the top of the PDL,
       incease accordingly the index for primary device.*/
    if (attributes.profiles == sink_swat)
    {
        memset(&attributes,0,sizeof(sink_attributes));
        deviceManagerGetIndexedAttributes(1, &attributes, &typed_ag_addr);
    }
#endif /* ENABLE_SUBWOOFER */

    /* extract bluetooth address from packed structure */
    ag_addr = typed_ag_addr.addr;
    
    /* check if this is the primary device? */
    if(BdaddrIsSame(&ag_addr,dev_addr))
    {
         /* send primary device disconnected event i*/
        if( !((mask & conn_hfp)||(mask & conn_a2dp)))
        {
           MessageSend (&theSink.task , EventSysPrimaryDeviceDisconnected , NULL );        
        }        
    }
    else
    {
        /* send secondary device disconnected event */
        if( !((mask & conn_hfp)||(mask & conn_a2dp)))
        {
           MessageSend (&theSink.task , EventSysSecondaryDeviceDisconnected , NULL );        
        }
    }
}

/****************************************************************************
NAME    
    deviceManagerDisconnectNonGaiaDevices
    
DESCRIPTION
    Disconnect all the devices which are not connected to GAIA (assuming that sink can support only one GAIA session)

RETURNS
    void
*/  
void deviceManagerDisconnectNonGaiaDevices(void)
{
#ifdef ENABLE_GAIA 
    if(theSink.rundata->gaia_data.gaia_transport == NULL)
#endif
    {    
        sinkDisconnectAllSlc();
        disconnectAllA2dpAvrcp(FALSE);
    }   
#ifdef ENABLE_GAIA 
    else
    {
        bdaddr bdaddr_non_gaia_device;
        if(gaiaGetBdAddrNonGaiaDevice(&bdaddr_non_gaia_device))
        {
            sinkDisconnectSlcFromDevice(&bdaddr_non_gaia_device);
            disconnectA2dpAvrcpFromDevice(&bdaddr_non_gaia_device);
        }
    }
#endif    
}

/****************************************************************************
NAME    
    deviceManagerRemoveAllDevices
    
DESCRIPTION
    Remove all devices from the PDL

RETURNS
    void
*/
void deviceManagerRemoveAllDevices(void)
{
    /* Clear the TDL and then clear the BLE whitelist */
    ConnectionSmDeleteAllAuthDevices(0);

    /* Trigger the event to clear the BLE whitelist */
    sinkBleClearWhiteListEvent();
}


/****************************************************************************
NAME    
    deviceManagerGetAttributeHfpVolume
    
DESCRIPTION
    Gets the HFP volume attribute associated with a paired device.
    The address of the paired device must be supplied as ag_addr.
    If the function returns TRUE, hfp_vol will contain the attribute value.

RETURNS
    TRUE if the HFP volume attribute was found, FALSE otherwise.
*/
bool deviceManagerGetAttributeHfpVolume(const bdaddr *ag_addr, uint16 *hfp_vol)
{
    sink_attributes attributes;
    if (deviceManagerGetAttributes(&attributes, ag_addr))
    {
        *hfp_vol = attributes.hfp.volume;
        return TRUE;
    }
    
    return FALSE;
}

