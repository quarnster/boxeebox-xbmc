/*
* File Name: ismd_audio.h
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


/** @defgroup ismd_audio Audio

<H3>Overview</H3>

The SMD Audio module provides many audio processing capabilities including
decoding, sample rate conversion, mixing, volume control, output, etc.

Audio capabilities are exposed via 'Audio Processors.' Each Audio Processor is a
logical object that accepts compressed or uncompressed audio via logical or
physical inputs, mixes the content from all inputs, and sends the mixed result
to one or more outputs. Applications first allocate an audio processor object
and then dynmamically add the required inputs and outputs to it. Each output can
be configured independently with settings for how the content should be sent to
each output (e.g. sample rate, channel configuration, etc.).

\dot
digraph g {
  rankdir="LR";
  subgraph clusterAudioProc {
    label = "Audio Processor";
    mixer [shape="box"]
    input0 -> mixer;
    input1 -> mixer;
    inputN -> mixer;
    mixer -> output0;
    mixer -> output1;
    mixer -> outputN;
    input0 -> output0 [style = "dashed", label = "passthrough"];
  }

  i0 -> input0;
  i1 -> input1;
  iN -> inputN;
  output0 -> o0;
  output1 -> o1;
  outputN -> oN;

  i0 [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
  i1 [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
  iN [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
  o0 [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
  o1 [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
  oN [label="",shape=circle,height=0.12,width=0.12,fontsize=1];
}
\enddot

Some configuration is performed using audio processors. For example, mixing,
volume, and speaker configuration are configured on audio processors.
Configuration of inputs and outputs is performed on the input or output objects,
themselves.

The most typical usage model is to use a single Audio Processor including all of
the platform's physical outputs and inputs for all of the streams currently
being played. However, clients may instantiate multiple audio processors, if
needed. Limitations on the maximum number of audio processors and their maximum,
combined capabilities are platform-specific.

<H3>Physical versus Logical Inputs and Outputs</H3>
Inputs and outputs may be <i>physical</i> or <i>logical</i> interfaces.
<i>Physical</i> interfaces correspond to specific, physical inputs or outputs on
a platform, for example, a specific 2-channel I2S output. The specific physical
audio interfaces supported by each platform are declared in hardware-specific
header files (e.g. hw_audio.h).

<i>Logical</i> interfaces are standard \ref smd_core "SMD Ports" where buffers
may be read or written by the client or which may be connected to upstream or
downstream modules.

<H3>Outputs and Output Formats</H3>

Audio processor outputs have many attributes for controlling how data should be
sent to each output including overall format, sample rate, sample size, channel
configuration, etc.

Output formats include <i>PCM</i>, <i>pass-through</i>, and <i>encoded</i>
formats:
- <i>PCM</i> (\ref ISMD_AUDIO_OUTPUT_PCM) indicates that the mixed result of all
inputs should be sent as uncompressed, PCM samples.
- One input on each audio processor may be specified as the <i>primary input</i>
for that processor. <i>Pass-through</i> (\ref ISMD_AUDIO_OUTPUT_PASSTHROUGH)
mode specifies that the primary input stream should be sent to an output without
mixing in other inputs and with as little modification as possible. This is
useful for transmitting an original, compressed AC3 stream via S/PDIF, for
example. If an output cannot transmit a particular primary input stream (e.g.
S/PDIF outputs cannot transmit Dolby(R) Digital Plus streams), the stream will
be converted to a format compatible with the output.
- <i>Encoded</i> formats (e.g. \ref ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL and
\ref ISMD_AUDIO_OUTPUT_ENCODED_DTS) indicate that the mixed result of all inputs
should be locally encoded/compressed into a specified format before output. This
is most useful for 5.1 channel output via S/PDIF.

<H3>Inputs</H3>

Audio processor inputs may either be <i>timed</i> or <i>untimed</i>.
- <i>Timed</i> inputs use presentation timestamps to control when each segment
of input audio data is mixed and output.
- <i>Untimed</i> inputs mix and output supplied data as soon as possible after
any previous data supplied to the input.

Since each input to an audio processor may carry an independent stream, each
timed input is a separate SMD Module and has an independent playback state (e.g.
stopped, playing, etc.) and independent timing (e.g. clocks, stream base time,
etc.).

One input may be specified as the <i>primary input</i> for an audio processor.
The primary input will be the stream sent to outputs configured for pass-through
mode and will also be given the highest priority for minimizing noise (e.g. it
will drive the sample rate at which mixing is performed).

One input may also be specified as the <i>secondary<i> input.  In certain audio
applications where multiple dependent streams are mixed together, the secondary
stream carries metadata that specifies how the primary and secondary streams
should be mixed togehter.

<H3>Global Audio Processor</H3>

It is common to want the audio from multiple, independent clients (e.g. multiple
applications) to be sent to all physical platform audio outputs. The outputs
might be configured and controlled by yet another client. In order to support
this, SMD Audio provides a way for any client to gain access to a singleton
audio processor instance, called the "Global Audio Processor," via
\ref ismd_audio_open_global_processor. The client that will control the physical
audio outputs adds and configures all of the desired physical outputs to the
Global Audio Processor. Each client wishing to output audio adds its own
input(s) to the Global Audio Processor for submitting its data.

<H3>Theory of Operation:</H3>

I. Setup/Initialization
-# Open the Global Audio Processor using \ref ismd_audio_open_global_processor.
-# Add, configure all physical output ports to be enabled to the Global Audio
Processor using \ref ismd_audio_add_phys_output and the various output

configuration functions.

II. Application audio output

-# Open the Global Audio Processor using \ref ismd_audio_open_global_processor.
-# Add one or more inputs using \ref ismd_audio_add_input_port.
-# Start any timed inputs by setting the appropriate inputs's state to "playing"
using \ref ismd_dev_set_state.
-# Supply audio data to the input port(s) using \ref ismd_port_write or by
connecting the port to an upstream port using \ref ismd_port_connect.

<H3>HDMI</H3>

When switching video modes, the HDMI driver resets it's internal state and will stop accepting data from the audio driver.

The client needs to follow below sequence when changing the HDMI video mode while HDMI audio is enabled.

-# call ismd_audio_output_disable() for the HDMI audio output
-# change video mode
-# verify that the current HDMI audio output configuration is valid for the new video mode.  (I.E. if the HDMI audio output is configured for 192kHz, 8-channel, 24-bit, PCM, then you switch the video mode from 1080p to 480i, it's not going to work).
-# If the new video mode does not support the previous HDMI audio format, then reconfigure the HDMI audio output to an appropriate setting.
-# Call ismd_audio_output_enable() for the HDMI audio output.

*/

#ifndef _ISMD_AUDIO_H_
#define _ISMD_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup ismd_audio */
/** @{ */

#include "ismd_msg.h"
#include "ismd_global_defs.h"
#include "ismd_audio_defs.h"
#include "gen3_audio.h"
#include "ismd_clock_recovery.h"

/*********************************************************************************************/

/*! @name Audio Processor APIs*/

/**
Open and return a handle to the single, global audio processor if the instance is available. If the
global processor is already created, the handle to the already created global processor is returned.
There exists only one global processor that can be accessed from multiple independent applications.

@param[out] global_processor_h : Handle to the global Audio Processor instance.

@retval ISMD_SUCCESS : The global Audio Processor instance returned successfully.
@retval ISMD_ERROR_NO_RESOURCES : No handles are available or unable to allocate required memory.
*/
ismd_result_t SMDCONNECTION ismd_audio_open_global_processor(ismd_audio_processor_t *global_processor_h);

/**
Opens and returns a handle to an Audio Processor if an instance is available. The handle will be used
to manage the audio processing and alows the user to add inputs and outputs to the instance.  If other
audio processors are already open, then the physical inputs and outputs assigned to other processors
cannot be added to this processor.

@param[out] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : The Audio Processor instance was created successfully.
@retval ISMD_ERROR_NO_RESOURCES : No handles are available or unable to allocate required memory.
*/
ismd_result_t SMDCONNECTION ismd_audio_open_processor(ismd_audio_processor_t *processor_h);


