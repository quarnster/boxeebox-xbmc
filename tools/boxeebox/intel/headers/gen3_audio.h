
/*
* File Name: gen3_audio.h
*/

/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2006-2009 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:

  Intel Corporation
  2200 Mission College Blvd.
  Santa Clara, CA  97052

  BSD LICENSE

  Copyright(c) 2009 Intel Corporation. All rights reserved.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
*	Authors:  Intel Audio Subsystem Software Team
*/


/** @ingroup ismd_audio
    @{
*/

#ifndef _GEN3_AUDIO_H_
#define _GEN3_AUDIO_H_

#include "ismd_audio.h"
#include "ismd_audio_defs.h"

/** Value to specify for hardware output for I2S0 (stereo output)*/
#define GEN3_HW_OUTPUT_I2S0 0x3D1
/** Value to specify for hardware output for I2S1 (multichannel output) */
#define GEN3_HW_OUTPUT_I2S1 0x3D2
/** Value to specify for hardware output for SPDIF */
#define GEN3_HW_OUTPUT_SPDIF 0x3D3
/** Value to specify for hardware output for HDMI */
#define GEN3_HW_OUTPUT_HDMI 0x3D4

/** Value to specify for hardware input device. (Capture) */
#define GEN3_HW_INPUT_I2S0 0x4D1

/** Defines the settings of the SRC. Set to preformance or quality. (currently not used)*/
typedef enum {
   ISMD_AUDIO_SRC_PERFORMANCE = 0xbb,
   ISMD_AUDIO_SRC_QUALITY = 0xbc
} audio_src_quality_t;

/** Currently this function is not implemented.
This configuration option is used to control which version of the sample rate converter (SRC) is to
be used for any necessary re-sampling needed.  It is recommended that this parameter
be set to fastest setting for all but the most demanding use cases.  Please note that setting parameter
to highest quality setting may result in significantly higher CPU load on the DSP or the host CPU.

@note This is a hardware-specific function.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] src_quality : Quality setting for the audio Sample Rate Converter.

@retval ISMD_SUCCESS : The sampling rate converstion quality was set successfully.
@retval ISMD_NULL_POINTER : Handle passed in was NULL.
*/
ismd_result_t SMDEXPORT ismd_audio_set_sample_rate_convert_quality(ismd_audio_processor_t processor_h,
                              audio_src_quality_t  src_quality);


/** @} */  // end of ingroup ismd_audio

#endif

