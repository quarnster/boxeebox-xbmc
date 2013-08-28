/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007-2009 Intel Corporation. All rights reserved.
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
 
#ifndef __ISMD_VIDREND_H__
#define __ISMD_VIDREND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ismd_global_defs.h"

/** @weakgroup ismd_vidrend Video Rendering

Video Renderer Module

The video renderer takes uncompressed video frames/fields as input. At every 
video field/frame interrupt, the renderer determines if the next field/frame 
in its internal queue is ready for display. If so, it sends the pointers to 
the display. If not, previous field/frame is repeated.

Theory of Operation:

I. Normal playback:
-# Open the renderer.
-# Set the video plane.
-# Set the clock for the renderer to use.
-# Set the basetime of the stream.
-# Set the display mode.
-# Start the renderer.
Steps 2 thru 5 can be done in any order. 

Basetime, presentation timestamps and current time are based off the same clock
and the renderer is notified of this clock in step 3 above.
The renderer start fails if the base time or a valid video plane is not set.

On stop, the renderer disables the video plane and releases all frame/field 
buffers.
On close, all resources are deallocated and everything is reset to the initial
state.

II. Trick modes:

Pause/Resume:
-# Pause playback.
-# Set new basetime.
-# Resume playback.

Frame advance/reverse:

The video renderer provides APIs to support frame advance in both forward
and backward directions. It does this via the \ref ismd_vidrend_advance_to_pts
method. The video renderer expects the following programming sequence to 
be followed. 

-# If reverse frame advance is desired, put the system into reverse mode and
flush the renderer. 
-# Pause playback. If \ref ismd_vidrend_advance_to_pts is called when the 
video renderer is not paused, or not started, the operation will fail. 
-# Query current linear time using \ref ismd_vidrend_get_stream_position
-# Call \ref ismd_vidrend_advance_to_pts with the linear pts value to jump
to. Time is specified in 90 KHz clock ticks. The value is relative to the 
current linear time. 
\n\n
Example: assume that \ref ismd_vidrend_get_stream_position 
returned 10000, and we had a stream that was 30 FPS. To advance forward one 
frame, we'd call \ref ismd_vidrend_advance_to_pts with the value 13000. To
advance backwards one frame (this requires the system to be in reverse
playback mode), the value 7000 would be specified. 
\n\n
Note: In the case that the linear pts value specified in 
\ref ismd_vidrend_get_stream_position is < the current linear stream position 
in forward playback (or > current linear position for reverse playback), an
error will be generated.
\n\n
Note: If \ref ismd_vidrend_advance_to_pts is called multiple times prior
to triggering the video frame flip, the value is overwritten and the last
value is used. 
\n\n
-# The frame will then be flipped at the appropriate time specified in the
previous step. The video renderer will return to the paused state. 
-# To advance to another frame, the application should repeat the process 
outlined above starting at step 3.
\n\n
Note: Transitioning between forward and reverse playback requires flushing
the video renderer prior to step 3. Other components must be configured
for the new direction appropriately. 
\n\n
-# To resume normal playback, the application should query the current clock
time using \ref ismd_clock_get_time, update both the video and audio 
renderers base time (use \ref ismd_dev_set_stream_base_time), then 
call \ref ismd_dev_set_state to the PLAY state. If the renderer is waiting to
advance to a pts, it will stop and immediately resume normal playback. 

Other trick modes:
-# Pause playback.
-# Flush the renderer.
-# Set new basetime.
-# Resume playback.
*/

/** @ingroup ismd_vidrend */
/** @{ */

/**
A total of 16 renderers can be initialized in the system at a time.
*/
#define NUM_OF_RENDERERS 16
/**
Number of event types supported by the renderer
*/
#define NUM_OF_EVENTS 10

/**
enum ismd_vidrend_flush_policy_t
To determine renderer behavior on flush.
*/
typedef enum {
   ISMD_VIDREND_FLUSH_POLICY_INVALID,       // invalid value
   ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME,  ///< repeat last frame/field
   ISMD_VIDREND_FLUSH_POLICY_DISPLAY_BLACK, ///< send black frame/field
} ismd_vidrend_flush_policy_t;

/**
enum ismd_vidrend_vsync_type_t
To determine vsync type from display.
*/
typedef enum {
   ISMD_VIDREND_VSYNC_TYPE_FRAME,        // frame
   ISMD_VIDREND_VSYNC_TYPE_FIELD_TOP,    // top field
   ISMD_VIDREND_VSYNC_TYPE_FIELD_BOTTOM, // bottom field
} ismd_vidrend_vsync_type_t;

