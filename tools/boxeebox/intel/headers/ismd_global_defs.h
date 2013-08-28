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
#define ECR200090_DPE_PART_ENABLE

#ifndef __ISMD_GLOBAL_DEFS_H__
#define __ISMD_GLOBAL_DEFS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup smd_core */
/** @{ */

/**
List of all ISMD return codes
*/
typedef enum {
   ISMD_SUCCESS                              =  0,
   ISMD_ERROR_FEATURE_NOT_IMPLEMENTED        =  1,
   ISMD_ERROR_FEATURE_NOT_SUPPORTED          =  2,
   ISMD_ERROR_INVALID_VERBOSITY_LEVEL        =  3,
   ISMD_ERROR_INVALID_PARAMETER              =  4,
   ISMD_ERROR_INVALID_HANDLE                 =  5,
   ISMD_ERROR_NO_RESOURCES                   =  6,
   ISMD_ERROR_INVALID_RESOURCE               =  7,
   ISMD_ERROR_INVALID_QUEUE_TYPE             =  8,
   ISMD_ERROR_NO_DATA_AVAILABLE              =  9,

   ISMD_ERROR_NO_SPACE_AVAILABLE             = 10,
   ISMD_ERROR_TIMEOUT                        = 11,
   ISMD_ERROR_EVENT_BUSY                     = 12,
   ISMD_ERROR_OBJECT_DELETED                 = 13,

   ISMD_ERROR_ALREADY_INITIALIZED            = 17,
   ISMD_ERROR_IOCTL_FAILED                   = 18,
   ISMD_ERROR_INVALID_BUFFER_TYPE            = 19,
   ISMD_ERROR_INVALID_FRAME_TYPE             = 20,
   
   ISMD_ERROR_QUEUE_BUSY                     = 21,
   ISMD_ERROR_NOT_FOUND                      = 22,
   ISMD_ERROR_OPERATION_FAILED               = 23,
   ISMD_ERROR_PORT_BUSY                      = 24,
   
   ISMD_ERROR_NULL_POINTER                   = 25,
   ISMD_ERROR_INVALID_REQUEST                = 26,
   ISMD_ERROR_OUT_OF_RANGE                   = 27,
   ISMD_ERROR_NOT_DONE                       = 28,

   ISMD_LAST_NORMAL_ERROR_MARKER,

   ISMD_ERROR_UNSPECIFIED                    = 99
} ismd_result_t;


/**
ismd_verbosity_level_t is defines the level of detail in the messages reported
by the SMD.
TODO: Create specific definitions for these.
*/
typedef enum {
   ISMD_LOG_VERBOSITY_OFF     = 0, /**< Logging is OFF */
   ISMD_LOG_VERBOSITY_LEVEL_1 = 1, /**< Minimal detail */
   ISMD_LOG_VERBOSITY_LEVEL_2 = 2,
   ISMD_LOG_VERBOSITY_LEVEL_3 = 3,
   ISMD_LOG_VERBOSITY_LEVEL_4 = 4  /**< Maximum detail */
} ismd_verbosity_level_t;

/** Playback rate in SMD is normalized to 10000. */
#define ISMD_NORMAL_PLAY_RATE 10000

/** A known invalid device handle is provided for convenience to developers. */
#define ISMD_DEV_HANDLE_INVALID -1

/** Opaque handle used for accessing a device */
typedef int ismd_dev_t;
typedef ismd_dev_t ismd_dev_handle_t; /* deprecated device type */

/** Device types supported by the streaming media drivers */
typedef enum {
   ISMD_DEV_TYPE_INVALID   = 0, /**< A known invalid device type. */
   ISMD_DEV_TYPE_CORE      = 1, /**< Fuctions/Features not specific to any one device. */
   ISMD_DEV_TYPE_TS_OUTPUT = 2, /**< Transport Stream Output */
   ISMD_DEV_TYPE_DEMUX     = 3, /**< Stream Demultiplexor */
   ISMD_DEV_TYPE_AUDIO     = 4, /**< Audio decode/mix/render/etc. */
   ISMD_DEV_TYPE_VIDDEC    = 5, /**< Video decoder */
   ISMD_DEV_TYPE_VIDPPROC  = 6, /**< Video pre-processor */
   ISMD_DEV_TYPE_VIDREND   = 7  /**< Video renderer */
} ismd_device_type_t;


