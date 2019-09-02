// *****************************************************************************
// Copyright (c) 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
// *****************************************************************************

//------------------------------------------------------------------------------
.module $kal_float_lib;
//------------------------------------------------------------------------------
// kalimba floating point library
// library is optimised for the use of coefficient calculation within the DSP
//------------------------------------------------------------------------------
// floats are stored as 24e8 stored in two 24 bit words
// 
//   +------------------------+------------------------+
//   |S---------------EEEEEEEE|MMMMMMMMMMMMMMMMMMMMMMMM|
//   +------------------------+------------------------+
//
// - top bit of first word is sign bit.  bottom 8 bits of first word is exponent
// - exponent is stored with an offset of 127
// - second word stores mantissa

// - there is no hidden mantissa bit
//------------------------------------------------------------------------------
// general register usage
//   on entry
//     r0:r1 = operand A
//     r2:r3 = operand B
//   on exit
//     r0:r1 = result
//   registers assumed destroyed
//     none
//------------------------------------------------------------------------------


#define WORD_SIZE		24


    .codesegment PM;
    .datasegment DM;


    .const  format.sign_bit             (1<<(WORD_SIZE-1));
    .const  format.exp_mask             0x0000ff;
    .const  format.exp_offset           127;

    .const  const.zero.exp              0x000000;
    .const  const.zero.mant             0x000000;
    
    .const  const.unity.exp             0x000080;
    .const  const.unity.mant            0x800000;

    .const  const.one_point_five.exp    0x000080;
    .const  const.one_point_five.mant   0xc00000;

    .const  const.half.exp              0x00007f;
    .const  const.half.mant             0x800000;

    .const  const.two.exp               0x000081;
    .const  const.two.mant              0x800000;


//------------------------------------------------------------------------------
// __zero:
//------------------------------------------------------------------------------
// return zero
//------------------------------------------------------------------------------
// on entry
//   void
// on exit
//   r0:r1 = zero
// registers destroyed
//   none
// stack usage
//   jump = 0
//   call = 1
//------------------------------------------------------------------------------

__zero.call_entry:
    push rLink;    
__zero.jump_entry:

    r0 = const.zero.exp;
    r1 = const.zero.mant;
    
    pop rLink;
    rts;


	
//------------------------------------------------------------------------------
// __renorm:
//------------------------------------------------------------------------------
// renormalise floating point number passed in r0:r1
// - shift mant left (and decrement exp) until most significant bit is set
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = number to renormalise
// on exit
//   r0:r1 = renormalised number
// registers destroyed
//   none
// stack usage
//   jump = 1
//   call = 2
//------------------------------------------------------------------------------

__renorm.call_entry:
    push rLink;    
__renorm.jump_entry:

    // if mantissa is zero, then return zero
    null = r1;
    if z jump __zero.jump_entry;
    
    push r4;
    
    r4 = r0 and format.exp_mask;
    r0 = r0 and format.sign_bit;
    null = r1;
    if neg jump __renorm_rebuild_and_exit;
    __renorm_loop:
        r4 = r4 - 1;
        r1 = r1 lshift 1;
        if pos jump __renorm_loop;
    __renorm_rebuild_and_exit:
    r4 = r4 and format.exp_mask;
    r0 = r0 or r4;
    
    pop r4;
    
    pop rLink;
    rts;



//------------------------------------------------------------------------------
// __add:
//------------------------------------------------------------------------------
// add r0:r1 and r2:r3 wih signbit r4, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
//   r4 = sign
// on exit
//   r0:r1 = result of addition
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

