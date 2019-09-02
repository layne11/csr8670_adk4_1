/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_


#define ANCS_NOTIFICATION_VALUE 0x0001
#define ANCS_INDICATION_VALUE 0x0002


/***************************************************************************
NAME
    makeANCSSetNSNotificationCfmMsg

DESCRIPTION
    Sends a GATT_ANCS_SET_NS_NOTIFICATION_CFM message to the application task.
*/
void makeAncsSetNotificationCfmMsg(GANCS *ancs, gatt_ancs_status_t status, uint8 characteristic);

/***************************************************************************
NAME
    handleAncsNotification

DESCRIPTION
    Handles the internal GATT_MANAGER_NOTIFICATION_IND message.
*/
void handleAncsNotification(GANCS *ancs, const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind);


/***************************************************************************
NAME
    ancsSetNSNotificationRequest

DESCRIPTION
    Handles the internal ANCS_INTERNAL_MSG_SET_xx_NOTIFICATION message.
*/
void ancsSetNotificationRequest(GANCS *ancs, bool notifications_enable, uint8 characteristic);

#endif /* GATT_APPLE_NOTIFICATION_CLIENT_NOTIFICATION_H_ */