/**
enum ismd_vidrend_event_type_t
To identify events that need to be monitored.
*/
typedef enum {
   /// An error occured in vidrend - skipped vsync, late flip, late frames
   /// or out of order frames. Fields out_of_order_frames, skipped_vsyncs,
   /// late_flips, frames_late in ismd_vidrend_stats_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_ERROR              = 0,

   /// Interrupt received from display with frame polarity
   /// vsyncs_frame_received in ismd_vidrend_stats_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_VSYNC_FRAME        = 1,

   /// Interrupt received from display with top field polarity.
   /// vsyncs_frame_received in ismd_vidrend_stats_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_VSYNC_FIELD_TOP    = 2,

   /// Interrupt recevied from display with bottom field polairty.
   /// vsyncs_frame_received in ismd_vidrend_stats_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_VSYNC_FIELD_BOTTOM = 3,

   /// Resolution change from next frame. The current frame on display has a
   /// different resolution from the next frame in the renderer.
   /// curr_frame_width, curr_frame_height, next_frame_width, next_frame_height
   /// in ismd_vidrend_status_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_RES_CHG            = 4,

   /// End of stream
   ISMD_VIDREND_EVENT_TYPE_EOS                = 5,

   /// Client ID Last seen by the renderer.
   /// client_id_last_seen in ismd_vidrend_status_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_CLIENT_ID_SEEN     = 6,
   
   /// End of segment
   ISMD_VIDREND_EVENT_TYPE_EOSEG              = 7,

   /// Underrun Event.
   /// underrun_amount in ismd_vidrend_status_t will be updated.
   ISMD_VIDREND_EVENT_TYPE_UNDERRUN           = 8,

   /// Start of segment
   ISMD_VIDREND_EVENT_TYPE_SOSEG              = 9

} ismd_vidrend_event_type_t;
   
/**
enum ismd_vidrend_interlaced_rate_t
To identify the refresh rate for interlaced display.
*/
typedef enum {
   ISMD_VIDREND_INTERLACED_RATE_INVALID,
   ISMD_VIDREND_INTERLACED_RATE_50,
   ISMD_VIDREND_INTERLACED_RATE_59_94,
} ismd_vidrend_interlaced_rate_t;

/**
enum ismd_vidrend_stop_policy_t
To determine renderer behavior on stop.
*/
typedef enum {
   ISMD_VIDREND_STOP_POLICY_INVALID,       // invalid value
   ISMD_VIDREND_STOP_POLICY_CLOSE_VIDEO_PLANE,  // close video plane when stop
   ISMD_VIDREND_STOP_POLICY_KEEP_VIDEO_PLANE, // keep video plane on screen
   ISMD_VIDREND_STOP_POLICY_DISPLAY_BLACK // keep video plane on screen, and flip a black frame
} ismd_vidrend_stop_policy_t;

/**
enum ismd_vidrend_pause_policy_t
To determine renderer behavior when paused.
*/
typedef enum {
   ISMD_VIDREND_PAUSE_POLICY_INVALID,  // invalid value
   ISMD_VIDREND_PAUSE_POLICY_NO_FLIP,  // do not flip frames while renderer is paused
   ISMD_VIDREND_PAUSE_POLICY_RE_FLIP,  // continuously re-flip the last frame while renderer is paused
} ismd_vidrend_pause_policy_t;

/**
enum ismd_vidrend_mute_policy_t
The policy to mute video.
*/
typedef enum

{
   ISMD_VIDREND_MUTE_NONE = 0, /* Normal video playback, use to "turn off" one of the following modes */
   ISMD_VIDREND_MUTE_DISPLAY_BLACK_FRAME, /* Black screen */
   ISMD_VIDREND_MUTE_HOLD_LAST_FRAME        /* hold last frame */
} ismd_vidrend_mute_policy_t;

/**
enum ismd_vidrend_message_t
To identify vidrend message type for MIB management.
*/
typedef enum {
   ISMD_VIDREND_MESSAGE_OUT_OF_SEGMENT = 0x0, /* The pts of frame is out of segment. This frame will be dropped. */   
   ISMD_VIDREND_MESSAGE_OUT_OF_ORDER = 0x1, /* The pts of frame is smaller than the one of previous frame. This frame will be dropped. */
} ismd_vidrend_message_t;

