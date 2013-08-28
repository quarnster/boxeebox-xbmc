/*
* File Name: ismd_viddec.h
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
/*
*  Authors:  Video Decoder Software Team
*/

/** @weakgroup ismd_viddec Video Decoder

<H3>Overview</H3>

The SMD video decoder module provides hardware accelerated decode of 
video elementary streams.  The following standards are supported:

- MPEG-1/MPEG-2 (MP\@ML - MP\@HL)<br>
- H.264 (supports High Profile up to Level 4.1)<br>
- VC-1  (Simple, Main, and Advanced Profile up to Level 3)<br>
- MPEG4 Part-2 (SP\@L0-L6 and ASP\@L0-L5)<br>
 
Video decode capabilities are exposed through a stream interface. For a given
stream, compressed video data of a certain format is passed into the video 
decoder as input. Decompressed video frames (in NV12 format) are emitted as 
output. Up to two instances of the decoder may be opened at once, and streams
are completely independent entities. The actions/operations on one stream
in the system will not affect other streams that are running concurrently. 

<H3>Input and Output Interfaces</H3>

The SMD viddec module interfaces are standard \ref smd_core "SMD Ports" where 
buffers may be read/written by the client or which may be connected to upstream
or downstream modules.

The SMD viddec module contains three port interfaces:

1) An input port that receives buffers containing ES data.<br>
2) An output port that emits frame buffers that contain images for display.<br>
3) A user data port that extracts user data from the bitstream and sends it
   to the application for processing.<br> 

The port interfaces are per stream entities. That is, each stream will have its
own input, output, and user data ports. 

It should be noted that passing a NULL pointer or an invalid pointer (that 
points to invalid address space) is an application error. The behavior of the 
decoder in this case is to crash. Here, the kernel dump can be used to figure 
out which line of the code received the invalid pointer. The decoder shall not
return NULL pointer error message.

<H3>Events</H3>

In addition to data channels, the SMD viddec module provides applications with 
notification of decoder events asynchronously. The application can be notified 
when:

1) The decoder is out of memory.<br>
2) The stream's resolution has changed.<br>
3) An end of stream condition has been detected.<br>
4) There was a bitstream error detected.<br>
5) The decoder dropped a frame.<br> 
6) Client id tags were processed by the decoder.<br>

<H3>Output Scaling</H3>

In addition to video decode, the SMD viddec module is capable of horizontally 
downscaling decoded frames by 1/2. 

<H3>Theory of Operation</H3>

I. Setup/Intialization

-# Open a new stream using \ref ismd_viddec_open. Specify the type of video
stream you'd like to decode.<br>  
-# Query the decoder for the input/output ports using \ref ismd_viddec_get_input_port and \ref ismd_viddec_get_output_port.<br> 
-# Query the decoder for the EOS event using \ref ismd_viddec_get_event with the ISMD_VIDDEC_EOS field.<br> 

II. Playback

-# Write compressed video data into the decoder using \ref ismd_port_write.<br>
-# Read decoded frames via \ref ismd_port_read.<br> 
-# Wait for EOS event using \ref ismd_event_wait.<br>

III. Shutdown

-# Shutdown stream using \ref ismd_dev_close.<br>

*/
#ifndef __ISMD_VIDDEC_H__
#define __ISMD_VIDDEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ismd_global_defs.h"

/** @ingroup ismd_viddec */
/** @{ */

/** Number of concurrent streams allowed. 
 *  
 *
 *  For dual stream, any combination of codecs is possible.  For multistream, 
 *  the application may not use more than 2 H.264 or 2 VC-1 codecs.
 *
 */ 
#define ISMD_VIDDEC_MAX_STREAMS   3   /** dual and multi stream decode of different \
                                          format types */
                                          
/** A special value to denote that all frames in a sequence are to be decoded/displayed.
 *
 *  The assumption is that the number of frames in a sequence/GOP will never exceed this value.
 *
 */
#define ISMD_VIDDEC_ALL_FRAMES 0xFFFFFFFF

/**
Determines the frame types that can be programmatically skipped by the decoder.
P and/or B frames can be skipped for MPEG-2, VC1 and MPEG4 Part-2 and non-reference 
frames can be skipped for H.264.
By default, no frames are skipped.
*/
typedef enum {
   ISMD_VIDDEC_SKIP_NONE               = 0 << 0, /**< Default-No skipped frames*/
   ISMD_VIDDEC_SKIP_FRAME_TYPE_P       = 1 << 0, /**< Skips P frames only */
   ISMD_VIDDEC_SKIP_FRAME_TYPE_B       = 1 << 1, /**< Skips B frames only */
   ISMD_VIDDEC_SKIP_FRAME_TYPE_NON_REF = 1 << 2, /**< Skips all non-reference
                                                 frames */
} ismd_viddec_frame_skip_policy_t;

