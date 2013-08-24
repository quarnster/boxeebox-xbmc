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


#ifndef _ISMD_AUDIO_DTSHD_PARAMS_H_
#define _ISMD_AUDIO_DTSHD_PARAMS_H_

#include "ismd_audio_dts.h"


/* These are the available user configurable options on the DTS-HD decoder.*/
#define ISMD_AUDIO_DTS_HD_SPKROUT                     ISMD_AUDIO_DTS_SPKROUT           /** speaker output configuration */
#define ISMD_AUDIO_DTS_HD_LFEDNMX                     ISMD_AUDIO_DTS_LFEDNMX           /** LFE Downmix */
#define ISMD_AUDIO_DTS_HD_DOWNMIX_MODE                ISMD_AUDIO_DTS_DOWNMIXMODE       /** downmix mode */
#define ISMD_AUDIO_DTS_HD_DRC_PERCENT                 ISMD_AUDIO_DTS_DRCPERCENT        /** DRC percent - optional*/
#define ISMD_AUDIO_DTS_HD_SEC_AUD_SCALING             ISMD_AUDIO_DTS_SEC_AUD_SCALING   /** Rev2Aux secondary audio scaling */
#define ISMD_AUDIO_DTS_HD_PCM_SAMPLE_WIDTH            0x107                            /** PCM sample width */
#define ISMD_AUDIO_DTS_HD_DECODE_TYPE                 0x108 
#define ISMD_AUDIO_DTS_HD_MULTILE_ASSET_DECODE        0x109                            /** multi-asset decode */
#define ISMD_AUDIO_DTS_HD_ENABLE_DTS_ES_61_MATRIX     0x110                            /** enabled DTS-ES 6.1 Matrix */
#define ISMD_AUDIO_DTS_HD_DISABLE_DIALNORM            0x111                            /** disable dialnorm */
#define ISMD_AUDIO_DTS_HD_NO_DEFAULT_SPKR_REMAP       0x112                            /** disable default speaker remapping */
#define ISMD_AUDIO_DTS_HD_AUDIO_PRESENTATION_SELECT   0x113                            /** audio presentation select */
#define ISMD_AUDIO_DTS_HD_REPLACEMENT_SET_SELECT      0x114                            /** replacement set select */
#define ISMD_AUDIO_DTS_HD_SPKR_REMAP_MODE             0x115                            /** speaker remapping mode */


/** The following are valid values to specify when calling ismd_audio_input_set_decode_param ***/

typedef enum {
  ISMD_AUDIO_DTS_HD_SAMPLE_WIDTH_16 = 16,
  ISMD_AUDIO_DTS_HD_SAMPLE_WIDTH_24 = 24
} ismd_audio_dts_hd_sample_width_t;

typedef enum {
   ISMD_AUDIO_DTS_HD_DECODE_NATIVE,                /* decode native  */
   ISMD_AUDIO_DTS_HD_DECODE_ONLY_96KFROM192K,      /* decode 96k from 192k streams */
   ISMD_AUDIO_DTS_HD_DECODE_ONLY_CORE,             /* decoder Core only */
   ISMD_AUDIO_DTS_HD_DECODE_ONLY_CORE_SUBSTREAM    /* decode Core sub-stream only */
} ismd_audio_dts_hd_decode_type_t;

typedef enum {
   ISMD_AUDIO_DTS_HD_SPKR_REMAP_MODE_EXTERNAL,        /* Use external (post-mix) speaker remapper */
   ISMD_AUDIO_DTS_HD_SPKR_REMAP_MODE_INTERNAL,        /* Use codec internal speaker remapper */
} ismd_audio_dts_hd_spkr_remap_mode_t;

typedef int ismd_audio_dts_hd_audio_prstn_select_t; 
typedef int ismd_audio_dts_hd_replacement_set_t; 
typedef int ismd_audio_dts_hd_secondary_audio_scale_t; /* valid values are 0 or 1 */

/* DTS core inherited values. */
typedef ismd_audio_dts_lfe_downmix_t ismd_audio_dts_hd_lfe_downmix_t;
typedef ismd_audio_dts_downmix_mode_t ismd_audio_dts_hd_downmix_mode_t;
typedef ismd_audio_dts_dynamic_range_compression_percent_t ismd_audio_dts_hd_dynamic_range_compression_percent_t;

/* DTS core inherited output speaker activity masks from DTS core*/
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_DEFAULT           ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_DEFAULT
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C                 ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_LFE1            ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R               ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R_LFE1          ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_LT_RT             ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_LT_RT
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R             ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1        ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R_CS            ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R_CS_LFE1       ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_CS_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_CS          ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_CS_LFE1     ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_CS_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R_LS_RS         ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_LFE1    ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_L_R_LS_RS_LFE1
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1  ISMD_AUDIO_DTS_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1


