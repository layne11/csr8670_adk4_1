/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    aghfp_link_manager.c
    
DESCRIPTION
    Handles the link mode information for Secure connection.
	
*/

#include "aghfp.h"
#include "aghfp_private.h"


/*! @brief Enables/disables secure connection on the existing given link.

	@param aghfp A pointer to the profile instance.
    @param sc_enable Whether the link is secure or not.
*/
void AghfpLinkSetLinkMode(AGHFP *aghfp, bool sc_enable)
{
    /* Secure connection is applicable for hfp1.5 and above. Errata:6835 raised which will make
	SC applicable for hfp1.7 and above if approved */
    if(aghfp->supported_profile  >= aghfp_handsfree_15_profile)
    {
       aghfp->link_mode_secure = sc_enable;
    }
    else
    {
       aghfp->link_mode_secure = FALSE;
    }
}

