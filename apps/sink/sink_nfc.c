/**
@file
@ingroup sink_app

@brief Handling NFC Connection Library Messages

Copyright (c) 2016 - 2016 Qualcomm Technologies International, Ltd.

*/

/*****************************************************************************/
/*!

\brief  Implementation file to handle nfc connection library messages 

@{        
*/
#include "sink_nfc.h"
#include "sink_private.h"
#include "sink_statemanager.h"
#include "sink_audio.h"
#include "assert.h"
#include "connection.h"
#include "sink_config.h"
#include "sink_slc.h"

/* PRIVATE MACROS ************************************************************/
/** 
 * Ref: Bluetooth core specification 4.2, section 4.2.2 connectable mode 
 * page 304 and 
 * Table 4.2 Page Scan Parameter for connection speed scenarios page 305
 * Fast R1 TGap(106) = 100ms   , 100/0.625=160 0xA0 
 * Fast R1 TGap(101) = 10.625ms, 10.625/0.625=17 or 0x11=HCI_PAGESCAN_WINDOW_MIN */
#define NFC_HCI_PAGESCAN_INTERVAL (0xA0)
#define NFC_HCI_PAGESCAN_WINDOW   (0x11)
#define NFC_HCI_PAGESCAN_TIMEOUT  (3000)

#ifdef DEBUG_NFC
    #define NFC_DEBUG(x) DEBUG(x)
#else
    #define NFC_DEBUG(x) 
#endif


#if defined(ENABLE_ADK_NFC)

/* PRIVATE VARIABLE DEFINITIONS **********************************************/
static bool NfcClFastConnect = TRUE;

/* PUBLIC FUNCTION DEFINITIONS ***********************************************/
void NfcEnableFastConnect(void)
{
    NfcClFastConnect = TRUE;
}

void NfcDisableFastConnect(void)
{
    NfcClFastConnect = FALSE;
}

void handleNfcClMessage (Task task, MessageId id, Message message)
{
    const NFC_CL_PRIM *p_nfc_cl = (const NFC_CL_PRIM *) message;

    UNUSED(task);
    switch (id)
    {
    case NFC_CL_READY_IND:
        /*assert(IS_VALID_PTR(p_nfc_cl));*/
        NFC_DEBUG(("NFC_APP: NFC_CL_READY_IND: mode=%d\n",
                    p_nfc_cl->m.ready_ind.mode));
        switch (p_nfc_cl->m.ready_ind.mode)
        {
        case NFC_VM_NONE:
            ConnectionReadLocalAddr(task);
            break;
        case NFC_VM_TT2:
            /* Already configured */
            break;
        default:
            NFC_DEBUG(("NFC_APP: Unhandled NFC MODE\n"));
            break;
        }
        break;
    case NFC_CL_TAG_WRITE_CH_CARRIERS_IND:
        /*assert(IS_VALID_PTR(p_nfc_cl));*/
        NFC_DEBUG(("NFC_APP: NFC_CL_TAG_WRITE_CH_CARRIERS_IND status=%d\n",p_nfc_cl->m.status));
        break;
    case NFC_CL_TAG_READ_STARTED_IND:
        /*assert(!IS_VALID_PTR(p_nfc_cl));*//* MUST be Empty message body */
        NFC_DEBUG(("NFC_APP: NFC_CL_TAG_READ_STARTED_IND => Connectable\n"));
        handleNfcClTagReadStarted(task);
        break;
    case  NFC_CL_STOP_FAST_PAGESCAN_IND:
        NFC_DEBUG(("NFC_APP: NFC_CL_STOP_FAST_PAGESCAN_IND\n"));
        /*assert(IS_VALID_PTR(theSink.conf2));*/
        ConnectionWritePagescanActivity(theSink.conf2->radio.page_scan_interval, theSink.conf2->radio.page_scan_window);
        ConnectionWritePageScanType(hci_scan_type_standard);
        break;
    default:
        NFC_DEBUG(("NFC_APP: Rx Unhandled NFC_CL (msg id=%d)\n",id));
        break;
    }
}

/** 
 * The code logic should be kept the same as 
 * handleUsrNFCTagDetected  (see below) without the fast 
 * paging. It is anticipated that the commonality will be 
 * merged in the future. */ 
void handleNfcClTagReadStarted(Task task)
{
    /* if not connected to an AG, go straight into pairing mode */
    if (stateManagerGetState() < deviceConnected)
    {        
        stateManagerEnterConnDiscoverableState(TRUE);
        if(TRUE==NfcClFastConnect)
        {
            NFC_DEBUG(("NFC_APP: NFC_CL_START_FAST_PAGESCAN_REQ\n"));
            /* Call ConnectionWritePagescanActivity after stateManagerEnterConnDiscoverableState 
             * NOTE: a timer (e.g. 10s) needs to be STARTED to restore the DEFAULT page scan. */
            ConnectionWritePagescanActivity(NFC_HCI_PAGESCAN_INTERVAL,
                                            NFC_HCI_PAGESCAN_WINDOW);
            ConnectionWritePageScanType(hci_scan_type_interlaced);
            MessageSendLater(task, NFC_CL_STOP_FAST_PAGESCAN_IND, NULL, NFC_HCI_PAGESCAN_TIMEOUT);
        }
    }
}
#endif /* ENABLE_ADK_NFC */

/** ADK 4.x code
 *  
 *  The code below should be replaced something similar to
 *  handleNfcClTagReadStarted(task) to enable fast connection.
 *  This would be triggered on the detection of NFC Field from
 *  the phone. The main difference is that the Sink app has NO
 *  guarantee that the reader has actually read the tag. The end
 *  user may just have waved his/her phone next to the tag but
 *  yet activated the tag or started reading the content of it.
 *  
 *  The TagReadStarted function has the advantage of informing
 *  the sink app that the PHONE has almost completed reading the
 *  content of the Sink App TAG */
void handleUsrNFCTagDetected(Task task,  Message message)
{
    (void)task;
	(void)message;
    /* if not connected to an AG, go straight into pairing mode */
    if (stateManagerGetState() < deviceConnected)
    {
        stateManagerEnterConnDiscoverableState( TRUE );
    }
}

/** @}*/
