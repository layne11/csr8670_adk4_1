// *****************************************************************************
// Copyright (c) 2008 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


// *****************************************************************************
// NAME:
//    Audio Process Library
//
// DESCRIPTION:
//    This library contains the following modules:
//       - delay
//       - peq: Parametric Equalizer
//       - hq_peq: Higher quality EQ
//       - bass manager
//       - speaker control system (combined crossover and bass manager)
//       - peak signal monitor
//       - stream gain
//       - stream mixer
//
// *****************************************************************************

#ifndef AUDIO_PROC_LIBRARY_INCLUDED
#define AUDIO_PROC_LIBRARY_INCLUDED

#include "flash.h"
#include "delay.asm"
#include "peq.asm"
#include "hq_peq.asm"
#include "peak_monitor.asm"
#include "stream_gain.asm"
#include "stream_mixer.asm"
#include "cmpd100.asm"
#include "stereo_3d_enhancement.asm"
#include "mute_control.asm"
#include "stereo_copy.h"
#include "bass_management.asm"
#include "dbe.asm"
#include "spkr_ctrl_system.asm"
#include "latency_measure.asm"
#include "meloD_expansion.asm"
#include "vse.asm"

#endif // AUDIO_PROC_LIBRARY_INCLUDED
