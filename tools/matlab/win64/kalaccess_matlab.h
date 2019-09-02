/*
 *  Contains function prototypes required by Matlab to let it load and use functions from kalaccess.dll
 *  Copyright (c) 2011 - 2016 Qualcomm Technologies International, Ltd.
 *  All Rights Reserved.
 *  Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef void* voidPtr;

typedef struct ka_err
{ 
    int32 err_code; 
    char *err_string; 
} ka_err, *ka_errPtr;

typedef struct ka_connection_details
{
    char const *transport_string;
    int subsys_id;
    unsigned processor_id;
    char const *dongle_id;
} ka_connection_details, *ka_connection_detailsPtr;

void ka_free_error(ka_err *err);

const char * ka_trans_get_var(voidPtr ka, const char * var);
void ka_trans_set_var(voidPtr ka, const char * var, const char * val);

ka_err *ka_trans_build_device_table(ka_connection_detailsPtr *table, int* count);
void ka_trans_free_device_table(ka_connection_details *table, int count);

ka_err *ka_connect(ka_connection_details const *conn_details, bool ignore_fw, voidPtr *kalaccess_result);
ka_err *ka_connect_simple(char const * transport_string, bool ignore_fw, voidPtr *kalaccess_result);
void ka_disconnect(voidPtr ka);

int32 ka_get_chip_rev(voidPtr ka);
const char * ka_get_chip_name(voidPtr ka);
uint32 ka_get_arch(voidPtr ka);
uint32 ka_get_arch_from_name(const char * name);

uint32 ka_get_register_width(voidPtr ka, int32 id);
int32 ka_get_register_id(voidPtr ka, const char * name);
const char * *ka_get_register_names();

uint32 ka_hal_get_max_pm_breakpoints(voidPtr ka);
ka_err *ka_hal_set_pm_breakpoint(voidPtr ka, uint32 breakpoint_num, uint32 addr);
ka_err *ka_hal_clear_pm_breakpoint(voidPtr ka, uint32 breakpoint_num);
ka_err *ka_hal_get_pm_breakpoint(voidPtr ka, uint32 breakpoint_num, uint32 *addr, bool *enabled);
ka_err *ka_hal_clear_all_pm_breakpoints(voidPtr ka);

uint32 ka_hal_get_max_dm_breakpoints(voidPtr ka);
ka_err *ka_hal_set_dm_breakpoint(voidPtr ka, uint32 breakpoint_num, uint32 startAddress, uint32 endAddress, bool triggerOnRead, bool triggerOnWrite);
ka_err *ka_hal_clear_dm_breakpoint(voidPtr ka, uint32 breakpoint_num);
ka_err *ka_hal_get_dm_breakpoint(voidPtr ka, uint32 breakpoint_num, uint32 *startAddress, uint32 *endAddress, bool *triggerOnRead, bool *triggerOnWrite);
ka_err *ka_hal_clear_all_dm_breakpoints(voidPtr ka);

ka_err *ka_hal_read_pm_block(voidPtr ka, uint32 start_addr, uint32 *data, uint32 num_words);
ka_err *ka_hal_write_pm_block(voidPtr ka, uint32 start_addr, uint32 *data, uint32 num_words);
ka_err *ka_hal_read_dm_block(voidPtr ka, uint32 start_addr, uint32 *data, uint32 num_words);
ka_err *ka_hal_write_dm_block(voidPtr ka, uint32 start_addr, uint32 *data, uint32 num_words);
ka_err *ka_hal_read_register(voidPtr ka, int32 reg, uint32 *result);
ka_err *ka_hal_write_register(voidPtr ka, int32 reg, uint32 val);

ka_err *ka_hal_spi_read(voidPtr ka, uint32 addr, uint16 *data);
ka_err *ka_hal_spi_write(voidPtr ka, uint32 addr, uint16 data);

ka_err *ka_hal_pcprofile(voidPtr ka, uint32 *sample_data, uint32 num_samples);

ka_err *ka_hal_run(voidPtr ka);
ka_err *ka_hal_pause(voidPtr ka);
ka_err *ka_hal_step(voidPtr ka);
ka_err *ka_hal_read_dsp_enable(voidPtr ka, bool *enabled);
ka_err *ka_get_chip_state(voidPtr ka, int *state);