/**
Determines how frames will be output by the decoder.
By default, frames will be output in display order.
*/
typedef enum {
   ISMD_VIDDEC_DISPLAY_ORDER          = 0x0, /**< Default - Frames will be
                                             emitted in display order */
   ISMD_VIDDEC_DECODE_ORDER           = 0x1, /**< Frames will be emitted in
                                             decode order */
   ISMD_VIDDEC_FIRST_IN_GROUP         = 0x2, /**< First frame from the group
                                             will be emitted */
   ISMD_VIDDEC_LAST_IN_GROUP          = 0x3, /**< Last frame from the group will
                                             be emitted */
   ISMD_VIDDEC_REVERSE_IN_GROUP_ORDER = 0x4, /**< Frames from group will be
                                             emitted in reverse order */
} ismd_viddec_frame_output_policy_t;

/**
Determines how the decoder deals with frames that have errors in them.
By default, only error frames that have errors concealed will be emitted.

These values may be OR'ed together to have the decoder output different
policies together.

Examples:

To emit frames that have macroblock errors concealed and have 
dangling fields, pass in:

ISMD_VIDDEC_EMIT_ERROR_CONCEAL_FRAMES | ISMD_VIDDEC_EMIT_DANGLING_FIELDS

To emit all error frames:

ISMD_VIDDEC_EMIT_ALL                  

To prevent the decoder from generating any error frames:

ISMD_VIDDEC_EMIT_NO_ERROR_FRAMES      
*/
typedef enum {
   ISMD_VIDDEC_EMIT_NO_ERROR_FRAMES      = 0, /**< Drop all frames with missing 
                                                   references, dangling fields 
                                                   and macroblock errors. */
   ISMD_VIDDEC_EMIT_ALL                  = 0x7, /**< Output all frames, 
                                                     including those with 
                                                     errors. Disabled
                                                     by default. */
   ISMD_VIDDEC_EMIT_FRAMES_MISSING_REF   = 1 << 0, /**< Output frames with 
                                                       missing references. 
                                                       Disabled by default. */
   ISMD_VIDDEC_EMIT_DANGLING_FIELDS      = 1 << 1, /**< Output frames with dangling
                                                       fields. Disabled by 
                                                       default. */
   ISMD_VIDDEC_EMIT_ERROR_CONCEAL_FRAMES = 1 << 2, /**< Output frames where 
                                                       macroblock errors were 
                                                       concealed. Enabled by
                                                       default. */
} ismd_viddec_frame_error_policy_t;

/**
Polices for interpolating PTS values.
By default, missing presentation timestamps are interpolated.
*/
typedef enum {
   ISMD_VIDDEC_INTERPOLATE_MISSING_PTS = 0x0, /**< Default - Interpolate between
                                             PTS values in stream */
   ISMD_VIDDEC_NO_PTS_INTERPOLATION    = 0x1, /**< Do not interpolate PTS values.
                                              */
   ISMD_VIDDEC_INTERPOLATE_ALL_PTS     = 0x2  /**< Ignore in-stream PTS values,
                                             interpolate based on frame rate
                                             specified in-stream or by user. */
} ismd_viddec_pts_interpolation_policy_t;

/**
Supported output scale values for the video decoder.
By default, no scaling is performed on the output image.
 */
typedef enum {
   ISMD_VIDDEC_SCALE_NONE    = 0x0,      /**< Default - Do not scale down output
                                         image */
   ISMD_VIDDEC_HALF_HORIZONTAL = 0x1,    /**< Scale output image by 1/2
                                         (horizontal only) */
   ISMD_VIDDEC_QUARTER_HORIZONTAL = 0x2, /**< Scale output image by 1/4
                                         (horizontal only) */
} ismd_viddec_output_scale_factor_t;

/**
Events that the decoder can generate.
*/
typedef enum {
   ISMD_VIDDEC_OUT_OF_MEMORY     = 0x0, /**< Buffer allocation failures */
   ISMD_VIDDEC_RESOLUTION_CHANGE = 0x1, /**< Resolution, aspect ratio, or 
                                             frame rate change */
   ISMD_VIDDEC_EOS               = 0x2, /**< End of stream detected */
   ISMD_VIDDEC_BITSTREAM_ERROR   = 0x3, /**< Bitstream error detected */
   ISMD_VIDDEC_DROPPED_FRAMES    = 0x4, /**< Error frames dropped */
   ISMD_VIDDEC_CLIENT_ID_SEEN    = 0x5  /**< Client id seen */
} ismd_viddec_event_t;

/**
Message generated by decoder.
*/
typedef enum {
   ISMD_VIDDEC_MESSAGE_MACRO_BLOCK		= 0x0, /**< Macro block detected */
   ISMD_VIDDEC_MESSAGE_FRAME_ERROR		= 0x1, /**< Frame error detected */
   ISMD_VIDDEC_MESSAGE_DECODER_ERROR   = 0x2, /**< Decoder error detected */
} ismd_viddec_message_t;

/**
This enum defines how the decoder should treat out of segment frames 
during forward playback.
*/
typedef enum {
   ISMD_VIDDEC_DO_NOT_DROP_OUTOFSEGMENT_FRAMES	= 0x0, /**< Default - do not drop out of segment frames */
   ISMD_VIDDEC_DROP_OUTOFSEGMENT_FRAMES		   = 0x1, /**< Drop out of segment frames during forward playback */
} ismd_viddec_out_of_segment_drop_policy_t;


