/*
 *  Contains function prototypes required by Matlab to let it load the kalelfreader.dll
 *  Copyright (c) 2011 - 2016 Qualcomm Technologies International, Ltd.
 *  All Rights Reserved.
 *  Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

typedef char * c_char_p;
typedef unsigned int uint32, *uint32Ptr;
typedef signed int int32, *int32Ptr;
typedef struct { int code; char *string; } KerError, *KerErrorPtr;
typedef void* voidPtr;
typedef struct { char *m_name; unsigned m_addr; unsigned m_loadAddr; unsigned m_numBytes; int m_type; } KerElfSectionHeader, *KerElfSectionHeaderPtr;
typedef struct { char *function_name; char *source_file; int line_number; uint32 low_pc; uint32 high_pc; } KerFunction, *KerFunctionPtr;
typedef struct { char *module; int sourceLine; uint32 address; char *sourceFile; } KerStatement, *KerStatementPtr;
typedef struct { uint32 type_id; char const *name; uint32 offset; uint32 bit_offset_from_offset; uint32 bit_size; } KerMember, *KerMemberPtr;
typedef struct { uint32 form; uint32 type_id; char const *name; uint32 num_words; uint32 ref_type_id; int member_count; KerMember *members; int array_count; uint32 array_type_id; } KerType, *KerTypePtr;
typedef struct { char const *name; uint32 type_id; uint32 size; uint32 addr; } KerVariable, *KerVariablePtr;
typedef struct { char const*name; uint32 value; } KerConstant, *KerConstantPtr;
typedef struct { char const*name; uint32 addr; } KerLabel, *KerLabelPtr;
typedef struct { int bitWidth; int byteAddressing; uint32 start; uint32 bytes; uint32 const*data; } KerData, *KerDataPtr;
typedef struct { int dspRev; int addrWidth; int dmDataWidth; int pmDataWidth; int dmByteAddressing; int pmByteAddressing; } KerArchitecture, *KerArchitecturePtr;
typedef struct { int severity; char const *text; } KerReport, *KerReportPtr;
typedef struct { char const *name; uint32 value; } KerEnum, *KerEnumPtr;

const char * ker_get_version();
KerError *ker_open_file(char const *elf_file_path, voidPtr *ker_result);
void ker_close_file(voidPtr self);
void ker_free_error(KerError *ker_error);
int32 ker_get_function_count(voidPtr self);
KerError *ker_get_function_by_index(voidPtr ker, int32 index, KerFunction *result);
KerError *ker_get_function_by_name(voidPtr ker, const char * name, KerFunction *result);
KerError *ker_get_function_by_addr(voidPtr ker, uint32 addr, KerFunction *result);
KerError *ker_get_variable_by_name(voidPtr ker, const char * name, KerVariable *result);
KerError *ker_get_elf_section_headers(voidPtr ker, int32 *count, KerElfSectionHeaderPtr *table);
KerError *ker_get_build_tool_versions(voidPtr ker, int32 *count, c_char_p *table);
KerError *ker_get_statements(voidPtr ker, int32 *count, KerStatementPtr *table);
KerError *ker_get_variables(voidPtr ker, int32 *count, KerVariablePtr *table);
KerError *ker_get_constants(voidPtr ker, int32 *count, KerConstantPtr *table);
KerError *ker_get_labels(voidPtr ker, int32 *count, KerLabelPtr *table);
KerError *ker_get_dm_data(voidPtr ker, int32 *count, KerDataPtr *table);
KerError *ker_get_pm_data(voidPtr ker, int32 *count, KerDataPtr *table);
KerError *ker_get_types(voidPtr ker, int32 *count, KerTypePtr *table);
KerError *ker_get_reports(voidPtr ker, int32 *count, KerReportPtr *table);
KerError *ker_get_enums(voidPtr ker, int32 *count1, KerEnumPtr *table1, int32 *count2, KerEnumPtr *table2);
int32 ker_statement_iter_init(voidPtr ker);
const char * ker_get_statement_module(voidPtr ker);
int32 ker_get_statement_source_line(voidPtr ker);
uint32 ker_get_statement_addr(voidPtr ker);
const char * ker_get_statement_source_file(voidPtr ker);
void ker_statement_iter_next(voidPtr ker);
int32 ker_variable_iter_init(voidPtr ker);
const char * ker_get_variable_name(voidPtr ker);
uint32 ker_get_variable_size(voidPtr ker);
uint32 ker_get_variable_addr(voidPtr ker);
void ker_variable_iter_next(voidPtr ker);
int32 ker_constant_iter_init(voidPtr ker);
const char * ker_get_constant_name(voidPtr ker);
uint32 ker_get_constant_value(voidPtr ker);
void ker_constant_iter_next(voidPtr ker);
int32 ker_label_iter_init(voidPtr ker);
const char * ker_get_label_name(voidPtr ker);
uint32 ker_get_label_addr(voidPtr ker);
void ker_label_iter_next(voidPtr ker);
int32 ker_get_dsp_revision(voidPtr self);
uint32 ker_get_machine_id(voidPtr self);
bool ker_is_big_endian(voidPtr self);
KerError *ker_get_architecture(voidPtr ker, KerArchitecture *record);
bool ker_is_overlapping_statements(voidPtr self);
KerError *ker_get_overlapping_statements(voidPtr ker, KerStatementPtr *kst);
void ker_free_elf_section_headers(KerElfSectionHeader *table);
void ker_free_build_tool_versions(const char * *table);
void ker_free_statements(KerStatement *table);
void ker_free_variables(KerVariable *table);
void ker_free_constants(KerConstant *table);
void ker_free_labels(KerLabel *table);
void ker_free_dm_data(KerData *table);
void ker_free_pm_data(KerData *table);
void ker_free_types(KerType *table);
void ker_free_reports(KerReport *table);
void ker_free_enums(KerEnum *table1, KerEnum *table2);
int32 ker_get_not_in_function_count(voidPtr self);
int32 ker_get_duplicate_statement_count(voidPtr self);