typedef enum {
   ISMD_DEV_STATE_INVALID = 0,
   ISMD_DEV_STATE_STOP    = 1,
   ISMD_DEV_STATE_PAUSE   = 2,
   ISMD_DEV_STATE_PLAY    = 3
} ismd_dev_state_t;


/** Codecs supported by streaming media drivers */
typedef enum {
   ISMD_CODEC_TYPE_INVALID = 0, /**< A known invalid codec type. */
   ISMD_CODEC_TYPE_MPEG2   = 1, /**< MPEG2 video codec type. */
   ISMD_CODEC_TYPE_H264    = 2, /**< H264 video codec type. */
   ISMD_CODEC_TYPE_VC1     = 3, /**< VC1 video codec type. */
   ISMD_CODEC_TYPE_MPEG4   = 4  /**< MPEG4 video codec type. */
} ismd_codec_type_t;

typedef uint32_t ismd_physical_address_t;

/******************************************************************************/
/* Event-related defines and datatypes */
/******************************************************************************/

/** A known invalid event handle is provided for convenience to developers. */
#define ISMD_EVENT_HANDLE_INVALID -1

/** Timeout value that indicates you should wait forever. */
#define ISMD_TIMEOUT_NONE ((unsigned int)0xFFFFFFFF)

/** Event handle */
typedef int ismd_event_t; 

/** Maximum number of events in an event list.  Relavent only to ismd_event_wait_multiple. */
/* TODO:  Profile what an appropriate maximum number of events should be. */
#define ISMD_EVENT_LIST_MAX 16

/** A list of events.  Relevant only to ismd_event_wait_multiple. */
typedef ismd_event_t ismd_event_list_t[ISMD_EVENT_LIST_MAX];

/******************************************************************************/
/* Message-related defines and datatypes */
/******************************************************************************/

typedef uint32_t ismd_message_context_t; 

/** Max message context id definition */
#define ISMD_MESSAGE_CONTEXT_MAX 		16
#define ISMD_MESSAGE_CONTEXT_INVALID 	ISMD_MESSAGE_CONTEXT_MAX 	

/** Maximum message type per message context*/
#define ISMD_MESSAGE_TYPE_MAX	16

/** Maximum subscribers one message support*/
#define ISMD_MESSAGE_MAX_SUBSCRIBERS_PER_MESSAGE	4

/** This is used to return message*/
typedef struct {
   uint32_t type;
   uint32_t count;
   /* Currently only 8 bytes of message data supported.
    * To extend this, you may use void * pointer plus a size parameter.
    */
   uint64_t data;
} ismd_message_info_t;

typedef struct {
   uint32_t num_of_items;
   ismd_message_info_t info[ISMD_MESSAGE_TYPE_MAX];
} ismd_message_array_t;

/******************************************************************************/
/* Port-related defines and datatypes */
/******************************************************************************/
typedef int ismd_port_handle_t;

/* A known invalid event handle is provided for convenience to developers. */
#define ISMD_PORT_HANDLE_INVALID -1

/* 
 * Specifying this as a watermark for a port effectively disables watermark 
 * events 
 */
#define ISMD_QUEUE_WATERMARK_NONE -1

/**
Structure used to get the status of a port.
*/
typedef struct {
   int                    max_depth;         /**< Maximum depth of the port queue, queue will refuse data if depth would become larger than this amount */
   int                    cur_depth;         /**< Current depth of the port queue in buffers. */
   int                    watermark;         /**< Current watermark setting. */
} ismd_port_status_t;

