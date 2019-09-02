/* Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd. */

/*!
\defgroup library library
\ingroup vm_libs

\brief Common header file for Bluelab libraries

\section library_intro INTRODUCTION
This header file defines common data types and values required for
all ADK libraries.
*/

/*@{*/

#ifndef _LIBRARY_H
#define _LIBRARY_H

#include <vmtypes.h>

/*!   Default RFCOMM Channels provided to RFCOMM based services.
*/


/*! @brief  Default SPP Service Channel */
#define     SPP_DEFAULT_CHANNEL   (0x01)

/*! @brief  Default DUN Service Channel */
#define     DUN_DEFAULT_CHANNEL   (0x02)

/*! @brief  Default FTP Service Channel */
#define     FTP_DEFAULT_CHANNEL   (0x03)

/*! @brief  Default OPP Service Channel */
#define     OPP_DEFAULT_CHANNEL   (0x04)

/*! @brief  Default PBAP Service Channel for PCE */
#define     PBAPC_DEFAULT_CHANNEL (0x05)

/*! @brief  Default PBAP Service Channel for PSE */
#define     PBAPS_DEFAULT_CHANNEL (0x06)

/*! @brief  Default MAP Service Channel for MNS */
#define     MAPC_DEFAULT_CHANNEL  (0x07)

/*! @brief  Default MAP Service Channel for MAS */
#define     MAPS_DEFAULT_CHANNEL  (0x08)

/*! @brief  Default HFP AG service channel */
#define     AGHFP_DEFAULT_CHANNEL (0x09)

/*! @brief   Default HFP Device Service Channel */
#define     HFP_DEFAULT_CHANNEL   (0x0A)

/*! @brief   Default HSP service channel */
#define     HSP_DEFAULT_CHANNEL   (0x0B)

/*! @brief   Default HFP Device Service Channel */
#define     HFP_DEFAULT_CHANNEL_2 (0x0C)

/*! @brief   Default HSP service channel */
#define     HSP_DEFAULT_CHANNEL_2 (0x0D)

/*! @brief   Default reserved service channel */
#define     RESERVED_DEFAULT_CHANNEL   (0x0E)


/*!   Base values for library messages 
*/


/*! @brief  Message base for profile libraries */
#define     CL_MESSAGE_BASE         0x5000
#define     SPP_MESSAGE_BASE        0x5100
#define     DUN_MESSAGE_BASE        0x5200

#define     GOEP_MESSAGE_BASE       0x5300
#define     OBEX_MESSAGE_BASE       0x5350
#define     FTPC_MESSAGE_BASE	    0x5400
#define     FTPS_MESSAGE_BASE       0x5450
#define     OPPC_MESSAGE_BASE	    0x5500
#define     OPPS_MESSAGE_BASE	    0x5550
#define     PBAPC_MESSAGE_BASE	    0x5600
#define     PBAPS_MESSAGE_BASE	    0x5650
#define     MAPC_MESSAGE_BASE       0x5700
#define     MAPS_MESSAGE_BASE       0x5750
#define     GATT_MESSAGE_BASE       0x5800

#define     HFP_MESSAGE_BASE	    0x5A00
#define     AGHFP_MESSAGE_BASE      0x5B00
#define     WBS_MESSAGE_BASE        0x5C00
#define     A2DP_MESSAGE_BASE	    0x5D00
#define     AVRCP_MESSAGE_BASE      0x5E00

#define     HDP_MESSAGE_BASE        0x6000

#define     HID_MESSAGE_BASE        0x6100
#define     HIDKP_MESSAGE_BASE	    0x6150

#define     BATT_REP_BASE           0x6200

#define     SWAT_MESSAGE_BASE       0x6300

#define     GATT_MANAGER_MESSAGE_BASE            0x6400
#define     GATT_GAP_SERVER_MESSAGE_BASE         0x6420
#define     GATT_BATTERY_SERVER_MESSAGE_BASE     0x6440
#define     GATT_BATTERY_CLIENT_MESSAGE_BASE     0x6460
#define     GATT_FINDME_MESSAGE_BASE             0x6480
#define     GATT_PROXIMITY_MESSAGE_BASE          0x64A0
#define     GATT_IMM_ALERT_SERVER_MESSAGE_BASE          0x64C0
#define     GATT_IMM_ALERT_CLIENT_MESSAGE_BASE          0x64E0
#define     GATT_TRANSMIT_POWER_SERVER_MESSAGE_BASE     0x6500
#define     GATT_TRANSMIT_POWER_CLIENT_MESSAGE_BASE     0x6520
#define     GATT_LINK_LOSS_SERVER_MESSAGE_BASE          0x6540
#define     GATT_LINK_LOSS_CLIENT_MESSAGE_BASE          0x6560
#define     GATT_DEVICE_INFO_CLIENT_MESSAGE_BASE        0x6580
#define     GATT_DEVICE_INFO_SERVER_MESSAGE_BASE        0x65A0
#define     GATT_HID_CLIENT_MESSAGE_BASE                0x65C0
#define     GATT_ANCS_MESSAGE_BASE                      0x65E0
#define     GATT_CLIENT_MESSAGE_BASE                    0x6600
#define     GATT_SCAN_PARAMS_CLIENT_MESSAGE_BASE        0x6620
#define     GATT_SERVER_MESSAGE_BASE                    0x6640
#define     GATT_HR_SERVER_MESSAGE_BASE                 0x6660
#define     GATT_HEART_RATE_CLIENT_MESSAGE_BASE         0x6680


/*! @brief  Message base for non profile libraries */
#define     AUDIO_DOWNSTREAM_MESSAGE_BASE        0x7000
#define     AUDIO_MESSAGE_BASE            AUDIO_DOWNSTREAM_MESSAGE_BASE
#define     BATTERY_MESSAGE_BASE                 0x7100
#define     CODEC_MESSAGE_BASE                   0x7200
#define     AUDIO_UPSTREAM_MESSAGE_BASE          0x7300
#define     DEBONGLE_MESSAGE_BASE                0x7400
#define     GAIA_MESSAGE_BASE                    0x7480
#define     USB_DEVICE_CLASS_MSG_BASE            0x7500
#define     DISPLAY_UPSTREAM_MESSAGE_BASE        0x7600
#define     DISPLAY_DOWNSTREAM_MESSAGE_BASE      0x7700
#define     POWER_MESSAGE_BASE                   0x7800
#define     CSR_SPEECH_RECOGNITION_MESSAGE_BASE  0x7900
#define     RESERVED_MESSAGE_BASE                0x7A00
#define     FM_UPSTREAM_MESSAGE_BASE             0x7B00
#define     FM_DOWNSTREAM_MESSAGE_BASE           0x7C00
#define     UPGRADE_UPSTREAM_MESSAGE_BASE        0x7D00
#define     UPGRADE_DOWNSTREAM_MESSAGE_BASE      0x7D80
#define     AUDIO_MIXER_COMMAND_MESSAGE_BASE     0x7E00
#define     AUDIO_MIXER_RESPONSE_MESSAGE_BASE    0x7E20

#define     CM_MESSAGE_BASE                 0x7D00
#define     DM_MESSAGE_BASE                 0x7D10
#define     DD_MESSAGE_BASE                 0x7D20
#define     NFC_MESSAGE_BASE                0x7E00
#endif /* _LIBRARY_H */

/*@}*/