/**
Structure to consolidate all information related to stream position.
*/
typedef struct {

   /// Base time currently set in the renderer.
   ismd_time_t base_time;

   /// Current time when stream position is queried.
   ismd_time_t current_time;

   /// The current segment time
   ismd_pts_t segment_time;
   
   /// The current unscaled local time.
   ismd_pts_t linear_time;
   
   /// The current scaled local time.
   ismd_pts_t scaled_time;

   /// First valid original pts processed by renderer. 
   /// A value of ISMD_NO_PTS implies that the "first_orig_pts" is not
   /// available.
   ismd_pts_t first_segment_pts;

   /// Last valid original pts processed by renderer. 
   /// A value of ISMD_NO_PTS implies that the "last_segment_pts" is not
   /// available and that "linear_of_last_segment_pts" is undefined.
   ismd_pts_t last_segment_pts;    /**< Least Significant Bit gets truncated in the pipeline */

   /// Rebased pts corresponding to the "last_segment_pts". 
   /// If "last_segment_pts" is ISMD_NO_PTS, this value is undefined.
   ismd_pts_t linear_of_last_segment_pts;

   /// The rebased pts of the frame/field last sent to display
   ismd_pts_t last_linear_pts;

   /// The rebased pts of the frame/field that will potentially
   /// be sent to display at the next framestart interrupt.
   ismd_pts_t next_linear_pts;

   /// Replaced by \ref ismd_vidrend_stream_position_info_t::last_segment_pts
   ismd_pts_t last_orig_pts; //!< @deprecated

   /// Replaced \ref ismd_vidrend_stream_position_info_t::linear_of_last_segment_pts
   ismd_pts_t local_of_last_orig_pts;  //!< @deprecated

   /// Replaced \ref ismd_vidrend_stream_position_info_t::last_linear_pts
   ismd_pts_t current_local_pts; //!< @deprecated

   /// Replaced \ref ismd_vidrend_stream_position_info_t::next_linear_pts
   ismd_pts_t next_local_pts; //!< @deprecated

   /// This is the clock time that was recorded when the frame was flipped.  
   ismd_pts_t flip_time;

   /// Current stream position between zero and the duration of the clip
   /// ISMD_NO_PTS will be returned if the driver does not have enough information
   /// to calculate this field 
   ismd_pts_t stream_position;

} ismd_vidrend_stream_position_info_t;

/**
Structure to maintain renderer statistics.
*/
typedef struct {
   unsigned int vsyncs_frame_received;  //!< Number of frame vsyncs since start
   unsigned int vsyncs_frame_skipped;   //!< Number of frame vsyncs skipped
   unsigned int vsyncs_top_received;    //!< Number of top field vsyncs since start
   unsigned int vsyncs_top_skipped;     //!< Number of top field vsyncs skipped
   unsigned int vsyncs_bottom_received; //!< Number of bottom field vsyncs since start
   unsigned int vsyncs_bottom_skipped;  //!< Number of bottom field vsyncs skipped
   unsigned int frames_input;           //!< Number of frames fed into the renderer
   unsigned int frames_displayed;       //!< Number of frames (progressive) /fields (interlaced) sent to display
   unsigned int frames_released;        //!< Number of frames released
   unsigned int frames_dropped;         //!< Number of frames drops
   unsigned int frames_repeated;        //!< Number of frames (progressive) /fields (interlaced) repeats
   unsigned int frames_late;            //!< Number of frames that are late
   unsigned int frames_out_of_order;    //!< Number of frames that are out of order
   unsigned int frames_out_of_segment;  //!< Number of frames that are out of segment range
   unsigned int late_flips;             //!< Number of flipped frames that are late
} ismd_vidrend_stats_t;

/**
Structure to maintain renderer's current status.
*/
typedef struct {
   /// Difference in the presentation timestamps of consecutive frames that are
   /// not out of order - in 90 kHz clock ticks
   unsigned int content_pts_interval;
   
   /// Frame time of the display in 90 kHz clock ticks - for interlaced display
   /// this time is the sum of its field times
   unsigned int display_frame_time;
   
   /// Difference between the predicted time of the next display interrupt and
   /// the (basetime + presentation timestamp of the frame chosen for display)
   int pts_phase;
   
   /// Client ID last seen by the renderer
   int client_id_last_seen;
   
   /// Content Resolution
   unsigned int curr_frame_width;  ///< Width of frame currently on display
   unsigned int curr_frame_height; ///< Height of frame currently on display
   unsigned int next_frame_width;  ///< Width of next frame in renderer
   unsigned int next_frame_height; ///< Height of next frame in renderer
} ismd_vidrend_status_t;

/**
RENDERER FUNCTIONS FOR NORMAL PLAYBACK
*/
/**
Opens an available instance of the video renderer. 
The default video renderer capabilities are enabled for every new instance and
the user can override them if required. All resources (port, threads) 
required for normal operation are allocated. This operation must succeed for 
any other renderer operations to be successful.
The operation fails if all available instances of the video renderer are 
already open.

@param[out] vidrend : handle to the allocated instance. This handle will 
                      be used to access the video renderer for subsequent 
                      operations.

@retval ISMD_SUCCESS : The instance is successfully allocated.
@retval ISMD_ERROR_NULL_POINTER : The handle pointer is NULL.
@retval ISMD_ERROR_NO_RESOURCES : No free instance of renderer is available.
@retval ISMD_ERROR_OPERATION_FAILED : Unrecoverable internal error occured 
                                      during resource allocation.
*/
ismd_result_t ismd_vidrend_open(ismd_dev_t *vidrend);

