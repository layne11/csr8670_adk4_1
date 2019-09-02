;****************************************************************************
;        %%fullcopyright(2005)
; FILE
;        native_if.inc - common defines for native code
;
; CONTAINS
;        Defines required by firmware and host code.
;
;****************************************************************************

;Locations/values agreed between the firmware and the app.
;DO NOT MODIFY THESE VALUES.

;Amount of scratch space available beneath app stack pointer
;UGLY: must be kept in step with dev/src/csr/vm/native_private.h
SHARED_STACK_COVER               EQU    4

;Worse case stack requirement of MMU routines (excludes application
;stack cover; worst case is 32-bit fetch/store which incurs extra enter)
SHARED_MMU_WORST_CASE_STACK      EQU    9

;The memory location the app puts the logical address for the native mmu to 
;translate. This shouldn't be used in Assisted mode: firstly, it's not 
;necessary, and secondly, the address may be used for something else 
;(SHARED_TRAP_HANDLER_ASSISTED_HWRD, at the time of writing).
SHARED_MMU_SAFE_LOCATION         EQU    H'FFFC

;Size of ENTERL performed by firmware MMU fetch routine
SHARED_MMU_FETCH_ENTER_SIZE      EQU    2

;Offset beneath stack pointer where firmware MMU fetch
;routine stores returned value
SHARED_MMU_FETCHED_VALUE_OFFSET  EQU    1

;Size of ENTERL performed by firmware MMU store routine
SHARED_MMU_STORE_ENTER_SIZE      EQU    2

;Offset beneath stack pointer where firmware MMU store
;routine expects to find the value to write
SHARED_MMU_STORED_VALUE_OFFSET   EQU    1

;The memory locations at which we store the trap handler address.
;We use different locations for Native and Assisted apps, in an
;attempt to use less scratch space in the latter. 
SHARED_TRAP_HANDLER_LWRD                EQU    H'FFF5
SHARED_TRAP_HANDLER_HWRD                EQU    H'FFF4
SHARED_TRAP_HANDLER_ASSISTED_LWRD       EQU    H'FFFD
SHARED_TRAP_HANDLER_ASSISTED_HWRD       EQU    H'FFFC

;Stack usage for trap calls
SHARED_TRAP_HANDLER_STACK_USAGE  EQU    3

;The memory location that holds the physical address of the bottom of the app stack
SHARED_STACK_LOW_LIMIT           EQU    H'FFF3

;The memory location that holds the physical address of the start of the globals block
SHARED_APP_GLOBALS_POS           EQU    H'FFF2

;The memory location that holds the physical address of the fw mmu code
SHARED_MMU_CODE_ADDRESS          EQU    H'FFF0

;Offsets to mmu routines
NATIVE_MMU_LD_HWRD               EQU    H'0000
NATIVE_MMU_LD_LWRD               EQU    H'0001
NATIVE_MMU_ST_HWRD               EQU    H'0002
NATIVE_MMU_ST_LWRD               EQU    H'0003
NATIVE_HANDLE_STACK_OFLOW_HWRD   EQU    H'0004
NATIVE_HANDLE_STACK_OFLOW_LWRD   EQU    H'0005

;Register storage in case of emergency
SHARED_AL_EMERGENCY              EQU    H'FFF8
SHARED_X_EMERGENCY               EQU    H'FFF9
SHARED_XH_EMERGENCY              EQU    H'FFFA


