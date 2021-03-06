/****************************************************************************

Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd. 

FILE NAME
    sink_hf_indicators.c

DESCRIPTION
    Handles HFP indicators to be sent to AG

NOTES

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_hf_indicators.h"

#ifdef TEST_HF_INDICATORS
/****************************************************************************
NAME    
    hfIndicatorNotify
    
DESCRIPTION
    Interprets the HF Indicator messages to be sent by HF to AG

RETURNS
    void
*/
void hfIndicatorNotify(hfp_indicators_assigned_id id, uint16 value)
{    
   if(id == hf_enhanced_safety)
   {
      if(theSink.hf_enhancedSafety)
         theSink.hf_enhancedSafety = FALSE;
      else
         theSink.hf_enhancedSafety = TRUE;	

      value = theSink.hf_enhancedSafety;
   }

    /* Send the HF indicator value on primary link */
    hfIndicatorTrigger(hfp_primary_link, id, value);
	
    /* Send the HF indicator value on secondary link also */
    hfIndicatorTrigger(hfp_secondary_link, id, value);	
}
#endif /* TEST_HF_INDICATORS */

/****************************************************************************
NAME    
    hfIndicatorTrigger
    
DESCRIPTION
    Send HF Indicator messages to hfp lib to be sent to AG if indicator is enabled

RETURNS
    void
*/
void hfIndicatorTrigger(hfp_link_priority priority, hfp_indicators_assigned_id id, uint16 value)
{
    HfpBievIndStatusRequest(priority, id, value);
}