/**
Gets the input port for the given renderer instance.
Raw video frames/fields are fed to the renderer through this port.

@param[in] vidrend : handle to the renderer instance.
@param[out] port : handle to the allocated input port.

@retval ISMD_SUCCESS : The port handle is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The port handle pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_input_port(ismd_dev_t         vidrend,
                                          ismd_port_handle_t *port);

/**
Sets the video plane that receives the frames/fields pushed by the renderer.
A valid plane must be set before the renderer can be successfully started.
The user is responsible for setting a valid unused plane id. The renderer
does not validate this.
 
@param[in] vidrend : handle to the renderer instance.
@param[in] video_plane_id : the desired video plane.

@retval ISMD_SUCCESS : The plane id is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/
ismd_result_t ismd_vidrend_set_video_plane(ismd_dev_t   vidrend, 
                                           unsigned int video_plane_id);

/**
Gets the video plane used by the renderer.

@param[in] vidrend : handle to the renderer instance.
@param[out] video_plane_id : plane set in the renderer.

@retval ISMD_SUCCESS : The plane is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The plane pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_video_plane(ismd_dev_t   vidrend,
                                           unsigned int *video_plane_id);

/**
Sets the display refresh rate for interlaced display.
The content rate and display refresh rate are used by the renderer to 
decide which frames/fields are sent to the display and when.
 
@param[in] rate : refresh rate of the display.

@retval ISMD_SUCCESS : The refresh rate is successfully set.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid refresh rate.
*/
ismd_result_t ismd_vidrend_set_interlaced_display_rate(
                                         ismd_vidrend_interlaced_rate_t rate);

/**
Gets the refresh rate set in the renderer.

@param[out] rate : refresh rate set in the renderer.

@retval ISMD_SUCCESS : The refresh rate is successfully returned.
@retval ISMD_ERROR_NULL_POINTER : The rate pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_interlaced_display_rate(
                                       ismd_vidrend_interlaced_rate_t *rate);

/**
Enables output port support for the renderer instance.

An output port is created for this instance. The frames, apart from being flipped 
to the display through the VBD interface, are written to this output port when 
it is enabled through this API.

Time Offset: If the rendering time of a frame is X, where X is specified in 90KHz 
clock ticks, then with the time offset(T) specified, the frame would be written to the
output port at (X - T). A non-zero time offset value is NOT supported currently and 
would return ISMD_ERROR_INVALID_PARAMETER. 

Port Depth: Output port depth is the number of frames to be queued in the port. It
cannot be less than 0.

@param[in] vidrend : handle to the renderer instance.
@param[in] time_offset : time offset specified in 90KHz clock ticks.
@param[in] port_depth : depth of the output port.
@param[out] output_port : output port handle for this instance.

@retval ISMD_SUCCESS : The output port handle successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : The time_offset or port_depth is invalid.
@retval ISMD_ERROR_NULL_POINTER : The output_port pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED : Enable output port operation failed.
*/
ismd_result_t ismd_vidrend_enable_port_output(ismd_dev_t vidrend,
                                              ismd_time_t time_offset,
                                              int port_depth,
                                              ismd_port_handle_t *output_port);

/**
Disables the output port support for the renderer instance.

The thread that writes frames to the output port is destroyed and the 
output port is de-allocated.

@param[in] vidrend : handle to the renderer instance.

@retval ISMD_SUCCESS : The output port is disabled successfully.
@retval ISMD_ERROR_HANDLE_INVALID : The renderer handle is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : Disable output port operation failed.
*/
ismd_result_t ismd_vidrend_disable_port_output(ismd_dev_t vidrend);

/**
Gets the output port for the given renderer instance, if enabled.
Video frames/fields are taken from the renderer through this port.

@param[in] vidrend : handle to the renderer instance.
@param[out] port : handle to the allocated output port.

@retval ISMD_SUCCESS : The port handle is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The port handle pointer is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : The output port is not enabled.
*/
ismd_result_t ismd_vidrend_get_output_port(ismd_dev_t         vidrend,
                                           ismd_port_handle_t *port);