/**
This enum defines how the MPEG-2 decoder should interpret content that does not
explicitly specify sequence display extensions
*/
typedef enum {
   ISMD_VIDDEC_ISO13818_2	= 0x0, /**< Use default MPEG-2 settings for sequence 
                                       display extensions */
   ISMD_VIDDEC_DVD		= 0x1,    /**< Use DVD default settings for sequence 
					                        display extensions */
} ismd_viddec_seq_disp_ext_default_policy_t;
/**
Stream properties extracted from the bitstream.
*/
typedef struct {
   ismd_codec_type_t  codec_type; /**< Type of stream (MPEG-2, MPEG4-2, VC1, or H.264 */
   unsigned long nom_bit_rate;    /**< Nominal bitrate of stream in Kbps */
   unsigned long frame_rate_num;  /**< Numerator of stream frame rate */
   unsigned long frame_rate_den;  /**< Denominator of stream frame rate */
   unsigned long coded_height;    /**< Display height of frame */
   unsigned long coded_width;     /**< Display width of frame */
   unsigned long display_height;  /**< Intended display's active region height */
   unsigned long display_width;   /**< Intended display's active region width */
   unsigned long profile;         /**< Profile  */
   unsigned long level;           /**< Level */
   bool is_stream_interlaced;     /**< Interlaced or progressive */
   ismd_aspect_ratio_t sample_aspect_ratio; /**< Sample aspect ratio - If not known, will be set to
                                             0/0 and square pixels will be used. */
} ismd_viddec_stream_properties_t;

/**
Statistics collected from the decode process.
NOTE: The statistics "current_bit_rate" value is currently not populated and does
not report a meaningful value. 
*/
typedef struct {
   unsigned long current_bit_rate; /**< Instantaneous bitrate of stream in Kbps*/
   unsigned long long total_bytes; /**< Total raw bytes received at the input */
   unsigned long frames_decoded;   /**< Total number of frame decoded */
   unsigned long frames_dropped;   /**< Total number of frame dropped */
   unsigned long error_frames;     /**< Total number of frame with errors*/

   unsigned long invalid_bitstream_syntax_errors;  /**< Total number of ES syntax errors*/
   unsigned long unsupported_profile_errors;
   unsigned long unsupported_level_errors;
   unsigned long unsupported_feature_errors;
   unsigned long unsupported_resolution_errors;
} ismd_viddec_stream_statistics_t;

/******************************************************************************/
/*!    @name Video decoder device initialization and teardown APIs */

/**
Opens an available video decoder device.
All resources required for normal operation are allocated and the default
decoder capabilites are enabled. This operation must succeed for subsequent
decoder operations to be successful.

The video decoder instance is returned in the viddec handle. Multiple decoder
instances can be created by calling this function multiple times.

The video decoder device is destroyed using the standard Intel SMD close
function.

   @param [in] initial_codec_type: Initial codec format. Supported formats are
                           H.264,VC1,MPEG-2 and MPEG4-2. Codec format may 
                           change if new codec format is detected on input port.
                           
   @param[out] viddec: Handle to the video decoder. Used in subsequent
                           calls to refer to a specific stream.

   @retval ISMD_SUCCESS: Video decoder handle is created successfully.
   @retval ISMD_ERROR_NO_RESOURCES: No decoder instance is available.
   @retval ISMD_ERROR_OPERATION_FAILED: Video decoder creation failed due to
                                    lack of system resources.
 */
ismd_result_t ismd_viddec_open(ismd_codec_type_t initial_codec_type,
                               ismd_dev_t *viddec);

