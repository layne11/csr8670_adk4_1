/***************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd. 
Part of ADK 4.1

FILE NAME
    sink_debug.h
    
DESCRIPTION
    
*/

#ifndef _SINK_DEBUG_H_
#define _SINK_DEBUG_H_


#ifndef RELEASE_BUILD /*allows the release build to ensure all of the below are removed*/
    
    /*The individual configs*/
    
 
#ifndef DO_NOT_DOCUMENT

#endif 
 /*end of DO_NOT_DOCUMENT*/

    /*The global debug enable*/ 
    #define DEBUG_PRINT_ENABLEDx
#ifdef HYDRACORE_TODO
    /* We want debug print turned on on
       For hydracore at the moment */
    #define DEBUG_PRINT_ENABLED
    #define DEBUG_SLC
    #define DEBUG_MAIN
    #define DEBUG_A2DP
    #define DEBUG_INIT
#endif

    #ifdef DEBUG_PRINT_ENABLED
        #define DEBUG(x) {printf x;}

        /*The individual Debug enables*/
        /*The custom private log*/
        #define DEBUG_CUSTOMx
            /*The main system messages*/
        #define DEBUG_MAINx
            /* RSSI pairing */
        #define DEBUG_INQx
                /*The button manager */
        #define DEBUG_BUT_MAN
            /*The low level button parsing*/
        #define DEBUG_BUTTONSx
            /*The call manager*/
        #define DEBUG_CALL_MANx
            /*The multipoint manager*/
        #define DEBUG_MULTI_MANx
            /*sink_audio.c*/
        #define DEBUG_AUDIOx
         /* sink_slc.c */
        #define DEBUG_SLCx
        /* sink_devicemanager.c */
        #define DEBUG_DEVx
        /* sink_link_policy.c */
        #define DEBUG_LPx
            /*The config manager */
        #define DEBUG_CONFIGx
            /*The LED manager */
        #define DEBUG_LMx
            /*The Lower level LED drive */
        #define DEBUG_LEDSx
            /*The Lower level PIO drive*/
        #define DEBUG_PIO
            /*The power Manager*/
        #define DEBUG_POWERx
            /*tones*/
        #define DEBUG_TONESx
            /*Volume*/
        #define DEBUG_VOLUMEx
            /*State manager*/
        #define DEBUG_STATESx
            /*authentication*/
        #define DEBUG_AUTHx
            /*dimming LEDs*/
        #define DEBUG_DIMx

        #define DEBUG_A2DPx

        #define DEBUG_LINKLOSSx


        #define DEBUG_PEERx
        #define DEBUG_PEER_SMx

        #define DEBUG_INITx

        #define DEBUG_AVRCPx

        #define DEBUG_AUDIO_PROMPTSx

        #define DEBUG_FILTER_ENABLEx

        #define DEBUG_CSR2CSRx      

        #define DEBUG_USBx

        #define DEBUG_MALLOCx 
            
        #define DEBUG_PBAPx

        #define DEBUG_MAPCx

        #define DEBUG_GAIAx
        
        #define DEBUG_SPEECH_RECx
        
        #define DEBUG_WIREDx
        
        #define DEBUG_AT_COMMANDSx
        
        #define DEBUG_GATTx
        
        #define DEBUG_GATT_MANAGERx
        
          /* BLE transport / messages debug */
        #define DEBUG_BLEx
        /* BLE GAP message debug */
        #define DEBUG_BLE_GAPx
        
          /* Debug GATT Client */
        #define DEBUG_GATT_CLIENTx
        
        #define DEBUG_GATT_ANCS_CLIENTx
        
        #define DEBUG_GATT_BATTERY_CLIENTx
        
        /* Debug Gatt HID client */
        #define DEBUG_GATT_HID_CLIENTx

        #define DEBUG_GATT_DIS_CLIENTx

        #define DEBUG_GATT_IAS_CLIENTx

        #define DEBUG_GATT_SPC_CLIENTx
        
        #define DEBUG_GATT_HRS_CLIENTx

        #define DEBUG_GATT_SERVICE_CLIENTx
        
        /* Debug HID Remote Control */
        #define DEBUG_GATT_HID_RCx
                
        #define DEBUG_GATT_BATTERY_SERVERx
        
        #define DEBUG_DUTx
        
            /* Device Id */
        #define DEBUG_DIx

        #define DEBUG_DISPLAYx
            
            /* Subwoofer debug */
        #define DEBUG_SWATx

        #define DEBUG_FMx
        
            /* Input manager debug */
        #define DEBUG_INPUT_MANAGERx
        
            /* IR Remote Control debug */
        #define DEBUG_IR_RCx
            
            /* Battery Reporting debug */
        #define DEBUG_BAT_REPx

            /* Auto Power off debug */
        #define DEBUG_AUTO_POWER_OFFx
        /* Debug NFC */
        #define DEBUG_NFC

    #else
        #define DEBUG(x) 
    #endif /*DEBUG_PRINT_ENABLED*/

        /* If you want to carry out cVc license key checking in Production test
           Then this needs to be enabled */
    #define CVC_PRODTESTx

         /* Audio Prompt/Tone Sink Event Queue debug */
         #define DEBUG_AUDIO_PROMPTS_TONES_QUEUEx

         /*Audio Indication module to play prompt and tones*/
         #define DEBUG_AUDIO_INDICATIONx


#else /*RELEASE_BUILD*/    

/*used by the build script to include the debug but none of the individual debug components*/
    #ifdef DEBUG_BUILD 
        #define DEBUG(x) {printf x;}
    #else
        #define DEBUG(x) 
    #endif
        
#endif

#ifdef DEBUG_PEER
#define PEER_DEBUG(x) DEBUG(x)
#else
#define PEER_DEBUG(x) 
#endif



#define INSTALL_PANIC_CHECK
#define NO_BOOST_CHARGE_TRAPS
#undef SINK_USB
#define HAVE_VBAT_SEL
#define HAVE_FULL_USB_CHARGER_DETECTION

#endif /*_SINK_DEBUG_H_*/