//__add.call_entry:         // 'call_entry' never used in currently library
//    push rLink;    
__add.jump_entry:

    //pushm <r4,r5>;        // pushed before entering routine
    pushm <r2,r3,r6,r7>;

    // if either mantissa is zero, use exponent from other word to prevent scaling
    null = r1;
    if z r0 = r2;
    null = r3;
    if z r2 = r0;
    
    r5 = r0 and format.exp_mask;
    r6 = r2 and format.exp_mask;
    r7 = r5 - r6;
    if neg jump __add_shift_op1;
        // op1 is greater than op2 so shift op2 down
        r7 = -r7;
        r3 = r3 lshift r7;
        r0 = r5 + 1;
        jump __add_add_and_exit;
    __add_shift_op1:
        // op2 is greater than op1 so shift op1 down
        r1 = r1 lshift r7;
        r0 = r6 + 1;
__add_add_and_exit:
    r1 = r1 lshift -1;
    r3 = r3 lshift -1;
    r1 = r1 + r3;
    r0 = r0 or r4;

    popm <r2,r3,r6,r7>;
    popm <r4,r5>;

    // jump because rts at end of __renorm
    jump __renorm.jump_entry;



//------------------------------------------------------------------------------
// __sub:
//------------------------------------------------------------------------------
// subtract r2:r3 from r0:r1 wih signbit r4, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
//   r4 = sign
// on exit
//   r0:r1 = result of subtraction
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

//__sub.call_entry:         // 'call_entry' never used in currently library
//    push rLink;    
__sub.jump_entry:

    //pushm <r4,r5>;        // pushed before entering routine
    pushm <r2,r3,r6,r7>;

    // if either mantissa is zero, use exponent from other word to prevent scaling
    null = r1;
    if z r0 = r2;
    null = r3;
    if z r2 = r0;
    
    r5 = r0 and format.exp_mask;
    r6 = r2 and format.exp_mask;
    r7 = r5 - r6;
    if neg jump __sub_shift_op1;
        // op1 is greater than op2 so shift op2 down
        r7 = -r7;
        r3 = r3 lshift r7;
        r0 = r5 + 1;
        jump __sub_sub_and_exit;
    __sub_shift_op1:
        // op2 is greater than op1 so shift op1 down
        r1 = r1 lshift r7;
        r0 = r6 + 1;
__sub_sub_and_exit:
    r1 = r1 lshift -1;
    r3 = r3 lshift -1;
    null = r1 - r3;
    if neg jump __sub_sub_a_from_b;
        // __sub_sub_b_from_a:
        r1 = r1 - r3;
        r0 = r0 or r4;
        popm <r2,r3,r6,r7>;
        popm <r4,r5>;
        jump __renorm.jump_entry;
    __sub_sub_a_from_b:
        r1 = r3 - r1;
        r4 = r4 xor format.sign_bit;
        r0 = r0 or r4;
        popm <r2,r3,r6,r7>;
        popm <r4,r5>;
        jump __renorm.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.q_to_float:
//------------------------------------------------------------------------------
// on entry
//   r0 = number to convert
//   r1 = bit shifts
// on exit
//   r0:r1 = result of conversion
// registers destroyed
//   none
// stack usage
//   jump = 3
//   call = 4
//------------------------------------------------------------------------------

    push rLink;    
q_to_float.jump_entry:

    null = r0;
    if z jump __zero.jump_entry;
    
    pushm <r2,r4>;

    r2 = r0;
    r4 = r0 and format.sign_bit;
    if nz r2 = -r2;
    
    r0 = r1 + 127;
    r0 = r0 or r4;
    r1 = r2;

    popm <r2,r4>;

    jump __renorm.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.float_to_q:
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = number to convert
//   r2 = bit shifts
// on exit
//   r0 = result of conversion
// registers destroyed
//   none
// stack usage
//   jump = 2
//   call = 3
//------------------------------------------------------------------------------

    push rLink;    
float_to_q.jump_entry:

    pushm <r3,r4>;

    r1 = r1 lshift -1;
    r3 = r0 and format.exp_mask;
    r3 = r3 - 1;
    r4 = r0 and format.sign_bit;
    if nz r1 = -r1;

    r3 = r3 - (format.exp_offset + WORD_SIZE);
    r3 = r3 + r2;
    
    r0 = r1 ashift r3;

    popm <r3,r4>;
    
    pop rLink;
    rts;