/**
Frees up the instance of the processor specifed by the handle. Cleans up any resources associated
with the instance. A call to close processor will also remove all of the inputs and outputs of the
processor.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : The Audio Processor instance was closed and resources freed.
@retval ISMD_ERROR_OPERATION_FAILED : Failed to close processor, abnormal termination.
@retval ISMD_ERROR_INVALID_HANDLE : The handle supplied was not a valid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_close_processor(ismd_audio_processor_t processor_h);


/**
Determin if a specific format (codec) is available/installed in the audio driver.

@param[in] format : The format to determine the availability of.

@retval ISMD_SUCCESS : The codec exists and is available to use in the driver.
@retval ISMD_ERROR_NOT_FOUND : The codec is not available for use.
*/
ismd_result_t SMDEXPORT ismd_audio_codec_available(ismd_audio_format_t format);

/**
Enable and set render_period of the Audio Processor.  
render_period is the audio basic process unit in time. Currently, the Audio Processor 
is processing audio data under default render_period with 10 millisecond each. 
Reduce render_period could help user achieving smaller overall latency of pipeline.
Call this function to adjust render_period as desired. 

Note:
this API could only be called before any input or output ports are added.
this API does not guarantee the user input of render_period is valid or not untill the first audio input port is added. 

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] render_period_ms : The render_period. only 10 or 5 ms accepted.

@retval ISMD_SUCCESS : Setting was successfully applied to the Audio Processor.
@retval ISMD_ERROR_OPERATION_FAILED : Error in enabling the new render_period.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid input paramters. 
*/
ismd_result_t SMDEXPORT ismd_audio_set_render_period(ismd_audio_processor_t processor_h, int render_period_ms);

/**
Retrieve the current Audio Processor's render_period setting.  
Can be called at any time with valid processor handle.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] render_period_ms : Current render_period.

@retval ISMD_SUCCESS : Setting was successfully applied to the Audio Processor.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_NULL_POINTER : null pointer.
*/
ismd_result_t SMDEXPORT ismd_audio_get_render_period(ismd_audio_processor_t processor_h, int* render_period_ms);


/**
Enable and set master volume of the mixed output of the Audio Processor.  Call
this function multiple times to adjust volume as desired.  When the audio
processor is muted, calls to this API succeed, but they do not take effect
until the processor is un-muted.   The master volume is added to each per-
channel volume setting to create the overall volume of each channel of the
mixed output.

Note that this API does not guarantee audio quality when gain values greater
than 0dB are specified for the master_volume.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] master_volume : Master volume level.

@retval ISMD_SUCCESS : Setting was successfully applied to the Audio Processor.
@retval ISMD_ERROR_OPERATION_FAILED : Error in enabling the master volume control.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_set_master_volume(ismd_audio_processor_t processor_h,
                              ismd_audio_gain_value_t master_volume);


/**
Retrieve the current master volume setting.  Note that this function will return
the last value specified in a call to ismd_audio_set_master_volume, even when
the audio processor is muted.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] master_volume : Current master volume level.

@retval ISMD_SUCCESS : Setting was successfully applied to the Audio Processor.
@retval ISMD_ERROR_OPERATION_FAILED : Error in returning the master volume setting.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_NULL_POINTER : null pointer.
*/
ismd_result_t SMDEXPORT ismd_audio_get_master_volume(ismd_audio_processor_t processor_h,
                              ismd_audio_gain_value_t *master_volume);

/**
Disable master volume control on the Audio Processor.  Calling this function sets the
master valume value to 0dB (no change).  When the audio processor is muted, calls to
this function succeed, but they do not take effect until the processor is un-muted.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : Master volume control was disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling the master volume control.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_disable_master_volume(ismd_audio_processor_t processor_h);


/**
Mute or unmute the mixed output of the Audio Processor.  A call to this function with
mute set to true effectively mutes all outputs of the audio processor simultaneously.
The previous mixer output gain settings are saved and will be restored when this function
is called with mute set to false.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] mute : Set true to mute, set false to un-mute.

@retval ISMD_SUCCESS : All outputs muted successfully.
@retval ISMD_ERROR_OPERATION_FAILED : Unexpected error occured while attempting to mute.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_mute(ismd_audio_processor_t processor_h, bool mute);

/**
Retrieve the mute status of the audio processor.   Note that this function will return
the last values specified in a call to ismd_audio_set_per_channel_volume, even when the
audio processor is muted.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] muted : mute status, true if processor is currently muted, otherwise false.

@retval ISMD_SUCCESS : mute status successfully retrieved.
@retval ISMD_ERROR_OPERATION_FAILED : Unexpected error occured while attempting to get status.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_NULL_POINTER : null pointer
*/
ismd_result_t SMDEXPORT ismd_audio_is_muted( ismd_audio_processor_t processor_h, bool *muted );

/**
Enable and set the volume of each channel of the mixed output of the audio processor.
Call this function multiple times to adjust per channel volume as desired.  The per-
channel volumes are added to the master volume setting to create the overall volume
fore each channel of the mixed output.  When the audio processor is muted, calls to
this API succeed, but they do not take effect until the processor is un-muted.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] channel_volume : Structure indicating the volume for each channel of the mixed output.

@retval ISMD_SUCCESS : Volume successfully set on audio channels in the stream.
@retval ISMD_ERROR_OPERATION_FAILED : Unexpected error occured while attempted to set the volume.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t  SMDEXPORT ismd_audio_set_per_channel_volume(ismd_audio_processor_t processor_h,
                            ismd_audio_per_channel_volume_t channel_volume);


/**
Retrieve the per-channel volume control settings of the mixed output of the audio processor.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[out] channel_volume : Struture to hold the volume for the each channel of the mixed output.

@retval ISMD_SUCCESS : Volume setting successfully returned for all audio channels in the stream.
@retval ISMD_ERROR_OPERATION_FAILED : Unexpected error occured while attempted to set the volume.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_NULL_POINTER : Null pointer.
*/
ismd_result_t  SMDEXPORT ismd_audio_get_per_channel_volume(ismd_audio_processor_t processor_h,
                            ismd_audio_per_channel_volume_t *channel_volume);


/**
Disable per-channel volume control of the mixed output of the Audio Processor.  This function
sets the per-channel volume back to 0dB (no change) for all channels.  When the audio processor
is muted, calls to this API succeed, but they do not take effect until the processor is un-muted
again.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : Per channel volume control was disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling volume control.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_disable_per_channel_volume(ismd_audio_processor_t processor_h);


/**
Enable bass managment, set the crossover frequency and speaker setting of each speaker. Can be
called multiple times to change setting as needed. Bass management is disabled by default if no call
is made to this function.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] crossover_freq : Variable to specify desired crossover frequency.
@param[in] speaker_settings : Specify which speakers are large or small. (LFE not considered)

@retval ISMD_SUCCESS : Bass management was enabled and setting was applied.
@retval ISMD_ERROR_OPERATION_FAILED : Error in applying settings.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_set_bass_management(ismd_audio_processor_t processor_h,
                              ismd_audio_crossover_freq_t crossover_freq,
                              ismd_audio_per_speaker_setting_t speaker_settings);


/**
Retrieve bass management settings.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[out] crossover_freq : Variable to specify desired crossover frequency.
@param[out] speaker_settings : Specify which speakers are large or small. (LFE not considered)

@retval ISMD_SUCCESS : Settings successfully returned.
@retval ISMD_ERROR_OPERATION_FAILED : Bass management is not enabled.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_get_bass_management(ismd_audio_processor_t processor_h,
                              ismd_audio_crossover_freq_t *crossover_freq,
                              ismd_audio_per_speaker_setting_t *speaker_settings);


/**
Disable bass managment in the Audio Processor.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : Bass management was successfully disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling bass management.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_disable_bass_management(ismd_audio_processor_t processor_h);


/**
Set and enable a per-channel delay in the range of 0 - 255 ms (1 ms step).
Call this function multiple times to change delay.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] channel_delay : Structure of delay values for each channel, range: 0 - 255 ms (1 ms step).

@retval ISMD_SUCCESS : The per channel delay was applied successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : One or more of the delay values specified was out of range.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_set_per_channel_delay(ismd_audio_processor_t processor_h,
                              ismd_audio_per_channel_delay_t channel_delay);


/**
Get current per-channel delay values.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] channel_delay : Structure of delay values for each channel, range: 0 - 255 ms (1 ms step).

@retval ISMD_SUCCESS : The per channel delay values were returned successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_get_per_channel_delay(ismd_audio_processor_t processor_h,
                              ismd_audio_per_channel_delay_t *channel_delay);


/**
Disables the per channel delay on the Audio Processor. No delay is applied after a call to this function.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : Per channel delay was successfully disabled.
@retval ISMD_ERROR_OPERATION_FAILED : An unexpected error occured while attempting to disable per-channel dealy.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_disable_per_channel_delay(ismd_audio_processor_t processor_h);


/**
Enables watermark detection on the Audio Processor.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] audio_wm_params : Watermark create time input parameter value.

@retval ISMD_SUCCESS : Watermark detection was successfully enabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in enabling watermark detection.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_enable_watermark_detection(ismd_audio_processor_t processor_h, audio_wm_params_block_t audio_wm_params);


/**
Disable watermark detection on the Audio Processor.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] audio_wm_params : Watermark parameter return value.

@retval ISMD_SUCCESS : Watermark detection was successfully disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling watermark detection.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_disable_watermark_detection(ismd_audio_processor_t processor_h, audio_wm_params_block_t *audio_wm_params);

/**
Configures watermark parameters on the Audio Processor.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] audio_wm_params : Watermark run time input parameter value.

@retval ISMD_SUCCESS : Watermark detection was successfully enabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in enabling watermark detection.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_set_watermark_detection(ismd_audio_processor_t processor_h, audio_wm_params_block_t audio_wm_params);


/**
Get last detected watermark from the Audio Processor.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[out] audio_wm_params : Watermark output parameter return value.
@param[out] awm_output_params_buffer : Watermark output with compliance test parameter return value.

@retval ISMD_SUCCESS : Watermark output parameters returned successfully.
@retval ISMD_ERROR_INVALID_REQUEST : Watermark  stage was not enabled.
@retval ISMD_ERROR_NULL_POINTER : NULL pointer to output parameters.
*/
ismd_result_t SMDEXPORT ismd_audio_get_watermarks(ismd_audio_processor_t processor_h,
                              audio_wm_params_block_t *audio_wm_params,
                              ismd_buffer_handle_t awm_output_params_buffer);


