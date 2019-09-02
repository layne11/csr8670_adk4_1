/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
*/
#ifndef _SINK_USB_H_
#define _SINK_USB_H_

#include <stdlib.h>
#include <usb_device_class.h>

#define USB_DEVICE_CLASS_AUDIO      (USB_DEVICE_CLASS_TYPE_AUDIO_SPEAKER | USB_DEVICE_CLASS_TYPE_AUDIO_MICROPHONE)

#if defined(ENABLE_USB)

typedef enum
{
    usb_audio_suspend_none,
    usb_audio_suspend_local,
    usb_audio_suspend_remote
} usb_audio_suspend_state;

typedef enum
{
    usb_plugin_stereo,
    usb_plugin_mono_nb,
    usb_plugin_mono_wb
} usb_plugin_type;

typedef struct
{
    sink_charge_current     i_disc;                        /* Current when USB disconnected but charger connected */
    sink_charge_current     i_susp;                        /* Current when suspended */
    sink_charge_current     i_susp_db;                     /* Current when suspended prior to enumeration with dead battery*/
    sink_charge_current     i_att;                         /* Current when attached but not enumerated */
    sink_charge_current     i_att_trickle;                 /* Current when attached but not enumerated and trickle charging */
    sink_charge_current     i_conn;                        /* Current when connected to host/hub */
    sink_charge_current     i_conn_trickle;                /* Current when connected to host/hub and trickle charging */
    sink_charge_current     i_chg;                         /* Current when connected to a charging host/hub */
    sink_charge_current     i_dchg;                        /* Current when connected to a dedicated charger */
    sink_charge_current     i_lim;                         /* Current when vbus drop is detected */
    unsigned                audio_always_on:1;             /* Route USB audio even if not in use */
    unsigned                pause_when_switching_source:1; /* Pause USB audio when switching away from USB source */
    usb_plugin_type         plugin_type:2;                 /* Mono/Stereo */
    unsigned                plugin_index:4;                /* USB plugin to use */
    unsigned                attach_timeout:4;              /* Time (seconds) after attach at which we set low power bootmode if not enumerated */
    unsigned                deconfigured_timeout:4;        /* Time (seconds) after deconfigure at which we set low power bootmode if not enumerated */
    usb_device_class_type   device_class;                  /* Class of device */
} usb_config; 

typedef struct
{
    usb_config          config;                 /* USB config */
    unsigned            ready:1;                /* Ready */
    unsigned            enumerated:1;           /* USB enumerated */
    unsigned            suspended:1;            /* Suspended */
    unsigned            mic_active:1;           /* USB Mic in use */
    unsigned            spkr_active:1;          /* USB Speaker in use */
    unsigned            vbus_okay:1;            /* VBUS above threshold */
    unsigned            dead_battery:1;         /* Battery is below dead battery threshold */
    unsigned            deconfigured:1;         /* USB has been deconfigured */
    unsigned            audio_suspend_state:2;  /* Current USB audio suspend state */
    unsigned            vol_event_allowed:1;    /* Volume event recently sent */
    unsigned            out_mute:1;             /* Speaker mute flag */
    unsigned            unused:4;
    
    int16               out_l_vol;              /* Speaker l volume from USB host */
} usb_info;

#if defined(ENABLE_USB_AUDIO)

typedef struct
{
    uint32                               sample_rate;
    const usb_device_class_audio_config* usb_descriptors;
    usb_plugin_type                      plugin_type;
} usb_plugin_info;

#endif

#define SINK_USB_INFO(sink_usb_info)    usb_info *sink_usb_info;
#else
#define SINK_USB_INFO(sink_usb_info)
#endif

/****************************************************************************
NAME 
    usbUpdateChargeCurrent
    
DESCRIPTION
    Set the charge current based on USB state
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbUpdateChargeCurrent(void);
#else
#define usbUpdateChargeCurrent() ((void)(0))
#endif


/****************************************************************************
NAME 
    usbSetBootMode
    
DESCRIPTION
    Set the boot mode to default or low power
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbSetBootMode(uint8 bootmode);
#else
#define usbSetBootMode(x) ((void)(0))
#endif



/****************************************************************************
NAME 
    handleUsbMessage
    
DESCRIPTION
    Handle firmware USB messages
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void handleUsbMessage(Task task, MessageId id, Message message);
#else
#define handleUsbMessage(task, id, message) ((void)(0))
#endif


/****************************************************************************
NAME 
    usbTimeCriticalInit
    
DESCRIPTION
    Initialise USB. This function is time critical and must be called from
    _init. This will fail if either Host Interface is not set to USB or
    VM control of USB is FALSE in PS. It may also fail if Transport in the
    project properties is not set to USB VM.
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbTimeCriticalInit(void);
#else
#define usbTimeCriticalInit() ((void)(0))
#endif


/****************************************************************************
NAME 
    usbInit
    
DESCRIPTION
    Initialisation done once the main loop is up and running. Determines USB
    attach status etc.
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbInit(void);
#else
#define usbInit() ((void)(0))
#endif


/****************************************************************************
NAME 
    usbSetVbusLevel
    
DESCRIPTION
    Set whether VBUS is above or below threshold
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbSetVbusLevel(voltage_reading vbus);
#else
#define usbSetVbusLevel(x) ((void)(0))
#endif


/****************************************************************************
NAME 
    usbSetDeadBattery
    
DESCRIPTION
    Set whether VBAT is below the dead battery threshold
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
void usbSetVbatDead(bool dead);
#else
#define usbSetVbatDead(x) ((void)(0))
#endif


/****************************************************************************
NAME 
    usbGetChargeCurrent
    
DESCRIPTION
    Get USB charger limits
    
RETURNS
    void
*/ 
#ifdef ENABLE_USB
sink_charge_current* usbGetChargeCurrent(void);
#else
#define usbGetChargeCurrent() (NULL)
#endif