/**
Resets the video decoder to its initial state (i.e., open)
This feature is currently not implemented.

   @param[in] viddec: Handle to the video decoder

   @retval ISMD_SUCCESS: Reset successful
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_reset(ismd_dev_t viddec);

/******************************************************************************/
/*!    @name Video decoder port interface */
/**
Gets the handle to video decoder input port.
Compressed video data is written to the decoder through this port.

Input data is encapsulated in an ismd buffer (ismd_buffer_handle_t). Timing
related attributes should be specified in the attributes of the buffer. If 
no timing information is present, these attributes should be initialized, 
otherwise the decoder will pass through unitialized date. 

Example: Assume and ES buffer with no PTS information. The buffer attributes 
should be specified as:

   // read the buffer descriptor
   ismd_buffer_read_desc(h, &d);

   // get the es attributes
   ismd_es_buf_attr_t *es_attrs = NULL;
   es_attrs = (ismd_es_buf_attr_t *)d.attributes;

   // Indicate that the buffer doesn't have a PTS value or discontinuity
   // information
   es_attrs->local_pts = ISMD_NO_PTS;
   es_attrs->original_pts = ISMD_NO_PTS;
   es_attrs->discontinuity = 0;

   ismd_buffer_update_desc(h, &d);

Note: Error checking has been removed for readability. 

   @param[in] viddec: Handle to the video decoder
   @param[out] port : Input port handle

   @retval ISMD_SUCCESS : Stream input port is successfully returned.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_input_port(ismd_dev_t viddec,
                                         ismd_port_handle_t *port);

/**
Gets the handle to video decoder output port.
The video decoder writes decoded frames to this port.

   @param[in] viddec: Handle to the video decoder.
   @param[out] port : Output port handle.

   @retval ISMD_SUCCESS : Stream output port is successfully returned.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_output_port(ismd_dev_t viddec,
                                          ismd_port_handle_t *port);

/**
Gets the handle to user data output port.
The video decoder writes user data to this port.

The different video codec formats allow user data to be encoded in various 
levels of the bitstream hierarchy. For example, in MPEG-2, user data may
be found at the sequence, GOP, or picture layer. The video decoder supports
extracting and providing user data at all points in the bitstream hierarchy. 

For user data elements found at the picture level, the presentation timestamp
of the corresponding frame is stored with the user data. This allows the client
programmer to render information related to the user data at the appropriate
time.  

Depending on which level in the hierarchy the user data is found, the 
following restrictions apply:

1) For any user data found above the frame/picture layer, all data is 
extracted and sent out this port. Data will be sent in 1024 byte packets.
As an example, if the user data is 4K in size, the decoder will send four
1K buffers.  

2) For user data found at the picture/frame layer, the following restrictions
apply:

a) User data is limited to the first 1024 bytes of data. 
b) Multiple user data elements may be encoded in the bitstream. Only the 
first 8 user data elements for the picture will be extracted and sent.  

The user data port provides ample buffering to allow applications to 
service the port on a non real-time basis. If however, the application is 
not servicing the port fast enough, the decoder will start dropping user
data rather than stall the decode process.  

Important note:

*) User data buffers are allocated by the decoder and sent to the user through
the the port handle returned by this function. These buffers must be freed
by the application once the application is done processing them. If the 
application does not free these buffers properly, the system will leak memory. 

   @param[in] viddec: Handle to the video decoder.
   @param[out] port : User data port handle.

   @retval ISMD_SUCCESS : Stream user data port is successfully returned.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t  ismd_viddec_get_user_data_port(ismd_dev_t viddec,
                                             ismd_port_handle_t *port);

/******************************************************************************/
/*!    @name Video decoder control interface */

/**
 *  start, stop, and flush operations use standard Intel SMD interface.
 */

/**
Sets the output order of frames as decribed by enum
ismd_viddec_frame_output_policy_t.
Frames will be emitted at the output port as per output frame policy settings.

   @param[in] viddec: Handle to the video decoder
   @param[in] policy: frame output policy

   @retval ISMD_SUCCESS : Successfully set the output frame policy
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_set_frame_output_policy(ismd_dev_t viddec,
                                    ismd_viddec_frame_output_policy_t policy);

/**
Gets the frame output policy set in the decoder.

   @param[in] viddec: Handle to the video decoder
   @param[out] policy: frame output policy

   @retval ISMD_SUCCESS : Successfully returned the output frame policy
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_frame_output_policy(ismd_dev_t viddec,
                                    ismd_viddec_frame_output_policy_t *policy);

/**
Set the frame rate for the stream. 

By default, the viddec module will use the frame rate encoded in the 
elementary stream, if one is available. If no frame rate is specified,
the decoder will default to 30 fps.

This method can be used to override the default setting for streams that
don't have frame rate information. In addition, the user may use this
function to override the frame rate specified in the bitstream. 

   @param[in] viddec: Handle to the video decoder
   @param[in] num: numerator for frame rate
   @param[in] den: denominator for frame rate 
   @param[in] override: if true, override stream frame rate (ignore the 
                        value specified by the bitstream)
                        
                        if false, use frame rate specified in stream if 
                        present
   
   Note: If the override flag is false, the frame rate supplied to this routine
         will be used if the frame rate is not specified in the bitstream.  

   @retval ISMD_SUCCESS : Interpolation parameters are set correctly
   @retval ISMD_ERROR_INVALID_PARAMETER : The denominator specified is 0. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_set_frame_rate(ismd_dev_t viddec,
                                 unsigned long num,
                                 unsigned long den, 
                                 bool override);

/**
This function gets the current frame rate

The default value is 30 fps.

This value will either be the frame rate found in the bitstream, or the 
user specified value if override == true. 

   @param[in] viddec: Handle to the video decoder
   @param[in] num: numerator for frame rate is returned here
   @param[in] den: denominator for frame rate is returned here
   @param[in] override: if true, stream frame rate has been overridden
   
   @retval ISMD_SUCCESS : Interpolation parameters are set correctly
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_frame_rate(ismd_dev_t viddec,
                                 unsigned long *num,
                                 unsigned long *den, 
                                 bool *override);

/**
Set the policy for interpolating PTSs of frames.
PTS values are only interpolated for local PTS values, original PTS values in
the stream are not changed.
When the policy is INTERPOLATE_ALL_PTS, an initial_pts
may be specified. This value is used as the starting value for the
interpolation. If this value is not specified, the default value of ISMD_NO_PTS 
will be used for the initial PTS value. This indicates that the decoder should
interpolate from the start time value seen in the new segment tag that comes
in at the start of the stream. 


   @param[in] viddec: Handle to the video decoder
   @param[in] policy: pts interpolation policy
   @param[in] initial_pts: base pts to be used, if applicable based on
		     interpolation policy. If ISMD_NO_PTS is specified, 
                     the initial value will be the start time populated
                     in incoming new segment tags. If no new segment is
                     received, the interpolation will start at 0.  

   @retval ISMD_SUCCESS : Interpolation parameters are set correctly
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The policy/initial_pts value is 
                                          invalid. 
*/
ismd_result_t ismd_viddec_set_pts_interpolation_policy(ismd_dev_t viddec,
                                 ismd_viddec_pts_interpolation_policy_t policy,
                                 ismd_pts_t initial_pts);