/**
Enables matrix decoding on the audio processor. Ensure that an output supporting more
than 2 channels is present to take advantage of the matrix decode. If there are
no outputs attached that are multichannel the matrix decoder will not be enabled. To
set matrix decoder specific parameters call \ref ismd_audio_set_matrix_decoder_param
prior to calling this function.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] matrix_decoder : The type of matrix decoder to enable.

@retval ISMD_SUCCESS : Matrix decoder was successfully enabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in enabling matrix decoder.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_INVALID_REQUEST: The outputs are not configured to handle more than 2 channels.
*/
ismd_result_t SMDEXPORT
ismd_audio_enable_matrix_decoder(ismd_audio_processor_t processor_h, ismd_audio_matrix_decoder_t matrix_decoder);


/**
Disable matrix decoding on the audio processor.

@param[in] processor_h : Handle to the Audio Processor instance.

@retval ISMD_SUCCESS : Matrix decoder was successfully disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling matrix decoder.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT
ismd_audio_disable_matrix_decoder(ismd_audio_processor_t processor_h);


/**
Set a matrix decoder specific parameter. Call this function to set all parameters
before enabling the matrix decoder (before calling \ref ismd_audio_enable_matrix_decoder).
If a change to paramters is needed at runtime, call \ref ismd_audio_disable_matrix_decoder
set all the paramters that need to be changed then re-enable the decoder by calling
\ref ismd_audio_enable_matrix_decoder.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] param_type : ID of the decoder parameter to set.
@param[in] param_value : Structure containing the decoder parameter value to set.

@retval ISMD_SUCCESS : Decoder parameter was set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The decoder parameter specified is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT
ismd_audio_set_matrix_decoder_param(ismd_audio_processor_t processor_h,
                              int param_type,
                              ismd_audio_decoder_param_t *param_value );

/**
This function sets up the master audio clock frequency.
Master audio clock frequencies currently supported on CE31xx  ( internal source 33868800, 36864000)
  and CE41xx (external source 22579200, 24576000, 33868800, 36864000)

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] frequency : Master clock frequency in Hz.
@param[in] clk_src : Source of master audio clock.

@retval ISMD_SUCCESS : Audio Master clock frequency was set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The clock source not supported or frequency is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : Clock control write operation failed.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT
ismd_audio_configure_master_clock(ismd_audio_processor_t processor_h,
                                          unsigned int frequency,
                                          ismd_audio_clk_src_t clk_src);


/*********************************************************************************************/

/*! @name Audio Input APIs*/

