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

This file defines the parameters for the Dolby Digital Plus decoder.  The 
defines and enums are intended to be used when calling the functions
ismd_audio_input_set_decoder_param() and ismd_audio_input_get_decoder_param().

Example usage: 

   void set_codec_parameters( ismd_audio_processor_t proc_handle,
                              ismd_dev_t             input_handle )
   {
      ismd_result_t                                  result;
      ismd_audio_codec_param_t                       param;
      ismd_audio_ddplus_karaoke_reproduction_mode_t *karaoke_mode;
      ismd_audio_ddplus_stereo_output_mode_t        *stereo_mode;

      karaoke_mode = (ismd_audio_ddplus_karaoke_reproduction_mode_t *)&param;
      *karoke_mode = ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE_RIGHT_VOCAL;
      result = ismd_audio_decoder_param_set( proc_handle, 
                                           input_handle, 
                                           ISMD_AUDIO_CODEC_PARAM_DDPLUS_KARAOKE_REPRODUCTION_MODE, 
                                           &param );

      stereo_mode = (ismd_audio_ddplus_stereo_output_mode_t *)&param;
      *stereo_mode = ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_STEREO;
      result = ismd_audio_decoder_param_set( proc_handle, 
                                           input_handle, 
                                           ISMD_AUDIO_CODEC_PARAM_DDPLUS_STEREO_OUTPUT_MODE, 
                                           &param );
      
      return;
   }

*/


#ifndef _ISMD_AUDIO_DDPLUS_PARAMS_H_
#define _ISMD_AUDIO_DDPLUS_PARAMS_H_

#define ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE                 0x100  /* see ismd_audio_ddplus_karaoke_reproduction_mode_t */
#define ISMD_AUDIO_DDPLUS_DYNAMIC_RANGE_COMPRESSION_MODE            0x101  /* see ismd_audio_ddplus_dynamic_range_compression_mode_t */
#define ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT                        0x102  /* see ismd_audio_ddplus_lfe_channel_output_t */
#define ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION                      0x103  /* see ismd_audio_ddplus_output_configuration_t */
#define ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE                        0x104  /* see ismd_audio_ddplus_stereo_output_mode_t */
#define ISMD_AUDIO_DDPLUS_DUAL_MONO_REPRODUCTION_MODE               0x105  /* see ismd_audio_ddplus_dual_mono_reproduction_mode_t */
#define ISMD_AUDIO_DDPLUS_UPSAMPLING_MODE                           0x106  /* see ismd_audio_ddplus_upsampling_mode_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_L                           0x107  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_C                           0x108  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_R                           0x109  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Ls                          0x10A  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Rs                          0x10B  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_LFE                         0x10C  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Lrs                         0x10D  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Rrs                         0x10E  /* see ismd_audio_ddplus_channel_route_t */
#define ISMD_AUDIO_DDPLUS_PCM_SCALE_FACTOR                          0x10F  /* see ismd_audio_ddplus_pcm_scale_factor_t */
#define ISMD_AUDIO_DDPLUS_DYNAMIC_RANGE_HIGH_FREQ_CUT_SCALE_FACTOR  0x110  /* see ismd_audio_ddplus_high_freq_dynamic_cut_scale_factor_t */
#define ISMD_AUDIO_DDPLUS_DYNAMIC_RANGE_LOW_FREQ_BOOST_SCALE_FACTOR 0x111  /* see ismd_audio_ddplus_low_freq_dynamic_boost_scale_factor_t */
#define ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS                       0x112  /* see ismd_audio_ddplus_num_output_channels_t */
#define ISMD_AUDIO_DDPLUS_QUITONERR                                 0x113  /* see ismd_audio_ddplus_quitonerr_t */
#define ISMD_AUDIO_DDPLUS_FRAMESTART                                0x114  /* see ismd_audio_ddplus_framestart_t */
#define ISMD_AUDIO_DDPLUS_FRAMEEND                                  0x115  /* see ismd_audio_ddplus_frameend_t */