/**
Get the policy for interpolating PTSs of frames and associated details.

   @param[in] viddec: Handle to the video decoder
   @param[out] policy: pts interpolation policy
   @param[out] initial_pts: initial_pts used for interpolation - if applicable 
		            based on interpolation policy.	

   Note: The value returned in the initial_pts parameter is is the value 
         specified by the user in set_pts_interpolation_policy, and not the
         value of the first pts found in the bitstream. If the user has not
         called set_pts_interpolation_policy(), then the default value used
         for the initial PTS is returned (ISMD_NO_PTS). 

   @retval ISMD_SUCCESS : Interpolation parameters are set correctly
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_pts_interpolation_policy(ismd_dev_t viddec,
                                 ismd_viddec_pts_interpolation_policy_t *policy,
                                 ismd_pts_t *initial_pts);

/**
Set the error handling policy for decoded frames. 

Multiple types may be OR'ed together to have the decoder output the 
specific frames needed. 

Examples:

To emit frames that have macroblock errors concealed and frames with 
dangling fields, set policy to:

policy = ISMD_VIDDEC_EMIT_ERROR_CONCEAL_FRAMES | ISMD_VIDDEC_EMIT_DANGLING_FIELDS

To emit all error frames:

policy = ISMD_VIDDEC_EMIT_ALL                  

To prevent the decoder from generating any error frames:

policy = ISMD_VIDDEC_EMIT_NO_ERROR_FRAMES      

   @param[in] viddec: Handle to the video decoder
   @param[in] policy: Frame error handling policy

   @retval ISMD_SUCCESS : Successfully set the output frame policy
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_set_frame_error_policy(ismd_dev_t viddec,
                                       ismd_viddec_frame_error_policy_t policy);

/**
Gets the error handling policy for decoded frames. 

   @param[in] viddec: Handle to the video decoder
   @param[out] policy: Frame error handling policy

   @retval ISMD_SUCCESS : Successfully set the output frame policy
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_frame_error_policy(ismd_dev_t viddec,
                                      ismd_viddec_frame_error_policy_t *policy);

/**
Skips the decoding of specified frame types.
The mask must be a valid ORed combination of the enum
ismd_viddec_frame_skip_policy_t.
The default mask is ISMD_VIDDEC_SKIP_NONE.

Only the lower 3 bits of the mask are considered.
For MPEG2 and VC1, P frames can be skipped only when B frames
are skipped as well.
If ISMD_VIDDEC_SKIP_FRAME_TYPE_NON_REF is set, all other skip
policies are ignored and B frames are skipped.

   @param[in] viddec: Handle to the video decoder
   @param[in] mask: Frame to skips (B, P or Non refererence)

   @retval ISMD_SUCCESS : Frame mask is set correctly
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The mask is invalid (only p frames skipped).
*/
ismd_result_t ismd_viddec_set_frame_mask(ismd_dev_t viddec,
                                          unsigned long mask);

