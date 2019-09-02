/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    hfp_common.c

DESCRIPTION
    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_common.h"
#include "hfp_link_manager.h"

#include <panic.h>


/*****************************************************************************/
ptr hfpSkipSpace(ptr p, ptr end_of_string)
{
    if(p)
    {
        while(p != end_of_string && (*p == ' ' || *p == ',' || *p == ';'))
            ++p;
    }
    return p;
}


/*****************************************************************************/
static __inline__ char toUpper(char c)
{
    return (char)('a' <= c && c <= 'z' ? c +'A'-'a' : c);
}


/*****************************************************************************/
ptr hfpMatchCharInString(ptr p, ptr end_of_string, char ch)
{
    return p && p != end_of_string && toUpper(*p) == toUpper(ch) ? p+1 : 0;
}


/*****************************************************************************/
ptr hfpFindAndSkipMatchingCharInString(ptr p, ptr end_of_string, char ch)
{
    if(p)
        while(p != end_of_string)
            if(*p++ == (uint8)ch)
                return p;
    return 0;
}


/*****************************************************************************/
void hfpSendCommonCfmMessageToApp(MessageId message_id, const hfp_link_data* link, hfp_lib_status status)
{
    MAKE_HFP_MESSAGE(HFP_COMMON_CFM_MESSAGE);
    message->priority = hfpGetLinkPriority(link);
    message->status = status;
    MessageSend(theHfp->clientTask, message_id, message);
}


/*****************************************************************************/
void hfpSendCommonInternalMessage(MessageId message_id, hfp_link_data* link)
{
    MAKE_HFP_MESSAGE(HFP_COMMON_INTERNAL_MESSAGE);
    message->link = link;
    MessageSend(&theHfp->task, message_id, message);
}


/*****************************************************************************/
void hfpSendCommonInternalMessagePriority(MessageId message_id, hfp_link_priority priority)
{
    MAKE_HFP_MESSAGE(HFP_COMMON_INTERNAL_MESSAGE);
    message->link = hfpGetLinkFromPriority(priority);
    MessageSend(&theHfp->task, message_id, message);
}


/*****************************************************************************/
hfp_number_type hfpConvertNumberType( uint16 type )
{
    switch ( type & 0x70 )
    {
    case 0x00:
        return hfp_number_unknown;
        
    case 0x10:
        return hfp_number_international;
        
    case 0x20:
        return hfp_number_national;
        
    case 0x30:
        return hfp_number_network;
        
    case 0x40:
        return hfp_number_dedicated;
        
    default:
        return hfp_number_unknown;
    }
}


/*****************************************************************************/
bool supportedProfileIsHfp(hfp_profile profile)
{
    if (profile & hfp_handsfree_all)
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************************/
bool supportedProfileIsHfp107(hfp_profile profile)
{
    if (profile & hfp_handsfree_107_profile)
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************************/
bool supportedProfileIsHsp(hfp_profile profile)
{
    if (profile & hfp_headset_all)
        return TRUE;
    else
        return FALSE;
}
