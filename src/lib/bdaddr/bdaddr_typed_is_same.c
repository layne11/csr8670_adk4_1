/* Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <bdaddr.h>

bool BdaddrTypedIsSame(const typed_bdaddr *first, const typed_bdaddr *second)
{
    return  first->type == second->type && 
            BdaddrIsSame(&first->addr, &second->addr);
}