/**
Gets the skipped frame policy configured in the decoder.

   @param[in] viddec: Handle to the video decoder
   @param[out] mask: Frame to skips (B, P or Non refererence)

   @retval ISMD_SUCCESS : Frame mask is set correctly
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_frame_mask(ismd_dev_t viddec,
                                         unsigned long *mask);
/**
Enables the scaling of the video data.
Only horizontal video downscaling by 2 and 4 are supported.

NOTE: This API cannot be called asynchronously. It must be called either at
stream startup, prior to feeding any input data, or after the decoder is paused
and flushed. The decoder may not output valid frame data if this API is called 
during steady state stream playback. 

   @param[in] viddec: Handle to the video decoder
   @param[in] scale_val: Scale value

   @retval ISMD_SUCCESS: Output scaling successfully enabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_enable_output_scaling(ismd_dev_t viddec,
                                 ismd_viddec_output_scale_factor_t scale_val);

/**
Disable output scaling.

NOTE: This API cannot be called asynchronously. It must be called either at
stream startup, prior to feeding any input data, or after the decoder is paused
and flushed. The decoder may not output valid frame data if this API is called 
during steady state stream playback. 

   @param[in] viddec: Handle to the video decoder

   @retval ISMD_SUCCESS: Successfully disabled output scaling.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_disable_output_scaling(ismd_dev_t viddec);

/**
Gets the output scale value set in the decoder.

   @param[in] viddec: Handle to the video decoder
   @param[out] scale_val: Scale value

   @retval ISMD_SUCCESS: Output scaling successfully enabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_get_output_scaling(ismd_dev_t viddec,
                                 ismd_viddec_output_scale_factor_t *scale_val);

/**
Change the codec format. The change takes place immediately, and is not
synchronized to any particular input buffer.  

This method is meant to be used after a call to flush, prior to passing in data 
from the new stream:

1) Flush pipeline
2) Set new codec format
3) Start feeding new data 

This feature is currently not implemented.

   @param[in] viddec: Handle to the video decoder
   @param[in] codec: New codec format. Supported formats are
                     H.264,VC1, MPEG4-2 and MPEG-2.  

   @retval ISMD_SUCCESS : The new codec format was successfully set. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_set_codec(ismd_dev_t viddec, ismd_codec_type_t codec);
                                    
/**
Query the decoder for the current codec format. 

This feature is currently not implemented.

   @param[in] viddec: Handle to the video decoder
   @param[in] codec: Current codec format. Supported formats are
                     H.264, VC1, MPEG4-2 and MPEG-2.  

   @retval ISMD_SUCCESS : The new codec format was successfully set. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_codec(ismd_dev_t viddec, ismd_codec_type_t *codec);

/**
Get the message context id of the current decoder

   @param[in] viddec: Handle to the video decoder
   @param[out] message_context: Used to return message context

   @retval ISMD_SUCCESS : Get context successfuly. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_message_context(ismd_dev_t viddec, uint32_t *message_context);

/*!    @name Video decoder stream properties APIs */
/**
Returns properties of stream associated with decoder instance.

   @param[in] viddec: Handle to the video decoder
   @param[out] prop:  Stream properties

   @retval ISMD_SUCCESS: Successfully read stream characterstics
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_get_stream_properties(ismd_dev_t viddec,
                                       ismd_viddec_stream_properties_t *prop);

/**
Get stream statistics of the decode process.

NOTE: The statistics "current_bit_rate" is currently not implemented.

   @param[in] viddec: Handle to the video decoder
   @param[out] stat: Receive the stream statistics

   @retval ISMD_SUCCESS: Successfully read stream statistics.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_get_stream_statistics(ismd_dev_t viddec,
                                         ismd_viddec_stream_statistics_t *stat);


/*!    @name Video decoder asynchronous notification APIs */
/**
Gets the event associated with the given event condition.

   @param[in] viddec: Handle to the video decoder.
   @param[in] event_type: Requests the specific event associated with this
                     type.
   @param[out] event: Event that is signalled when the given condition occurs.

   @retval ISMD_SUCCESS: Requested event is returned successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The event type is invalid.
 */
ismd_result_t ismd_viddec_get_event(ismd_dev_t viddec,
                                    ismd_viddec_event_t event_type,
                                    ismd_event_t *event);


/**
Sets the maximum number of frames to decode per sequence. 
If the value is ISMD_VIDDEC_ALL_FRAMES, all frames in the sequence are decoded. 

A video sequence is defined as a portion of a bitstream in forward order that 
is free from stream discontinuities caused by the source element skipping
portions of the input stream. 

A video sequence ends when the decoder is flushed, or when a buffer is sent
to the decoder with the discontinuity flag set. 

Note: This function sets the number of frames in the sequence to decode. It 
will not decode any more frames in the sequence after it has reached the
specified limit. It will only resume decoding data once the previous sequence
is terminated using the discontinuity flag or by flushing the decoder.

If the user wishes to have the decoder decode N frames from a particular point
in a bitstream (e.g., the start of a GOP), it should feed data into the decoder
in a suitable fashion. E.g., 

*) Call ismd_viddec_set_max_frames_to_decode
*) Send first sequence (first GOP)
*) For the second sequence (second GOP), set the discontinuity flag on the first 
buffer of the second sequence to indicate to the decoder that a new sequence has
arrived. The decoder will resume decoding at this point. 

Examples:

All examples assume the following declarations:

ismd_result_t result = ISMD_SUCCESS;
ismd_dev_t viddec;

1) To decode all frames in a sequence:

result = ismd_viddec_set_max_frames_to_decode(viddec, 
                                              ISMD_VIDDEC_ALL_FRAMES);

2) To decode the first 5 frames in a sequence:

result = ismd_viddec_set_max_frames_to_decode(viddec, 
                                              5);

Note: During reverse playback, if the max_frames_to_decode is set as N 
      and the Nth frame in the GOP happens to be a reference frame, followed
      by non-reference frame, then this Nth reference frame will be dropped.

   @param[in] viddec: Handle to the video decoder
   @param[in] num_frames: Max number of frames to decode.

   @retval ISMD_SUCCESS : The value was successfully set. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : num_frames is 0.
*/
ismd_result_t ismd_viddec_set_max_frames_to_decode(ismd_dev_t viddec, 
                                                   unsigned int num_frames);

