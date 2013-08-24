/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2008-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2006-2009 Intel Corporation. All rights reserved.
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

#ifndef _ISMD_AUDIO_DTS_H_
#define _ISMD_AUDIO_DTS_H_

/* These are the available user configurable options on the DTS core decoder.*/
#define ISMD_AUDIO_DTS_SPKROUT										0x100
#define ISMD_AUDIO_DTS_DRCPERCENT									0x101
#define ISMD_AUDIO_DTS_NUM_OUTPUT_CHANNELS						0x102
#define ISMD_AUDIO_DTS_LFEDNMX										0x104
#define ISMD_AUDIO_DTS_DOWNMIXMODE									0x105
#define ISMD_AUDIO_DTS_SEC_AUD_SCALING	      	   			0x106


/** Values to specify when calling \ref ismd_audio_input_set_decoder_param using \ref ISMD_AUDIO_DTS_LFEDNMX option.*/
typedef enum {
   ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE, /** Ignore LFE data */
   ISMD_AUDIO_DTS_LFE_DOWNMIX_10DB  /** Mix LFE data into front channels with 10dB boost */
} ismd_audio_dts_lfe_downmix_t;


/** Values to specify when calling \ref ismd_audio_input_set_decoder_param using \ref ISMD_AUDIO_DTS_DOWNMIXMODE option.*/
typedef enum {
   ISMD_AUDIO_DTS_DOWNMIX_MODE_INVALID = -1,
   ISMD_AUDIO_DTS_DOWNMIX_MODE_EXTERNAL,   /** This will tell the decoder to perform no downmixing internally. The downmix, if needed, will be handled in a post mixer stage.*/
   ISMD_AUDIO_DTS_DOWNMIX_MODE_INTERNAL,   /** All downmixing will happen internal to the decoder */ 
   ISMD_AUDIO_DTS_DOWNMIX_MODE_PARTIAL  /** Decoder partially downmixes output to 5.1 */ 
} ismd_audio_dts_downmix_mode_t;


typedef int ismd_audio_dts_dynamic_range_compression_percent_t; /* valid values are 0 through 100 */
typedef int ismd_audio_dts_speaker_output_configuration_t; /* valid values are */
typedef int ismd_audio_dts_num_output_channels_t;  /* valid values are 1 through 6 */
typedef int ismd_audio_dts_secondary_audio_scale_t; /* valid values are 0 or 1 */


/* These are helper defines to create the many output speaker activity masks. */
#define AUDIO_DTS_CHANNEL_MASK_C       0x00001 // Center in front of listener
#define AUDIO_DTS_CHANNEL_MASK_L_R     0x00002 // Left/right in front
#define AUDIO_DTS_CHANNEL_MASK_LS_RS   0x00004 // Left/right surround on side in rear
#define AUDIO_DTS_CHANNEL_MASK_LFE1    0x00008 // Low frequency effects subwoofer
#define AUDIO_DTS_CHANNEL_MASK_CS      0x00010 // Center surround in rear
#define AUDIO_DTS_CHANNEL_MASK_LT_RT   0x40000 // Left total/right total


/* Speaker activity masks available to apply to DTS core decoder's output */
#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_DEFAULT              0

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C                    AUDIO_DTS_CHANNEL_MASK_C       // 0x0001 (1) - ISMD_AUDIO_DOWNMIX_1_0

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_LFE1               AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x0009 (9) - ISMD_AUDIO_DOWNMIX_1_0_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R                  AUDIO_DTS_CHANNEL_MASK_L_R     // 0x0002 (2) - ISMD_AUDIO_DOWNMIX_2_0

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LFE1             AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x000A (10) - ISMD_AUDIO_DOWNMIX_2_0_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_LT_RT                AUDIO_DTS_CHANNEL_MASK_LT_RT   //0x40000 (262144) - ISMD_AUDIO_DOWNMIX_2_0_LTRT

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R                AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_L_R     // 0x0003 (3) - ISMD_AUDIO_DOWNMIX_3_0

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1           AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x000B (11) - ISMD_AUDIO_DOWNMIX_3_0_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS               AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_CS      // 0x0012 (18) - ISMD_AUDIO_DOWNMIX_2_1

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS_LFE1          AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_CS |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x001A (26) - ISMD_AUDIO_DOWNMIX_2_1_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS             AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_CS      // 0x0013 (19) - ISMD_AUDIO_DOWNMIX_3_1

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS_LFE1        AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_CS |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x001B (27) - ISMD_AUDIO_DOWNMIX_3_1_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS            AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LS_RS   // 0x0006 (6) - ISMD_AUDIO_DOWNMIX_2_2

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_LFE1       AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x000E (14) - ISMD_AUDIO_DOWNMIX_2_2_LFE

#define ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1     AUDIO_DTS_CHANNEL_MASK_C |\
                                                                  AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                  AUDIO_DTS_CHANNEL_MASK_LFE1    // 0x000F (15) - ISMD_AUDIO_DOWNMIX_3_2_LFE

/*******************************************************************/
/********************* DEPRECATED! *********************************/
/*******************************************************************/
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_DEFAULT           ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_DEFAULT
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C                 ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_LFE1            ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R               ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R_LFE1          ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_LT_RT             ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_LT_RT
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_L_R             ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1        ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R_CS            ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R_CS_LFE1       ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_L_R_CS          ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_L_R_CS_LFE1     ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R_LS_RS         ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_LFE1    ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_LFE1
#define AUDIO_DTSHD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1  ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1

typedef int ismd_audio_dts_bitspersample_t;
typedef int enum_dtshd_speaker_activity_mask_t;
typedef int ismd_audio_dts_downmixmode_t;
typedef ismd_audio_dts_lfe_downmix_t enum_dtshd_lfe_downmix_t;
#define AUDIO_DTSHD_LFE_DOWNMIX_NONE ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE
#define AUDIO_DTSHD_LFE_DOWNMIX_10DB ISMD_AUDIO_DTS_LFE_DOWNMIX_10DB

/*******************************************************************/
/******************** END DEPRECATED *******************************/ 
/*******************************************************************/

#endif /* _ISMD_AUDIO_DTS_H_ */

