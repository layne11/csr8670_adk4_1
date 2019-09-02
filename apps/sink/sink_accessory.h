/****************************************************************************

Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_accessory.h

DESCRIPTION
    Support for Accessory. 

    The feature is included in  an add-on installer 

*/
#ifndef _SINK_ACCESSORY_H_
#define _SINK_ACCESSORY_H_

/****************************************************************************
NAME
    sinkAccesoryIsEnabled

DESCRIPTION
    Returns TRUE if Accessory is enabled. FALSE otherwise.
*/
#define sinkAccesoryIsEnabled() (FALSE)


/****************************************************************************
NAME
    sinkAccessoryInit

DESCRIPTION
    Initialisation of Accessory.
*/    
#define sinkAccessoryInit() ((void)(0))


/******************************************************************************
NAME
    sinkHandleAccessoryConnect

DESCRIPTION
    Handles the Accessory connection
*/
#define sinkHandleAccessoryConnect(link_no) ((void)(0))


/******************************************************************************
NAME
    sinkHandleAccessoryDisconnect

DESCRIPTION
     Handles the Accessory disconnect
*/
#define sinkHandleAccessoryDisconnect(link_no) ((void)(0))


/******************************************************************************
NAME
    sinkHandleAccessoryReportUserEvent

DESCRIPTION
    Handles the Accesory user event.
*/
#define sinkHandleAccessoryReportUserEvent(id) ((void)(0))


/******************************************************************************
NAME
    sinkHandleSendHidVolumeToDevice

DESCRIPTION
    Handles the HID Volume to device.
*/
#define sinkHandleSendHidVolumeToDevice(MessageId) FALSE

#endif /* _SINK_ACCESSORY_H_ */
