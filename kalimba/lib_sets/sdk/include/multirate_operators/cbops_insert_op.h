// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CBOPS_INSERT_OP_HEADER_INCLUDED
#define CBOPS_INSERT_OP_HEADER_INCLUDED

// Note: Insert operator is inplace
//		 Requires unique routing in operator chain
.CONST $cbops.insert_op.BUFFER_INDEX_FIELD      0;
.CONST $cbops.insert_op.MAX_ADVANCE_FIELD       1; 
// Internal working data.  Initialize to zero
.CONST $cbops.insert_op.DATA_AMOUNT_FIELD       2;
.CONST $cbops.insert_op.STRUC_SIZE              3;


// Note: Insert Copy operator can not be inplace
.CONST $cbops.insert_copy_op.INPUT_BUFFER_INDEX_FIELD      0;
.CONST $cbops.insert_copy_op.OUTPUT_BUFFER_INDEX_FIELD     1;
.CONST $cbops.insert_copy_op.MAX_ADVANCE_FIELD			   2; 
.CONST $cbops.insert_copy_op.STRUC_SIZE                    3;


#endif // CBOPS_INSERT_OP_HEADER_INCLUDED