/**
Gets the maximum number of frames to decode per sequence as programmed in the 
decoder.

   @param[in] viddec: Handle to the video decoder.
   @param[out] num_frames: maximum frames to decoder per sequence.

   @retval ISMD_SUCCESS: Requested flag is returned successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_max_frames_to_decode(ismd_dev_t viddec,
                                                   unsigned int *num_frames);
/**
Sets the maximum number of ready to display frames to wait for an I frame.  This
value controls the quality of playback for content without I frames/slices.  
Higher values will decrease the chance of seeing missing reference macroblocking
but will increase channel change times for content 

@note The default value can be controled at runtime using the platform_config.hcfg 
      by setting default_max_display_frames_to_wait_for_I_frame in 
      the platform.software.drivers.smd.viddec section.

@note The value of max_display_frames_to_wait_for_I_frame should not be less than
      three to allow the state machine to work.

   @param[in] viddec: Handle to the video decoder
   @param[in] num_frames: maximum # of ready for display frames to wait for an I frame.

   @retval ISMD_SUCCESS : The value was successfully set. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : num_frames is less than 3.
*/
ismd_result_t ismd_viddec_set_max_display_frames_to_wait_for_I_frame(ismd_dev_t viddec, 
                                                                     unsigned int num_frames);

/**
Gets the maximum number of ready to display frames to wait for an I frame.  This
value controls the quality of playback for content without I frames/slices.

   @param[in] viddec: Handle to the video decoder.
   @param[out] num_frames: maximum # of ready for display frames to wait for an I frame.

   @retval ISMD_SUCCESS: Requested flag is returned successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_max_display_frames_to_wait_for_I_frame(ismd_dev_t viddec,
                                                                     unsigned int *num_frames);
/**
This API allows the user to tell the decoder what default values
should be used when the sequence display extensions are not specified in the
bitstream. Current default settings include ISO13818, which specifies the 
default display is HD, and DVD, which specifies the default settings should
be SD. The default settings are applied per the respective specifications. 

This API is only valid for MPEG-2 content. It will have no effect for other
codec types.  

   @param[in] viddec: Handle to the video decoder.
   @param[in] policy: The policy that the application wants the video
                      decoder to use when sequence display extensions are not
                      present in the bitsream.  

   @retval ISMD_SUCCESS: Policy set succesfuly. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : Specified policy is not valid.
   @retval ISMD_ERROR_INVALID_REQUEST : Setting this is not valid for current
                                        codec type (this is only valid for MPEG-2).
*/
ismd_result_t ismd_viddec_set_seq_disp_ext_default_policy(ismd_dev_t viddec,
                                              ismd_viddec_seq_disp_ext_default_policy_t policy);

/**
Returns the policy that is currently being used to set the display 
extensions when they are not present in the bitstream. 

   @param[in] viddec: Handle to the video decoder.
   @param[out] policy: Returns the current policy for display extensions.

   @retval ISMD_SUCCESS: Policy set succesfuly. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_REQUEST : Setting this is not valid for current
                                        codec type (this is only valid for MPEG-2).
*/
ismd_result_t ismd_viddec_get_seq_disp_ext_default_policy(ismd_dev_t viddec,
                                              ismd_viddec_seq_disp_ext_default_policy_t *policy);


/**
In this mode the decoder will ignore the frame polarity encoded in the bitstream and 
mark the polarity of all output frames as interlaced. This will be done ONLY if 
following conditions are met:
   1. The sequence is interlaced
   2. The sequence is SD. For HD contents, this mode is inactive.
   3. Frame rate is <= 30 fps.

This special mode has been added in the decoder to handle the error case where the 
video is captured as interlaced fields, but the encoder generated all frames as 
progressive. Consequently the post-processor doesn't perform deinterlacing for these
frames. This results in severe deinterlacing/combing artifact. To solve this, the 
decoder provides this mode to the application. 

   @param[in] viddec: Handle to the video decoder.

   @retval ISMD_SUCCESS: Decoder configured in polarity override mode.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_enable_override_frame_polarity(ismd_dev_t viddec);


/**
Disable polarity override mode.

   @param[in] viddec: Handle to the video decoder.

   @retval ISMD_SUCCESS: Polarity override mode disabled successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_disable_override_frame_polarity(ismd_dev_t viddec);


/**
Sends an EOF flag to decoder to cause the currently decoded frame to be evicted.

   @param[in] viddec: Handle to the video decoder.

   @retval ISMD_SUCCESS: EOF flag sent.
*/
ismd_result_t ismd_viddec_evict_frame(ismd_dev_t viddec);