/**
Definition of queue events.  These events are used with ports to enable 
event-driven I/O using the ports.

Note that not all events are available in all situations - the availability of 
events depends on which end of the port is being referred to.
Each port, whether it is an input or output port, has an input end and an output 
end.  The input end is the end that buffers are written into and the output
end is the end that buffers are read from.  

For output ports, the input end is generally used by the port's owning module 
and the output end is either directly connected to an input port or is read by
a client application.  

For input ports, the output end is generally used by the port's owning module 
and the input end is either directly connected to an output port or is written
to by a client application.

The terms "input end" and "output end" are used to specify where the events can
be used.
*/
typedef enum {
    ISMD_QUEUE_EVENT_NONE           = 0,
    ISMD_QUEUE_EVENT_NOT_EMPTY      = 1 << 0,  /**< Queue has gone from empty to not empty. Only available for the output end of the port. */
    ISMD_QUEUE_EVENT_NOT_FULL       = 1 << 1,  /**< Queue has gone from full to not full. Only available for the input end of the port. */
    ISMD_QUEUE_EVENT_HIGH_WATERMARK = 1 << 2,  /**< Queue depth crossed the high watermark. Only available for the output end of the port. */
    ISMD_QUEUE_EVENT_LOW_WATERMARK  = 1 << 3,  /**< Queue depth crossed the low watermark. Only available for the input end of the port. */
    ISMD_QUEUE_EVENT_EMPTY          = 1 << 4,  /**< Queue has become empty. Only available for the input end of the port. */
    ISMD_QUEUE_EVENT_FULL           = 1 << 5,  /**< Queue has become full. Only available for the output end of the port. */

    ISMD_QUEUE_EVENT_ALWAYS         = 1 << 6   /**< Enable all available events */
} ismd_queue_event_t;



/******************************************************************************/
/* Clock-related defines and datatypes */
/******************************************************************************/

#define ISMD_CLOCK_TYPE_INVALID       (0)
#define ISMD_CLOCK_TYPE_FIXED         (1)
#define ISMD_CLOCK_TYPE_SW_CONTROLLED (2)
#define ISMD_CLOCK_HANDLE_INVALID       -1
/* clock types >2 are platform-specific; usually specific, physical clocks */

typedef int ismd_clock_t;

typedef uint64_t ismd_time_t;

typedef int ismd_clock_alarm_t;



/******************************************************************************/
/* Buffer-related defines and datatypes */
/******************************************************************************/
typedef ismd_time_t  ismd_pts_t;
#define ISMD_NO_PTS  ((ismd_pts_t)-1)


typedef int ismd_buffer_id_t;
typedef ismd_buffer_id_t ismd_buffer_handle_t;
#define ISMD_BUFFER_HANDLE_INVALID -1

#define ISMD_FRAME_BUFFER_MANAGER_NO_CONTEXT -1

/** Defines the physical address attributes of the buffer */
typedef struct ismd_phys_buf_node {   
   ismd_physical_address_t      base;             /**< Physical base address. */
   int                          size;             /**< Physical size of the buffer allocated in bytes. */
   int                          level;            /**< Actual amount of data in the buffer in bytes (level <= size) */
   struct ismd_phys_buf_node   *next_node;        /**< Should always be NULL. Should be deprecated after modules are updated. */
} ismd_phys_buf_node_t;

/** Defines the virtual address attributes of the buffer */
typedef struct ismd_virt_buf_node {
   void                       *base;              /**< Virtual starting address. */
} ismd_virt_buf_node_t;

/** Defines the type of the ismd_buffer */
typedef enum {
   ISMD_BUFFER_TYPE_INVALID = 0,
   ISMD_BUFFER_TYPE_PHYS = 1,                     /**< Linear physical buffer */
   ISMD_BUFFER_TYPE_VIDEO_FRAME = 2,              /**< for strided video frame buffers.  This is a physical type */
   ISMD_BUFFER_TYPE_VIDEO_FRAME_REGION = 3,       /**< for frame buffer memory region descriptors.  This should only be used internal to the ISMD core */
   ISMD_BUFFER_TYPE_ALIAS = 4,                    /**< Aliased buffers. This should only be used internal to the ISMD core */
   ISMD_BUFFER_TYPE_PHYS_CACHED = 5               /**< Linear physical buffer with a cached virtual memory address.  Available only internal to ISMD core. */
} ismd_buffer_type_t;

struct ismd_buffer_descriptor_struct;

/** Reserved for use by SMD infrastructure. */
typedef ismd_result_t (* ismd_release_func_t)(void *context, struct ismd_buffer_descriptor_struct *buffer_descriptor);

