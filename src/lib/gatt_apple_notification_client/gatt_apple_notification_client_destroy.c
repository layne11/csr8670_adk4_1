/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <string.h>
#include <stdio.h>

#include "gatt_apple_notification_client_private.h"

#include "gatt_apple_notification_client_msg_handler.h"


/****************************************************************************/
gatt_ancs_status_t GattAncsDestroy(GANCS *ancs)
{
    gatt_ancs_status_t result = gatt_ancs_status_invalid_parameter;
    /* Check parameters */
    if (ancs != NULL)
    {
        /* Register with the GATT Manager and verify the result */
        if (GattManagerUnregisterClient(&ancs->lib_task) == gatt_manager_status_success)
        {
            result = gatt_ancs_status_success;
        }
        else
        {
            result = gatt_ancs_status_failed;
        }
        
        /* Clear pending messages */
        MessageFlushTask(&ancs->lib_task);
    }

    return result;
}

