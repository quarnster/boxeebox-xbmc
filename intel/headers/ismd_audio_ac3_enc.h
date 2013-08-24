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

/* 

This file defines the parameters for the AC3 encoder.  The 
defines and enums are intended to be used when calling the functions
ismd_audio_input_set_decoder_param() and ismd_audio_input_get_decoder_param().

Example usage: 

   void set_codec_parameters( ismd_audio_processor_t proc_handle,
                              ismd_dev_t             input_handle )
   {
      ismd_result_t                                  result;
      ismd_audio_codec_param_t                       param;
      ismd_audio_ac3_karaoke_reproduction_mode_t *karaoke_mode;
      ismd_audio_ac3_stereo_output_mode_t        *stereo_mode;

      karaoke_mode = (ismd_audio_ac3_karaoke_reproduction_mode_t *)&param;
      *karoke_mode = ISMD_AUDIO_AC3_KARAOKE_REPRODUCTION_MODE_RIGHT_VOCAL;
      result = ismd_audio_decoder_param_set( proc_handle, 
                                           input_handle, 
                                           ISMD_AUDIO_CODEC_PARAM_AC3_KARAOKE_REPRODUCTION_MODE, 
                                           &param );

      stereo_mode = (ismd_audio_ac3_stereo_output_mode_t *)&param;
      *stereo_mode = ISMD_AUDIO_AC3_STEREO_OUTPUT_MODE_STEREO;
      result = ismd_audio_decoder_param_set( proc_handle, 
                                           input_handle, 
                                           ISMD_AUDIO_CODEC_PARAM_AC3_STEREO_OUTPUT_MODE, 
                                           &param );
      
      return;
   }

*/


#ifndef _ISMD_AUDIO_AC3_ENC_PARAMS_H_
#define _ISMD_AUDIO_AC3_ENC_PARAMS_H_

#define ISMD_AUDIO_AC3_ENC_ACMOD                0x950 
#define ISMD_AUDIO_AC3_ENC_WDSZ                 0x951  
#define ISMD_AUDIO_AC3_ENC_DATA_RATE_CODE       0x952  
#define ISMD_AUDIO_AC3_ENC_LFEFILTINUSE         0x953  
#define ISMD_AUDIO_AC3_ENC_NUM_ICH              0x954  
#define ISMD_AUDIO_AC3_ENC_COMPCHAR             0x955  
#define ISMD_AUDIO_AC3_ENC_COMPCHAR2            0x956  
#define ISMD_AUDIO_AC3_ENC_LOFREQEFFON          0x957  
#define ISMD_AUDIO_AC3_ENC_TESTMODEON           0x958  
#define ISMD_AUDIO_AC3_ENC_SAMP_FREQ            0x959  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_L      0x960  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_C      0x961  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_R      0x962  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_Ls     0x963  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_Rs     0x964  
#define ISMD_AUDIO_AC3_ENC_CHANNEL_ROUTE_LFE    0x965  


/*
   acmod – Audio coding mode
   .. 0 – 1+1 (L, R)
   .. 1 – 1/0 (C)
   .. 2 – 2/0 (L, R)
   .. 7 – 3/2 (L, C, R, l, r) (default). Note that this is the only audio coding mode
   supported by the DDCO library.
*/
typedef int ismd_audio_ac3_enc_acmod_t;


/* 
– Input PCM sample bit size.
.. 16 – 16-bit input PCM samples (default)
.. 24 – 24-bit input PCM samples.
*/
typedef int ismd_audio_ac3_enc_pcm_wdsz_t;

/* 
Code indicating the data rate of the encoded stream. This
parameter is also referred to as frame size code in the Dolby reference documents.
See the table below for a list of valid data rate codes and the corresponding valid
audio coding modes (acmod). Note that the only data rate supported by the DDCO
library is 640 kbps (data rate code 18).

Table 3-1 Data Rate Codes
Code  Rate(kbps)  Coding Mode
4     64          1
6     96          1
8     128         0, 1, 2
10    192         0, 2
12    256         0, 2
14    384         0, 2, 7
15 (DDCE dflt)    448 7
18 (DDCO dflt)    640 7
*/
typedef int ismd_audio_ac3_enc_data_rate_code_t;

/* 
lfefiltinuse – Activate the LFE (Low Frequency Effect) low pass filter.
.. 0 – LFE filter disabled
.. 1 – LFE filter enabled (default)
*/
typedef int ismd_audio_ac3_enc_lfe_filter_in_use_t;


/* 
num_ich – Total number of input PCM channels available in the input buffer.
.. Valid values: 1, 2, 5 or 6 (default). Note that the DDCO library supports only 6
input channels.
*/
typedef int ismd_audio_ac3_enc_num_ich_t;

/* 
compchar – Control dynamic range compression (DRC). In dual-mono mode DRC is
applied only on the L channel. In the other encoding modes, DRC is applied across all
input channels.
.. 0 – Dynamic range compression inactive (default)
.. 1 – Dynamic range compression active.
*/

typedef int ismd_audio_ac3_enc_compchar_t;

/* 
compchar2 – Control dynamic range compression of the R (or the 2nd) channel in
dual-mono mode. This option has no effect in other encoding modes.
.. 0 – Dynamic range compression inactive (default)
.. 1 – Dynamic range compression active.
*/
typedef int ismd_audio_ac3_enc_compchar2_t;

/* 
lofreqeffon – Presence of LFE (Low Frequency Effect) channel at the input. If the
LFE channel is present, it is encoded only if the acmod value is 7.
.. 0 – LFE channel absent
.. 1 – LFE channel present (default)
*/
typedef int ismd_audio_ac3_enc_low_freq_eff_on_t;

/* 
samp_freq – Input sampling frequency of the input channels.
.. Valid value: 48000 (default). This Dolby Digital (AC-3) Encoder implementation
does not support any other sampling rates.
*/
typedef int ismd_audio_ac3_enc_samp_freq_t;

/* 
testmodeon – Enable or disable white-box test mode of the encoder library.
.. 0 – Test mode inactive (default)
.. 1 – Test mode active.
*/
typedef int ismd_audio_ac3_enc_test_mode_on_t;

/* 
chanrouting – Channel routing in the input PCM buffer. Channel indices 0, 1, 2, 3,
   4, and 5 correspond to L, C, R, l, r, and LFE respectively. The channel offset (0
   through 5) is specified in bits 8 through 13 of chanrouting, while the channel index
   is specified in bits 0 through 5. You need to set this parameter multiple times to route
   different channels.

.. Default routing: [0:L 1:C 2:R 3:l 4:r 5:LFE] (i.e., the samples in the input
   buffer will be interleaved in the following order throughout the entire input length:
   Left channel sample, Center channel sample, Right channel sample, Leftsurround
   channel sample, Right-surround channel sample, and LFE channel
   sample).
*/
typedef int ismd_audio_ac3_enc_ch_routing_t;


#endif /* _ISMD_AUDIO_AC3_ENC_PARAMS_H_ */