/** Descriptor for ISMD buffers. */
typedef struct ismd_buffer_descriptor_struct {
   int                                   state;            /**< Reserved for use by SMD infrastructure.                                */
   ismd_buffer_type_t                    buffer_type;      /**< Linear physical buffer or frame buffer                                 */
   ismd_phys_buf_node_t                  phys;             /**< Physical address attributes                                            */
   ismd_virt_buf_node_t                  virt;             /**< Virtual address attributes                                             */
   ismd_buffer_id_t                      unique_id;        /**< System-wide unique identifier assigned to this buffer by the SMD Core.
                                                                May be used in calls to ismd_buffer_read_desc                          */
   struct ismd_buffer_descriptor_struct *next;             /**< Reserved for use by SMD infrastructure.                                */
   int                                   reference_count;  /**< Number of drivers currently processing this buffer.  Should be updated
                                                                by whoever allocates the buffer.                                       */
   ismd_release_func_t                   release_function; /**< Function to call when reference_count == 0.  This function allows one
                                                                device to be notified when all other devices have finished processing
                                                                the buffer.                                                            */
   void                                 *release_context;  /**< Context information passed unmodifed to the release function.          */
   int                                   data_type;        /**< Media data type.                                                       */
   int                                   tags_present;     /**< Indicates whether or not this buffer has associated tags.  0 if no tags
                                                                and nonzero if there are tags.                                         */
   unsigned char                         attributes[256];  /**< Space for buffer-specific attributes.  Drivers can define their own    */
                                                           /**< buffer attribute structures and cast this space to those structures.   */
} ismd_buffer_descriptor_t;

/**
The following time domains are referred to in the description of ismd_newsegment_tag_t.
Segment time - original time domain of content (segment start, stop is in this domain)
Linear time  - Local monotonically increasing 1x timeline (before rate adjustment)
Scaled time  - Local timeline after accounting for playback rate
Clock time   - actual counter time in smd clock corresponding to scaled time.
*/
/** Defines the data for the newsegment buffer tag */
typedef struct {
   ismd_pts_t start;                      /**<First point in the segment to be presented in forward playback. 
                                              Expressed in original media time. 
                                              Used to correlate phase between original media time and local 
                                              presentation time for forward segments. It corresponds to 
                                              local_presentation_time in forward playback.
                                              If this value is specified as ISMD_NO_PTS, it is ignored and the 
                                              segment is played from the beginning. */
   ismd_pts_t stop;                       /**<First point in the segment to be presented in reverse playback. 
                                              Expressed in original media time. 
                                              Used to correlate phase between original media time and local 
                                              presentation time for reverse segments. It corresponds to 
                                              local_presentation_time in reverse playback.
                                              If this value is specified as ISMD_NO_PTS, it is ignored and the 
                                              segment is played till the end. */
   ismd_pts_t linear_start;               /**<Local/play time corresponding to start/stop 
                                              (depending on playback direction). */
   int        requested_rate;             /**<Overall play rate required for the segment. Expressed as a fixed point value (when
                                              expressed in decimal) with 4 digits of fractional part. 
                                              For example, 10000 indicates 1.0X playback; 5000 -> 0.5X, -25000 -> -2.5X, etc. */
   int        applied_rate;               /**<Play rate achieved for this segment, by earlier elements in the pipeline. Expressed 
                                              in the same fixed point format as \a requested_rate. If \a applied_rate equals \a 
                                              requested_rate for a segment, downstream elements do not need to do anything further 
                                              to achieve the final play rate. */
   bool rate_valid;                       /**<A flag to indicate if the requested_rate in this segment is valid or not. 
                                              If false, the requested_rate from this segment is ignored and elements
                                              continue playing at the previous requested_rate (from newsegment or API). */
   ismd_pts_t segment_position;           /**<The segment's position in the stream. This should be a value between zero
                                              and the duration of the clip to guarantee a correct calculation of the stream position. 
                                              ISMD_NO_PTS is allowed for this field if application doesn't need to maintain the stream position across segments.
                                              If ISMD_NO_PTS passed, the stream position query APIs in renderers will not report the correct stream_position 
                                              (other fields returned by query function is still correct)*/
} ismd_newsegment_tag_t;

