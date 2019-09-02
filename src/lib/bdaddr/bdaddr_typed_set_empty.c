/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <bdaddr.h>

void BdaddrTypedSetEmpty(typed_bdaddr *in)
{
    in->type = TYPED_BDADDR_INVALID;
    BdaddrSetZero(&in->addr);
}
