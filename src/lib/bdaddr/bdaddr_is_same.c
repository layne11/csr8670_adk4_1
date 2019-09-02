/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <bdaddr.h>

bool BdaddrIsSame(const bdaddr *first, const bdaddr *second)
{ 
    return  first->nap == second->nap && 
            first->uap == second->uap && 
            first->lap == second->lap; 
}
