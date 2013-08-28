/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2010 Intel Corporation. All rights reserved.

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

  Copyright(c) 2010 Intel Corporation. All rights reserved.
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


#ifndef _ISMD_AUDIO_AAC_PARAMS_H_
#define _ISMD_AUDIO_AAC_PARAMS_H_

/* Use these defines to change the AAC decoder parameters */

/** Flag to specify duplication of mono signals to stereo. See \ref ismd_audio_aac_mono_to_stereo_t */
#define ISMD_AUDIO_AAC_MONO_TO_STEREO_MODE                 0x3000  
/** Flag to dynamically enable/disbale AAC downmixing. See \ref ismd_audio_aac_downmixer_t */
#define	ISMD_AUDIO_AAC_DOWNMIXER_ON_OFF				        0x5000

/** Flag to specify duplication (and scaling by 0.707) of /S into Ls and Rs. See \ref ismd_audio_aac_surround_mono_to_stereo_t*/
#define ISMD_AUDIO_AAC_SURROUND_MONO_TO_STEREO_MODE        0x7000  


//TODO: Add more params 



/** Valid values to specify for decode param \ref ISMD_AUDIO_AAC_MONO_TO_STEREO_MODE */
typedef enum {
   ISMD_AUDIO_AAC_MONO_TO_STEREO_SINGLE_CHANNEL,   /** Mono streams are presented as a single channel routed to C */
   ISMD_AUDIO_AAC_MONO_TO_STEREO_2_CHANNELS        /** Mono streams are presented as 2 identical channels (default) L = R = 0.707xC */
} ismd_audio_aac_mono_to_stereo_t;

/** Valid values to specify for decode param \ref ISMD_AUDIO_AAC_DOWNMIXER_ON_OFF */
typedef enum {
   ISMD_AUDIO_AAC_DOWNMIXER_DISABLE,   /** Disable AAC downmixer */
   ISMD_AUDIO_AAC_DOWNMIXER_ENABLE     /** Enable AAC downmixer */
}  ismd_audio_aac_downmixer_t;

/** Valid values to specify for decode param \ref ISMD_AUDIO_AAC_SURROUND_MONO_TO_STEREO_MODE */
typedef enum {
   ISMD_AUDIO_AAC_SURROUND_MONO_TO_STEREO_SINGLE_CHANNEL,   /** Ls = /S */
   ISMD_AUDIO_AAC_SURROUND_MONO_TO_STEREO_2_CHANNELS        /** Ls = Rs = 0.707 x /S */
}  ismd_audio_aac_surround_mono_to_stereo_t;





#endif /* _ISMD_AUDIO_AAC_PARAMS_H_ */
