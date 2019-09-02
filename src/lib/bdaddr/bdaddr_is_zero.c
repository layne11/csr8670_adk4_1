/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <bdaddr.h>

bool BdaddrIsZero(const bdaddr *in)
{ 
    return !in->nap && !in->uap && !in->lap; 
}
