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


#ifndef _ISMD_AUDIO_DTS_ENC_PARAMS_H_
#define _ISMD_AUDIO_DTS_ENC_PARAMS_H_

#define ISMD_AUDIO_DTS_ENC_LFE_PRESENT     0x1950 
#define ISMD_AUDIO_DTS_ENC_NUM_INPUT_CH    0x1951  


/*
   0 - LFE not present
   1 - LFE present
*/
typedef int ismd_audio_dts_enc_lfe_present_t;


/* 
   2 - 2 ch pcm input
   4 - 4 ch pcm input
   6 - 6 ch pcm input
*/
typedef int ismd_audio_dts_enc_num_input_ch_t;


#endif /* _ISMD_AUDIO_DTS_ENC_PARAMS_H_ */
