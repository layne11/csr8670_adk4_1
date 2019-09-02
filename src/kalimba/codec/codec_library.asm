// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifdef DEBUG_ON
   #define ENABLE_PROFILER_MACROS
#endif

#include "profiler.h"
#include "segments.asm"
#include "stream_encode.asm"
#include "stream_decode.asm"
#include "stream_decode_sync.asm"
#include "stream_relay.asm"

#ifdef BUILD_WITH_C_SUPPORT
  #include "codec_library_c_stubs.asm"
#endif