/**
Add an input to an instance of the Audio Processor. The call to open an instance of the
processor must be called before adding any inputs. The input must be configured in either
a timed or non-timed mode. If configured as a timed input, the audio data will be rendered
out based on the presentation time stamp (PTS) value associated with each audio frame.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] timed_stream : Indicates whether or not the input will be rendered based on PTS values.
@param[out] input_h : Handle to the audio input instance.
@param[out] port_h : Handle to the smd input port, used as the entrance point for the data.

@retval ISMD_SUCCESS : Input was successfully added to the Audio Processor instance.
@retval ISMD_ERROR_OPERATION_FAILED : Could not create input or no inputs available.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDCONNECTION ismd_audio_add_input_port(ismd_audio_processor_t processor_h,
                              bool timed_stream,
                              ismd_dev_t *input_h,
                              ismd_port_handle_t *port_h);


/**
Add a physical (hardware) input to an instance of the audio processor. The call to open
an instance of the processor must be called before adding any inputs. The input must be configured in either
a timed or non-timed mode. If configured as a timed input, the audio data will be rendered
out based on the presentation time stamp (PTS) value associated with each audio frame.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] hw_id : A unique ID that idetifies the hardware input device to use.
@param[in] timed_stream : Indicates whether or not the input will be rendered based on PTS values.
@param[out] input_h : Handle to the audio input instance.

@retval ISMD_SUCCESS : Input was successfully added to the audio processor instance.
@retval ISMD_ERROR_OPERATION_FAILED : Could not create input or hardware input is not available.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDCONNECTION ismd_audio_add_phys_input(ismd_audio_processor_t processor_h,
                              int hw_id,
                              bool timed_stream,
                              ismd_dev_t *input_h);


/**
Remove an input from the currently playing stream. This configuration command can be issued
at any time. It will remove a previously added input stream and not disrupt the playback of any
other input streams.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle of input to remove.

@retval ISMD_SUCCESS : Input was successfully removed for this audio processor instance.
@retval ISMD_ERROR_OPERATION_FAILED : Error in removing input.
@retval ISMD_ERROR_INVALID_PARAMETER: Input does not exist.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_remove_input(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h);


/**
Disable the input. Default state of an input is that it is enabled.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.

@retval ISMD_SUCCESS : Input was disabled.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_disable(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h);


/**
Enabled the input after a call to \ref ismd_audio_input_disable has been called.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.

@retval ISMD_SUCCESS : Input was enabled.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_enable(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h);


/**
Get the input port handle after an input port has been added.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] port_h : Handle to the audio input port.

@retval ISMD_SUCCESS : Input port handle was returned.
@retval ISMD_ERROR_INVALID_REQUEST : The input is not a software input.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_port(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_port_handle_t *port_h);


/**
Get decoder parameters on an input configured to decode the input audio data.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] param_type : ID of the decoder parameter to be returned.
@param[out] param_value : Structure containing the decoder parameter value.

@retval ISMD_SUCCESS : Decoder parameter was returned successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The decoder parameter specified is invalid.
@retval ISMD_ERROR_INVALID_RESOURCE : The input was not setup with a decoder.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_decoder_param(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int param_type,
                              ismd_audio_decoder_param_t *param_value );


/**
Get the audio compression algorithm format used on the current stream playing in the audio pipeline.
If no audio stream is present at the input, the function will return an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] fmt : Algorithm of audio stream about to enter the pipeline.

@retval ISMD_SUCCESS : The algorithm was successfully returned.
@retval ISMD_ERROR_OPERATION_FAILED : There was no input audio data present.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_data_format(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_format_t *fmt);


/**
Get the sample rate of the incoming audio stream. If no audio stream is present at the input,
the function will return an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] sample_rate : The input stream's sample rate in Hz.

@retval ISMD_SUCCESS : The sampling rate was successfully returned.
@retval ISMD_ERROR_OPERATION_FAILED : There was no input audio data present.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_sample_rate(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int *sample_rate);


/**
Gets the sample size of the incoming audio stream. If no audio stream is present at the input,
the function will return an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] sample_size : Audio sample size.

@retval ISMD_SUCCESS : The input sample size was successfully retrieved.
@retval ISMD_ERROR_OPERATION_FAILED: There was no input audio data present.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_sample_size(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int *sample_size);


/**
Get the bit rate of the current audio stream in the pipeline. If no audio stream is present
at the input, the function will return an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] bitrate : Bit rate of the current audio stream in Kbps.

@retval ISMD_SUCCESS : The bit rate was successfully obtained.
@retval ISMD_ERROR_OPERATION_FAILED : There was no input audio data present.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_bitrate(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int *bitrate);

/**
Get the message context id of the current audio stream.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] message_context: used to return message context id.

@retval ISMD_SUCCESS : The message context was successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_message_context(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              uint32_t *message_context);


/**
Get bit rate, sample rate, sample size, channel config and audio format (algorithm) for the
incoming stream returned all in one structure. If there is no input audio stream present at
the input, the function will return an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] stream_info : Bit rate of the current audio stream in Kbps.

@retval ISMD_SUCCESS : The bit rate was successfully obtained.
@retval ISMD_ERROR_OPERATION_FAILED : There was no audio data in the stream.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_stream_info(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_stream_info_t *stream_info);


/**
Get extened format specific stream information of the current input stream. Some codecs return a large amount of
stream information and metadata, so it is required to send a handle to an ISMD buffer to place all of this information.
An example of how to do this:
 1. Call ismd_buffer_alloc() - This returns the handle to the buffer.
 2. Call ismd_buffer_read_desc - This gets the physical address of the buffer.
 3. Call this function ismd_audio_input_get_format_specific_stream_info giving it the buffer handle.
 4. Map the physical address to get the virtual address to access the stream information. Use the struct ismd_audio_format_specific_stream_info_t
    to access the data.
  - Ex. stream_info = (ismd_audio_format_specific_stream_info_t *) OS_MAP_IO_TO_MEM_NOCACHE(desc.phys.base, desc.phys.size);
 5. Use the stream_info pointer to get all the stream information you need.
 6. Un map the memory.
 7. Dereference the buffer.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] stream_info_buffer : The handle of the buffer that will contain the stream information after the call.

@retval ISMD_SUCCESS : The stream information was successfully obtained.
@retval ISMD_ERROR_OPERATION_FAILED : There was no audio data in the stream.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_format_specific_stream_info(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_buffer_handle_t stream_info_buffer);

/**
Get the channel configuration present in the current incoming audio stream. If there is no audio
streaming at the time of the call a failure result will be retruned. Channel configuration returned in
ch_config as bitmask.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[out] ch_config : Channel configuration returned as bitmask. (See channel config bitmask defines)

@retval ISMD_SUCCESS : The input stream's number of channels was successfully obtained.
@retval ISMD_ERROR_OPERATION_FAILED : There was no audio data in the stream.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_get_channel_config(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int *ch_config);



/**
Set decoder parameters on an input configured to decode the input audio data. Call this function after
calling \ref ismd_audio_input_set_data_format and before setting the input to a playing state.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] param_type : ID of the decoder parameter to set.
@param[in] param_value : Structure containing the decoder parameter value to set.

@retval ISMD_SUCCESS : Decoder parameter was set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The decoder parameter specified is invalid.
@retval ISMD_ERROR_INVALID_RESOURCE : The input was not setup with a decoder.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_decoder_param(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int param_type,
                              ismd_audio_decoder_param_t *param_value );



/**
Sets the input to be the primary input and will be the source for outputs set in pass through mode.
If the output does not support the incoming stream, the Audio Processor will decode the stream and send
out LPCM data instead. Call this function again on the same input to update the list of supported
algorithms or to enable/disable pass through mode.

If application software calls this function  to either select an input source or change the input source
for pass-through mode while the Audio Processor is in the middle of streaming audio data,
the Audio Processor starts processing the input audio data from the newly selected input source
for pass-through mode.  The pass-through audio data from the newly selected input source
traverses through the audio pipeline and eventually reaches pass-through output point

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] pass_through_config : Pass through configuration structure.

@retval ISMD_SUCCESS : The algorithm was successfully returned.
@retval ISMD_ERROR_INVALID_REQUEST : No outputs are configured to recieve the pass through data,
            or another input is already assigned as the primary.
@retval ISMD_ERROR_OPERATION_FAILED : Could not perform operation, unexpected failure.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_as_primary(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_input_pass_through_config_t pass_through_config );


/**
Set the mode of the input that is going to be sent as passthrough to an output that is in 
pass through mode. (See \ref ismd_audio_pass_through_mode_t). By default the  input
pass through mode is ISMD_AUDIO_PASS_THROUGH_MODE_IEC61937. This
means by default when an input is set as primary and the format is NOT PCM (not encoded data)
and passed through to an output, it will be formatted to the IEC61937 specification. To disable this 
mode and to have the input directly passed to the output with no formatting, specify 
ISMD_AUDIO_PASS_THROUGH_MODE_DIRECT. An exmple use case of the direct mode is the
case where the audio processor is asked to process a DTS stream that originated from a 
CD and an output is configured to output the pass through stream. Since no IEC formatting is
needed in this case, the application will call this function and have the stream sent out the output as is (direct).
If the stream being passed through directly is expected to be rendered at the same rate as the 
original stream, please set the sample rate of the output to match the input stream's sample rate. 

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] mode : The mode that specifies how the passthrough stream should be output.

@retval ISMD_SUCCESS : The pass through mode was set successfully. 
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid mode specified. 
@retval ISMD_ERROR_OPERATION_FAILED : Could not perform operation, unexpected failure.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_pass_through_mode(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_pass_through_mode_t mode );



/**
Sets the input to be the secondary input on the processor.  This input can supply metadata
that describes how the primary and secondary streams should be mixed together.  Currently,
this metadata must be encoded into either a DTS-LBR or Dolby Digital Plus stream and there
will be no affect when other stream types are set as secondary inputs.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.

@retval ISMD_SUCCESS : The algorithm was successfully returned.
@retval ISMD_ERROR_INVALID_REQUEST : A secondary input already exists on this processor
or the specified input is a physical input or the primary input on this processor.
@retval ISMD_ERROR_OPERATION_FAILED : Could not perform operation, unexpected failure.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT
ismd_audio_input_set_as_secondary( ismd_audio_processor_t processor_h,
                                   ismd_dev_t input_h );



/**
Set which algorithm is used for decode of new data into the audio pipeline. In the event
that data has been already put into the audio pipeline of another algorithm, all old data will be played
out.  To avoid this behavior, please call the flush function prior to changing decode algorithm. It is
required to call this function at least once before using the audio subsystem.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] format : Algorithm of audio stream about to enter the pipeline.

@retval ISMD_SUCCESS : The algorithm was successfully set.
@retval ISMD_ERROR_INVALID_PARAMETER: Algorithm supplied is not supported or is invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_data_format(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_format_t format);


/**
If using and MPEG decoder algorithm, call this function to notify the driver MPEG type
and layer to use. Call this function after \ref ismd_audio_input_set_data_format.
*** THIS FUNCTION NO LONGER NECESSARY AND HAS BEEN IS DEPRECATED ***

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] mpeg_layer : Type of MPEG algorithm to decode.

@retval ISMD_SUCCESS : The MPEG layer was successfully set.
@retval ISMD_ERROR_INVALID_PARAMETER: mpeg_layer parameter is invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_data_format_mpeg_layer(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_mpeg_layer_t mpeg_layer);

/**
Set the sample size of a physical hardware input interface (ie capture). This call does not
apply audio data at software input port because the sample size of the input audio data
at a software input port is an inherent attribute of the input audio data (or the sample
size is specified in the descriptor of the input audio data).

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] sample_size : Desired input sample size.

@retval ISMD_SUCCESS : Sample size was successfully applied to input.
@retval ISMD_ERROR_INVALID_PARAMETER: Sample size is not supported or is invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_sample_size(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int sample_size);


/**
Use this function to set WMA stream parameters, usually obtained from an ASF parser.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] wma_stream_format : WMA encoded audio stream parameters.

@retval ISMD_SUCCESS : Settings applied successfully.
@retval ISMD_ERROR_INVALID_PARAMETER: One or more of the parameters is not supported or is invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_wma_format(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_wma_format_t wma_stream_format);


/**
Set the format of input PCM audio data. This serves as an out-of-band call
to notify the audio driver of the input PCM audio data format. Call this function when in-band
notification of the PCM audio data format is not being used and before the audio stream
is set to the playing state.
In case of physical input (audio capture) this API needs to be called even for compressed audio formats.  
The parameters should specific the transport format of the audio data.  For example for a 48kHz AC3 
compressed stream the parameters should be:
   sample_rate = 48000
   sample_size = 16
   channel_config = AUDIO_CHAN_CONFIG_2_CH

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] sample_size : Desired input sample size.
@param[in] sample_rate : Input sample rate in Hz. (ie. 48000 for 48kHz sampling rate)
@param[in] channel_config : Channel configuration. (Please see \ref ismd_audio_defs.h for channel location bitmask defines. )

@retval ISMD_SUCCESS : PCM audio data format was successfully applied to input.
@retval ISMD_ERROR_INVALID_PARAMETER: One or more of the parameters is not supported or is invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_pcm_format(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int sample_size,
                              int sample_rate,
                              int channel_config);


/**
Set destination channel location and signal strength of input channels when
mixing.  The specified channel mix may be augmented by metadata in the
secondary audio stream when mix metadata is enabled.  The channel mix is
also augmented by the gain specified in ismd_audio_set_master_volume.

Note that this API does not guarantee audio quality when gain values greater
than 0dB are specified in the ch_mix_config.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] ch_mix_config: Stucture to specify mix configuration.

@retval ISMD_SUCCESS : Input mix configuration was successfully applied.
@retval ISMD_ERROR_OPERATION_FAILED : Could not set mix configuration.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_channel_mix(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              ismd_audio_channel_mix_config_t ch_mix_config);


/**
Enable or disable processing of mixing metadata in this processor.  Mixing
metadata is extracted from the secondary stream and is generally embedded
in the compressed bitstream (e.g., DTS-LBR or Dolby Digital Plus).  If
mixing metadata is enabled, then the mixer will add the gains specified in
the secondary stream metadata to the user-specified gains specified in
ismd_audio_input_set_channel_mix.  If mixing metadata is disabled or there
is no mixing metadata in the secondary stream, the mixer will use only the
user-specified gain values.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] enable: Enable (true) or disable (false) mix metadata.

@retval ISMD_SUCCESS : Mix metadata was successfully enabled or disabled.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT
ismd_audio_enable_mix_metadata( ismd_audio_processor_t processor_h,
                                bool                   enable );


/**
Set the channel configuration mode of the mixer. This optional call allows control
over how the mixer will operate on its inputs and the channel configuration it
will output.

If ch_cfg_mode is set to ISMD_AUDIO_MIX_CH_CFG_MODE_AUTO
the processor will determine the least computationally intensive (MIPS saving)
setup based on the input and output configurations. This is the default mode
of the mixer.

If ch_cfg_mode is set to ISMD_AUDIO_MIX_CH_CFG_MODE_PRIMARY
this means the mixer will always look at the primary input to the mixer and output
it's channel configuration (mode used in Blu-Ray), if no primary input has been
selected the mixer will fall back to ISMD_AUDIO_MIX_CH_CFG_MODE_AUTO until
a primary input is specified.

If ch_cfg_mode is set to ISMD_AUDIO_MIX_CH_CFG_MODE_FIXED the ch_cfg
variable will be used as the configuration the mixer operates on and outputs. An
example of a valid fixed ch_cfg (channel config) would be 0xFFFFFF20 (Left and Right only).
Please see \ref ismd_audio_channel_t for channel defines.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] ch_cfg_mode: The channel configuration mode the mixer will operate on.
@param[in] ch_cfg: The channel configuration used when ch_cfg_mode is
set to ISMD_AUDIO_MIX_CH_CFG_MODE_FIXED.

@retval ISMD_SUCCESS : The mixer channel configuration was set.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_INVALID_PARAMETER : ch_cfg_mode or ch_cfg is invalid.
*/
ismd_result_t SMDEXPORT
ismd_audio_set_mixing_channel_config( ismd_audio_processor_t processor_h,
                                ismd_audio_mix_ch_cfg_mode_t ch_cfg_mode,
                                int ch_cfg);