/* These are helper defines to create the extended output speaker activity masks for DTSHD */
#define AUDIO_DTS_HD_CHANNEL_MASK_LH_RH   0x00020 // Left/right height in front
#define AUDIO_DTS_HD_CHANNEL_MASK_LSR_RSR 0x00040 // Left/right surround in rear
#define AUDIO_DTS_HD_CHANNEL_MASK_CH      0x00080 // Center height in front
#define AUDIO_DTS_HD_CHANNEL_MASK_OH      0x00100 // Over the listeners head
#define AUDIO_DTS_HD_CHANNEL_MASK_LC_RC   0x00200 // Between left/right and center in front
#define AUDIO_DTS_HD_CHANNEL_MASK_LW_RW   0x00400 // Left/right on side in front
#define AUDIO_DTS_HD_CHANNEL_MASK_LSS_RSS 0x00800 // Left/right surround on side
#define AUDIO_DTS_HD_CHANNEL_MASK_LFE2    0x01000 // Second low frequency efftects subwoofer
#define AUDIO_DTS_HD_CHANNEL_MASK_LHS_RHS 0x02000 // Left/right height on side
#define AUDIO_DTS_HD_CHANNEL_MASK_CHR     0x04000 // Center height in rear
#define AUDIO_DTS_HD_CHANNEL_MASK_LHR_RHR 0x08000 // Left/right height in rear
#define AUDIO_DTS_HD_CHANNEL_MASK_CLF     0x10000 // Center low front
#define AUDIO_DTS_HD_CHANNEL_MASK_LLF_RLF 0x20000 // Left/right low front


/* DTS-HD extended output speaker activity masks */
#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_LHS_RHS   AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LHS_RHS   // 0x200F (8207)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LFE1_LSR_RSR_LSS_RSS AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LSR_RSR |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LSS_RSS   // 0x084B (2123)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_LH_RH     AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LH_RH     // 0x002F (47)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_LSR_RSR   AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LSR_RSR   // 0x004F (79)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_CS_CH     AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_CHANNEL_MASK_CS |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_CH        // 0x009F (159)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_CS_OH     AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_CHANNEL_MASK_CS |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_OH        // 0x011F (287)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_LW_RW     AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_HD_CHANNEL_MASK_LW_RW     // 0x040F (1039)

#define ISMD_AUDIO_DTS_HD_SPEAKER_ACTIVITY_MASK_C_L_R_LS_RS_LFE1_CS        AUDIO_DTS_CHANNEL_MASK_C |\
                                                                           AUDIO_DTS_CHANNEL_MASK_L_R |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LS_RS |\
                                                                           AUDIO_DTS_CHANNEL_MASK_LFE1 |\
                                                                           AUDIO_DTS_CHANNEL_MASK_CS        // 0x001F (31)
                                                                        




/*******************************************************************/
/********************* DEPRECATED! *********************************/
/*******************************************************************/
#define	ISMD_AUDIO_DTS_HD_NUMCHANNELS                0x200 /* decoded data channel count */
#define	ISMD_AUDIO_DTS_HD_SAMPLERATE                 0x201 /* sample rate */
#define	ISMD_AUDIO_DTS_HD_BITSPERSAMPLE              0x202 /* decoded data sample width */
#define	ISMD_AUDIO_DTS_HD_CH_MASK                    0x203 /* decode data channel mask */
#define	ISMD_AUDIO_DTS_HD_SPRK_REMAP_COEFF           0x204 /* speaker remapping coefficients */
#define  ISMD_AUDIO_DTS_HD_DOWN_MIX_COEFF             0x205 /* downmix coefficients */

#define ISMD_AUDIO_DTS_HD_DOWNMIX_MODE_PARTIAL  ISMD_AUDIO_DTS_DOWNMIX_MODE_PARTIAL
#define ISMD_AUDIO_DTS_HD_DOWNMIX_MODE_INTERNAL ISMD_AUDIO_DTS_DOWNMIX_MODE_INTERNAL
#define ISMD_AUDIO_DTS_HD_DOWNMIX_MODE_EXTERNAL ISMD_AUDIO_DTS_DOWNMIX_MODE_EXTERNAL

#define ISMD_AUDIO_DTS_HD_LFE_DOWNMIX_NONE ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE
#define ISMD_AUDIO_DTS_HD_LFE_DOWNMIX_10DB ISMD_AUDIO_DTS_LFE_DOWNMIX_10DB

typedef int ismd_audio_dts_hd_speaker_activity_mask_t;

/*******************************************************************/
/******************** END DEPRECATED *******************************/ 
/*******************************************************************/


#endif /* _ISMD_AUDIO_DTSHD_PARAMS_H_ */
