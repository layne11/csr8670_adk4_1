/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
*/

#include "sink_config.h"
#include "sink_debug.h"
#include "sink_leds.h"

#include <ps.h>
#include <string.h>
#include <panic.h>
#include <stdlib.h>
#include <file.h>
#include <stream.h>
#include <print.h>

#ifdef DEBUG_CONFIG
#define CONFIG_DEBUG(x) DEBUG(x)
#else
#define CONFIG_DEBUG(x) 
#endif


/****************************************************************************
NAME 
 	getConfigFilename

DESCRIPTION
 	This function finds the filename of a given config item.
    
RETURNS
 	true if this config can be stored in a file, else false if
         PSKEY only.
    pPskey - the PSKEY base id
    pFilename - config filename, if applicable.
*/
#ifdef ENABLE_FILE_CONFIG
static bool getConfigInfo(uint16 configId, uint16* pPskey, char** pFilename)
{   
    *pPskey = 0;
    *pFilename = NULL;
    
    /*Currently the PSKEY id is the same as the identifier*/    
    *pPskey = configId;       
    
    switch(configId)
    {                           
        case CONFIG_FEATURE_BLOCK:
            *pFilename = "config/features.snk";          
            break;        
        case CONFIG_LENGTHS:
            *pFilename = "config/lengths.snk";          
            break;
        case CONFIG_TIMEOUTS:
            *pFilename = "config/timeouts.snk";          
            break; 
        case CONFIG_DEVICE_ID:
            *pFilename = "config/deviceid.snk";          
            break;              
        case CONFIG_BATTERY:
            *pFilename = "config/battery.snk";
            break;   
        case CONFIG_RADIO:
            *pFilename = "config/radio.snk";          
            break;              
        case CONFIG_VOLUME_CONTROL:
            *pFilename = "config/volume.snk";          
            break;               
        case CONFIG_HFP_INIT:
            *pFilename = "config/hfpinit.snk";          
            break;  
        case CONFIG_ADDITIONAL_HFP_SUPPORTED_FEATURES:
            *pFilename = "config/hfpadd.snk";          
            break;  
        case CONFIG_RSSI_PAIRING:
            *pFilename = "config/rssi.snk";          
            break;               
 	    case CONFIG_PIO:
            *pFilename = "config/pio.snk";          
            break;                
        case CONFIG_BUTTON:
            *pFilename = "config/btncnf.snk";          
            break;         
        case CONFIG_BUTTON_TRANSLATION:
            *pFilename = "config/btntrans.snk";          
            break;              
        case CONFIG_BUTTON_PATTERN_CONFIG:
            *pFilename = "config/btnpat.snk";          
            break;  
        case CONFIG_LED_FILTERS:
            *pFilename = "config/ledfilt.snk";          
            break;              
        case CONFIG_LED_STATES:
            *pFilename = "config/ledstate.snk";          
            break; 
        case CONFIG_TRI_COL_LEDS:
            *pFilename = "config/ledstri.snk";          
            break;             
        case CONFIG_LED_EVENTS:
            *pFilename = "config/ledevent.snk";          
            break;  
        case CONFIG_EVENTS_A:
            *pFilename = "config/events1.snk";          
            break;   
        case CONFIG_EVENTS_B:
            *pFilename = "config/events2.snk";          
            break;   
        case CONFIG_EVENTS_C:
            *pFilename = "config/events3.snk";               
            break;  
        case CONFIG_TONES:
            *pFilename = "config/tones.snk";          
            break;  
        case CONFIG_USER_TONES:
            *pFilename = "config/usrtones.snk";          
            break;     
        case CONFIG_AUDIO_PROMPTS:
            *pFilename = "config/ap.snk";          
            break;   
        case CONFIG_SSR_PARAMS:
            *pFilename = "config/ssr.snk";          
            break;            
        case CONFIG_USB_CONFIG:
            *pFilename = "config/usb.snk";          
            break;   
        case CONFIG_FM_CONFIG:
            *pFilename = "config/fmrx.snk";          
            break;              
        case CONFIG_AT_COMMANDS:
            *pFilename = "config/atcmd.snk";          
            break;           
        case CONFIG_I2S_INIT_CONFIGURATION:
            *pFilename = "config/i2sinit.snk";
            break;
        case CONFIG_I2S_INIT_DATA:
            *pFilename = "config/i2sdata.snk";
            break;
        case CONFIG_INPUT_MANAGER:
            *pFilename = "config/inputmanager.snk";
            break;
        case CONFIG_BLE_REMOTE_CONTROL:
            *pFilename = "config/bleremotecontrol.snk";
            break;
        case CONFIG_IR_REMOTE_CONTROL:
            *pFilename = "config/irremotecontrol.snk";
            break;
        case CONFIG_AUDIO_ROUTING:
            *pFilename = "config/audio_routing.snk";
            break;
        case CONFIG_PMU_MONITOR_CONFIG:
            *pFilename = "config/pmu_monitor.snk";
            break;
        /* Runtime data exception list */
		case CONFIG_SESSION_DATA:
        case CONFIG_SUBWOOFER:
        case CONFIG_PHONE_NUMBER:   
        case CONFIG_FM_FREQUENCY_STORE:
        default:
            /* This config is never stored in a file */
            return FALSE;
    }           
    
    return TRUE;
}    
#endif


