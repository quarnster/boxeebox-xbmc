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

/** @weakgroup playback_doc SMD playback documentation

SMD high level features.
\anchor AV_Synch
<H2>Audio/Video Synchronization</H2>

The following is a typical sequence for enabling synchronized audio/video
presentation timing:

Setting up a synchronized pipeline: 
-# Allocate a new clock using \ref ismd_clock_alloc.
-# Lock the audio and video outputs to the clock using \ref ismd_clock_make_primary.
-# If the display supports multiple output pipes/timing generators, use
\ref ismd_clock_set_vsync_pipe to specify which display pipe should be
used for video presentation timing. Internally, the clock will capture the time
of every VSYNC for the specified display pipe.
-# Set all of the modules in the pipeline to use the new clock using 
\ref ismd_dev_set_clock.
-# Configure the video decoder to interpolate a PTS value for pictures that
do not have have a PTS in the original stream using
\ref ismd_viddec_set_pts_interpolation_policy.
-# When adding the audio processor input to be used for playing the audio
portion of the stream, be sure to create a "timed" input by setting the
"timed_stream" parameter of \ref ismd_audio_add_input_port.

Starting playback:
-# Calculate the time to start playback by 1) reading the current time using 
\ref ismd_clock_get_time and 2) adding a small amount of time to account for 
the time required to set the base time on all of the pipeline elements.
-# Set the start time calculated above on all of the pipeline modules by calling
\ref ismd_dev_set_stream_base_time. 
-# Set all of the pipeline modules into the ISMD_DEV_STATE_PLAY state using 
\ref ismd_dev_set_state.

\anchor Trick_modes
<H2>Trick Mode Transitions</H2>

SMD is very flexible in how timing is managed in trick modes. In general, 
SMD uses a newsegment "tag" to communicate rate changes "in-band".  This tag 
contains information describing when the data following it should be played, 
and at which speed it should be played.  The one exception to using newsegment 
tags to initiate trick modes is the "Dynamic Rate Change" or out of band mode, 
described below in <A HREF="#Dynamic_Rate_Change">Dynamic Rate Change</A>. 

The following is a typical sequence for changing the playback speed:

-# Stop the source feeding the pipeline.
-# Flush the pipeline to clear the data in all the devices. Call 
\ref ismd_dev_flush on devices in the order in which the data flows - 
starting from the demux, viddec, vidpproc, vidrend, audio.
-# Insert a new segment tag at the beginning of the pipeline - 
   -# Allocate a zero sized buffer. Call \ref ismd_buffer_alloc with size 0
   -# Create a new segment tag of type \ref ismd_newsegment_tag_t - the 
requested rate should be the normalized trick mode rate
   -# Assign the tag to the buffer. Call \ref ismd_tag_set_newsegment with 
buffer id and tag details.
   -# Write it to the input port of the first element in the pipeline, typically 
the demux.
-# Set the frame mask in the viddec to mask P and B frames by calling 
\ref ismd_viddec_set_frame_mask. A special case of frame masking is I-frame 
only trick modes, where all non-reference frames are dropped by the video decoder.
-# Pause the pipeline. Call \ref ismd_dev_set_state on all the devices with 
state ISMD_DEV_STATE_PAUSE.
-# Recalculate and set the correct basetime on all devices that need it. 
Call \ref ismd_dev_set_stream_base_time. The basetime can be calculated by 
the formula:
     new_base_time = (current_time - initial_localized_pts in the new segment tag)
-# Restart the pipeline. Call \ref ismd_dev_set_state on all devices with 
state ISMD_DEV_STATE_PLAY.

Another important point is that if data is being fed discontiguously (such as 
reverse or I-frame modes) the discontinuity flag should be set at the points 
that represent a break in the input data (between GOPs in reverse, for instance).  
This will cause the viddec to discard old reference frames to prevent macroblocking.  
This can be done by setting the discontinuity flag in the buffer descriptor's 
ES attributes to "true" before sending the buffer to the demux input.  

\anchor Dynamic_Rate_Change
<H3>Dynamic Rate Change</H3>

This mode can be used if the direction of playback is not changing and the 
format of the input does not change.  This mode gives the smoothest trick mode 
transition experience as it does not require to flush the pipeline and rebuffer
before the transition to the new playback speed.

To use this mode the following must apply:

-# Playback must not change direction (forward to forward or reverse to reverse only).
-# The input structure is not changing.  For example, if going from a mode that 
plays all GOPs to one where the source only supplies every other GOP, a flush must 
be used.
-# The video decoder frame masking is not changing.
-# Once this mode is activated the renderers ignore rate changes communicated in 
newsegment tags.  The other fields in the newsegment tags are still used.

The sequence of operations is as follows:
-# Optionally enable the viddec downscaler, if using high rates on HD content (to 
ease workload on vidpproc)
-# Pause pipeline by calling \ref ismd_dev_set_state on all the devices with state 
ISMD_DEV_STATE_PAUSE.
-# Get the stream position from the video renderer by calling 
\ref ismd_vidrend_get_stream_position
-# Set the play rate on the renderers by calling \ref ismd_dev_set_play_rate
-# Set base time to (current time - current scaled PTS)
-# Restart the pipeline. Call \ref ismd_dev_set_state on all devices with state 
ISMD_DEV_STATE_PLAY.

\anchor Fixed_Framerate_Playback
<H3>Fixed frame-rate Playback</H3>

The video renderer (vidrend) allows fixed frame-rate playback (PTS values ignored) 
which may be simpler to use for some faster trick modes or for reverse I-frame 
trick modes.  Note that audio should be disabled when using this feature 
(automatically happens for faster trick modes and reverse playback). To enable 
fixed framerate playback the video renderer's \ref ismd_vidrend_enable_fixed_frame_rate() 
function is used to set the rate, and \ref ismd_audio_input_disable() to 
disable audio. The rate is expressed as the number of 90kHz ticks that 
separate the frames.  

\anchor Partial_GOP_Decode
<H3>Partial GOP decode</H3>

The video decoder (viddec) supports an option to only decode an arbitrary number 
of frames, N, from a segment of data, where N can be configured through 
\ref ismd_viddec_set_max_frames_to_decode().  This can be used in conjunction 
with fixed frame-rate and sourcing a subset of the input file to create a 
fast forward "bursty" trick mode effect that looks smoother than simple frame 
masking in some instances (dependent on the content motion).

The sequence of operations is:
-# Stop the source feeding the pipeline.
-# Flush the pipeline to clear the data in all the devices. Call 
\ref ismd_dev_flush on devices in the order in which the data flows - 
starting from the demux, viddec, vidpproc, vidrend, audio.
-# Insert a new segment tag at the beginning of the pipeline - 
   -# Allocate a zero sized buffer. Call \ref ismd_buffer_alloc with size 0.
   -# Create a new segment tag of type ismd_newsegment_tag_t - the requested 
rate should be the normalized trick mode rate.
   -# Assign the tag to the buffer. Call \ref ismd_tag_set_newsegment with 
buffer id and tag details.
   -# Write it to the input port of the first element in the pipeline, maybe 
the demux.
-# Configure the video decoder: 
   -# Set the frame mask, \ref ismd_viddec_set_frame_mask.
   -# Set the maximum frames to decode, \ref ismd_viddec_set_max_frames_to_decode.
   -# And set error concealment, \ref ismd_viddec_set_frame_error_policy.
-# Enable or disable fixed-framerate mode in the video renderer, 
\ref ismd_vidrend_enable_fixed_frame_rate or \ref ismd_vidrend_disable_fixed_frame_rate.
-# Enable or disable audio, \ref ismd_audio_input_disable or 
\ref ismd_audio_input_enable.
-# Configure the filesource as desired.
-# Pause the pipeline, set the base time to the current time, and resume playback.

The other important aspect of this functionality is how the data is sourced.  
A file source is required to set the input buffer ES attributes' discontinuity 
flag for each new data unit sourced through the pipline.  This flag 
being set does a few important things:
-# Causes the viddec to flush all reference frames and halt decoding until 
the next reference frame is found
-# Causes the viddec to reset its frame counter for the partial-GOP decode 
feature, so it starts decoding again until the count is reached.
-# Causes the demux to abstain from sending a newsegment tag when the actual 
discontinuity is encountered.


\anchor Smooth_Reverse
<H3>Smooth reverse trick mode</H3>

In addition to I-frame trick modes to accomplish reverse playback, the viddec 
supports reversing the frames in a GOP to get a smoother reverse effect. 
The GOPs must be fed in reverse order relative to each other for this 
functionality to work. This mode is triggered automatically whenever the 
newsegment contains a negative rate AND the video renderer is configured 
in fixed frame rate mode. See <A HREF="#Fixed_Framerate_Playback">Fixed frame-rate Playback</A>.

Due to the amount of video memory required to store decoded frames for reversal, 
the video decoder (viddec) is designed with an upper limit of 18 frames within a 
video segment. Therefore, to enable smooth reverse trick mode, the viddec needs
to be configured using \ref ismd_viddec_set_max_frames_to_decode to set the maximum
number of frames to decode. If the value (max_frames_to_decode) specified by 
an application is greater than 18, the decoder will default to 18 for GOP reversal. 
The remaining frames within that video segment will be dropped until the next video 
segment is received.
 
It is important that the application sets a discontinuity flag at the points 
that represent a start of a new video segment. This tells the decoder to output
the reversed frames buffered from that segment. The discontinuity flag can be 
set in the buffer descriptor's ES attributes to "true" before sending the buffer 
to the demux input.

To transition from normal playback to smooth reverse trick mode:

-# Stop the source feeding the pipeline.
-# Flush the pipeline to clear the data in all the devices. Call 
\ref ismd_dev_flush on devices in the order in which the data flows - 
starting from the demux, viddec, vidpproc, vidrend, audio.
-# Insert a new segment tag at the beginning of the pipeline with a negative rate.
-# Configure the video decoder: 
   -# Set the frame mask, \ref ismd_viddec_set_frame_mask.
   -# Set the maximum frames to decode, \ref ismd_viddec_set_max_frames_to_decode.
   -# And set error concealment, \ref ismd_viddec_set_frame_error_policy.
-# Enable fixed-framerate mode in the video renderer, 
\ref ismd_vidrend_enable_fixed_frame_rate. Note that future versions of the software
will not require fixed frame rate when enabling smooth reverse trick modes.
-# Disable audio \ref ismd_audio_input_disable.
-# Configure the filesource to feed data to the pipeline in reverse order.
-# Pause the pipeline, set the base time to the current time, and resume playback.

To transition from smooth reverse trick modes to normal playback:

-# Stop the source feeding the pipeline.
-# Reset the number of frames to decode on video decoder by calling
\ref ismd_viddec_set_max_frames_to_decode using ISMD_VIDDEC_ALL_FRAMES. 
-# Disable fixed frame rate mode on video renderer by calling 
\ref ismd_vidrend_enable_fixed_frame_rate.
-# Enable audio by calling \ref ismd_audio_input_enable.
-# Flush the pipeline to clear the data in all the devices. Call 
\ref ismd_dev_flush on devices in the order in which the data flows - 
starting from the demux, viddec, vidpproc, vidrend, audio.
-# Insert a new segment tag at the beginning of the pipeline with a negative rate.
-# Configure the filesource to start feeding data forward.
-# Pause the pipeline, set the base time to the current time, and resume playback. 

 \anchor Single_Frame_Step
 <H3>Single Frame Step</H3>

This mode can step frames one by one when the pipeline is paused. This is done
by telling the video renderer (vidrend) and audio renderer (audio) to advance to the
next frame in forward or backward direction.
The sequence of operations is as follows:
-# Pause pipeline by calling \ref ismd_dev_set_state on all the devices with state
ISMD_DEV_STATE_PAUSE.
-# Get the stream position from the video renderer by calling
\ref ismd_vidrend_get_stream_position (or from audio renderer by calling
\ref ismd_audio_input_get_stream_position).
-# Advance to the next frame by calling
\ref ismd_vidrend_advance_to_pts on video renderer or \ref ismd_audio_input_advance_to_pts
on audio renderer.
-# To advance to another frame, the application should repeat the above step.
To resume normal playback, the application should follow below programming sequence:
-# Query the current clock time using \ref ismd_clock_get_time.
-# Adding a new variable to the recalculation of base time.
The base time is normally incremented by the amount of time the clock ran and decremented
by the amount of media that was played in the paused state.
-# Update both the video and audio renderers base time by calling
\ref ismd_dev_set_stream_base_time.
-# Restart the pipeline. Call \ref ismd_dev_set_state on all devices with state
ISMD_DEV_STATE_PLAY.
Note: This can be used during forward and reverse playback.
If reverse frame advance is desired, first put the system into reverse mode and
flush the pipeline. Direction changes (step forward,
then back, then forward again) would require a pipeline flush, newsegment to set the correct
play rate/direction, and a reconfiguration of the smart source.

Note: find more description on frame advance/reverse feature
of \ref ismd_vidrend "video renderer".


\anchor Dual_Stream
<H2>Dual Stream overview</H2>
SMD provides capibilities to decode multiple streams, one use case is to display
two (or more) video streams on the display while playback of one audio is heard.
This example will decode two streams and display the video of the second stream in
a small window overlaid on top of the first (Picture in Picture) while listening to
the primary stream audio.

\dot
digraph PIP {
  rankdir="LR";
  subgraph PipelineA {
    label = "Pipeline A";
    color = "orange";
    Demux [shape="Msquare"];
    Viddec [shape="doubleoctagon", labelURL="group__ismd__viddec.html", tooltip="the best driver in SMD"];
    Vidpproc [shape="box"];
    Vidrend [shape="box"];

    Demux -> input0[label="Audio ES",color = "orange"];
    Demux -> Viddec[label="Video ES",color = "orange"];
    Viddec -> Vidpproc[label="NV12 4:2:0",color = "orange"];
    Vidpproc -> Vidrend[label="NV16 4:2:2",color = "orange"];

    Demux -> input1[label="Audio ES",color = "turquoise1"];
    Demux -> Viddec[label="Video ES",color = "turquoise1"];
    Viddec -> Vidpproc[label="NV12 4:2:0",color = "turquoise1"];
    Vidpproc -> Vidrend[label="NV16 4:2:2",color = "turquoise1"];
  }

  subgraph Audio {
      rankdir="LR";
      subgraph clusterAudio {
        label = "Audio";
        audio_cloud [shape = "egg"];
        input0 -> audio_cloud[color = "orange"];
        input1 -> audio_cloud[arrowhead="tee", label="mute", color = "red", fontcolor = "red"];
        audio_cloud -> output0[color = "orange"];
      }
      output0 -> o0[color = "orange"];
      o0 [label="",shape=circle,height=0.12,width=0.12];
    }

  in_0[color = "orange"]; 
  in_0 -> Demux [label="TS/PS in",shape=circle,height=0.12,width=0.12, color = "orange"];
  Vidrend -> Display [label="VBD UPP_A",color = "orange"];

  in_1[color = "turquoise1"]; 
  in_1 -> Demux [label="TS/PS in",shape=circle,height=0.12,width=0.12, color = "turquoise1"];
  Vidrend -> Display [label="VBD UPP_B",color = "turquoise1"];
}
\enddot

-# Follow the same steps in <A HREF="#AV_Synch"><B>Audio/Video Synchronization</B></A> for the first pipeline, use UPP A for video.
-# For the second pipeline, change the following
   -# Use another universal pixel plane (UPP) example UPP_B for display output.
   -# Do NOT call \ref ismd_clock_make_primary.
   -# Mute or disable the audio input.
   -# Setup the vidpproc to downscale the video (optionally use the viddec horizontal downscaling).
   -# Position the scaled video on the display plane using x and y offsets (be sure to stay inbounds for the target areas).
-# Follow the same steps in <A HREF="#AV_Synch"><B>Audio/Video Synchronization</B></A> for Starting playback for each stream.
<H3> To PIP Swap </H3>
-# The pipelines should not need to change to do PIP swap.
-# Video Swap:
   -# Swap the output scale factors and possibly other settings of Viddec
   -# Swap the output scale factors and possibly other settings of Vidpproc
   -# Swap the display planes of Vidrend
-# Audio Swap:
   -# Disable and flush (or mute) the input that is being swapped to PIP display
   -# Enable (or unmute) the input that is being swapped to the main display
   -# tbd - more to do here
-# Clock Swap:
   -# Call \ref ismd_clock_reset_primary on the pipeline that is being swapped to PIP display
   -# Call \ref ismd_clock_make_primary on the pipeline that is being swapped to main display

\anchor PSI_parsing
<H2>Turn on and setup section filters for PSI parsing</H2>
\addindex PSI filtering
Review the MPEG specification for how to do PSI parsing.
Suggested steps for PSI parsing and retrieve the PSI tables: (Review the \ref ismd_demux "demux api" for specifics.)

-# Open filter using ismd_demux_filter_open()
-# Open section filter using ismd_demux_section_filter_open()
-# Set mask, see ismd_demux_section_filter_set_params()

Note: One can also create events to trigger on filter output using ismd_event_alloc() with ismd_demux_filter_get_output_port() and ismd_port_attach().

Sample code for the above is available in ../smd_sample_apps/common_utils/psi_handler/psi_handler.c::init_psi_context().

*/

