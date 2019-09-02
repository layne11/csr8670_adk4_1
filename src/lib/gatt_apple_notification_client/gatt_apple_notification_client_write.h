/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_WRITE_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_WRITE_H_


#include <gatt_manager.h>

#include "gatt_apple_notification_client.h"
#include "gatt_apple_notification_client_private.h"


/***************************************************************************
NAME
    makeAncsWriteCPCfmMsg

DESCRIPTION
    The function sends the message to application in case of CP write error.
*/
void makeAncsWriteCPCfmMsg(GANCS *ancs, gatt_ancs_status_t status);

/***************************************************************************
NAME
    writeClientConfigValue

DESCRIPTION
    Write Client Configuration descriptor value by handle.
*/
bool writeClientConfigNotifyValue(GANCS *ancs, bool notifications_enable, uint16 handle);

/***************************************************************************
NAME
    ancsWriteGetAttrValue

DESCRIPTION
    Write Control Point Characteristic value by handle.
*/
bool ancsWriteCharValue(GANCS *ancs, const ANCS_INTERNAL_MSG_WRITE_CP_CHARACTERISTIC_T* req,
                                                          uint16 handle);

/***************************************************************************
NAME
    handleAncsWriteValueResp

DESCRIPTION
    Handle response as a result of writing a characteristic value.
*/
void handleAncsWriteValueResp(GANCS *ancs, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm);


#endif /* GATT_APPLE_NOTIFICATION_CLIENT_WRITE_H_ */