typedef enum {
   ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE_NO_VOCAL,
   ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE_LEFT_VOCAL,
   ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE_RIGHT_VOCAL,
   ISMD_AUDIO_DDPLUS_KARAOKE_REPRODUCTION_MODE_BOTH_VOCALS // should be default
} ismd_audio_ddplus_karaoke_reproduction_mode_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_CUSTOM_MODE_ANALOG_DIALOG_NORMALIZATION,
   ISMD_AUDIO_DDPLUS_CUSTOM_MODE_DIGITAL_DIALOG_NORMALIZATION,
   ISMD_AUDIO_DDPLUS_LINE_OUT_MODE, //default
   ISMD_AUDIO_DDPLUS_RF_REMOTE_MODE
} ismd_audio_ddplus_dynamic_range_compression_mode_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_NONE,
   ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_ENABLED, //default
   ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_DUAL
} ismd_audio_ddplus_lfe_channel_output_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_RAW_MODE = -1, //default
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_RESERVED,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_1_0_C,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_0_LR,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_0_LCR,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_1_LRl,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_1_LCRl,        
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_LRlr,           
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_LCRlr,       
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_0_1_LCRCvh,    
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_1_LRlrTs,    
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_1_LCRlrTs,   
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_1_LCRlrCvh,  
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_0_2_LCRLcRc,   
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_2_LRlrLwRw,  
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_2_LRlrLvhRvh,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_2_LRlrLsdRsd,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_2_2_2_LRlrLrsRrs,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrLcRc, 
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrLwRw, 
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrLvhRv,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrLsdRs,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrLrsRr,
   ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_2_LCRlrTsCvh
} ismd_audio_ddplus_output_configuration_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_AUTO_DETECT, //default
   ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE, 
   ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_STEREO
} ismd_audio_ddplus_stereo_output_mode_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_DUAL_MONO_REPRODUCTION_MODE_STEREO, //default
   ISMD_AUDIO_DDPLUS_DUAL_MONO_REPRODUCTION_MODE_LEFT_MONO, 
   ISMD_AUDIO_DDPLUS_DUAL_MONO_REPRODUCTION_MODE_RIGHT_MONO,
   ISMD_AUDIO_DDPLUS_DUAL_MONO_REPRODUCTION_MODE_MIXED_MONO
} ismd_audio_ddplus_dual_mono_reproduction_mode_t;


typedef enum {
   ISMD_AUDIO_DDPLUS_UPSAMPLING_MODE_USE_BITSTREAM, //default
   ISMD_AUDIO_DDPLUS_UPSAMPLING_MODE_ALWAYS, 
   ISMD_AUDIO_DDPLUS_UPSAMPLING_MODE_NEVER
} ismd_audio_ddplus_upsampling_mode_t;

typedef enum {
   ISMD_AUDIO_DDPLUS_QUITONERR_DISABLED, //default
   ISMD_AUDIO_DDPLUS_QUITONERR_ENABLED, 
} ismd_audio_ddplus_quitonerr_t;

/* 
Use this value to specify the channel location in the output buffer. For the AC3 decoder.
These parameters should be used if setting the ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION
parameter and the ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS parameter. If the channel 
routing is not changed there is no garuantee that the correct channels will come
out in the decoder output buffer. 

Example of a valid setup:

ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION   --> 2
ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS    --> 2
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_L        --> 0  //Means left channel will be in buffer offset location 0.
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_R        --> 1  //Means right channel will be in buffer offset location 1.

//It is mandatory to initialize the rest of these parameters with the remaining offset locations to 
//avoid unpredictable behavior.
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_C        --> 2
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Ls       --> 3
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Rs       --> 4
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_LFE      --> 5
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Lrs      --> 6
ISMD_AUDIO_DDPLUS_CHANNEL_ROUTE_Rrs      --> 7
*/
typedef int ismd_audio_ddplus_channel_route_t; /* valid values are 0 through 7 */

#define ISMD_AUDIO_DDPLUS_MIN_NUM_OUTPUT_CHANNELS 1
#define ISMD_AUDIO_DDPLUS_MAX_NUM_OUTPUT_CHANNELS 8

/* Set the num output channels to this value to force the decoder to always 
   output the same number of channels as the input stream. */
#define ISMD_AUDIO_DDPLUS_DEFAULT_NUM_OUTPUT_CHANNELS -1 

/* Number of channels to output from the decoder.  Valid values are 
  ISMD_AUDIO_DDPLUS_MIN_OUTPUT_CHANNELS through 
  ISMD_AUDIO_DDPLUS_MAX_OUTPUT_CHANNELS or 
  ISMD_AUDIO_DDPLUS_DEFAULT_NUM_OUTPUT_CHANNELS */
typedef int ismd_audio_ddplus_num_output_channels_t;  

/* Valid scale factors use 1.23 fixed point intergers. */
/* range is from 0 through 0x7FFFFF */
/* where 0=OFF, and MAX(1.0) = 0x7FFFFF,  */
/* 50% = (int)(0x7FFFFF*0.50), 25% = (int)(0x7FFFFF*0.25), ..etc */
typedef int ismd_audio_ddplus_pcm_scale_factor_t;
typedef int ismd_audio_ddplus_high_freq_dynamic_cut_scale_factor_t;
typedef int ismd_audio_ddplus_low_freq_dynamic_boost_scale_factor_t;

typedef int ismd_audio_ddplus_framestart_t;
typedef int ismd_audio_ddplus_frameend_t;

#endif /* _ISMD_AUDIO_DDPLUS_PARAMS_H_ */