/**
Use this optional function call to set the sampling rate mode of the mixer.

If sample_rate_mode is set to ISMD_AUDIO_MIX_SAMPLE_RATE_MODE_AUTO
the processor will determine the least computationally intensive setup
based on the input and output configurations. This is the default mode
of the mixer.

If sample_rate_mode is set to ISMD_AUDIO_MIX_SAMPLE_RATE_MODE_PRIMARY
this means the processor will always look at the primary input to the mixer and use
it's sampling frequency and convert all other inputs to that frequency before mixing. If no primary
input has been selected the mixer will fall back to ISMD_AUDIO_MIX_SAMPLE_RATE_MODE_AUTO.
until a primary input is specified.

If sample_rate_mode is set to ISMD_AUDIO_MIX_SAMPLE_RATE_MODE_FIXED the sample_rate
variable will be used to specify the sampling rate that all inputs to the mixer will be converted
to before mixing.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] ch_cfg_mode: The mixing sample rate mode the mixer will operate in.
@param[in] ch_cfg: The sample rate used when sample_rate is
set to ISMD_AUDIO_MIX_SAMPLE_RATE_MODE_FIXED.

@retval ISMD_SUCCESS : The mixing sampling rate was set.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
@retval ISMD_ERROR_INVALID_PARAMETER : sample_rate_mode or sample_rate is invalid.
*/
ismd_result_t SMDEXPORT
ismd_audio_set_mixing_sample_rate( ismd_audio_processor_t processor_h,
                                ismd_audio_mix_sample_rate_mode_t sample_rate_mode,
                                int sample_rate);



/**
Set the timing accuracy to be used on the input stream. This function allows for a specified
range to be set when looking presentation time stamps and deciding when the audio samples
should be rendered. For example: if allowed_ms_drift_ahead were to be set to 10, the audio timing
would allow audio samples that arrive with a PTS that is up to 10 ms ahead of the time it was supposed
to be played to be rendered out at that time 10ms early. The same would apply to the behind case. If
allowed_ms_drift_behind was set to 10ms then audio samples arriving with a PTS value that is up to 10ms
behind the time it was supposed to be played will be rendered out 10ms late. If both values
are set to 0, this means that the audio samples contained in the stream must represent exactly the
time specified by each PTS value attached to the audio samples (exact accuracy required). This fuction
call is not required to be called for normal timed or untimed audio operation.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] allowed_ms_drift_ahead: The amount if time in milliseconds to allow
   samples to be played if the PTS values are positive or ahead (early).
@param[in] allowed_ms_drift_behind: The amount if time in milliseconds to allow
   samples to be played if the PTS values are negative or behind (late).

@retval ISMD_SUCCESS : Input timing accruacy set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : Range values specified are invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_timing_accuracy(ismd_audio_processor_t processor_h,
                              ismd_dev_t input_h,
                              int allowed_ms_drift_ahead,
                              int allowed_ms_drift_behind);


/**
Advance through the stream until a specific PTS value is reached.  This function
may be called for a specific audio input only when that input is in the paused
state and the stream play rate is not reverse rate.

@param[in] processor_h : Handle to an audio processor instance.
@param[in] input_h : Handle to an input on the audio processor.
@param[in] linear_pts : the linear PTS value of the next audio sample to be rendered.
All audio samples prior to this PTS value are discarded.

@retval ISMD_SUCCESS : The input advanced successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The specified processor or input handle
is stale or invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The specified audio input is not
currently in the paused state or play rate is under reverse rate.
@retval ISMD_ERROR_INVALID_PARAMETER : The value specified for local PTS is
equal to ISMD_NO_PTS.
*/
ismd_result_t SMDEXPORT
ismd_audio_input_advance_to_pts( ismd_audio_processor_t    processor_h,
                                 ismd_dev_t                input_h,
                                 ismd_pts_t                linear_pts );