/****************************************************************************
NAME 
 	configCopy

DESCRIPTION
 	This function copies data from the src to the dest, packing bytes into
    words as the data is read from the file stream in this way.
 
RETURNS
 	void
*/
#ifdef ENABLE_FILE_CONFIG 
static uint16 configCopy(uint16 *dest, const uint8 *src, uint16 size_t)
{
      uint16 srcIndex =0;
      uint16 destIndex =0;    
      
      for(destIndex =0; destIndex < size_t; destIndex++)
      {
          dest[destIndex] = MAKEWORD(src[srcIndex+1],src[srcIndex]);
          srcIndex+=2;
      }
      
      return destIndex;
}
#endif

/****************************************************************************
NAME 
    ConfigRetrieve

DESCRIPTION
    This function is called to read a configuration key.  If the key exists
    in persistent store it is read from there.  If it does not exist then
    the default is read from constant space

    The len parameter should match the value given by sizeof(). On the XAP 
    processor this will be in the unit of words.
 
RETURNS
    0 if no data was read otherwise the length of data in words. If comparing
    against a sizeof() then the sizeof() value should be converted to words
    using the PS_SIZE_ADJ() macro
*/
uint16 ConfigRetrieve(uint16 config_id, void* data, uint16 len)
{
    uint16 ret_len = 0;
    uint16 requested_words;
#ifdef ENABLE_FILE_CONFIG   
    bool inFile = FALSE;  
    FILE_INDEX index=FILE_NONE; 
    Source confSource = NULL;
    char* pFileName = NULL ;
    uint16 pskeyId = 0;
#endif

    CONFIG_DEBUG(("CONF: kevcvy_id[%u]\n",config_id));

#ifdef HYDRACORE
    if ((unsigned)data & 0x1)
    {
        Panic();    /* Address is not aligned */
    }
#endif

    requested_words = PS_SIZE_ADJ(len);

#ifdef ENABLE_FILE_CONFIG   
    /* If file config in use then find if this config can be stored in a file */
    inFile = getConfigInfo(config_id,&pskeyId,&pFileName);
#ifndef ENABLE_PSKEY_CONFIG   
    /* If File but not PS config is in use check if this is a key that is always 
       stored in PS */
    if(!inFile)  
#endif    
#endif
    {     
        uint16 key_len = PsRetrieve(config_id, NULL, 0);
        /* read from PS */
        CONFIG_DEBUG(("CONF:PS config_id[%u]\n", config_id)) ;	            
        /* Read requested key from PS if it exists */

        if (key_len != requested_words)
        {
            CONFIG_DEBUG(("Key Len Mismatch for USR%d. Want %d, have %d",config_id,requested_words,key_len));
        }

        ret_len = PsRetrieve(config_id, (uint16 *)data, requested_words);
            
        /* Check if the pskey was read */
        if(!ret_len)
        {
            /* No PSKEY exists */
            CONFIG_DEBUG(("CONF:No PSKEY[%u]\n",config_id)) ;	
        }
    }    
    
#ifdef ENABLE_FILE_CONFIG          
    if(inFile && !ret_len)
    {         
         /* see if the config file actually exists */
        index = FileFind(FILE_ROOT,(const char *) pFileName ,strlen(pFileName));   
                
        if(index != FILE_NONE) 
        {
            /* read from file if found */
            const uint8 *pConfFileData = NULL;
            
            /* try and find the config file */
            confSource = StreamFileSource(index);        
            
            /* get size of config file, note this returns size in Bytes */
            ret_len = SourceSize(confSource);     
            
            CONFIG_DEBUG(("CONF:File[%s]type[%x]key_id[%u]size[0x%x]\n",pFileName,FileType(index), config_id,ret_len)) ;	                         
            
            /* Check if config file size OK and copy data.
             * Note that the file API works in octets, not words 
             * we use len rather than requested_words in case happens to be odd */
            if(ret_len >= len)
            {            
                pConfFileData = (void*)SourceMap(confSource);
                if(pConfFileData)
                {
                    ret_len = configCopy(data, pConfFileData, requested_words);
                }
                else
                {
                    /* error, failed to map file */
                    ret_len = 0;
                }
            }
            else
            {
                /* error, file wrong size */
                ret_len = 0;
            }
            
            SourceClose(confSource);
        }
        else
        {
            CONFIG_DEBUG(("CONF:File[%s] not found\n",pFileName)) ;	  
        }
    }
#endif    

    /* check data length */
     switch(config_id)
     {
            /* Config where it's ok for (ret_len != len) 
                or where it doesn't matter if it exists */
            case CONFIG_DEVICE_ID:
            case CONFIG_USER_TONES:
            case CONFIG_SESSION_DATA:
            case CONFIG_SUBWOOFER:
            case CONFIG_PHONE_NUMBER:
            case CONFIG_FM_FREQUENCY_STORE:
            case CONFIG_IR_REMOTE_CONTROL_LEARNED_CODES:
            case CONFIG_SQIF_PARTITIONS:
            case CONFIG_SOFTWARE_VERSION_ID:
            case CONFIG_BLE_DIS_MANUFACTURER_NAME:
                break;
            /* Config where it's ok for zero length */
            case CONFIG_RADIO:
                if (ret_len == 0)
                    break;
            default:
                if (ret_len != requested_words)
                {
                     PRINT(("CONF:BADLEN![%u][0x%x][0x%x]\n",config_id, ret_len, requested_words)) ;
                     LedsIndicateError(config_id) ;
                }
                break;
     } 

 
 	return ret_len;
}

/****************************************************************************
NAME 
    ConfigStore

DESCRIPTION
    This function is called to store a configuration key.  This will always
    in PS Store

    The len parameter should match the value given by sizeof(). On the XAP 
    processor this will be in the unit of words. If comparing against a sizeof() 
    then the sizeof() value should be converted to words using the PS_SIZE_ADJ() 
    macro
 
RETURNS
    0 if no data was stored otherwise the length of data in words
*/
uint16 ConfigStore(uint16 config_id, const void *data, uint16 len)
{
    uint16 storeLen = 0;

#ifdef HYDRACORE
    if ((unsigned)data & 0x1)
    {
        Panic();    /* Address is not aligned */
    }
#endif

    /* stored keys */
    storeLen = PsStore(config_id, (const uint16 *)data, PS_SIZE_ADJ(len));
    CONFIG_DEBUG(("CONF:Stored[%u]len[0x%x]words\n",config_id, storeLen)) ;
    return storeLen;
}


void ConfigIndicateConfigError(const uint16 config_id)
{
    LedsIndicateError(config_id);
}

