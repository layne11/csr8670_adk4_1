/* Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <bdaddr.h>

bool BdaddrTpIsSame(const tp_bdaddr *first, const tp_bdaddr *second)
{
    return  first->transport == second->transport && 
            BdaddrTypedIsSame(&first->taddr, &second->taddr);
}
