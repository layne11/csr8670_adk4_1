/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_apple_notification_client_private.h"

#include "gatt_apple_notification_client_notification.h"
#include "gatt_apple_notification_client_discover.h"
#include "gatt_apple_notification_client_write.h"


/****************************************************************************
Internal functions
****************************************************************************/

/****************************************************************************/
void makeAncsSetNotificationCfmMsg(GANCS *ancs, gatt_ancs_status_t status, uint8 characteristic)
{
    switch(characteristic)
    {
        case GATT_APPLE_NOTIFICATION_NS:
        {
           MAKE_APPLE_NOTIFICATION_MESSAGE(GATT_ANCS_SET_NS_NOTIFICATION_CFM);
            memset(message, 0, sizeof(GATT_ANCS_SET_NS_NOTIFICATION_CFM_T));
            message->ancs = ancs;
            message->status = status;
            MessageSend(ancs->app_task, GATT_ANCS_SET_NS_NOTIFICATION_CFM, message);
            
            /* No command pending */
            ancs->pending_cmd = ancs_pending_none;
        }
        break;
        case GATT_APPLE_NOTIFICATION_DS:
        {
            MAKE_APPLE_NOTIFICATION_MESSAGE(GATT_ANCS_SET_DS_NOTIFICATION_CFM);
            memset(message, 0, sizeof(GATT_ANCS_SET_DS_NOTIFICATION_CFM_T));
            message->ancs = ancs;
            message->status = status;
            MessageSend(ancs->app_task, GATT_ANCS_SET_DS_NOTIFICATION_CFM, message);
            
            /* No command pending */
            ancs->pending_cmd = ancs_pending_none;
        }
        break;
        default:
        break;
    }
}

/***************************************************************************/
void handleAncsNotification(GANCS *ancs, const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind)
{
    /* first validate the payload */
    if (ind->size_value) 
    {
        /* This is notification for NS */
        if(ind->handle == ancs->notification_source)
        {
            /* Check if application has set interest in this category */
            if(CHECK_CATEGORY(ancs->ns_notification_mask, ind->value[2]))
            {
                uint32 notUID = 0;
                /* Send the Apple notification to the application */
                MAKE_APPLE_NOTIFICATION_MESSAGE(GATT_ANCS_NS_IND);
                message->ancs = ancs;
                /* The first byte has the EventId */
                message->event_id = ind->value[0];
                message->event_flag = ind->value[1];
                message->category_id = ind->value[2];
                message->category_count = ind->value[3];
                notUID = (uint32)ind->value[4] << 24;
                notUID |= ((uint32)ind->value[5] << 16);
                notUID |= ((uint32)ind->value[6] << 8);
                notUID |= ind->value[7];

                message->notification_uid = notUID;
                MessageSend(ancs->app_task, GATT_ANCS_NS_IND, message);
            }
            /* Else ignore the unwanted category notification */
        }
        /* This is notification for DS */
       /* NOTE: According to Apple Notification Service Client Specification Version:1.1
        * " As with a response to a Get Notification Attributes command, if the response to a Get App Attributes command
            is larger than the negotiated GATT Maximum Transmission Unit (MTU), it is split into multiple fragments by
            the NP. The NC must recompose the response by splicing each fragment. The response is complete when the
            complete tuples for each requested attribute has been received. "
            Here the parsing is done with the assumtion that each of the multiple fragmented packet shall still have the 
            header information like command_id, App Identifier or command_id, NotificationUID followed by the fragmented part.
            This assumption has to be taken because, none of the packet tells the total length of the entire indication.
            */
        else if(ind->handle == ancs->data_source)
        {
            /* Reset the pending write CP request only if its required */
            if((ancs->pending_cmd == ancs_pending_write_cp_attr) || 
               (ancs->pending_cmd == ancs_pending_write_cp_app_attr))
           {
               makeAncsWriteCPCfmMsg(ancs, gatt_ancs_status_success);
           }
            /* The notification can be either for attribute or application attribute */
            switch(ind->value[0])
            {
                case gatt_ancs_notification_attr:
                {
                     uint32 notUID = 0;
                    /* Send the Apple notification to the application */
                    MAKE_APPLE_NOTIFICATION_MESSAGE_WITH_LEN(GATT_ANCS_DS_ATTR_IND, (ind->size_value - 5));
                    message->ancs = ancs;
                    notUID = (uint32)ind->value[1] << 24;
                    notUID |= ((uint32)ind->value[2] << 16);
                    notUID |= ((uint32)ind->value[3] << 8);
                    notUID |= ind->value[4];
                    message->notification_uid = notUID;
                    /* First 5 bytes consist of command_id and Notification UID */
                    message->size_value = (ind->size_value - 5);
                    memcpy(message->value, &ind->value[5], message->size_value);
                    MessageSend(ancs->app_task, GATT_ANCS_DS_ATTR_IND, message);
                }
                break;

                case gatt_ancs_notification_app_attr:
                {
                    uint16 appIdLen = 0;
                    /* Send the Apple notification to the application */
                    MAKE_APPLE_NOTIFICATION_MESSAGE_WITH_LEN(GATT_ANCS_DS_APP_ATTR_IND, (ind->size_value - 1));
                    message->ancs = ancs;
                    while((ind->value[appIdLen+1] != 0) && (appIdLen != 0xffff))
                        appIdLen ++;
                    /* if not invalid length, copy the name */
                    if(appIdLen != 0xffff)
                    {
                        /* Copy the application identifier including the NULL character */
                        memcpy(&message->value[0], &ind->value[1], (appIdLen + 1));
                        /* First byte is the command Id */
                        message->size_value = (ind->size_value - 1);
                        memcpy(&message->value[appIdLen + 1], &ind->value[appIdLen + 2], message->size_value - appIdLen - 1);
                        MessageSend(ancs->app_task, GATT_ANCS_DS_APP_ATTR_IND, message);
                    }
                    else
                    {
                        /* Invalid indication, free the allocated memory */
                        free(message);
                    }
                }
                break;

                default:
                break;
            }
        }
    }
}