//------------------------------------------------------------------------------
// external functions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
$kal_float_lib.int_to_float:
//------------------------------------------------------------------------------
// convert integer to floating point number
//------------------------------------------------------------------------------
// on entry
//   r0 = integer to convert
// on exit
//   r0:r1 = floating point version
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

    push rLink;    
    r1 = WORD_SIZE;
    jump q_to_float.jump_entry;
    


//------------------------------------------------------------------------------
$kal_float_lib.frac_to_float:
//------------------------------------------------------------------------------
// convert fraction to floating point number
//------------------------------------------------------------------------------
// on entry
//   r0 = fraction to convert
// on exit
//   r0:r1 = floating point version
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

    push rLink;    
    r1 = 1;
    jump q_to_float.jump_entry;
    


//------------------------------------------------------------------------------
$kal_float_lib.float_to_int:
//------------------------------------------------------------------------------
// convert floating point number to integer
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = floating point number to convert
// on exit
//   r0 = integer version
// registers destroyed
//   none
// stack usage
//   5
//------------------------------------------------------------------------------

    pushm <r2,rLink>;
    r2 = 2;
    call float_to_q;
    popm <r2,rLink>;
    rts;
   


//------------------------------------------------------------------------------
$kal_float_lib.float_to_frac:
//------------------------------------------------------------------------------
// convert floating point number to fraction
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = floating point number to convert
// on exit
//   r0 = fractional version
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

    pushm <r2,rLink>;
    r2 = WORD_SIZE+1;
    call float_to_q;
    popm <r2,rLink>;
    rts;
   


//------------------------------------------------------------------------------
$kal_float_lib.neg:
//------------------------------------------------------------------------------
// negate r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = number to negate
// on exit
//   r0:r1 = result of negation
// registers destroyed
//   none
// stack usage
//   0
//------------------------------------------------------------------------------

    r0 = r0 xor format.sign_bit;
    rts;



//------------------------------------------------------------------------------
$kal_float_lib.add:
//------------------------------------------------------------------------------
// add r0:r1 and r2:r3, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
// on exit
//   r0:r1 = result of addition
// registers destroyed
//   none
// stack usage
//   7
//------------------------------------------------------------------------------

    push rLink;
    
    pushm <r4,r5>;      // these are popped in __add and __sub routines

    r4 = r0 and format.sign_bit;
    r5 = r2 and format.sign_bit;
    
    null = r4 - r5;
    if eq jump __add.jump_entry;
    jump __sub.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.sub:
//------------------------------------------------------------------------------
// add r2:r3 from r0:r1, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
// on exit
//   r0:r1 = result of subtraction
// registers destroyed
//   none
// stack usage
//   7
//------------------------------------------------------------------------------

    push rLink;    

    pushm <r4,r5>;      // these are popped in __add and __sub routines

    r4 = r0 and format.sign_bit;
    r5 = r2 and format.sign_bit;
    
    null = r4 - r5;
    if eq jump __sub.jump_entry;
    jump __add.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.mul:
//------------------------------------------------------------------------------
// multiply r0:r1 and r2:r3, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
// on exit
//   r0:r1 = result of multiplication
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------

    push rLink;
    
    // if either mantissa is zero, then return zero
    null = r1;
    if z jump __zero.jump_entry;
    null = r3;
    if z jump __zero.jump_entry;
    
    pushm <r3,r4,r5>;

    // create exponent
    r4 = r0 and format.exp_mask;
    r5 = r2 and format.exp_mask;
    r4 = r4 + r5;
    r4 = r4 - format.exp_offset;
    r5 = r0 xor r2;
    r5 = r5 and format.sign_bit;
    r0 = r4 or r5;

    // calculate mantissa
    r1 = r1 lshift -1;
    r3 = r3 lshift -1;
    rmac = r1 * r3;
    r1 = rmac;
    r1 = r1 lshift 1;
    
    popm <r3,r4,r5>;

    jump __renorm.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.recip:
