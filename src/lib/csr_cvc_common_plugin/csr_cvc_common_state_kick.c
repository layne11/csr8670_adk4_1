/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_state_kick.c

DESCRIPTION
    State machine helper to kick with the next event.
    
NOTES
    
*/

#include <csr_cvc_common_state_machine.h>
#include "audio_plugin_if.h"

/*******************************************************************************
DESCRIPTION
    Kick the state machine with the next event if necessary. Not to be called
    except from within the state machine.
*/
void csrCvcCommonStateMachineKick(cvc_event_id_t id)
{
    UNUSED(id);
}
