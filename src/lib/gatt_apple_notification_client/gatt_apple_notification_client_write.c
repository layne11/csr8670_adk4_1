/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_apple_notification_client_private.h"

#include "gatt_apple_notification_client_write.h"
#include "gatt_apple_notification_client_notification.h"

void makeAncsWriteCPCfmMsg(GANCS *ancs, gatt_ancs_status_t status)
{

    MAKE_APPLE_NOTIFICATION_MESSAGE(GATT_ANCS_WRITE_CP_CFM);
    memset(message, 0, sizeof(GATT_ANCS_WRITE_CP_CFM_T));
    message->ancs = ancs;
    message->status = status;
    if(ancs->pending_cmd == ancs_pending_write_cp_attr)
    {
        message->command_id = gatt_ancs_notification_attr;
    }
    else if(ancs->pending_cmd == ancs_pending_write_cp_app_attr)
    {
        message->command_id = gatt_ancs_notification_app_attr;
    }
    else
    {
        message->command_id = gatt_ancs_notification_action;
    }

    MessageSend(ancs->app_task, GATT_ANCS_WRITE_CP_CFM, message);

    /* No command pending */
    ancs->pending_cmd = ancs_pending_none;
}

/*******************************************************************************
 * Helper function to map gatt_status_t to apple notification specific error code
 */
static gatt_ancs_status_t get_ancs_error_code(gatt_status_t gatt_status)
{
    switch((uint8)gatt_status)
    {
        case gatt_status_success: return gatt_ancs_status_success;
        case 0xa0:return gatt_ancs_status_unknown_command;
        case 0xa1:return gatt_ancs_status_invalid_command;
        case 0xa2:return gatt_ancs_status_invalid_parameter;
        case 0xa3:return gatt_ancs_status_action_failed;
        default: return gatt_ancs_status_failed;
    }
}

/*******************************************************************************
 * Helper function to handle a response to a write of the client configuration descriptor.
 */
static void handleWriteClientConfigResp(GANCS *ancs, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm)
{
    switch(ancs->pending_cmd)
    {
        case ancs_pending_write_ns_cconfig:
        {
            makeAncsSetNotificationCfmMsg(ancs, get_ancs_error_code(write_cfm->status),
                                                              GATT_APPLE_NOTIFICATION_NS);
        }
        break;

        case ancs_pending_write_ds_cconfig:
        {
            makeAncsSetNotificationCfmMsg(ancs, get_ancs_error_code(write_cfm->status),
                                                              GATT_APPLE_NOTIFICATION_DS);
        }
        break;

        default:
        break;
    }
}

/*******************************************************************************
 * Helper function to handle a response to a write of the Control Point Characteristic.
 */
static void handleWriteControlPointResp(GANCS *ancs, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm)
{
    makeAncsWriteCPCfmMsg(ancs,get_ancs_error_code(write_cfm->status));
}

static bool writeClientValue(GANCS *ancs, uint16 handle, uint16 size_value, const uint8* value)
{
    GattManagerWriteCharacteristicValue((Task)&ancs->lib_task, handle, size_value, value);
    return TRUE;
}

/****************************************************************************/
bool writeClientConfigNotifyValue(GANCS *ancs, bool notifications_enable, uint16 handle)
{
    uint8 value[2];
    
    value[1] = 0;
    value[0] = notifications_enable ? ANCS_NOTIFICATION_VALUE : 0;
    
    return writeClientValue(ancs, handle, sizeof(value), value);
}

/****************************************************************************/
bool ancsWriteCharValue(GANCS *ancs, const ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC_T* req,
                                                         uint16 handle)
{
    return writeClientValue(ancs, handle, req->size_command_data, req->command_data);
}

