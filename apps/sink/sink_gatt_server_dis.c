/****************************************************************************
Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_dis.c

DESCRIPTION
    Routines to handle messages sent from the GATT Device Information Server Task.
*/


/* Firmware headers */
#include <csrtypes.h>
#include <message.h>

/* Application headers */
#include "sink_gatt_db.h"
#include "sink_gatt_server_dis.h"

#include "sink_debug.h"
#include "sink_gatt_server.h"
#include "sink_private.h"
#include "sink_config.h"

#ifdef GATT_DIS_SERVER


#ifdef DEBUG_GATT
#define GATT_DEBUG(x) DEBUG(x)
#else
#define GATT_DEBUG(x) 
#endif

static gatt_dis_init_params_t dis_init_params;

/*******************************************************************************/
static bool sinkGattGetDeviceInfoParams(void)
{
    uint16 str_len = 0;

    dis_init_params.dis_strings = malloc(sizeof(gatt_dis_strings_t));

    if(dis_init_params.dis_strings != NULL)
    {
        memset(dis_init_params.dis_strings, 0, sizeof(gatt_dis_strings_t));

        dis_init_params.dis_strings->manufacturer_name_string = NULL;
        dis_init_params.dis_strings->model_num_string = NULL;
        dis_init_params.dis_strings->serial_num_string = NULL;
        dis_init_params.dis_strings->hw_revision_string = NULL;
        dis_init_params.dis_strings->fw_revision_string = NULL;
        dis_init_params.dis_strings->sw_revision_string = NULL;

        /* Check for minimum length of the buffer necessary to hold the contents. */
        str_len = ConfigRetrieve(CONFIG_BLE_DIS_MANUFACTURER_NAME, NULL, 0);
#ifdef HYDRACORE_TODO
# error "Need to fix Config Retrieve"
#endif

        /* Allow only first 31 characters of the Manufacturer name information. */
        if(str_len >= GATT_DIS_MAX_MANUF_NAME_LEN)
        {
            /* Extra word is required for storing NULL character at the end of the string. */
            str_len = GATT_DIS_MAX_MANUF_NAME_LEN;
        }

        if (str_len)
        {
            /* Allocate extra one word for storing NULL character at the end of the string. */
            dis_init_params.dis_strings->manufacturer_name_string = malloc(str_len + 1);

            if (dis_init_params.dis_strings->manufacturer_name_string != NULL)
            {
                memset((void*)dis_init_params.dis_strings->manufacturer_name_string, 0, (str_len + 1));

                /* Read the DIS Manufacturer Name information from the persistent store */
                ConfigRetrieve(CONFIG_BLE_DIS_MANUFACTURER_NAME, (void*)dis_init_params.dis_strings->manufacturer_name_string, str_len);
            }
        }
        return TRUE;
    }

    /* Failed to allocate memory */
    GATT_DEBUG(("GATT Device Info Server failed to allocate memory\n"));
    return FALSE;
}

/*******************************************************************************/
static void sinkGattFreeDisPtrs(void)
{
    /* Free the allocated memories. */
    if(dis_init_params.dis_strings->manufacturer_name_string != NULL)
        free((void*)dis_init_params.dis_strings->manufacturer_name_string);

    if(dis_init_params.dis_strings != NULL)
        free(dis_init_params.dis_strings);
}

/*******************************************************************************/
bool sinkGattDeviceInfoServerInitialise(uint16 **ptr)
{
    if (ptr)
    {
        /* Read the device information service to be initialized */
        if(sinkGattGetDeviceInfoParams())
        {
            if (GattDeviceInfoServerInit(sinkGetBleTask(), (gdiss_t*)*ptr, &dis_init_params,
                                    HANDLE_DEVICE_INFORMATION_SERVICE,
                                    HANDLE_DEVICE_INFORMATION_SERVICE_END))
            {
                GATT_DEBUG(("GATT Device Info Server initialised\n"));
                /* The size of DISS is also calculated and memory is alocated.
                 * So advance the ptr so that the next Server while initializing.
                 * shall not over write the same memory */
               *ptr += sizeof(gdiss_t);
                return TRUE;
            }

            /* Failed to initialize Device Information server */
            GATT_DEBUG(("GATT Device Info Server init failed [%x]\n", dis_status));
            /* Free the allocated memory */
            sinkGattFreeDisPtrs();
        }
    }
    return FALSE;
}

#endif /* GATT_DIS_SERVER */
