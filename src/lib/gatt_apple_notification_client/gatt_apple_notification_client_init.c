/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <string.h>
#include <stdio.h>

#include "gatt_apple_notification_client_private.h"

#include "gatt_apple_notification_client_discover.h"
#include "gatt_apple_notification_client_msg_handler.h"


/****************************************************************************/
gatt_ancs_status_t GattAncsInit(GANCS *ancs, Task app_task,
                                                    uint16 cid, uint16 start_handle, uint16 end_handle)
{
    gatt_ancs_status_t result = gatt_ancs_status_invalid_parameter;

    /* Check parameters */
    if ((app_task != NULL) && (ancs != NULL))
    {
        gatt_manager_client_registration_params_t registration_params;
        
        /* Set memory contents to all zeros */
        memset(ancs, 0, sizeof(GANCS));
        
        /* Set up library handler for external messages */
        ancs->lib_task.handler = appleNotificationClientMsgHandler;
        
        /* Store the Task function parameter.
           All library messages need to be sent here */
        ancs->app_task = app_task;

        /* Reset all other handles */
        ancs->notification_source = APPLE_NOTIFICATION_INVALID_HANDLE;
        ancs->data_source = APPLE_NOTIFICATION_INVALID_HANDLE;
        ancs->control_point = APPLE_NOTIFICATION_INVALID_HANDLE;

        /* Reset all Client Configuration handles */
        ancs->ns_ccd = APPLE_NOTIFICATION_INVALID_HANDLE;
        ancs->ds_ccd = APPLE_NOTIFICATION_INVALID_HANDLE;
        
        /* Setup data required for Battery Service to be registered with the GATT Manager */
        registration_params.client_task = &ancs->lib_task;
        registration_params.cid = cid;
        registration_params.start_handle = start_handle;
        registration_params.end_handle = end_handle;
        
        /* Register with the GATT Manager and verify the result */
        if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
        {
            /* Discover all characteristics after successful registration */
            result = gatt_ancs_status_initiating;
            GattManagerDiscoverAllCharacteristics(&ancs->lib_task);
            ancs->pending_cmd = ancs_pending_discover_all_characteristics;
        }
        else
        {
            result = gatt_ancs_status_failed;
        }
    }

    return result;
}