/**
RENDERER FUNCTIONS FOR TRICK MODE OPERATIONS
*/
/**
Gets the base time last used by the renderer.

@param[in] vidrend : handle to the renderer instance.
@param[out] basetime : basetime set in the renderer.

@retval ISMD_SUCCESS : The basetime is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The basetime pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_base_time(ismd_dev_t  vidrend,
                                         ismd_time_t *basetime);

/** 
This function is deprecated and should not be use. It is only here for backwards compatibility.
Please use the ismd_vidrend_mute API. 

Mutes the video on the connected video plane.
The renderer continues its operations but no new frames/fields are sent 
to the display.
If the flag is true, a black frame/field is displayed.
If not, the renderer sends new frames/fields to the display.
This operation is invalid if the renderer is not started.
 
@param[in] vidrend : handle to the renderer instance.
@param[in] flag : flag to indicate if video is to be muted or not.

@retval ISMD_SUCCESS : The operation is successfully completed.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The renderer handle is not running.
@retval ISMD_ERROR_OPERATION_FAILED : Internal operations failed.
*/
ismd_result_t ismd_vidrend_mute_video(ismd_dev_t vidrend,
                                      bool       flag);

/**
Sets the flush behavior of the renderer.
If the policy is "DISPLAY_BLACK", a black frame/field is displayed until 
flush completes and video data starts flowing through the renderer again.
If the policy is "REPEAT_FRAME", the last frame/field sent to the display is
maintained till new video data starts flowing through the renderer.
The default flush policy is REPEAT_FRAME.

@param[in] vidrend : handle to the renderer instance.
@param[in] flush_policy : the policy to set in the renderer.

@retval ISMD_SUCCESS : The flush behavior is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid flush policy.
*/
ismd_result_t ismd_vidrend_set_flush_policy(ismd_dev_t          vidrend, 
                                    ismd_vidrend_flush_policy_t flush_policy);

/**
Gets the flush policy for this renderer.

@param[in] vidrend : handle to the renderer instance.
@param[out] flush_policy : flush policy in instance.

@retval ISMD_SUCCESS : The policy is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The policy pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_flush_policy(ismd_dev_t         vidrend, 
                                   ismd_vidrend_flush_policy_t *flush_policy);

/**
Enables fixed frame rate mode. For interlaced content, renderer expects it to be
deinterlaced.

@param[in] vidrend : handle to the renderer instance.
@param[in] frame_pts_inc : frame PTS increment value in 90KHz clock ticks.

@retval ISMD_SUCCESS : Fixed frame rate is enabled successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid frame_pts_inc value.
*/
ismd_result_t ismd_vidrend_enable_fixed_frame_rate(ismd_dev_t vidrend, 
                                                   unsigned int frame_pts_inc);

/**
Disable fixed frame rate mode

@param[in] vidrend : handle to the renderer instance.

@retval ISMD_SUCCESS : Fixed frame rate is disabled.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/
ismd_result_t ismd_vidrend_disable_fixed_frame_rate(ismd_dev_t vidrend);

/**
Get the frame PTS increment value for this instance, when fixed frame rate 
mode is enabled.

@param[in] vidrend : handle to the renderer instance.
@param[out] frame_pts_inc : frame PTS increment value in 90KHz clock ticks.

@retval ISMD_SUCCESS : Frame PTS increment value is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : frame_pts_inc pointer is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : fixed frame rate is not enabled.
*/
ismd_result_t ismd_vidrend_get_fixed_frame_rate(ismd_dev_t   vidrend,
                                            unsigned int *frame_pts_inc);

/**
Get the stream position of the renderer. When fixed frame rate mode is enabled,
the scaled_time value in position is set to ISMD_NO_PTS.

@note The last_segment_pts returned in the stream position, has LSB truncated.

@param[in] vidrend : handle to the renderer instance.
@param[out] position : stream position details.

@retval ISMD_SUCCESS : The stream position is successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : info pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_stream_position(ismd_dev_t         vidrend, 
                              ismd_vidrend_stream_position_info_t *position);

/**
Sends frame/field with the given presentation timestamp to the display.
All undisplayed frames/fields in the renderer, whose linear_pts that are <=
(or > in the case the system is in reverse mode) the given pts, are dropped.

The linear_pts value is specified in units of 90 KHz clock ticks. 

@param[in] vidrend : handle to the renderer instance.
@param[in] linear_pts : rebased pts of the frame/field to be displayed.

@retval ISMD_SUCCESS : The pts to advance to is valid. 
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The renderer is not paused or not started.
@retval ISMD_ERROR_INVALID_PARAMETER : The linear_pts is not valid. A pts
is not valid if it is < than the current position for forward frame advance or
> than the current position for reverse. 

Note: This API is intended to skip between relatively small 
periods of time time. Skipping between long periods of time requires more
system configuration to be responsive. Example. Assume current stream position
is 10000. Skipping to 5*90K = 450000 clock ticks will result in a noticeable 
delay unless data is also dropped at the source. 
*/
ismd_result_t ismd_vidrend_advance_to_pts(ismd_dev_t vidrend, 
                                          ismd_pts_t linear_pts);