/** The maximum number of dynamic input entries that can 
  * be set on any single buffer as any single attribute.  
  * This can be expanded, but requires that the ISMD_MAX_TAG_SIZE 
  * internal define is also expanded.
  *
  * Currentle max tag size is 256 bytes.  16 of these is too big, so using 15.
  */
#define ISMD_MAX_DYN_INPUT_ENTRIES 15

/** 
  * Defines data for the dynamic input attributes. These are used for changing
  * the PID and/or the formatting of the incoming data.  This is meant to be
  * only for buffers that are at the input of the demux.  If format change needs
  * to be communicated after this point, there is a data_type field in the buffer
  * descriptor that is used to specify format.
  */
typedef struct ismd_dyn_input_buf_attr {
   unsigned             length;           /**< How many entries are valid in this structure */
   struct {
      union {
         struct {
            uint16_t         pid;                        /**< PID to change to */
         } transport_stream;
         struct {
            uint8_t         stream_id;                   /**< SID to change to */
            uint8_t         substream_id;                /**< substream to change to */
            unsigned        data_offset;                 /**< Data offset of substream */
         } program_stream;
      } stream_info ;
      unsigned               filter_id;                  /**< Demux filter that the PID is mapped to */
      ismd_codec_type_t      codec_format;               /**< Format of the data on the new PID */
   } dyn_filter[ISMD_MAX_DYN_INPUT_ENTRIES];
} ismd_dyn_input_buf_tag_t;



/******************************************************************************/
/* Media specific attributes.  These are placed the buffer attributes field */
/******************************************************************************/

/**
defines attributes to be carried for compressed (ES) data.
*/
typedef struct {
   ismd_pts_t              original_pts;  /**< Original unmodified presentation
                                               timestamp from the
                                               content.  The bit width of this
                                               value depends on the content.
                                               The resolution of this timestamp
                                               also depends on the content. 
                                               ISMD_NO_PTS indicates no PTS. */
                                          
   ismd_pts_t              local_pts;     /**< Local modified presentation 
                                               timestamp for this buffer.
                                               64-bit value representing 90kHz
                                               ticks. Rollovers should
                                               be masked so that this value
                                               never decreases during normal
                                               playback of a single stream. 
                                               ISMD_NO_PTS indicates no PTS. */
                                               
   bool                    discontinuity; /**< If true, this buffer is the first
                                               buffer after a discontinuity has
                                               been detected. */
   bool                    pvt_data_valid;  /**< If true, Valid data is present 
                                                                                    in pvt_data array */
   uint8_t                 pvt_data[16];    /**< The Audio Description stream can
                                                                                       carry the pan-scan information in
                                                                                        this array.*/
                                               
   // We may need to add some audio-specific attributes here as well...
} ismd_es_buf_attr_t;

/**
defines attributes to be carried for TS data.
*/
typedef struct {
  uint32_t    packet1_num; /**< The Packet number of the first TS packet
                                contained in this buffer. Packet numbers
                                start from zero when the stream data
                                starts flowing. 0xFFFFFFFF is invalid. */
  uint32_t    packet_curr_num; /**< The current packet number of the current
                                (last) TS packet. This is the current packet
                                number the fw is dealing with when the buffer
                                was emitted.*/
  // In future, the indexing data (max 15 records) could be contained here as well.
} ismd_ts_buf_attr_t;