//------------------------------------------------------------------------------
// find reciprocal of r0:r1, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = number to find reciprocal of
// on exit
//   r0:r1 = result
// registers destroyed
//   none
// stack usage
//   9
//------------------------------------------------------------------------------

    push rLink;

    pushm <r2,r3,r4,r5,r10>;
    
    r2 = (1 + 2*format.exp_offset) - r0;       // r2:r3 = xold
    r3 = r1 ashift (WORD_SIZE-1);
    
    pushm <r0,r1>;
        
    r10 = 6;
    do _recip_iteration_loop;
        popm <r0,r1>;
        pushm <r0,r1>;
        call $kal_float_lib.mul;        // r0:r1 = a * xold
        pushm <r0,r1,r2,r3>;
        popm <r2,r3,r4,r5>;             // r2:r3 = a * xold, r4:r5 = xold
        r0 = format.exp_offset + 1;            // r0:r1 = 1.0
        r1 = (1<<(WORD_SIZE-1));        //
        call $kal_float_lib.sub;        // r0:r1 = 1 - a * xold
        r2 = r4;                        // r2:r3 = xold
        r3 = r5;
        call $kal_float_lib.mul;
        r2 = r4;                        // r2:r3 = xold
        r3 = r5;
        call $kal_float_lib.add;
        r2 = r0;
        r3 = r1;
    _recip_iteration_loop:
    popm <r0,r1>;

    r0 = r2;
    r1 = r3;
    
    popm <r2,r3,r4,r5,r10>;

    jump __renorm.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.div:
//------------------------------------------------------------------------------
// divide r0:r1 and r2:r3, returning the result in r0:r1
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = operand 1
//   r2:r3 = operand 2
// on exit
//   r0:r1 = result of division
// registers destroyed
//   none
// stack usage
//   15
//------------------------------------------------------------------------------

    push rLink;

    pushm <r2,r3>;
    pushm <r0,r1>;
    r0 = r2;
    r1 = r3;
    call $kal_float_lib.recip;
    r2 = r0;
    r3 = r1;
    popm <r0,r1>;
    call $kal_float_lib.mul;
    popm <r2,r3>;
    jump __renorm.jump_entry;



//------------------------------------------------------------------------------
$kal_float_lib.sqrt:
//------------------------------------------------------------------------------
// calculate the approximate square root of the input parameter
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = value to calculate square root of
// on exit
//   r0:r1 = square root
// registers destroyed
//   none
// stack usage
//   4
//------------------------------------------------------------------------------
// conversion uses a truncated power series to calculate root of mantissa
//------------------------------------------------------------------------------

    push rLink;
	
    pushm <r2,r3,r4>;

    // calculate exponent section
    // - divide exponent by two (take a copy of lowest bit as will need later)
	
    r2 = r0 and 0x000001;
    r3 = r0 and format.sign_bit;
    r0 = r0 and format.exp_mask;
    r0 = r0 - (format.exp_offset + 1);
    r0 = r0 lshift -1;
    r0 = r0 or r3;
	
    // calculate mantissa section
    // - truncated power series

    r0 = r0 + (format.exp_offset + 2);
    r1 = r1 lshift -1;
    rmac = 0.184891857;
    r4 = 0.78755;
    rmac = rmac + r1 * r4;
    r3 = r1 * r1;
    r4 = -0.364;
    rmac = rmac + r3 * r4;
    r3 = r3 * r1;
    r4 = 0.098665143;
    rmac = rmac + r3 * r4;
    r1 = rmac;

    r2 = r2;
    if eq jump _sqrt_exit;
	
        r4 = 0.707107;
        r1 = r1 * r4;
        r0 = r0 + 1;
		
_sqrt_exit:

    popm <r2,r3,r4>;
	
    jump __renorm.jump_entry;



.endmodule; // $kal_float_lib
