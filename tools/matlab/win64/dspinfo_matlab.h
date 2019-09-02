/*
 *  Contains function and structure prototypes required by Matlab to let it load and use functions from dspinfo.dll
 *  Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
 *  All Rights Reserved.
 *  Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */
 
typedef enum
{
    KALIMBA_ARCH_UNKNOWN,
    KALIMBA_ARCH_1,
    KALIMBA_ARCH_2,
    KALIMBA_ARCH_3,
    KALIMBA_ARCH_4,
    KALIMBA_ARCH_5
}
KalimbaArch;

typedef struct
{
    /* The instruction set architecture revision. */
    KalimbaArch kal_arch;

    /* If true then each DM address refers to an octet, otherwise a word. */
    bool dm_octet_addressing;

    /* If true then each PM address refers to an octet, otherwise a word. */
    bool pm_octet_addressing;

    /* If true then DM words are in big endian format. */
    bool dm_big_endian;

    /* If true then PM words are in big endian format. */
    bool pm_big_endian;

    /* The number of bits in an address for either DM or PM */
    int address_width;

    /* The number of bits in one word of DM */
    int dm_data_width;

    /* The number of bits in one word of PM */
    int pm_data_width;
}
kalarchinfo;

const kalarchinfo * kalarchinfo_from_arch(KalimbaArch kal_arch);
