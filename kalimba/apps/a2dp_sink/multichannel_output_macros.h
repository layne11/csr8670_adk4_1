// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef MULTICHANNEL_OUTPUT_MACROS_HEADER_INCLUDED
#define MULTICHANNEL_OUTPUT_MACROS_HEADER_INCLUDED

// Run-time operator macros for configuration and state
// (this allows operator fields to be changed and read at run-time)
#define TEMPLATE_OP_CONFIG(name, var_name, var_offset, register)                                                                                \
   M[name##.##var_name + var_offset] = register;

#define TEMPLATE_OP_STATE(name, var_name, var_offset, register)                                                                                 \
   register = M[name##.##var_name + var_offset];

#define DYNAMIC                           0                       // Place holder macro for run-time configured parameters


// Mix a mono tone into a mono signal (no resampling here!)
#define TEMPLATE_MIX_OP(name, next_op, in_idx, in_tone_cbuf)                                                                                    \
                                                                                                                                                \
   .VAR/DM1CIRC name##.##_hist[$cbops.auto_resample_mix.TONE_FILTER_HIST_LENGTH];                                                               \
                                                                                                                                                \
   .BLOCK $##name;                                                                                                                              \
      .VAR name##.##next = $##next_op;                                                                                                          \
      .VAR name##.##func = $cbops.auto_upsample_and_mix;                                                                                        \
      .VAR name##.##param[$cbops.auto_resample_mix.STRUC_SIZE] =                                                                                \
         in_idx,                          /* Input index to first channel */                                                                    \
         -1,                              /* Input index to second channel, -1 for no second channel */                                         \
         in_tone_cbuf,                    /* cbuffer structure containing tone samples */                                                       \
         $sra_coeffs,                     /* coefs for resampling */                                                                            \
         $current_dac_sampling_rate,      /* pointer to variable containing dac rate received from vm (if 0, default 48000hz will be used) */   \
         name##.##_hist,                  /* history buffer for resampling */                                                                   \
         $current_dac_sampling_rate,      /* pointer to variable containing tone rate received from vm (if 0, default 8000hz will be used) */   \
         0.5,                             /* tone volume mixing (set by vm) */                                                                  \
         0.5,                             /* audio volume mixing */                                                                             \
         0 ...;                           /* Pad out remaining items with zeros */                                                              \
   .ENDBLOCK;


#define TEMPLATE_SIGNAL_DETECT_OP(name, next_op, num_chan,                                              \
        in0_idx, in1_idx, in2_idx, in3_idx, in4_idx, in5_idx)                                           \
                                                                                                        \
   .VAR name##.##num_chan_ptr = name##.##param + $cbops.signal_detect_op.NUM_CHANNELS;                  \
                                                                                                        \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.signal_detect_op;                                                     \
      .VAR name##.##param[$cbops.signal_detect_op.STRUC_SIZE_STEREO + 4] =                              \
         signal_detect_coeffs,      /* pointer to coefficients */                                       \
         num_chan,                  /* number of channels */                                            \
         in0_idx,                   /* In0 index */                                                     \
         in1_idx,                   /* In1 index */                                                     \
         in2_idx,                   /* In2 index */                                                     \
         in3_idx,                   /* In3 index */                                                     \
         in4_idx,                   /* In4 index */                                                     \
         in5_idx;                   /* In5 index */                                                     \
   .ENDBLOCK;


#define TEMPLATE_MUTE_OP(name, next_op, mute_dir, in0_idx, out0_idx)                                    \
                                                                                                        \
   /* Template channel cbops operator chain */                                                          \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.soft_mute;                                                            \
      .VAR name##.##param[$cbops.soft_mute_op.STRUC_SIZE_MONO] =                                        \
         mute_dir,      /* mute direction (1 = unmute audio) */                                         \
         0,             /* mute index (internal state) */                                               \
         1,             /* number of channels */                                                        \
         in0_idx,       /* In 0 index */                                                                \
         out0_idx;      /* Out 0 index */                                                               \
   .ENDBLOCK;


#define TEMPLATE_DC_REMOVE_OP(name, next_op, in_idx, out_idx)                                           \
                                                                                                        \
   /* Template channel cbops operator chain */                                                          \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.dc_remove;                                                            \
      .VAR name##.##param[$cbops.dc_remove.STRUC_SIZE] =                                                \
         in_idx,                             /* Input index */                                          \
         out_idx,                            /* Output index */                                         \
         0,                                  /* DC estimate field (MSB) */                              \
         0;                                  /* DC estimate field (LSB) */                              \
   .ENDBLOCK;

// Additional space for temp buffer used in resampler
#define BUFF_PAD                            10

// Buffer size needed for temp buff in resampler with 192kHz Fs @ timer of 0.5ms
// Max size is for 48k->192k(4up1down)==4*0.5*192==384
#define TEMP_BUFF_SIZE                      (384 + BUFF_PAD)

#define TEMPLATE_IIR_RESAMPLEV2_OP(name, next_op, in_idx, out_idx, filter_def, in_shift, out_shift, prec_16_24bit)  \
                                                                                                        \
   .VAR/DM name##.##iir_temp[TEMP_BUFF_SIZE];                                                           \
                                                                                                        \
   /* Template channel cbops operator chain */                                                          \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops_iir_resamplev2;                                                       \
      .VAR name##.##param[$iir_resamplev2.OBJECT_SIZE] =                                                \
         in_idx,                             /* Input index */                                          \
         out_idx,                            /* Output index */                                         \
         filter_def,                         /* Filter definition pointer */                            \
         in_shift,                           /* Input scale shift */                                    \
         out_shift,                          /* Output scale shift */                                   \
         name##.##iir_temp,                  /* INTERMEDIATE_CBUF_PTR_FIELD */                          \
         length(name##.##iir_temp),          /* INTERMEDIATE_CBUF_LEN_FIELD */                          \
         0,                                  /* RESET_FLAG_FIELD */                                     \
         prec_16_24bit,                      /* DBL_PRECISSION_FIELD */                                 \
         0 ...;                              /* Zero the rest */                                        \
   .ENDBLOCK;

#define TEMPLATE_SWITCH_OP(name, next_op, en, mask)                                                     \
                                                                                                        \
   /* Switch operator chains according to whether rate matching is enabled */                           \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.switch_op;                                                            \
      .VAR name##.##param[$cbops.switch_op.STRUC_SIZE] =                                                \
         en,   /* Pointer to switch (0: next disable, 1: next enable) */                                \
         0,    /* ALT_NEXT_FIELD, pointer to alternate cbops chain */                                   \
         mask, /* SWITCH_MASK_FIELD */                                                                  \
         0;    /* INVERT_CONTROL_FIELD */                                                               \
   .ENDBLOCK;


#define TEMPLATE_SWITCH_ALT_OP(name, next_op, alt_next_op, en, mask)                                    \
                                                                                                        \
   /* Switch operator chains according to whether rate matching is enabled */                           \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.switch_op;                                                            \
      .VAR name##.##param[$cbops.switch_op.STRUC_SIZE] =                                                \
         en,                                 /* Pointer to switch (1: next, 0: alt_next) */             \
         $##alt_next_op,                     /* ALT_NEXT_FIELD, pointer to alternate cbops chain */     \
         mask,                               /* SWITCH_MASK_FIELD */                                    \
         0;                                  /* INVERT_CONTROL_FIELD */                                 \
   .ENDBLOCK;


#define TEMPLATE_DITHER_AND_SHIFT_OP(name, next_op, in_idx, out_idx, shift, dither_type)                \
                                                                                                        \
   .VAR/DM1CIRC name##.##dither_hist[$cbops.dither_and_shift.FILTER_COEFF_SIZE];                        \
                                                                                                        \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.dither_and_shift;                                                     \
      .VAR name##.##param[$cbops.dither_and_shift.STRUC_SIZE] =                                         \
         in_idx,                                   /* Input index */                                    \
         out_idx,                                  /* Output index */                                   \
         shift,                                    /* Amount of shift after dithering */                \
         dither_type,                              /* Type of dither */                                 \
         name##.##dither_hist,                     /* History buffer for dithering */                   \
         0;                                        /* Enable compressor */                              \
   .ENDBLOCK;

// Always use history buffers large enough for the HD adjustment filter
// (this will work with the standard quality or HD quality filter)
#define TEMPLATE_RATE_ADJUSTMENT_AND_SHIFT_OP(name, next_op, in0_idx, out0_idx, in1_idx, out1_idx, shift_amount, coeffs, sra_rate_ptr) \
                                                                                                        \
   .VAR/DM1CIRC name##.##_sr_hist0[$cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE];        \
   .VAR/DM1CIRC name##.##_sr_hist1[$cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE];        \
                                                                                                        \
   .BLOCK $##name;                                                                                      \
      .VAR name##.##next = $##next_op;                                                                  \
      .VAR name##.##func = $cbops.rate_adjustment_and_shift;                                            \
      .VAR name##.##param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =                                \
         in0_idx,                                   /* Input0 index */                                  \
         out0_idx,                                  /* Output0 index */                                 \
         in1_idx,                                   /* Input1 index */                                  \
         out1_idx,                                  /* Output1 index */                                 \
         shift_amount,                              /* amount of shift*/                                \
         coeffs,                                                                                        \
         name##.##_sr_hist0,                                                                            \
         name##.##_sr_hist1,                                                                            \
         sra_rate_ptr,                                                                                  \
         $cbops.dither_and_shift.DITHER_TYPE_NONE,  /* type of dither */                                \
         0 ...;                                                                                         \
   .ENDBLOCK;

#endif