/**
Enumeration of video formats.  Note that not all stages of the pipeline
support all of these formats.

See http://www.fourcc.org/yuv.php for explanations of these formats
*/
typedef enum {
    /* RGB types */
    ISMD_PF_ARGB_32,          /**< ARGB 32bpp 8:8:8:8 LE */
    ISMD_PF_RGB_32,           /**< xRGB 32bpp 8:8:8:8 LE */
    ISMD_PF_RGB_24,           /**< RGB 24bpp 8:8:8 LE */
    ISMD_PF_ARGB_16_1555,     /**< ARGB 16bpp 1:5:5:5 LE */
    ISMD_PF_ARGB_16_4444,     /**< ARGB 16bpp 4:4:4:4 LE */
    ISMD_PF_RGB_16,           /**< RGB  16bpp 5:6:5 LE */
    ISMD_PF_RGB_15,           /**< xRGB 16bpp 1:5:5:5 LE */

    /* Pseudo color types */
    ISMD_PF_RGB_8,            /**< RGB  8bpp - 24bpp palette RGB 8:8:8 */
    ISMD_PF_ARGB_8,           /**< ARGB 8bpp - 32bpp palette ARGB 8:8:8:8 */

    /* Packed YUV types */
    ISMD_PF_YUY2,             /**< YUV 4:2:2 Packed */
    ISMD_PF_UYVY,             /**< YUV 4:2:2 Packed */
    ISMD_PF_YVYU,             /**< YUV 4:2:2 Packed */
    ISMD_PF_VYUY,             /**< YUV 4:2:2 Packed */

    /* Planar YUV types */
    ISMD_PF_YV12,             /**< YVU 4:2:0 Planar (V plane precedes U) */
    ISMD_PF_YVU9,             /**< YUV 4:2:0 Planar */
    ISMD_PF_I420,             /**< YUV 4:2:0 Planar (U plane precedes V) */
    ISMD_PF_IYUV=ISMD_PF_I420,/**< Synonym for I420 */
    ISMD_PF_I422,             /**< YUV 4:2:2 Planar (U plane precedes V) */
    ISMD_PF_YV16,             /**< YVU 4:2:2 Planar (V plane precedes U) */

    /* Pseudo-planar YUV types */
    ISMD_PF_NV12,             /**< YUV 4:2:0 Pseudo-planar */
    ISMD_PF_NV15,
    ISMD_PF_NV16,             /**< YUV 4:2:2 Pseudo-planar */
    ISMD_PF_NV20,

    ISMD_PF_A8,               /**< 8-bit Alpha only surface. For 3D graphics*/

    ISMD_PF_COUNT             /**< Number of defined pixel formats */
} ismd_pixel_format_t;

/**
Enumeration of reverse gamma correction types.
*/
typedef enum {
    ISMD_GAMMA_NTSC,
    ISMD_GAMMA_PAL,
    ISMD_GAMMA_SECAM,
    ISMD_GAMMA_HDTV,
    ISMD_GAMMA_LINEAR
} ismd_gamma_t;

/**
Enumeration of color spaces.
*/
typedef enum {
    ISMD_SRC_COLOR_SPACE_BT601,        /**< BT601 */
    ISMD_SRC_COLOR_SPACE_BT709,        /**< BT709 */
    ISMD_SRC_COLOR_SPACE_BYPASS,
    ISMD_SRC_COLOR_SPACE_UNKNOWN       /**< Content did not specify */
} ismd_src_color_space_t;


/**
Pan-scan window offsets.
*/
typedef struct {
   unsigned int horz_offset;           /**< Horizontal offset of window start
                                            (how many pixels on the left side
                                            are skipped) in pixels */
   unsigned int vert_offset;           /**< Vertical offset of window start
                                            (how many pixels on the top
                                            are skipped) in pixels */
   unsigned int horz_size;             /**< Width of the pan-scan window*/
   unsigned int vert_size;             /**< Height of the pan-scan window*/

} ismd_pan_scan_offset_t;

/**
Polarity for this buffer.  Is it a top field, bottom field, or progressive?
*/
typedef enum {
    ISMD_POLARITY_FRAME          =1,   /**< Progressive - frame based */
    ISMD_POLARITY_FIELD_TOP      =2,   /**< Interlaced - field based, 
                                            top/odd field */
    ISMD_POLARITY_FIELD_BOTTOM   =3    /**< Interlaced - field based, 
                                            bottom/even field */
} ismd_frame_polarity_t;

/**
Aspect ratio is expressed as a fraction: numerator and denominator.
*/
typedef struct {
   unsigned int numerator;
   unsigned int denominator;
} ismd_aspect_ratio_t;


/**
Generic rectangle size descriptor - generally used to express video frame size.
*/
typedef struct {
   unsigned int width;                 /**< in pixels */
   unsigned int height;                /**< in pixels */
} ismd_rect_size_t;

