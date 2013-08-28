/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2009 Intel Corporation. All rights reserved.

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

///////////////////////////////////////////////////////////////////////////////
// DTS-Neo6 Application Layer API Functions
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISMD_AUDIO_DTS_NEO6_PARAMS_H_
#define _ISMD_AUDIO_DTS_NEO6_PARAMS_H_


#define ISMD_AUDIO_DTS_NEO6_MODE									   0x100
#define ISMD_AUDIO_DTS_NEO6_CGAIN									0x101
#define ISMD_AUDIO_DTS_NEO6_NUM_OUTPUT_CHANNELS					0x102

   
/* Valid values are 1 for CINEMA and 2 for MUSIC */
typedef enum ENUM_DTS_NEO6_MODE{
   AUDIO_DTS_NEO6_MODE_CINEMA = 1,
   AUDIO_DTS_NEO6_MODE_MUSIC = 2   
}ismd_audio_dts_neo6_mode_t; 

typedef int ismd_audio_dts_neo6_num_output_channels_t;  /* valid values are 3 through 7 */

// TODO: describe this parameter better
/* Valid values are between 0-100 (0 - 1.0) representing .1 steps.  */
typedef int ismd_audio_dts_neo6_cgain_t;

#define ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_C       0x00001 // Center in front of listener
#define ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R     0x00002 // Left/right in front
#define ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LS_RS   0x00004 // Left/right surround on side in rear
#define ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_CS      0x00010 // Center surround in rear
#define ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LSR_RSR 0x00040 // Left/right surround in rear

//Speaker mask
typedef enum{
         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_DEFAULT               = 0,  

         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_L_R_C                 = ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_C, //0x0003(3)

         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_L_R_LS_RS             = ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LS_RS, //0x0006(6)

         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_C           = ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LS_RS|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_C, //0x0007(7)

         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_C_CS        = ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LS_RS|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_C|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_CS, //0x0017(23)

         ISMD_AUDIO_DTS_NEO6_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_C_LSR_RSR   = ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_L_R|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LS_RS|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_C|\
                                                                           ISMD_AUDIO_DTS_NEO6_CHANNEL_MASK_LSR_RSR, //0x0047(71)
                                                                           
}ismd_audio_dts_neo6_speaker_activity_mask_t;
 

#endif