/**
Audio received the client id

   @param[in] processor_h : Handle to an audio processor instance.
   @param[in] input_h : Handle to an input on the audio processor.
   @param[out] client_id

   @retval ISMD_SUCCESS: Client id returned successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified processor or input handle is invalid.
   @retval ISMD_ERROR_NULL_POINTER : The client_id pointer is NULL.
*/
ismd_result_t SMDEXPORT
ismd_audio_input_get_client_id(ismd_audio_processor_t    processor_h,
                                 ismd_dev_t                    input_h,
                                 int                           *client_id );


/**
Get the stream position of the audio renderer.

@param[in] processor_h : Handle to an audio processor instance.
@param[in] input_h : Handle to an input on the audio processor.
@param[out] position : stream position details.

@retval ISMD_SUCCESS : The stream position is successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : info pointer is NULL.
*/
ismd_result_t SMDEXPORT
ismd_audio_input_get_stream_position( ismd_audio_processor_t    processor_h,
                              ismd_dev_t         input_h,
                              ismd_audio_stream_position_info_t *position);




/*********************************************************************************************/

/*! @name Audio Ouput APIs*/

/**
Add an output to an instance of the audio processor. It can be called when the processor is in any state.
In the event that the processor is currently in the playing state the audio processor tries to get stream
output enabled as fast as possible without impacting the other currently playing streams. An instance
of the audio processor must have been created before calling this function. After the output has been
added, the output is available for configuration but is in a disabled state. \ref ismd_audio_output_enable must
be called to recieve audio at the output. 

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] config : Structure containing the desired configuration for the audio output.
@param[out] output_h : Handle to the audio output instance.
@param[out] port_h : Handle to the smd output port, used as the exit point for the data.

@retval ISMD_SUCCESS : Output was successfully added to the audio processor instance.
@retval ISMD_ERROR_OPERATION_FAILED : Error while adding output.
@retval ISMD_NO_RESOURCES : No resources available to addoutput.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_add_output_port(ismd_audio_processor_t processor_h,
                              ismd_audio_output_config_t config,
                              ismd_audio_output_t *output_h,
                              ismd_port_handle_t *port_h);


/**
Add a physical (hardware rendering) output to an instance of the audio processor. It can be called when the processor
is in any state.  In the event that the processor is currently in the playing state the audio processor tries
to get stream output enabled as fast as possible without impacting the other currently playing streams. An
instance of the audio processor must have been created before calling this function. After the output has been
added, the output is available for configuration but is in a disabled state. \ref ismd_audio_output_enable must
be called to recieve audio at the output. 

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] hw_id : A unique ID that idetifies the hardware output device to use.
@param[in] config : Structure containing the desired configuration for the audio output.
@param[out] output_h : Handle to the audio output instance.

@retval ISMD_SUCCESS : Output was successfully added to the audio processor instance.
@retval ISMD_ERROR_INVALID_PARAMETER: Output Hardware ID was invalid.
@retval ISMD_ERROR_OPERATION_FAILED : Error in creating output or no outputs available.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_add_phys_output(ismd_audio_processor_t processor_h,
                              int hw_id,
                              ismd_audio_output_config_t config,
                              ismd_audio_output_t *output_h);


/**
Remove an output from the currently playing stream. This configuration command can be issued
at any time.  Any other outputs from the same stream will continue to play un-interrupted.
The audio pipeline instance must also clean up any resources currently being used to output
data to the output which is being removed.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle of output to remove.

@retval ISMD_SUCCESS : Output was successfully removed for this audio processor instance.
@retval ISMD_ERROR_OPERATION_FAILED : Error in removing the output.
@retval ISMD_ERROR_INVALID_PARAMETER: Output does not exist.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_remove_output(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h);


/**
Disables the output to perform a reconfiguration of the output. The output
must have been previously added to the Audio Processor instance.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.

@retval ISMD_SUCCESS : Output instance was disabled.
@retval ISMD_ERROR_OPERATION_FAILED : Error in disabling the output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_disable(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h);


/**
Enables the output on a processor. If the output is a hardware output, this
starts the rendering process. After the output is enabled no configuration
changes can be made untill the output is disabled again. This call is required
to recieve audio to the added output. By default the output added is disabled. 

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.

@retval ISMD_SUCCESS : Output instance was enabled.
@retval ISMD_ERROR_OPERATION_FAILED: Error in enabling the output.
@retval ISMD_NULL_POINTER : Handle passed in was NULL.
*/
ismd_result_t SMDEXPORT ismd_audio_output_enable(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h);



/**
This is an OPTIONAL call to specify a specific downmix to be transmitted via an
output. The caller must ensure that the output interface that is configured to
receive a specific downmix can support the number of channels in the downmix
by calling \ref ismd_audio_output_set_channel_config to setup the output with
the appropriate number of channels. This call is especially useful if the caller wants
to recieve LtRt or LoRo  on a stereo output. This call can also be used in conjunction
with setting up the DTS decoder to output a speaker output via the decoder
parameters then set the output downmix mode to match the speaker output to receive
that downmix configuration from that output. Currently the audio driver only supports one
downmix mode on one or more outputs configured identically. So if a call was made
to setup an output to receive LtRt, that same mode can be set on multiple
outputs if they are all configured the same (ie. same channel count etc.). The
audio driver will make every attempt possible to output the correct downmix mode, however
if the caller sets up the decoder params incorrectly and/or the output cannot support
the downmix mode set, the default downmixer will be used for that output.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] dmix_mode : Type of downmix configuration to be recieved at the output.

@retval ISMD_SUCCESS : Output instance was downmix mode was set.
@retval ISMD_ERROR_INVALID_PARAMETER: The dmix_mode passed in was invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Handle passed in was not found.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_downmix_mode(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_downmix_mode_t dmix_mode);



/**
Get the output port handle after the output has been added.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] port_h: Output port handle.

@retval ISMD_SUCCESS : The output port handle was returned.
@retval ISMD_ERROR_INVALID_REQUEST : The output is not a software output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_port(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_port_handle_t *port_h);


/**
Get the channel configuration of the stream on the specified output.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] ch_config: Output channel configuration.

@retval ISMD_SUCCESS : The output stream's number of channels was successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_channel_config(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_channel_config_t *ch_config);


/**
Get the current audio sample size setting on the pipeline.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] sample_size : Audio sample size.

@retval ISMD_SUCCESS : The audio sample size was returned.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_sample_size(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int *sample_size);


/**
Get the output sample rate of the audio output instance, as set by the user.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] sample_rate : Output audio stream's sample rate.

@retval ISMD_SUCCESS : The sampling rate was successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_sample_rate(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int *sample_rate);


/**
Get the output mode of the specified output instance.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] out_mode : Audio stream output format.

@retval ISMD_SUCCESS : The output mode was successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_mode(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_output_mode_t  *out_mode);

/**
Get the currently set output delay specified in milliseconds.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] delay_ms : Output delay time in miliseconds. Range 0-255 ms (1 ms step).

@retval ISMD_SUCCESS : Delay specified was successfully returned.
@retval ISMD_ERROR_OPERATION_FAILED : Could not perform operation.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_delay(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int *delay_ms);


/**
Get this configuration parameter used to set the bit clock divider inside of the render H/W.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] div_val : Value to be applied to divider.

@retval ISMD_SUCCESS : Clock divider value was returned.
@retval ISMD_ERROR_OPERATION_FAILED : Could not get clock divider value.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_external_bit_clock_div(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int *div_val);


/**
Get the handle of the physical hardware output interface identified by the hardware ID
and returns the handle to the caller. If the chipset does not have the specific physical
hardware output interface or if the specific hardware output interface is not added to
the Audio Processor, the operation fails and returns an error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] hw_id : The hardware ID of the desired output.
@param[out] output_h : Handle to the audio output instance.

@retval ISMD_SUCCESS : Hardware ID was returned.
@retval ISMD_ERROR_OPERATION_FAILED : Specified output is not added to the Audio Processor.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_handle_by_hw_id(ismd_audio_processor_t processor_h,
                                 int hw_id,
                                 ismd_audio_output_t *output_h);


/**
 * Get status of indicated input workload.
 * @param[in] processor_h : Handle for the Audio Processor instance.
 * @param[in] input_h : Handle to the audio input instance.
 * @param[in] type : the type of information that application needs to get from audio processor.
 * @param[out] value : the information details application requested from audio processor.
 *
 * @retval ISMD_SUCCESS: State successfully returned for input port.
 * @retval ISMD_ERROR_OPERATION_FAILED : Unexpected error occured while attempted to get the state.
 * @retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
 * @retval ISMD_ERROR_NULL_POINTER : Null pointer.
 * @retval ISMD_ERROR_INVALID_REQUEST : Unsupported status query.
**/