#ifdef ECR200090_DPE_PART_ENABLE
/**
Generic rectangle position descriptor - generally used to express video frame position.
*/
typedef struct {
   unsigned int x;                 /**< in pixels */
   unsigned int y;                /**< in pixels */
} ismd_point_t;
#endif

/**
Definition for cropping window.  Includes origin and size.
The cropping window is the subset of the source frame that is scaled
for display.
The cropping window's right and bottom edges should not go outside of the
frame.
*/
typedef struct {
   unsigned int h_offset;              /**< Horizontal offset of window start
                                            (how many pixels on the left side
                                            are skipped) in pixels */
   unsigned int v_offset;              /**< Vertical offset of window start
                                            (how many pixels on the top
                                            are skipped) in pixels */
   unsigned int width;                 /**< Window width in pixels. */
   unsigned int height;                /**< Window height in pixels. */
} ismd_cropping_window_t;

/**
Struct for NTSC (EIA-608) closed captions on line 21 extracted from CEA-708 data.
In EIA-608, each field carrise 16 bits of closed-captioning data on line 21.
Each 16-bit chunk contains two characters, each using bit 7 for parity.
The 16 bits can also be used for control words.
*/
typedef struct {
   unsigned char cc_valid;
   unsigned char cc_type;
   unsigned char cc_data1;
   unsigned char cc_data2;
} ismd_608_cc_t;


/**
The video renderer may need to know which field on the frame it receives is
unaltered.
Deinterlacing may take a field and make up data for the other field to generate
a whole frame.
If rate conversion needs to be done in the renderer, it needs to know if this
was done so it can use the unaltered field for best quality.
If both fields were changed (due to scaling) then neither field is original.  
If the input source was progressive then both fields are original.
*/
typedef enum {
   ISMD_TOP_FIELD,
   ISMD_BOTTOM_FIELD,
   ISMD_BOTH_FIELDS,
   ISMD_NEITHER_FIELD,
} ismd_original_field_t;