/****************************************************************************
NAME 
    usbIsAttached
    
DESCRIPTION
    Determine if USB is attached
    
RETURNS
    TRUE if USB is attached, FALSE otherwise
*/ 
#ifdef ENABLE_USB
bool usbIsAttached(void);
#else
#define usbIsAttached() (FALSE)
#endif


/****************************************************************************
NAME 
    usbAudioIsAttached
    
DESCRIPTION
    Determine if USB Audio is attached
    
RETURNS
    TRUE if USB Audio is attached, FALSE otherwise
*/ 
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbAudioIsAttached(void);
#else
#define usbAudioIsAttached() (FALSE)
#endif

/****************************************************************************
NAME 
 usbCheckDeviceTrimVol

DESCRIPTION
 check whether USB streaming is ongoing and if currently active and routing audio to the device, adjust the volume up or down
 as appropriate.

RETURNS
 bool   TRUE if volume adjusted, FALSE if no streams found
    
*/
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbCheckDeviceTrimVol(volume_direction dir, tws_device_type tws_device);
#else
#define usbCheckDeviceTrimVol(dir,tws_device) (FALSE)
#endif


/****************************************************************************
NAME 
    usbAudioSinkMatch
    
DESCRIPTION
    Compare sink to the USB audio sink
    
RETURNS
    TRUE if sink matches USB audio sink, otherwise FALSE
*/ 
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbAudioSinkMatch(Sink sink);
#else
#define usbAudioSinkMatch(sink) (FALSE)
#endif

#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbAudioPopulateConnectParameters(audio_connect_parameters *connect_parameters);
#else
#define usbAudioPopulateConnectParameters(x) (FALSE)
#endif

/*************************************************************************
NAME    
    usbAudioResume
    
DESCRIPTION
    Issue HID consumer control command to attempt to resume current USB stream

RETURNS
    None
    
**************************************************************************/
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
void usbAudioResume(void);
#else
#define usbAudioResume() ((void)(0))
#endif

/****************************************************************************
NAME 
    usbAudioDisconnect
    
DESCRIPTION
    Disconnect USB audio stream
    
RETURNS
    void
*/ 
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
void usbAudioPostDisconnectConfiguration(void);
#else
#define usbAudioPostDisconnectConfiguration() ((void)(0))
#endif

/****************************************************************************
NAME 
    usbAudioGetMode
    
DESCRIPTION
    Get the current USB audio mode if USB in use
    
RETURNS
    void
*/ 
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
void usbAudioGetMode(AUDIO_MODE_T* mode);
#else
#define usbAudioGetMode(x) ((void)(0))
#endif

/****************************************************************************
NAME 
    usbGetAudioSink
    
DESCRIPTION
    check USB state and return sink if available
    
RETURNS
   sink if available, otherwise 0
*/ 
#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
Sink usbGetAudioSink(void);
#else
#define usbGetAudioSink() ((Sink)(NULL))
#endif

#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbIfCurrentSinkIsUsb(void);
#else
#define usbIfCurrentSinkIsUsb() (FALSE)
#endif

#if defined(ENABLE_USB) && defined(ENABLE_USB_AUDIO)
bool usbIfUsbSinkExists(void);
#else
#define usbIfUsbSinkExists() (FALSE)
#endif

#if defined(ENABLE_USB)
void sinkUsbVolumeIncrement(void);
#else
#define sinkUsbVolumeIncrement() ((void)0)
#endif

#if defined(ENABLE_USB)
void sinkUsbVolumeDecrement(void);
#else
#define sinkUsbVolumeDecrement() ((void)0)
#endif

#if defined(ENABLE_USB)
#define sinkUsbDeviceClassMatch(class)  (theSink.usb->config.device_class & class)
#else
#define sinkUsbDeviceClassMatch(x)  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbIsMicrophoneActive()  (theSink.usb->mic_active)
#else
#define sinkUsbIsMicrophoneActive()  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbIsSpeakerActive()  (theSink.usb->spkr_active)
#else
#define sinkUsbIsSpeakerActive()  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbIsUsbPluginTypeStereo()  (theSink.usb->config.plugin_type == usb_plugin_stereo)
#else
#define sinkUsbIsUsbPluginTypeStereo()  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbIsUsbPluginTypeMonoNb()  (theSink.usb->config.plugin_type == usb_plugin_mono_nb)
#else
#define sinkUsbIsUsbPluginTypeMonoNb()  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbIsUsbPluginTypeMonoWb()  (theSink.usb->config.plugin_type == usb_plugin_mono_wb)
#else
#define sinkUsbIsUsbPluginTypeMonoWb()  (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbAudioIsSuspended()   (!theSink.usb->audio_suspend_state == usb_audio_suspend_none)
#else
#define sinkUsbAudioIsSuspended()   (FALSE)
#endif

#if defined(ENABLE_USB)
#define sinkUsbAudioIsSuspendedLocal()   (theSink.usb->audio_suspend_state == usb_audio_suspend_local)
#else
#define sinkUsbAudioIsSuspendedLocal()   (FALSE)
#endif

#if defined(ENABLE_USB)
bool sinkUsbProcessEventUsb(const MessageId EventUsb);
#else
#define sinkUsbProcessEventUsb(x) (FALSE)
#endif

#if defined(ENABLE_USB) && defined(ENABLE_USB_HUB_SUPPORT)
void usbAttachToHub(void);
#else
#define usbAttachToHub(x) ((void)0)
#endif

#endif /* _SINK_USB_H_ */