/**
Gets boolean value of the flag that enables the feature of sending an END_OF_FRAME
if an empty buffer with a client id tag is seen on the input of the viddec driver.

   @param[in] viddec: Handle to the video decoder.
   @param[out] enabled: bool value true if enabled, else false

   @retval ISMD_SUCCESS: Requested flag is returned successfully.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_id_tag_empty_buf_send_end_of_frame(ismd_dev_t viddec,
                                                                 bool *enabled);
/**
Sets the flag that enables the feature of sending an END_OF_FRAME if an empty 
buffer with a client id tag is seen on the input of the viddec driver.  

@note The enabling this mode can cause macroblocking if a empty buffer with a
      client_id tag is sent to the input of the viddec before the ES data of a
      frame has been completed.  This will cause the MFD FW to eject a frame
      even if it has not been fully decoded.

   @param[in] viddec: Handle to the video decoder
   @param[in] enable: true to enable the feature, false to disable.

   @retval ISMD_SUCCESS : The value was successfully set. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_set_id_tag_empty_buf_send_end_of_frame(ismd_dev_t viddec,
                                                                 bool enable);

/**
Adds a dependent view corresponding to the view_index to the viddec output. 
The video decoder will return an output port corresponding to this dependent view.

@note A base view cannot be added using this interface.

   @param[in] viddec: Handle to the video decoder
   @param[in] view_index: The view_index corresponding to the dependent view.
   @param[out] port: the output port corresponding to the dependent view.

   @retval ISMD_SUCCESS : The dependent view was successfully added.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The specified view_index was invalid.
*/
ismd_result_t ismd_viddec_add_view (ismd_dev_t viddec,
                                    unsigned int view_index,
                                    ismd_port_handle_t *port);

/**
Enables a dependent view corresponding to the view_index. 
If the view was previously disabled, this interface programs the video
decoder to decode frames from this view.

   @param[in] viddec: Handle to the video decoder
   @param[in] view_index: The view_index that needs to be enabled

   @retval ISMD_SUCCESS : The dependent view was successfully enabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER: The specified view_index was invalid.
*/
ismd_result_t ismd_viddec_enable_view (ismd_dev_t viddec,
                                       unsigned int view_index);

/**
Disables a dependent view corresponding to the view_index. 
If the view was previously enabled, this interface programs the video
decoder not to decode frames from this view.

@note A base view cannot be disabled using this interface.

   @param[in] viddec: Handle to the video decoder
   @param[in] view_index: The view_index that needs to be disabled

   @retval ISMD_SUCCESS : The dependent view was successfully disabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The specified view_index was invalid.
*/
ismd_result_t ismd_viddec_disable_view (ismd_dev_t viddec,
                                        unsigned int view_index);

/**
Removes a dependent view corresponding to the view_index. 
If the view was previously added, all resources allocated for this particular
view will be freed.
NOte: A base view can never be removed.

   @param[in] viddec: Handle to the video decoder
   @param[in] view_index: The view_index that needs to be removed

   @retval ISMD_SUCCESS : The dependent view was successfully removed.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : The specified view_index was invalid.
*/
ismd_result_t ismd_viddec_remove_view (ismd_dev_t viddec,
                                       unsigned int view_index);

/**
This API allows the user to tell the decoder how to deal with out of 
segment frames. Current default setting is not to drop these frames in
the decoder.

   @param[in] viddec: Handle to the video decoder.
   @param[in] policy: The policy that the application wants the video
                      decoder to use when it encounters out of segment frames.

   @retval ISMD_SUCCESS: Policy set succesfuly. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
   @retval ISMD_ERROR_INVALID_PARAMETER : Specified policy is not valid.
*/
ismd_result_t ismd_viddec_set_outofsegment_drop_policy(ismd_dev_t viddec,
                                                       ismd_viddec_out_of_segment_drop_policy_t policy);

/**
Returns the policy that is currently being used to drop out of segment
frames in the decoder.

   @param[in] viddec: Handle to the video decoder.
   @param[out] policy: Returns the current policy for out of segment frames.

   @retval ISMD_SUCCESS: Policy set succesfuly. 
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
*/
ismd_result_t ismd_viddec_get_outofsegment_drop_policy(ismd_dev_t viddec,
                                                       ismd_viddec_out_of_segment_drop_policy_t *policy);

/**
Enables pan-scan propagation to downstream element.
By default, pan-scan data is propagated.

   @param[in] viddec: Handle to the video decoder
   @param[in] scale_val: Scale value

   @retval ISMD_SUCCESS: Pan-scan propagation successfully enabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_pan_scan_enable(ismd_dev_t viddec);

/**
Disable sending pan-scan data to the downstream element.
Sample aspect ratio will be adjusted, if needed. 
For example, sample aspect ratio in mpeg2 is calculated based on 
the display resolution if a sequence display extension is present 
in the bitstream. If pan-scan propagation is disabled, the sample
aspect ratio will be calculated based on the content resolution.
By default, pan-scan data is propagated.

   @param[in] viddec: Handle to the video decoder

   @retval ISMD_SUCCESS: Pan-scan propagation successfully disabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_pan_scan_disable(ismd_dev_t viddec);

/**
Gets the pan-scan propagation flag value in the decoder.

   @param[in] viddec: Handle to the video decoder
   @param[out] panscan_flag: Pan-scan propagation flag.

   @retval ISMD_SUCCESS: Output scaling successfully enabled.
   @retval ISMD_ERROR_INVALID_HANDLE : The specified decoder handle was invalid.
 */
ismd_result_t ismd_viddec_get_pan_scan_flag(ismd_dev_t viddec,
                                            bool *panscan_flag);


/** @} */ // end of ingroup ismd_viddec
/** @} */ // end of weakgroup ismd_viddec


#ifdef __cplusplus
}
#endif

#endif // __ISMD_VIDDEC_H__