#define ISMD_MAX_PAN_SCAN_VECTORS 4
/**
This structure decsribes each video frame/field.  This is stored in the
"attributes" field of each ismd buffer descriptor.
*/
typedef struct {

   /////////////////////////////////////////////////////////////////////////////
   // Basic attributes (timestamp format, and address)
   
   ismd_pts_t              original_pts;  /**< Original unmodified presentation
                                               timestamp from the
                                               content.  The bit width of this
                                               value depends on the content.
                                               The resolution of this timestamp
                                               also depends on the content. 
                                               ISMD_NO_PTS indicates no PTS. */
                                          
   ismd_pts_t              local_pts;     /**< Local modified presentation 
                                               timestamp for this buffer.
                                               64-bit value representing 90kHz
                                               ticks. Rollovers should
                                               be masked so that this value
                                               never decreases during normal
                                               playback of a single stream. 
                                               ISMD_NO_PTS indicates no PTS. */
  
  
   ismd_pixel_format_t     pixel_format;  /**< Layout of pixel data
                                               (NV12, etc...) */

   ismd_physical_address_t y;             /**< Physical offset to start of 
                                               first line of Y data in this 
                                               buffer, relative to the buffer's 
                                               physical base address */
                                    
   ismd_physical_address_t u;             /**< Physical offset to start of 
                                               first line of U data in this 
                                               buffer, relative to the buffer's 
                                               physical base address */
                                    
   ismd_physical_address_t v;             /**< Physical offset to start of 
                                               first line of V data in this 
                                               buffer, relative to the buffer's 
                                               physical base address
                                               Ignored for pseudoplanar
                                               formats */

   ismd_gamma_t            gamma_type;    /**< Type of reverse gamma correction.
                                               This is optional in the stream
                                               and if it is not known from the
                                               content, a default value is
                                               assumed depending on the content
                                               specification. */

   ismd_src_color_space_t  color_space;   /**< Color Space (601/709/etc).
                                               This is optional in the stream
                                               and if it is not known from the
                                               content the value 
                                               ISMD_SRC_COLOR_SPACE_UNKNOWN
                                               will be used. */

   /////////////////////////////////////////////////////////////////////////////
   // Size and related attributes
   
   
   ismd_rect_size_t        cont_size;     /**< Content frame size in pixels.
                                               This is a required value. */

   ismd_aspect_ratio_t     cont_ratio;    /**< Aspect ratio of content.
                                               If not known, will be set to
                                               0/0 and square pixels are 
                                               used. */

   ismd_rect_size_t        cont_disp_res; /**< Display resolution encoded in 
                                               the content.  This is optional
                                               in the content and is not used
                                               for display purposes.  This is 
                                               supplied in case the client 
                                               needs to know this information.
                                               If not known, this will be 
                                               0,0. */

   ismd_pan_scan_offset_t  cont_pan_scan[ISMD_MAX_PAN_SCAN_VECTORS]; 
                                               /**< Pan-scan coordinates extracted 
                                               from the content.  If pan-scan
                                               is enabled, these should already
                                               be accounted for in the
                                               src_window. */
   unsigned char           num_pan_scan_windows; /**< number of active pan-scan windows */
   
   ismd_cropping_window_t  src_window;    /**< Cropping window offset and size.
                                               If no cropping, offset is 0,0
                                               and size is same as cont_size. */


   ismd_rect_size_t        dest_size;     /**< Origin and pixel dimensions for the
                                               destination of the frame
                                               (usually a vieo canvas). */
                                          
#ifdef ECR200090_DPE_PART_ENABLE
   ismd_point_t         dest_origin;      /**< Pixel position for the
                                               destination of the frame.*/
   bool                    dest_rect_change; /**< Flag to indicate destination rectangle change */
#endif

   // Scaling is a function of the source cropping window and the destination
   // size.  The final destination aspect ration is calculated as a function
   // of the content aspect ratio (or content size if no ratio specified) and
   // the scaling.

   /////////////////////////////////////////////////////////////////////////////
   // Misc attributes

   bool                    discontinuity; /**< If true, this frame is the first
                                               frame after a discontinuity has
                                               been detected.  The decoder must
                                               set this flag if the
                                               corresponding buffer on its
                                               input indicates a 
                                               discontinuity. */

   ismd_frame_polarity_t   polarity;      /**< Progressive vs. even or odd
                                               field. */
                                               
// a picture could contain cc data for Top Field, Bottom Field and a Repeated Field.                                               
#define ISMD_608_CC_ARRAY_MAX 3

   ismd_608_cc_t           EIA_608_capt[ISMD_608_CC_ARRAY_MAX]; /**< If 608 
                                               (line 21) captions are present, 
                                               this structure will indicate so.  
                                               708 captions are not stored as 
                                               frame attributes but are 
                                               forwarded directly to the user
                                               space application. */
   
   unsigned int            cont_rate;     /**< Framerate specified in the
                                               content. Stored in 90kHz ticks.
                                               If not known, this is 0. */
   
   bool                    repeated;      /**< This is a repeated field.
                                               (3:2 pulldown) */

   ismd_original_field_t   orig_field;    /**< Specifies which field is original
                                               (not altered due to 
                                               deinterlacing or scaling) for 
                                               use with rate conversion. */

#ifndef ECR200090_DPE_PART_ENABLE
   unsigned char           debug[32];     /**< General-purpose memory for
                                               debugging. */
#else
   unsigned char           debug[24];     /**< General-purpose memory for
                                               debugging. */
#endif

} ismd_frame_attributes_t;

/// Demux section filter information which is passed back in the buf_desc->attributes[]
typedef struct {
   unsigned int     stream_id;
   unsigned int     filter_id;             /**< section filter id */
   unsigned long    section_filter_handle;
} ismd_filter_info_t;

// MPEG layer for error reporting
enum eMpegLayer
{
   ML_BLOCK,
   ML_MACROBLOCK,
   ML_SLICE,
   ML_PICTURE,
   ML_GOP,
   ML_SEQUENCE,
   ML_ES,
   ML_PES,
   ML_SECTION,      // PES and sections are actually on a similar layer, but enumerated seperately.
   ML_SYSTEM        // Program or transport
};


/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* __ISMD_GLOBAL_DEFS_H__ */