/**
Get the play rate set in the renderer.
Returns 10000 in case of normal playback.

@param[in] vidrend : handle to the renderer instance.
@param[out] rate : play rate set in the renderer normalized to 10000.

@retval ISMD_SUCCESS : The renderer is successfully flushed.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : rate pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_play_rate(ismd_dev_t vidrend, 
                                         int *rate);

/**
RENDERER FUNCTIONS TO CONTROL OPERATION
*/
/**
Enables the max hold time feature.
Max hold time is the maximum time that can elapse before a new frame is 
rendered (provided the frame exists in the renderer). It is a robustness
feature that allows video renderer to gracefully recover in the presence
of abnormally far apart presentation timestamps in the stream, without 
causing a video freeze. 
Max hold time is specified as a ratio (denominator over numerator) of 
the nominal frame rate and must be greater than 1. Example, if the 
frame rate is 60fps and the ratio is 2, then hold time is 1/30 sec.

By default, max hold time feature is disabled.

@param[in] vidrend : handle to the renderer instance.
@param[in] hold_time_num : numerator of the hold time ratio. 
@param[in] hold_time_den : denominator of the hold time ratio. 

@retval ISMD_SUCCESS : The hold time is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : If hold_time_num/hold_time_den is 0.
*/
ismd_result_t ismd_vidrend_enable_max_hold_time(ismd_dev_t vidrend, 
                                                unsigned int hold_time_num,
                                                unsigned int hold_time_den);

/**
Disables the max hold time feature in the renderer.

@param[in] vidrend : handle to the renderer instance.

@retval ISMD_SUCCESS : The hold time is disabled successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/
ismd_result_t ismd_vidrend_disable_max_hold_time(ismd_dev_t vidrend); 

/**
Gets the max hold time for this instance.

@param[in] vidrend : handle to the renderer instance.
@param[out] hold_time_num : numerator of the hold time ratio. 
@param[out] hold_time_den : denominator of the hold time ratio. 

@retval ISMD_SUCCESS : The hold time is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : One/more of the holdtime pointers is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : The hold time is not enabled.
*/
ismd_result_t ismd_vidrend_get_max_hold_time(ismd_dev_t vidrend, 
                                             unsigned int *hold_time_num,
                                             unsigned int *hold_time_den);

/**
RENDERER FUNCTIONS FOR FRAME/FIELD ATTRIBUTES
*/
/**
Gets attributes of the current frame/field.

@param[in] vidrend : handle to the renderer instance.
@param[out] frame_attr : pointer to frame/field attributes.
 
@retval ISMD_SUCCESS : The attributes are successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : frame_attr pointer is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : The renderer is not started.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : Current frame/field not available.
*/
ismd_result_t ismd_vidrend_get_video_attr(ismd_dev_t             vidrend, 
                                         ismd_frame_attributes_t *frame_attr);

/**
RENDERER FUNCTIONS FOR STATISTICS
*/
/**
Gets the current statistics.

@param[in] vidrend : handle to the renderer instance.
@param[out] stats : pointer to renderer statistics.
 
@retval ISMD_SUCCESS : The statistics are successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The stats pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_stats(ismd_dev_t           vidrend, 
                                     ismd_vidrend_stats_t *stats);

/**
Resets the statistics.

@param[in] vidrend : handle to the renderer instance.
 
@retval ISMD_SUCCESS : The statistics are successfully reset.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/
ismd_result_t ismd_vidrend_reset_stats(ismd_dev_t vidrend);

/**
RENDERER FUNCTIONS FOR CURRENT STATUS
*/
/**
Gets the current status.

@param[in] vidrend : handle to the renderer instance.
@param[out] status : pointer to renderer status.
 
@retval ISMD_SUCCESS : The status is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The status pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_status(ismd_dev_t           vidrend, 
                                     ismd_vidrend_status_t *status);

/**
RENDERER FUNCTIONS FOR EVENT NOTIFICATION
*/
/**
Gets the event that is set when vsync interrupts occur. The vsync type
requested could be set to frame, top field or bottom field (as defined in
the enum ismd_vidrend_vsync_type_t).

@param[in] vidrend : handle of the renderer instance.
@param[in] vsync_type: type of vsync interrupt to strobe on.
@param[out] on_vsync : vsync event set by the renderer

@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_NULL_POINTER : The on_vsync pointer is NULL.
@retval ISMD_ERROR_INVALID_HANDLE: The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER: The vsync_type parameter is invalid.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_vsync_event(ismd_dev_t   vidrend,
                                           ismd_vidrend_vsync_type_t vsync_type,
                                           ismd_event_t *on_vsync);

/**
Gets the event that is set when errors occur.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_error : error event set by the renderer  
 
@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_error pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_error_event(ismd_dev_t   vidrend, 
                                           ismd_event_t *on_error);

/**
Gets the event that is set when end of stream is reached.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_eos : eos event set by the renderer  
 
@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_eos pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_eos_event(ismd_dev_t   vidrend, 
                                         ismd_event_t *on_eos);

/**
Gets the event that is set when resolution change is detected.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_res_chg : resolution change event set by the renderer  
 
@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_res_chg pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_resolution_change_event(ismd_dev_t vidrend, 
                                                   ismd_event_t *on_res_chg);

/**
Gets the event that is set when client id tag is seen in the buffer.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_client_id_seen : client id seen event set by the renderer

@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_client_id_seen pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_client_id_seen_event(ismd_dev_t vidrend,
                                                   ismd_event_t *on_client_id_seen);

/**
Gets the event that is set when end of segment is reached.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_eoseg : end of segment event set by the renderer  
 
@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_eoseg pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_eoseg_event(ismd_dev_t   vidrend, 
                                         ismd_event_t *on_eoseg);

/**
Gets the event that is set when start of segment is read from the input port.

@param[in] vidrend : handle of the renderer instance.
@param[out] on_soseg : start of segment event set by the renderer  
 
@retval ISMD_SUCCESS : The event is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The on_soseg pointer is NULL.
@retval ISMD_ERROR_OPERATION_FAILED: Unable to allocate event.
*/
ismd_result_t ismd_vidrend_get_soseg_event(ismd_dev_t   vidrend, 
                                         ismd_event_t *on_soseg);