ismd_result_t SMDEXPORT ismd_audio_input_get_status( ismd_audio_processor_t processor_h,
                              ismd_dev_t                      input_h,
                              ismd_audio_input_status_type_t  type,
                              ismd_audio_input_status_t       *value );
/**
Sets the desired channel configuration the audio pipeline is to render at the output. If the incoming stream's
number of channels is not the same as the outgoing setting the incoming stream will be up-mixed or
down-mixed as necessary.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] ch_config: Desired output channel configuration.

@retval ISMD_SUCCESS : The channel configuration was applied to the output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_channel_config(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_channel_config_t ch_config);


/**
Set the output to the desired audio sample size.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] sample_size : Audio sample size.  Valid values are 16, 20, 24, and 32.
                         32-bit samples are equivalent to unpacked 24-bit samples.
                         20-bit samples are supported on only the HDMI output.

@retval ISMD_SUCCESS : Successfully set the output sample size.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_sample_size(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int sample_size);


/**
Set the sample rate of an Audio Procesor output. If the sample rate is not supported the function
will return in error.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] sample_rate : Output audio stream's sample rate.

@retval ISMD_SUCCESS : The sampling rate was successfully returned.
@retval ISMD_ERROR_INVALID_PARAMETER: Specified sample rate not supported.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_sample_rate(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int sample_rate);


/**
Set the output mode of the specified output. Can be called mid-stream to change output mode.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] out_mode : Audio stream output format.

@retval ISMD_SUCCESS : The mode was successfully changed on the output.
@retval ISMD_ERROR_INVALID_PARAMETER: Invalid mode was supplied.
@retval ISMD_ERROR_OPERATION_FAILED : Could not change mode of the output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_mode(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_output_mode_t  out_mode);


/**
Set end of audio pipeline output delay in milliseconds.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] delay_ms : Output delay time in miliseconds. Range 0-255 ms (1 ms step).

@retval ISMD_SUCCESS : Delay specified was successfully applied to the output.
@retval ISMD_ERROR_INVALID_PARAMETER : Value was out of 0-255 ms range.
@retval ISMD_ERROR_OPERATION_FAILED : Could not perform operation.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_delay(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int delay_ms);


/**
Set the size of the audio buffers to be produced from a previously created audio output port.

@param[in] processor_h : Audio Processor containing the port to be configured.
@param[in] output_h : Output port to configure.
@param[in] buffer_size : Size of the output buffers in bytes.

@retval ISMD_SUCCESS : Buffer size successfully set.
@retval ISMD_ERROR_INVALID_PARAMETER : Buffer size specified was invalid.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_port_buffer_size(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              unsigned int buffer_size);


/** \deprecated 
This configuration parameter is used to set the bit clock divider inside of the render H/W.  This will
end up dictating the bit clock that goes into the rendering hardware (i.e. DACs HDMI).

NOTE: This function is deprecated. Please use \ref ismd_audio_configure_master_clock.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] div_val : Value to be applied to divider.

@retval ISMD_SUCCESS : Setting was successfully applied.
@retval ISMD_ERROR_OPERATION_FAILED : Could not set clock divider.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_external_bit_clock_div(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              int div_val);



/**
Set encoder parameters on an ouput configured to encode the output audio data. Call this function
before enabling the outputs.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] param_type : ID of the encoder parameter to set.
@param[in] param_value : Structure containing the encoder parameter value to set.

@retval ISMD_SUCCESS : Encoder parameter was set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The encoder parameter specified is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : An unspecified error occured.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_encode_params(ismd_audio_processor_t processor_h,
                              int param_type,
                              ismd_audio_decoder_param_t *param_value );

/**
Set channels status information on an output configured to output the SPDIF interface. Ensure the output
that is being configured is configured for SPDIF.
This API has been deprecated, but still retained here for backward compatibility.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] channel_status : Structure containing the spdif channel status info.

@retval ISMD_SUCCESS : Channel status was set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : The encoder parameter specified is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : The output was not configured for SPDIF output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_spdif_channel_status(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_spdif_channel_status_t *channel_status );


/**
Sets the "channel status" parameters for an IEC60958-compatible output
(e.g. S/PDIF, HDMI with IEC60958 packetization enabled).

@param[in] processor_h : Audio processor containing the output to be configured.
@param[in] output_h : Output to configure.
@param[in] channel_status : Channel status parameters.

@retval ISMD_SUCCESS : Parameters were set successfully.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid channel status parameter setting.
@retval ISMD_ERROR_OPERATION_FAILED : The output is not an IEC60958-compatible output.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_iec60958_channel_status(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_iec60958_channel_status_t *channel_status);

/**
Mute the volume of the specified output.


@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] mute : True to mute, false to un-mute.
@param[in] ramp_ms : Ramp up/down in units of 0.1 milliseconds.

@retval ISMD_SUCCESS : Output audio successfully muted.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_mute(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              bool  mute,
                              int ramp_ms);


/**
Get the current mute status of the output.


@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] is_muted : True if muted, false if not muted.

@retval ISMD_SUCCESS : Output audio successfully muted.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_is_muted(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              bool *is_muted);


/**
Set the volume of the output.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[in] gain : The gain value to be applied to the output.
@param[in] ramp_ms : Ramp up/down in milliseconds.

@retval ISMD_SUCCESS : The volume setting was applied.
@retval ISMD_ERROR_INVALID_PARAMETER: The gain value supplied is invalid or not supported.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_set_volume(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_gain_value_t  gain,
                              int ramp_ms);


/**
Get the current volume setting on the output.

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] output_h : Handle to the audio output instance.
@param[out] gain : The current gain value being applied to the output.

@retval ISMD_SUCCESS : The volume setting was returned.
@retval ISMD_ERROR_INVALID_PARAMETER: The gain value supplied is invalid or not supported.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_get_volume(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_gain_value_t  *gain);


/*********************************************************************************************/
/*! @name Audio Output Quality Control APIs*/


