/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.1
*******************************************************************************/

#ifndef _VMTYPES_H
#define _VMTYPES_H

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
#endif

#ifndef BITFIELD
#define BITFIELD    unsigned
#endif

#define BITFIELD_CAST(bit_width, value) (((1 << bit_width) - 1) & (unsigned)value)

#ifndef PACK_STRUCT
#define PACK_STRUCT
#endif

#ifndef UNUSED
#define UNUSED(var)     (void)(var)
#endif

/* Make sure that any sizes (in uint16 resolution) of structures mapping
   directly into PSKeys are passed as the correct number of uint16s.
   ie round up sizes rather than round down if sizeof() has octet resolution */
#define PS_SIZE_ADJ(X) (((X) + sizeof(uint16) - 1) / sizeof(uint16))

#ifdef HYDRACORE
/* ENUM SIZES

 Crescendo enums are variable size.
 For compatibility, we need at least 16 bits to be allocated in some cases
 ( possibly all ).

 Need the parameter to get unique symbols otherwise kalcc complains. Adding
 __LINE__ might help but a header with an enum on the same line would fail.
 Sigh.
.
 Also looked at a macro that could sit at the head of the enum. Unfortunately,
 we end up needing to define a value as -1 to ensure the first real enum value
 is zero. That is fine, but means you are limited to 15 bit enum values 
 otherwise compiler overflows to a long type. Cannot fit -1 and 32768 
 into 16 bits.
 
 Could define the macro to specify 3 enums 
        dummy1  // With whatever the next value was 
        0x7FFFu // 15 bits 
        dummy = dummy1-1 // forces enum following to continue earlier sequence 
*/

#define FORCE_ENUM_TO_MIN_16BIT(tag) dummy_enum_entry__##tag##__ = 0x7FFFu

#else /* HYDRACORE */
/*
For xap we want to leave things alone so define the end enum to be 0
*/

#define FORCE_ENUM_TO_MIN_16BIT(tag) dummy_enum_entry__##tag##__ = 0

#endif /* HYDRACORE */

#endif