/****************************************************************************/
void handleAncsWriteValueResp(GANCS *ancs, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm)
{
    switch (ancs->pending_cmd)
    {
        case ancs_pending_write_ns_cconfig:
        case ancs_pending_write_ds_cconfig:
        {
            handleWriteClientConfigResp(ancs, write_cfm);
        }
        break;

        case ancs_pending_write_cp_attr:
        case ancs_pending_write_cp_app_attr:
        {
            handleWriteControlPointResp(ancs, write_cfm);
        }
        break;

        default:
        {
            /* No other pending write values expected */
            GATT_APPLE_NOTIFICATION_DEBUG_PANIC(("ANCS: Write value response not expected [0x%x]\n", ancs->pending_cmd));
        }
        break;
    }
}

/****************************************************************************
Public API
****************************************************************************/

/****************************************************************************/
void GattAncsGetNotificationAttributes(const GANCS *const ancs, 
                                       uint32 notification_uid, 
                                       uint16 size_attribute_list, 
                                       uint8 *attribute_list)
{
    /* First find the size of attr list */
    MAKE_APPLE_NOTIFICATION_MESSAGE_WITH_LEN(ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC, CALCULATE_SIZEOF_GET_NOTIFICATION_ATTRIBUTES(size_attribute_list));
    message->size_command_data = CALCULATE_SIZEOF_GET_NOTIFICATION_ATTRIBUTES(size_attribute_list);
    message->command_data[0] = gatt_ancs_notification_attr;
    message->command_data[4] = notification_uid & 0xFF;
    message->command_data[3] = (notification_uid >> 8) & 0xFF;
    message->command_data[2] = (notification_uid >> 16) & 0xFF;
    message->command_data[1] = (notification_uid >> 24) & 0xFF;
    memcpy(&message->command_data[5], attribute_list, size_attribute_list);
    message->pending_cmd = ancs_pending_write_cp_attr;
    MessageSendConditionally((Task)&ancs->lib_task, ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC, message, &ancs->pending_cmd);
}

/****************************************************************************/
void GattAncsGetAppAttributes(const GANCS *const ancs, 
                              uint16 size_app_id, 
                              uint8 * app_id, 
                              uint16 size_attribute_list, 
                              uint8 *attribute_list)
{
    /* Only if valid app_id */
    if(size_app_id)
    {
        MAKE_APPLE_NOTIFICATION_MESSAGE_WITH_LEN(ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC, CALCULATE_SIZEOF_GET_APP_ATTRIBUTES(size_app_id, size_attribute_list));
        message->size_command_data = CALCULATE_SIZEOF_GET_APP_ATTRIBUTES(size_app_id, size_attribute_list);
        message->command_data[0] = gatt_ancs_notification_app_attr;
        memcpy(&message->command_data[1], app_id, size_app_id);
        memcpy(&message->command_data[1+size_app_id], attribute_list, size_attribute_list);
        message->pending_cmd = ancs_pending_write_cp_app_attr;
        MessageSendConditionally((Task)&ancs->lib_task, ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC, message, &ancs->pending_cmd);
    }
}

void GattAncsPerformNotificationAction(const GANCS *const ancs, 
                                       uint32 notification_uid, 
                                       gatt_ancs_action_id action_id)
{
    MAKE_APPLE_NOTIFICATION_MESSAGE_WITH_LEN(ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC, CALCULATE_SIZEOF_PERFORM_NOTIFICATION_ACTION);
    message->size_command_data = CALCULATE_SIZEOF_PERFORM_NOTIFICATION_ACTION;
    message->command_data[0] = gatt_ancs_notification_action;
    message->command_data[4] = notification_uid & 0xFF;
    message->command_data[3] = (notification_uid >> 8) & 0xFF;
    message->command_data[2] = (notification_uid >> 16) & 0xFF;
    message->command_data[1] = (notification_uid >> 24) & 0xFF;
    message->command_data[5] = action_id;
    message->pending_cmd = ancs_pending_none;
    MessageSendConditionally((Task)&ancs->lib_task,
                                            ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC,
                                            message,
                                            &ancs->pending_cmd);
}