/**
Gets the drop frame count for this instance.

@param[in] vidrend : handle to the renderer instance.
@param[out] frame_count : drop frame count in instance

@retval ISMD_SUCCESS : The drop count was successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The frame count pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_drop_frame_count(ismd_dev_t vidrend,
                                                uint32_t   *frame_count);

/**
Sets the drop frame count - the number of frames to be dropped at startup before frames
are sent to the display.
The default drop_count is 0 and no frames are dropped.
The operation fails if the renderer is already started because frames cannot be
dropped after startup.

@param[in] vidrend : handle to the renderer instance.
@param[in] frame_count : determines dropped frame count

@retval ISMD_SUCCESS : The control is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The renderer is started.
*/
ismd_result_t ismd_vidrend_set_drop_frame_count(ismd_dev_t vidrend,
                                                uint32_t   frame_count);

/**
Disables PTS discontinuity handling based on the flag set.
The video renderer is capable of detecting PTS discontinuities in the stream
and modifies the PTS values to handle them. It keeps modifying PTS values
till it receives a New Segment tag from the demux or times out after quarter
of a second, after which it again starts using the PTS values coming in the frames.
By default PTS discontinuity handling is enabled.

@param[in] vidrend : handle to the renderer instance.
@param[in] disable_flag : flag to disable discontinuity handling.

@retval ISMD_SUCCESS : The flag is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/
ismd_result_t ismd_vidrend_disable_discontinuity_handling(ismd_dev_t vidrend,
                                                  bool       disable_flag);         


/**
Return the current displayed frame in video render. 

@param[in] vidrend : handle to the renderer instance.
@param[out] buffer : the current displayed frame.

@retval ISMD_SUCCESS : The current frame is successfully returned.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE: No frame is currently in display.
*/
ismd_result_t ismd_vidrend_get_current_frame(ismd_dev_t vidrend,
                                                  ismd_buffer_handle_t *buffer);


/**
Sets the behavior of the video renderer when it's stopped.
If the policy is "CLOSE_VIDEO_PLANE", video render will close the video plane which is used for
this stream.
If the policy is "KEEP_VIDEO_PLANE", video render will keep the video plane current status.
If the policy is "DISPLAY_BLACK", video render will flip a black screen to plane, rather 
than close the plane.
The default stop policy is CLOSE_VIDEO_PLANE.

@param[in] vidrend : handle to the renderer instance.
@param[in] stop_policy : the policy to set in the renderer.

@retval ISMD_SUCCESS : The stop behavior is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid flush policy.
*/
ismd_result_t ismd_vidrend_set_stop_policy(ismd_dev_t          vidrend, 
                                    ismd_vidrend_stop_policy_t stop_policy);