/****************************************************************************/
void ancsSetNotificationRequest(GANCS *ancs, bool notifications_enable, uint8 characteristic)
{
    bool success = FALSE;
    uint16 endHandle = 0;
    uint16 startHandle = 0;
    gatt_ancs_status_t status = gatt_ancs_status_failed;
    gatt_manager_client_service_data_t client_data;

    client_data.start_handle = APPLE_NOTIFICATION_INVALID_HANDLE;
    client_data.end_handle = APPLE_NOTIFICATION_INVALID_HANDLE;
    if (!GattManagerGetClientData(&ancs->lib_task, &client_data))
    {
        GATT_APPLE_NOTIFICATION_PANIC(("ANCS: Could not get client data\n"));
    }
    
    switch(characteristic)
    {
        case GATT_APPLE_NOTIFICATION_NS:
        {
            if (CHECK_VALID_HANDLE(ancs->notification_source))
            {
                /* First check if ccd handle is found, else find it */
                if(!CHECK_VALID_HANDLE(ancs->ns_ccd))
                {
                    startHandle = ((ancs->notification_source + 1) > client_data.end_handle) ? client_data.end_handle : (ancs->notification_source + 1);
                    endHandle = findEndHandleForCharDesc(ancs, startHandle, client_data.end_handle, GATT_APPLE_NOTIFICATION_NS);
                    /* Find client configuration descriptor */
                    if(startHandle && endHandle)
                    {
                        success = ancsDiscoverAllCharacteristicDescriptors(ancs, startHandle, endHandle);
                    }
                }
                else
                {
                     if (writeClientConfigNotifyValue(ancs, notifications_enable, ancs->ns_ccd))
                     {
                        ancs->pending_cmd = ancs_pending_write_ns_cconfig;
                        return;
                     }
                }
            }
        }
        break;

        case GATT_APPLE_NOTIFICATION_DS:
        {
            if (CHECK_VALID_HANDLE(ancs->data_source))
            {
                /* First check if the ccd handle is found, else find it */
                if(!CHECK_VALID_HANDLE(ancs->ds_ccd))
                {
                    startHandle = ((ancs->data_source + 1) > client_data.end_handle) ? client_data.end_handle : (ancs->data_source + 1);
                    endHandle = findEndHandleForCharDesc(ancs, startHandle, client_data.end_handle, GATT_APPLE_NOTIFICATION_DS);
                    /* Find client configuration descriptor */
                    if(startHandle && endHandle)
                    {
                        success = ancsDiscoverAllCharacteristicDescriptors(ancs, startHandle, endHandle);
                    }
                }
                else
                {
                    if (writeClientConfigNotifyValue(ancs, notifications_enable, ancs->ds_ccd))
                    {
                        ancs->pending_cmd = ancs_pending_write_ds_cconfig;
                        return;
                    }
                }
            }
            else
            {
                /* Data Source Characteristics are not available. That's fine, because Apple Specification does
                 * not mandate this charateristic. Respond to app that remote doesn't support it */
                 status = gatt_ancs_status_not_supported;
            }
        }
        break;

        default:
            /* This is not a desirable chareteristic for notification */
            GATT_APPLE_NOTIFICAITON_DEBUG_INFO(("Not a valid characteristic for notifications"));
        break;
    }

    if (success)
    {
        if (notifications_enable)
        {
            ancs->pending_cmd = (characteristic == GATT_APPLE_NOTIFICATION_NS) ? ancs_pending_set_ns_notify_enable :
            ancs_pending_set_ds_notify_enable;
        }
        else
        {
            ancs->pending_cmd = (characteristic == GATT_APPLE_NOTIFICATION_NS) ? ancs_pending_set_ns_notify_disable :
            ancs_pending_set_ds_notify_disable;
        }
    }
    else
    {
        makeAncsSetNotificationCfmMsg(ancs, status, characteristic);
    }
}

/****************************************************************************
Public API
****************************************************************************/


/****************************************************************************/
void GattAncsSetNSNotificationEnableRequest(const GANCS *ancs,
                                                                                    bool notifications_enable,
                                                                                    uint16 notification_mask)
{
    MAKE_APPLE_NOTIFICATION_MESSAGE(ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION);
    memset(message, 0, sizeof(ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION_T));
    message->notifications_enable = notifications_enable;
    /* Updating the category masks if notification has to be enabled/disabled */
    message->notifications_mask = notification_mask;

    MessageSendConditionally((Task)&ancs->lib_task,
                                            ANCS_INTERNAL_MSG_SET_NS_NOTIFICATION,
                                            message,
                                            &ancs->pending_cmd);
}


/****************************************************************************/
void GattAncsSetDSNotificationEnableRequest(const GANCS *ancs,
                                                                                    bool notifications_enable)
{
    MAKE_APPLE_NOTIFICATION_MESSAGE(ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION);
    memset(message, 0, sizeof(ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION_T));
    message->notifications_enable = notifications_enable;
    MessageSendConditionally((Task)&ancs->lib_task,
                                            ANCS_INTERNAL_MSG_SET_DS_NOTIFICATION,
                                            message,
                                            &ancs->pending_cmd);
}