/**
Releases all allocations of all filter handles and control handles and puts the Audio Quality pipeline in bypass mode.
This function is the starting point when there is a need to set up an entirely new Audio Quality pipeline. It also can
be used to disable the Audio Quality pipeline. Changes made by this function will go into effect after calling the
\ref ismd_audio_output_quality_start function.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.

@retval ISMD_SUCCESS : The resources associated to the Audio Quality pipeline are released.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_reset(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h );



/**
After makeing changes to the Audio Quality pipeline configuration, or flow, and the parameters and types for each
of the stages, this function is called to make the latest configuration effective.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.

@retval ISMD_SUCCESS : The resources associated to the Audio Quality pipeline are released.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_start(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h );


/**
Allocate an audio filter to be used a audio quality pipeline and return the handle to that filter.
See \ref AUDIO_QUALITY_MAX_FILTERS for the number of filters available.
Multiple channels and stages in the quality pipeline can reference this filter.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] type : Type of filter. 
@param[out]  filter_h: Handle use to reference this filter instance.

@retval ISMD_SUCCESS : Filter is allocated and handle returned successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
@retval ISMD_ERROR_NO_RESOURCES : No more filter instances available.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_filter_alloc(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_filter_type_t type,
                              ismd_audio_quality_filter_t *filter_h );


/**
Allocate an audio control to be used a audio quality pipeline and return the handle to that control.
See \ref AUDIO_QUALITY_MAX_CONTROLS for the number of controls available.
Multiple channels and stages in the quality pipeline can reference this control. There are some
controls that can only be allocated once they are Volume, Mute and the AVL control.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] type : Type of control. 
@param[out]  filter_h: Handle use to reference this filter instance.

@retval ISMD_SUCCESS : Control is allocated and handle returned successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
@retval ISMD_ERROR_NO_RESOURCES : No more control instances available.
@retval ISMD_ERROR_INVALID_REQUEST : A request to allocate more than one of a control that may only be allocated once.

*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_control_alloc(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_control_type_t type,
                              ismd_audio_quality_control_t *control_h );



/**
Set the parameters of a audio filter. A valid handle to the filter must be allocated first. The
type of the filter describes whether this is a peaking, or shelf, or low or high pass, and of what order, etc..
For some filters, the Q factor and/or Gain are not relevant. In this case, the parameters are ignored, set to 0.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] filter_h: Handle use to reference this filter instance.
@param[in] type: Specifies the type of filter to be used.
@param[in] params: Specifies the filter parameters. See \ref ismd_audio_quailty_filter_params_t.

@retval ISMD_SUCCESS : Filter settings applied successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_filter_config(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_filter_t filter_h,
                              ismd_audio_quailty_filter_params_t params);


/**
Sets the parameters of an audio control. A valid handle to a control must be obtained first. The number of
parameters can vary by control. Use \ref ismd_audio_quailty_control_params_t structure to specify the paramters
and the parameters count. Also refer to ismd_audio_quailty_control_params_t for detailed description of params
needed for each control.

@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] filter_h: Handle use to reference this filter instance.
@param[in] params: Specifies the control parameters. See \ref ismd_audio_quailty_filter_params_t.

@retval ISMD_SUCCESS : Control settings applied successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_control_config(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_control_t control_h,
                              ismd_audio_quailty_control_params_t params);


/**
Sets a particular channel and stage of the Audio Quality pipeline within that channel to a control.
Several channels or stages may all refer to the same control with no confusion. The stage numbering starts at 1.
Note that stage 1 is the first stage the audio encounters for a given channel, stage 2 is next,
and so on. This function sets up the order stages are called for each channel. In other words, the pipeline configuration.
If a stage is set to the invalid handle - handle 0, it signals that that is the end of the line for processing.
Therefore a long chain of stages can be cut short by setting one of them to handle 0. At that point, the data is no
longer passed down to other stages.

NOTE: The ISMD_AUDIO_CONTROL_AVL type control should NOT be added using this function. Once the AVL
control is allocated and configured. It will then be a part of the pipeline and always be processed as the last stage. 
There is no need to track the stage number of the AVL control. 


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] control_h: Handle to a filter or control.
@param[in] channel:  The channel to apply the control to.
@param[in] stage: The stage location of the control.

@retval ISMD_SUCCESS : Settings applied successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor handle, output handle or control handle.
@retval ISMD_ERROR_INVALID_REQUEST: An attempt was made to add the AVL control to a specific stage. 
@retval ISMD_ERROR_INVALID_PARAMETER: Invalid stage location. 
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_stage_config_control(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_control_t control_h,
                              ismd_audio_channel_t channel,
                              int stage);


/**
Sets a particular channel and stage of the Audio Quality pipeline within that channel to a filter.
Several channels or stages may all refer to the same filter with no confusion. The stage numbering starts at 1.
Note that stage 1 is the first stage the audio encounters for a given channel, stage 2 is next,
and so on. This function sets up the order stages are called for each channel. In other words, the pipeline configuration.
If a stage is set to the invalid handle - handle 0, it signals that that is the end of the line for processing.
Therefore a long chain of stages can be cut short by setting one of them to handle 0. At that point, the data is no
longer passed down to other stages.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] filter_h: Handle to a filter.
@param[in] channel:  The channel to apply the filter.
@param[in] stage: The stage location of the filter.

@retval ISMD_SUCCESS : Settings applied successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_stage_config_filter(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_quality_filter_t filter_h,
                              ismd_audio_channel_t channel,
                              int stage);


/**
Turn bypass mode on or off for a given stage in the given channel. This will result in no application of the control/filter
for the given channel. If a stage is bypassed, and later bypassing is turned off, it reverts to the control/filter
assignment given previously.


@param[in] processor_h : Handle to the Audio Processor instance.
@param[in] output_h : Output handle.
@param[in] channel:  The channel to operate on.
@param[in] stage: The stage location of the control.
@param[in] bypass: Specify true or false to bypass or turn off bypass.

@retval ISMD_SUCCESS : Settings applied successfully.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or output handle.
*/
ismd_result_t SMDEXPORT ismd_audio_output_quality_stage_bypass(ismd_audio_processor_t processor_h,
                              ismd_audio_output_t output_h,
                              ismd_audio_channel_t channel,
                              int stage,
                              bool bypass);


/* Audio Quality Pipeline Usage Examples

Example1: A handle to a filter is obtained. The filter is set to be a low pass first order filter with Fc=300Hz.
This handle for a filter is used for the first stage and the second stage for channel 1 and the first stage
AND the first stage and the second stage for channel 2. The data for channel 1 and channel two are each
still processed independently through this stage. However, now that there are two filters cascaded together,
the net effect is a filter with 40dB per decade (or a second order filter).

Example2: A handle to a control is obtained. After setting its type and parameters, it is plugged into
the audio control pipeline at the 4th stage of channel 1 AND channel 2. The data flowing thru channel 1 and 2
are kept separate, but they both use this control. Any state information necessary for the control to function
is kept separate for each channel.

*/




/*********************************************************************************************/




/*********************************************************************************************/
/*! @name Audio Asynchronous Notification APIs*/


/**
 Retrieve the handle of the SMD event associated with an internal condition of
 an input of an audio processor.  The audio subsystem will "set" the SMD event
 when the condition occurs and any thread waiting for the event using
 ismd_event_wait() or ismd_event_wait_multiple() will wake up.

 @param[in] processor_h : Handle to an audio processor instance.
 @param[in] input_h: Handle to an input on the audio processor.
 @param[in] type : The type of event.
 @param[out] event_h : Location to store a handle to an SMD event.

 @retval ISMD_SUCCESS : The SMD event handle was successfully returned.
 @retval ISMD_ERROR_INVALID_HANDLE : The specified processor or input handle is
 stale or invalid.
 @retval ISMD_ERROR_NO_RESOURCES : No resources were available to allocate an
 event associated with the specified type.
 @retval ISMD_ERROR_INVALID_PARAMETER : The specified type is invalid.
 */
ismd_result_t SMDEXPORT
ismd_audio_input_get_notification_event( ismd_audio_processor_t    processor_h,
                                         ismd_dev_t                input_h,
                                         ismd_audio_notification_t type,
                                         ismd_event_t             *event_h );



/**
 Reset an internal condition in the audio subsystem.  This will also cause the
 SMD event associated with the condition to trasition to the "reset" state.

 @param[in] processor_h : Handle to an audio processor instance.
 @param[in] input_h : Handle to an input on the audio processor.
 @param[in] type : The type of audio condition to reset.

 @retval ISMD_SUCCESS : The audio condition and associated SMD event was
 successfully reset.
 @retval ISMD_ERROR_INVALID_HANDLE : The specified processor or input handle is
 stale or invalid.
 @retval ISMD_ERROR_INVALID_PARAMETER : The specified type is invalid.
 */
ismd_result_t SMDEXPORT
ismd_audio_reset_notification_event( ismd_audio_processor_t    processor_h,
                                     ismd_dev_t                input_h,
                                     ismd_audio_notification_t type );

/**
Set the clock synchronization handle for an audio input device.  The device will 
use the specified handle for clock recovery.   This is typically used only when the 
input is a physical input.

Note that in order for clock recovery to be functional, the capture audio capture 
device must be driven by an external clock source, the clock set on the input 
must be the primary AV clock, and the input must be the primary audio input.
To disable the clock recovery call with sync_clock set to ISMD_CLOCK_HANDLE_INVALID. 

@param[in] processor_h : Handle for the Audio Processor instance.
@param[in] input_h : Handle to the audio input instance.
@param[in] sync_clock : handle to a sync_clock.

@retval ISMD_SUCCESS : the sync clock was successfully set for the device.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t SMDEXPORT ismd_audio_input_set_sync_clock(ismd_audio_processor_t processor_h,
                                                               ismd_dev_t input_h,
                                                               clock_sync_t sync_clock);



/**@}*/ // end of ingroup ismd_audio
/**@}*/ // end of weakgroup ismd_audio

#ifdef __cplusplus
}
#endif

#endif