/**
Gets the stop policy for this renderer.

@param[in] vidrend : handle to the renderer instance.
@param[out] stop_policy : stop policy in instance.

@retval ISMD_SUCCESS : The policy is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The policy pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_stop_policy(ismd_dev_t         vidrend, 
                                   ismd_vidrend_stop_policy_t *stop_policy);

/**
Sets the pause behavior of the renderer.
If the policy is "NO_FLIP", frames will not be flipped to the plane
while in the paused state.
If the policy is "RE_FLIP", the last flipped frame will continue to be
re-flipped to the plane while in the paused state.  This will re-activate a
plane that was disabled.  GDL Plane configuration changes (position, etc...)
will take effect on the next vsync following gdl_plane_config_end() while the
video render is in the paused state.
The default pause policy is ISMD_VIDREND_PAUSE_POLICY_NO_FLIP
 
@param[in] vidrend : handle to the renderer instance.
@param[in] pause_policy : the policy to set in the renderer.

@retval ISMD_SUCCESS : The pause behavior is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : Invalid pause policy.
*/
ismd_result_t ismd_vidrend_set_pause_policy(ismd_dev_t          vidrend, 
                                    ismd_vidrend_pause_policy_t pause_policy);

/**
Gets the pause policy for this renderer.

@param[in] vidrend : handle to the renderer instance.
@param[out] pause_policy : pause policy in instance.

@retval ISMD_SUCCESS : The policy is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_NULL_POINTER : The policy pointer is NULL.
*/
ismd_result_t ismd_vidrend_get_pause_policy(ismd_dev_t         vidrend, 
                                   ismd_vidrend_pause_policy_t *pause_policy);


/** 
Configure a vidrend instance to scale and crop incoming frames if they do not
match the desired size. Can also, optionally, cause vidrend to override the
destination video window display position.

Note: 'ismd_vidrend_set_vidpproc' must be called before calling 
'ismd_vidrend_set_dest_params'.

@param[in] vidrend : handle to the renderer instance.
@param[in] image_size : size of the full image after scaling.
@param[in] portion_to_display :  sub-region of the scaled image to be displayed.
@param[in] override_window_position : optionally override the final video window position.
@param[in] window_pos_x : optionally override the final video window position.
@param[in] window_pos_y : optionally override the final video window position.

@retval ISMD_SUCCESS : The policy is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST: The vidpproc handle in render is invalid.
*/
ismd_result_t ismd_vidrend_set_dest_params(   ismd_dev_t vidrend,
                                   ismd_rect_size_t image_size,
                                   ismd_cropping_window_t portion_to_display,
                                   bool override_window_position,
                                   unsigned int window_pos_x,
                                   unsigned int window_pos_y);

/** 
Do not scale incoming frames. Used to disable/stop the effects of any previous
calls to 'ismd_vidrend_set_dest_params'.

@param[in] vidrend : handle to the renderer instance.

@retval ISMD_SUCCESS : The policy is obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
*/ 
ismd_result_t ismd_vidrend_disable_scaling(ismd_dev_t vidrend);


/** 
This function replaces the ismd_vidrend_mute_video() API.  

This function allows the client programmer to tell the vidrend to stop displaying frames, 
and to either display a black frame or hold the last displayed frame. 
When the user wants to re-enable the display, the application should call this function passing 
in the value of ISMD_VIDREND_MUTE_NONE into the mode parameter. 

If the mode is ISMD_VIDREND_MUTE_DISPLAY_BLACK_FRAME, a black frame/field is displayed.
If the mode is ISMD_VIDREND_MUTE_HOLD_LAST_FRAME, last frame is kept on the screen.
If the mode is ISMD_VIDREND_MUTE_NONE, the renderer sends new frames/fields to the display. 
This is the default value after vidrend is initialized. 

This operation is invalid if the renderer is not started or plane id is not set.
 
@param[in] vidrend : handle to the renderer instance.
@param[in] mode : the mode to freeze frame.

@retval ISMD_SUCCESS : The operation is successfully completed.
@retval ISMD_ERROR_INVALID_HANDLE : The renderer handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The renderer handle is not running.
@retval ISMD_ERROR_OPERATION_FAILED : Internal operations failed.
*/
ismd_result_t ismd_vidrend_mute(   ismd_dev_t vidrend,
                                   ismd_vidrend_mute_policy_t mode);

/**
Get the message context id of the video render instance.

@param[in] vidrend : Handle for the video render instance.
@param[out] message_context: used to return message context id. 

@retval ISMD_SUCCESS : The message context was successfully obtained.
@retval ISMD_ERROR_INVALID_HANDLE : Invalid processor or input handle.
*/
ismd_result_t ismd_vidrend_get_message_context(ismd_dev_t          vidrend, 
                                    ismd_message_context_t *message_context);


/**@}*/ // end of ingroup ismd_vidrend
/**@}*/ // end of weakgroup ismd_vidrend

#ifdef __cplusplus
}
#endif

#endif /* __ISMD_VIDREND_H__ */
