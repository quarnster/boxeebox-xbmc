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
 * This file contains the enumerations and function declarations that
 * comprise the the Demux APIs.
 */

#ifndef __ISMD_DEMUX_H__
#define __ISMD_DEMUX_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @weakgroup ismd_demux Transport/Program Stream Processing

<H3>Demux Module</H3>

The demux processes MPEG2 transport streams or DVD/HD-DVD program streams for
playback or MPEG2 transport streams for PVR recording.  Useful data from
an input stream is extracted and undesired data is dropped.


<H3>Basic Operation</H3>

The demux uses three core types of elements: streams, pids, and filters.
A stream is synonymous with an input data stream, filters are output data
streams, and pids are the numeric packet ids found in transport and
program streams.

The mapping of pids to a filter determines which packets will become part
of a data output stream.  A filter is configured with a format type which
determines the elements of a packet to send.  For example, a transport
stream filter would output the complete TS packet, while an ES filter
would extract the ES payload and timestamp and drop the rest of the packet.


<H3>Playback</H3>

For playback of an input stream, the demux demultiplexes the input stream
into individual data output streams for the app and decoders.

Basic playback steps:

1)open stream

2)open ES output filters for audio and video

3)create pids

4)map pids to filters

5)enable stream

<H3>TSI (Tuner)</H3>

For Transport Stream Input (TSI, the tuner/demod interface) playback the
demux needs to configured in TSI mode. For TSI mode, a demux handle is
obtained by calling \ref ismd_tsin_open() instead of \ref ismd_demux_stream_open().
The TSI demux is then configured in much the same way that the normal
demux is configured, with the following differences:

1) there is no input port

2) a singular output filter is created, and set for TS packet output.

3) the audio, video, and PCR PID are added to this singular output.

Note that in this mode timing is very important.  Too little buffering
causes audio to mute and video starts to repeat frames, too much buffering
causes macroblocking.  There are two major components to maintaining the
proper timing:

1) Pre-buffer the pipeline before starting.
This is handled by the "demux buffering event".  This is an event in the
demux driver that applications can register for.  Once the demux believes
that enough data has passed through it the event will be set.

2) Use clock recovery to keep the presentation clock counting at the same
average speed as the data is being received.  This keeps the drivers from
playing "too fast" or "too slow" and keeps the buffering levels relatively
constant.

Clock recovery is always enabled when doing TSI playback, by the call to
\ref ismd_demux_tsin_enable_clock_recovery()


<H3>MSPOD</H3>

The TSI can be configured for MSPOD mode, where the data is routed up to
an MSPOD unit for descrambling before feeding it to TSD. There are
basically two MSPOD modes: single-stream and multi-stream.  In single-stream,
a single TSI input (can be an MPTS) is sent unchanged to the MSPOD unit.
In multi-stream mode several TSI inputs are "muxed" with a special header
and sent to the MSPOD, then they are de-muxed again (in the MSPOD unit itself
before feeding it to TSD) when they come back and behave like independent
TSIN streams after that.

If MSPOD is being used in single-stream mode or multi-stream mode it should
be set accordingly.

There is one other important option with MSPOD: the silicon has an option to
"bypass" the MSPOD unit (but still do the transport stream muxing and demuxing).
This is useful for checking out the functionality without actually needing to
set up a MSPOD unit.

The basic steps to enable MSPOD are:

1) Open a TSI demux

2) Set the TSI demux in open cable mode \ref ismd_demux_tsin_set_open_cable_mode

3) Optionally set an Local Transport Stream ID using \ref ismd_demux_tsin_set_open_cable_ltsid
   or obtain the internally generated LTSID using \ref ismd_demux_tsin_get_open_cable_ltsid

The LTSID is used in the Open Cable MUX headers to identify the streams.

<H3>Recording</H3>

The demux can also be used to generate an index record for transport streams.
The index record is intended for DVR functionality and indexes video stream
events such as the occurance of PTSs.

Basic record and index steps:
1)open stream

2)open TS output filter

3)create pids

4)map pids to filter

5)open indexing filter

6)map indexer to TS output filter

7)configure indexing of video pid

8)enable stream

For more info see the special section \ref dvr_doc "SMD DVR and indexing documentation"
 */
/** @{ */

#ifndef ISMD_DEMUX_DEFINES_ONLY
#include "stdint.h"
#include "osal.h"
#include "ismd_msg.h"
#include "ismd_core.h"
#include "ismd_clock_recovery.h"
#endif

/**
   Identifies the various crypto algorithms the Demux supports
   @enum ismd_demux_crypto_algorithm_t
*/
typedef enum
{
   ISMD_DEMUX_PASSTHROUGH = 0x01,        /**< Indicates direct passthrough. */
   ISMD_DEMUX_DES_ENCRYPT_ECB =  0x08,   /**< Indicates DES ECB encryption. */
   ISMD_DEMUX_DES_DECRYPT_ECB = 0x09,    /**< Indicates DES ECB decryption. */
   ISMD_DEMUX_DES_ENCRYPT_CBC = 0x0A,    /**< Indicates DES CBC encryption. */
   ISMD_DEMUX_DES_DECRYPT_CBC = 0x0B,    /**< Indicates DES CBC decryption. */
   ISMD_DEMUX_3DES_ENCRYPT_ECB = 0x0C,   /**< Indicates 3DES ECB encryption. */
   ISMD_DEMUX_3DES_DECRYPT_ECB = 0x0D,   /**< Indicates 3DES ECB decryption. */
   ISMD_DEMUX_3DES_ENCRYPT_CBC = 0x0E,   /**< Indicates 3DES CBC encryption. */
   ISMD_DEMUX_3DES_DECRYPT_CBC = 0x0F,   /**< Indicates 3DES CBC decryption. */
   ISMD_DEMUX_DVB_CSA_DESCRAMBLE = 0x12, /**< Indicates DVB descrambling. */
   ISMD_DEMUX_MULTI2_DECRYPT = 0x1A,     /**< Indicates MULTI2 decryption. */
   ISMD_DEMUX_AES128_ENCRYPT_ECB = 0x1018, /**< Indicates AES128 ECB encryption. */
   ISMD_DEMUX_AES128_DECRYPT_ECB = 0x1019, /**< Indicates AES128 ECB decryption. */
   ISMD_DEMUX_AES128_ENCRYPT_CBC = 0x1118, /**< Indicates AES128 CBC encryption. */
   ISMD_DEMUX_AES128_DECRYPT_CBC = 0x1119, /**< Indicates AES128 CBC decryption. */
   ISMD_DEMUX_AES128_ENCRYPT_CTR = 0x1e18, /**< Indicates AES128 CTR encryption. */
   ISMD_DEMUX_AES128_DECRYPT_CTR = 0x1e19, /**< Indicates AES128 CTR decryption. */
	ISMD_DEMUX_UNKNOWN_CRYPTO_ALG = 0x61200216, /**< Indicates Unknow decryption. */
} ismd_demux_crypto_algorithm_t;

#define ISMD_DEMUX_MAX_KEY_LEN (192)

/**
   Storage structure for crypto informatino such as keys or IV values
*/
typedef struct {
	unsigned char crypto_info[ISMD_DEMUX_MAX_KEY_LEN];
} ismd_demux_crypto_info_t;
/**
   Identifies the various crypto residual modes the Demux supports
   @enum ismd_demux_crypto_residual_mode_t
*/
typedef enum
{
   ISMD_DEMUX_CLEAR_RESIDUAL         = 0x00,
   ISMD_DEMUX_TEXT_STEALING_RESIDUAL =  0x02,
   ISMD_DEMUX_TERMINATION_BLOCK_PROCESSING_RESIDUAL =  0x04,
} ismd_demux_crypto_residual_mode_t;

/**
   Specifies odd or even key when setting crypto key.  Ignored when using
   raw input streams.
   @enum ismd_demux_key_type_t
*/
typedef enum {
   ISMD_DEMUX_KEY_TYPE_EVEN = 2,
   ISMD_DEMUX_KEY_TYPE_ODD
} ismd_demux_key_type_t;

typedef enum {
   ISMD_DEMUX_INPUT_PORT_MODE,
   ISMD_DEMUX_INPUT_PHY_MODE,
} ismd_demux_input_mode_t;

/**
   Defines the payload type of the PID being added to a TSD handle.
   @enum ismd_demux_pid_type_t
*/
typedef enum {
	ISMD_DEMUX_PID_TYPE_PSI = 0,/**< No special processing done for PSI. */
	ISMD_DEMUX_PID_TYPE_PES     /**< Standard PES PID. */
} ismd_demux_pid_type_t;


/**
   This structure is used to specify the pid
   @struct ismd_demux_pid_t
*/
typedef struct {
	uint16_t               pid_val;
	ismd_demux_pid_type_t    pid_type;
}ismd_demux_pid_t;

/**
   This structure is used to specify the stream id
   @struct ismd_demux_sid_t
*/
typedef struct {
        uint8_t                stream_id;
} ismd_demux_sid_t;

#define ISMD_DEMUX_MAX_PID_LIST 40
#define ISMD_DEMUX_MAX_SID_LIST 8

/**
   Used to specify a set of pids
   @struct ismd_demux_pid_list_t
*/
typedef struct {
   ismd_demux_pid_t pid_array[ISMD_DEMUX_MAX_PID_LIST];
} ismd_demux_pid_list_t;

/**
   Used to specify a set of stream id's
   @struct ismd_demux_sid_list_t
*/
typedef struct {
   ismd_demux_sid_t sid_array[ISMD_DEMUX_MAX_SID_LIST];
} ismd_demux_sid_list_t;

typedef unsigned ismd_demux_filter_handle_t;

/**
    Defines the filter output formats supported by the Demux
    @enum ismd_demux_filter_output_format_t
*/
typedef enum {
   ISMD_DEMUX_OUTPUT_WHOLE_TS_PACKET = 0,/**< Output the entire transport stream
                                            packets. The input must be a
                                            transport stream to use this option
                                            */
   ISMD_DEMUX_OUTPUT_TS_PACKET_PAYLOAD,  /**< Output only the payload of the
                                            transport stream data.  This will
                                            generate MPEG2 PES data if the input
                                            is an MPEG2 transport stream.  The
                                            input must be a transport stream
                                            to use this option*/
   ISMD_DEMUX_OUTPUT_INDEXING_INFO,      /**< Output indexing information for the
                                            transport stream on the input.  The
                                            input must be a transport
                                            stream to use this option. */
   ISMD_DEMUX_OUTPUT_TS_PACKET_PAYLOAD_WITHOUT_PTS_SPLIT,  /**< Output the payload of
                                            the transport stream data without spliting the PES buffer every time a PTS is present.
                                            This will increase the PES buffer output performance. For example: blu-ray PG/IG PES data.
                                            Application need to handle multiple PTS values in one PES buffer.
                                            The input must be a transport stream to use this option*/
   RESERVED_FT1,                          /**< Reserved. */
   ISMD_DEMUX_OUTPUT_ES,                 /**< Output ES data along with timestamps.
                                            The output must be using linked-list
                                            mode if this option is used.
                                            The input must be a transport stream
                                            or a program stream
                                            to use this option*/
   ISMD_DEMUX_OUTPUT_PSI,                /**< Output program system information
                                            (PSI). The input must be a transport
                                            stream to use this option. */
   ISMD_DEMUX_OUTPUT_NOTHING,         /**< Throw away the output. */
   ISMD_DEMUX_OUTPUT_MAX
} ismd_demux_filter_output_format_t;



/**
    Defines the stream input formats supported by the Demux
    @enum ismd_demux_input_stream_type_t
*/
typedef enum
{
   ISMD_DEMUX_STREAM_TYPE_MPEG2_TRANSPORT_STREAM = 1,
   /**< Indicates MPEG2 transport stream. */
   ISMD_DEMUX_STREAM_TYPE_DVD_PROGRAM_STREAM,
   /**< DVD program stream. */
   ISMD_DEMUX_STREAM_TYPE_MPEG2_TRANSPORT_STREAM_192,
   /**< Indicates MPEG2 transport stream with additional 4 byte timestamp header
   preceeding each TS packet. */
   ISMD_DEMUX_STREAM_TYPE_MAX
} ismd_demux_input_stream_type_t;

/**
    Defines the stat selection indices to determine which value is returned
    @enum ismd_demux_stat_t
*/
typedef enum {
    ISMD_DEMUX_STAT_PES_COUNT=0,
    ISMD_DEMUX_STAT_PES_ERRORS=1,
    ISMD_DEMUX_STAT_TS_ERRORS=2,
    ISMD_DEMUX_STAT_PTS_ERRORS=3,
    ISMD_DEMUX_STAT_MAX
} ismd_demux_stat_t;

/**
   Statistics for the Demux and Clock Recovery used by ismd_demux_get_stats()
   @struct ismd_demux_stats_t
*/
typedef struct {
   unsigned int pes_count;                     /**< Count of PES Headers as determined by valid PUSI bit */
   unsigned int pes_errors;                    /**< Deprecated ? */
   unsigned int pts_errors;                    /**< Deprecated ? */
   unsigned int ts_errors;                     /**< increments when first byte of TS Packet is not a sync byte or Transport Error Indicator bit is set */
   unsigned int pcr_discontinuities_count;     /**< increments when PCR Diff > PCR Threshold which usually results in a New Segment event */
   unsigned int continuity_cntr_errors;        /**< tbd - will increment when TS continuity_counter is not consecutive */
#ifndef ISMD_DEMUX_DEFINES_ONLY
   clock_sync_t clock_sync_handle;             /**< If == ISMD_DEV_HANDLE_INVALID, then clock recovery is not being used, clock_sync_stats are not valid.  If valid can be used to call clock_sync API's directly. */
   clock_sync_statistics_t  clock_sync_stats;  /**< Includes clock_locked flag */
#endif
} ismd_demux_stats_t;

/**
   Specifies whether the NIM is in serial or parallel mode.
   @enum ismd_demux_tsin_nim_mode_t
*/
typedef enum {
   ISMD_DEMUX_TSIN_NIM_SERIAL = 0x55,      /**< The NIM will operate in serial mode. */
   ISMD_DEMUX_TSIN_NIM_PARALLEL            /**< The NIM will operate in parallel mode. */
} ismd_demux_tsin_nim_mode_t;


/**
   Specifies the NIM clock polarity.
   @enum ismd_demux_tsin_clock_polarity_t
*/
typedef enum {
   ISMD_DEMUX_TSIN_POS_CLOCK = 0x66,       /**< Samples are taken on positive clock edges. */
   ISMD_DEMUX_TSIN_NEG_CLOCK               /**< Samples are taken on negative clock edges. */
} ismd_demux_tsin_clock_polarity_t;


/**
   Specifies the NIM data valid polarity.
   @enum ismd_demux_tsin_dv_polarity_t
*/
typedef enum {
   ISMD_DEMUX_TSIN_POS_DV = 0x77,          /**< Data valid signal is active high. */
   ISMD_DEMUX_TSIN_NEG_DV                  /**< Data valid signal is active low. */
} ismd_demux_tsin_dv_polarity_t;


/**
   Specifies the NIM error polarity.
   @enum ismd_demux_tsin_error_polarity_t
*/
typedef enum {
   ISMD_DEMUX_TSIN_POS_ERR = 0x88,          /**< Error signal is active high. */
   ISMD_DEMUX_TSIN_NEG_ERR                  /**< Error signal is active low. */
} ismd_demux_tsin_error_polarity_t;


/**
   Specifies the NIM sync polarity.
   @enum ismd_demux_tsin_sync_polarity_t
*/
typedef enum {
   ISMD_DEMUX_TSIN_POS_SYNC = 0x99,         /**< Sync is pulsed high. */
   ISMD_DEMUX_TSIN_NEG_SYNC                 /**< Sync is pulsed low. */
} ismd_demux_tsin_sync_polarity_t;


/**
   Structure used to set the NIM configuration.
   This should be set to match the external NIM.
   @struct ismd_demux_tsin_nim_config_t
*/
typedef struct {
   ismd_demux_tsin_nim_mode_t        nim_mode;         /**< specifies ISMD_DEMUX_PI_NIM_SERIAL or ISMD_DEMUX_PI_NIM_PARALLEL */
   ismd_demux_tsin_clock_polarity_t  clk_polarity;     /**< specifies ISMD_DEMUX_PI_POS_CLOCK or ISMD_DEMUX_PI_NEG_CLOCK */
   ismd_demux_tsin_dv_polarity_t     dv_polarity;      /**< specifies ISMD_DEMUX_PI_POS_DV or ISMD_DEMUX_PI_NEG_DV */
   ismd_demux_tsin_error_polarity_t  err_polarity;     /**< specifies ISMD_DEMUX_PI_POS_ERR or ISMD_DEMUX_PI_NEG_ERR */
   ismd_demux_tsin_sync_polarity_t   sync_polarity;    /**< specifies ISMD_DEMUX_PI_POS_SYNC or ISMD_DEMUX_PI_NEG_SYNC*/
} ismd_demux_tsin_nim_config_t;


typedef unsigned long ismd_demux_section_filter_handle_t;

#define ISMD_DEMUX_SF_MASK_SIZE 	24
#define ISMD_DEMUX_MAX_SECTION_SIZE 	4096

typedef struct ismd_demux_section_filter_params
{
    uint8_t        tid;
    uint8_t        section_offset;
    uint8_t        positive_filter[ISMD_DEMUX_SF_MASK_SIZE];
    uint8_t        positive_mask[ISMD_DEMUX_SF_MASK_SIZE];
    uint8_t        negative_filter[ISMD_DEMUX_SF_MASK_SIZE];
    uint8_t        negative_mask[ISMD_DEMUX_SF_MASK_SIZE];
} ismd_demux_section_filter_params_t;


enum ismd_demux_open_cable_mode {
   ISMD_DEMUX_NO_OPEN_CABLE,     // no open cable mux/demux (don't route to POD)
   ISMD_DEMUX_SINGLE_STREAM,     // open cable single stream (route to POD)
   ISMD_DEMUX_MULTI_STREAM,      // open cable multi-stream (route to POD)
};

#ifndef ISMD_DEMUX_DEFINES_ONLY
/*! @name Initialization/teardown functions. */

/**
Gets a stream handle to a Demux device if one is available.

@param[in] stream_type : Specifies the data format of the stream to be processed.
@param[out] handle : Pointer to a stream handle ID that will get filled in.
@param[out] port_handle : Will be populated with a handle to the input port if in port input mode.  The returned value is undefined in phy mode.
@retval ISMD_SUCCESS : Handle was successfully obtained.
@retval ISMD_ERROR_NULL_POINTER : handle was supplied as NULL.
@retval ISMD_ERROR_INVALID_REQUEST : stream_type is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : No handles are available or unable to allocate required memory.
*/
ismd_result_t SMDCONNECTION ismd_demux_stream_open(
                                        ismd_demux_input_stream_type_t stream_type,
                                        ismd_dev_t        *handle,
                                        ismd_port_handle_t *port_handle);

/**
Gets a stream handle to a Demux device bound to a Transport hardware input if one is available.

@param[in] interface_id : identifier for the TS input to bind to.
@param[out] handle : Pointer to a stream handle ID that will get filled in.
@retval ISMD_SUCCESS : Handle was successfully obtained.
@retval ISMD_ERROR_NULL_POINTER : handle was supplied as NULL.
@retval ISMD_ERROR_INVALID_REQUEST : stream_type is invalid.
@retval ISMD_ERROR_OPERATION_FAILED : No handles are available or unable to allocate required memory.
*/
ismd_result_t SMDCONNECTION ismd_tsin_open(
                                        int interface_id,
                                        ismd_dev_t        *handle);
/**
Closes the specified stream in the Demux device. All resources allocated
for the stream and filters it owns are automatically freed.

@param[in] handle : handle to the demux stream.  The handle should
not be used after this function has been called.

@retval ISMD_SUCCESS : The stream is closed successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is not a valid, active Demux stream handle.
*/
ismd_result_t SMDEXPORT ismd_demux_stream_close(ismd_dev_t        handle);


/**
Open and start an output filter for a stream.

Used ONLY to open a pes filter or a psi filter for the specified
stream.
To open an indexer filter, use ismd_demux_filter_open_indexer();

@param[in] handle : Demux stream handle to get the filter from.
@param[in] op_format : Specifies the output format of the data for this filter.
When a transport stream is being processed, it can be entire transport stream
packets, transport stream packet payloads, or ES data.

@param[in] output_buf_size: Specifies the maximum size of each output buffer.  Note that buffers written by the filter
may not use the entire space available.
@param[out] filter_handle : Demux filter handle of the allocated filter.
@param[out] port_handle : handle to the filter's output port.

@retval ISMD_SUCCESS : The filter was successfully obtained.
@retval ISMD_ERROR_NULL_POINTER : filter_handle was supplied as NULL.
@retval ISMD_ERROR_INVALID_HANDLE : stream_handle is not a valid Demux stream handle.
@retval ISMD_ERROR_NO_RESOURCES : No filters are available.
@retval ISMD_ERROR_INVALID_REQUEST : op_format is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_open(ismd_dev_t        handle,
                                        ismd_demux_filter_output_format_t op_format,
                                        unsigned output_buf_size,
                                        ismd_demux_filter_handle_t *filter_handle,
                                        ismd_port_handle_t *port_handle);



/**
Opens an indexer filter, if available, for the specified TS filter.

Indexer filters are used for the DVR feature called indexing and are only required if
this feature is needed.

See also \ref dvr_doc "SMD DVR and indexing documentation"

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle that is exporting the SPTS data.
@param[out] indexer_filter_handle : new Demux indexer filter handle if successful.
@param[out] port_handle : handle to the filter's output port.

@retval ISMD_SUCCESS : The filter was successfully obtained.
@retval ISMD_ERROR_NULL_POINTER : indexer_filter_handle was supplied as NULL.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is not a valid Demux filter handle.
@retval ISMD_ERROR_NO_RESOURCES : No index filter is available.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_open_indexer(ismd_dev_t        handle,
                                                ismd_demux_filter_handle_t filter_handle,
                                                ismd_demux_filter_handle_t *indexer_filter_handle,
                                                ismd_port_handle_t *port_handle);



/**
Closes a previously allocated Demux filter handle.  Any resources allocated
to this filter are automatically freed.  PIDs mapped to this filter are
autmoatically unmapped, but remain mapped to other filters.  This can be used
for indexer filters and normal filters.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
The filter should not be used after this function has been called.

@retval ISMD_SUCCESS : Always returns success.  Error conditions (invalid
                       handles, etc.) are detected but not reported.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_close(ismd_dev_t        handle,
                                         ismd_demux_filter_handle_t filter_handle);


/**
Gets the smd port handle associated with the filter handle. The port handle was created
during filter opening.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[out] port_handle : handle to the filter's output port.

@retval ISMD_SUCCESS : The filter info has been obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is not a valid active filter handle.
@retval ISMD_ERROR_NULL_POINTER : port_handle was supplied as NULL.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_get_output_port(ismd_dev_t        handle,
                                            ismd_demux_filter_handle_t filter_handle,
                                            ismd_port_handle_t *port_handle);


/**
Gets the smd port handle associated with the stream handle. The port handle was created
during stream opening.

@param[in] handle : Demux stream handle.

@param[out] port_handle : handle to the stream's input port.


@retval ISMD_SUCCESS : The filter info has been obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is not a valid active stream handle.
@retval ISMD_ERROR_NULL_POINTER : input parameter was supplied as NULL.
*/
ismd_result_t SMDEXPORT ismd_demux_stream_get_input_port(ismd_dev_t        handle,
                                            ismd_port_handle_t *port_handle);


/*! @name Basic Configuration functions. */

/**
Sets the PCR PID for this stream.  The Demux needs to know this
PID when rebasing.  If this is not set, pids with rebasing enabled will never
generate PTS values.  If none of the PIDs are set for rebasing, then this
function need not be used.  Note that this is global to the
entire input stream.  If there are multiple programs in the stream and each has
a different PCR PID with a different timebase, only one of the programs can
have rebasing enabled.

The following default settings will be used if this function is not called: <br>
- No PCR PID set, rebasing is not possible <br>

@param[in] handle : Demux stream handle.
@param[in] pcr_pid : Specifies the PCR PID for the stream.

@retval ISMD_SUCCESS : The PID is successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The stream is not a transport stream.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_pcr_pid(ismd_dev_t        handle,
                                           uint16_t pcr_pid);



/**
Enable PCR discontinuity detection logic on the demux. Current demux discontinuity detection is based on
PCR .

By default, this is enabled.

@param[in] handle : handle for the demux.

@retval ISMD_SUCCESS : .
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_enable_discontinuity_detection(ismd_dev_t handle);


/**
Disable PCR discontinuity detection logic on the demux.

By default, this is enabled.

@param[in] handle : Demux stream handle.

@retval ISMD_SUCCESS : .
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_disable_discontinuity_detection(ismd_dev_t handle);


/**
Enables or disables dynamic pid filtering on this filter.
Dynamic PID filters do not have matching PIDs as configured by
ismd_demux_filter_map_to_pids.  Instead, the PID match information is supplied
as meta-data with the stream's input buffers.  A filter cannot be both a normal
filter and a dynamic filter at the same time.  Attempting to enable dynamic
mode while PIDs are explicitly mapped to the filter will return an error.

This can only be used when the stream and filter are in port mode.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] enable : True to enable on this filter, false to disable.

@retval ISMD_SUCCESS : dynamic filtering was successfully enabled or disabled.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : Cannot enable dynamic filtering while pids are mapped
to the filter.
*/
ismd_result_t SMDEXPORT ismd_demux_set_dynamic_filtering(ismd_dev_t        handle,
                                               ismd_demux_filter_handle_t filter_handle,
                                               bool enable);



/**
Adds PIDs to a filter MAP.
Maps a filter to PIDs and starts the filter.
If a PID is invalid, an error is returned.
If the PID is a duplicate, the function returns no error.  The duplicate
PID is not added to the filter.

This fuction causes the Demux to send TS packets or portions with this PID
from the stream's (the handle used to allocate this filter) input to
the specified filter if the stream, PID, and filter are enabled.  This
must be used with a filter derived from ismd_demux_filter_open.

The following default settings will be used if this function is not called: <br>
The filter-PID mappings will not exist.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid_list : array of pids to be mapped.
@param[in] list_size : Specifies the number of valid elements in pid_list.

@retval ISMD_SUCCESS : The filter is successfully mapped to the PID.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_NO_RESOURCES : The stream is already processing the maximum
number of PIDs or the filter is already mapped to the maximum number of
PIDs or the PID is already mapped to the maximum number of filters.
@retval ISMD_ERROR_INVALID_REQUEST : The filter is an indexer, which can't
have PIDs mapped to it.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_map_to_pids(ismd_dev_t        handle,
                                               ismd_demux_filter_handle_t filter_handle,
                                               ismd_demux_pid_list_t pid_list,
                                               unsigned   list_size);


/**
Removes the filter-PID mapping.  This will cause the Demux to stop
sending data with this PID to the specified filter.  This must not be used with an index filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid_list : Specifies the PID list array.
@param[in] list_size : Specifies the number of valid items in the pid list to be unmapped.

@retval ISMD_SUCCESS : The filter is successfully unmapped from the PIDs.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The filter is an indexer, which can't
have PIDs unmapped from it.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_unmap_from_pids(ismd_dev_t        handle,
                                                   ismd_demux_filter_handle_t filter_handle,
                                                   ismd_demux_pid_list_t pid_list,
                                                   unsigned   list_size);



/**
Adds a single PID to a filter MAP.
Maps a filter to PID and starts the filter.
If a PID is invalid, an error is returned.
If the PID is a duplicate, the function returns no error.  The duplicate
PID is not added to the filter.

This fuction causes the Demux to send TS packets or portions with this PID
from the stream's (the handle used to allocate this filter) input to
the specified filter if the stream, PID, and filter are enabled.  This
must be used with a filter derived from ismd_demux_filter_open.

The following default settings will be used if this function is not called: <br>
The filter-PID mappings will not exist.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid_val : pid to be mapped.
@param[in] pid_type : pid type for the pid to be mapped.

@retval ISMD_SUCCESS : The filter is successfully mapped to the PID.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_NO_RESOURCES : The stream is already processing the maximum
number of PIDs or the filter is already mapped to the maximum number of
PIDs or the PID is already mapped to the maximum number of filters.
@retval ISMD_ERROR_INVALID_REQUEST : The filter is an indexer, which can't
have PIDs mapped to it.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_map_to_pid(ismd_dev_t        handle,
                                                   ismd_demux_filter_handle_t filter_handle,
                                                   uint16_t pid_val,
                                                   ismd_demux_pid_type_t pid_type);



/**
Removes the filter-PID mapping.  This will cause the Demux to stop
sending data with this PID to the specified filter.  This must not be used with an index filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid_val : Specifies the pid to be unmapped.

@retval ISMD_SUCCESS : The filter is successfully unmapped from the PIDs.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The filter is an indexer, which can't
have PIDs unmapped from it.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_unmap_from_pid(ismd_dev_t        handle,
                                                   ismd_demux_filter_handle_t filter_handle,
                                                   uint16_t pid_val);



/**
Retrieves the pids associated to the filter.

This must not be used with an index filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid_list : Specifies the PID list.
@param[in,out] list_size : When the function is called, contains the maximum
expected number of array entries, upon successful
return this is chenged to be the actual number of elements the driver wrote
into the pid_list.

@retval ISMD_SUCCESS : The list was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_get_mapped_pids(ismd_dev_t        handle,
                                          ismd_demux_filter_handle_t filter_handle,
                                          ismd_demux_pid_list_t *pid_list,
                                          unsigned   *list_size);


/**
Adds a stream_id_extension filter to a PID already added to a filter MAP.
If a PID is invalid, an error is returned.
If the stream_id_extension filter has already been added to this PID, the
function returns no error.  The new stream_id_extension replaces a previous
stream_id_extension.  Only one stream_id_extension can be associated with a
PID at any one time.

This fuction causes the Demux to send TS packets or portions with this PID
and matching stream_id_extension from the stream's (the handle used to
allocate this filter) input to the specified filter.  The function must be
used with a filter derived from ismd_demux_filter_open.

The following default settings will be used if this function is not called: <br>
The PID will have no stream_id_extension associated with it for filtering.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid : The PID to associate the stream_id_extension with.
@param[in] stream_id_extension : The stream_id_extension to be associated with
the PID.

@retval ISMD_SUCCESS : The stream_id_extension is successfully associated with
the PID.
@retval ISMD_ERROR_INVALID_HANDLE : stream handle or filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The PID specified has not been mapped to
the specified filter.
@retval ISMD_ERROR_OPERATION_FAILED : The stream_id_extension could not be
associated with the PID.
*/
ismd_result_t SMDEXPORT ismd_demux_add_stream_id_extension_to_pid(
                                       ismd_dev_t        handle,
                                       ismd_demux_filter_handle_t filter_handle,
                                       ismd_demux_pid_t pid,
                                       uint8_t   stream_id_extension);


/**
Removes the stream_id_extension associated with a PID.  The Demux will continue
sending data with this PID to the specified filter but it will no longer be
subject to filtering based on the stream_id_extension.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] pid : Specifies the PID to disassociated with the
stream_id_extension.

@retval ISMD_SUCCESS : The stream_id_extension is successfully disassociated
with the PID.
@retval ISMD_ERROR_INVALID_HANDLE : stream handle or filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The specified PID has not been mapped to
the filter.
@retval ISMD_ERROR_OPERATION_FAILED : The stream_id_extension could not be
disassociated from the PID.
*/
ismd_result_t SMDEXPORT ismd_demux_remove_stream_id_extension_from_pid(
                                       ismd_dev_t        handle,
                                       ismd_demux_filter_handle_t filter_handle,
                                       ismd_demux_pid_t pid);


/**
Map a filter to a sid and optional substream; this function must not be used with a
transport stream.
This can only be used with program streams.
Upon successful return the sid will be started.


This must not be used with an index filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] stream_id : Specifies the stream to associate with
@param[in] substream_id : Specifies the substream to associate with
@param[in] data_offset : Not currently implemented; size in bytes of additional header information in the packet payload

@retval ISMD_SUCCESS : The list was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
*/

ismd_result_t SMDEXPORT ismd_demux_filter_map_to_sid(ismd_dev_t        handle,
                                               ismd_demux_filter_handle_t filter_handle,
                                               uint8_t stream_id,
                                               uint8_t substream_id,
                                               unsigned int data_offset);
/**
Unmap a filter from a sid and optional substream.
This can only be used with program streams.
Upon successful return the filter will have been unmapped from the sid.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] stream_id : Specifies the stream to associate with
@param[in] substream_id : Specifies the substream to associate with

@retval ISMD_SUCCESS : The list was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : pid derived from stream_id is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_unmap_from_sid(ismd_dev_handle_t handle,
                                               ismd_demux_filter_handle_t filter_handle,
                                               uint8_t stream_id,
                                               uint8_t substream_id);

/**
Get a the stream & substream ids mapped to a specified filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] stream_id : pointer to area to store stream id mapped to filter
@param[in] substream_id : pointer to area to store substream id mapped to filter

@retval ISMD_SUCCESS : The list was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_NULL_PTR : if either the stream_id or substream_id pointers are null.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_get_mapped_sid(ismd_dev_handle_t handle,
                                          ismd_demux_filter_handle_t filter_handle,
                                          uint8_t *stream_id,
                                          uint8_t *substream_id);




/**
Resets the index count to the specified number.  Indexing output
stores the TS packet number (count) that contains a certain PES or ES header.
This function can be used to initialize that count.  This can only be used
on indexer filters derived from ismd_demux_open_indexer_filter.
-- this function doesn't appear to exist

@param[in] handle : Demux stream handle.
@param[in] filter_handle : indexer filter handle.
@param[in] reset_value : Specifies the value to which the index count is reset.

@retval ISMD_SUCCESS : The indexer filter's count is successfully reset.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : filter_handle is not an indexer filter.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_reset_index_count(ismd_dev_t        handle,
                                                 ismd_demux_filter_handle_t filter_handle,
                                                 unsigned int reset_value);


/**
Enables or disables indexing on a specific pid.

@param[in] handle : Demux stream handle.
@param[in] pid : Specifies the video pid to index.
@param[in] enable_indexing :  Enable indexing if true, false otherwise

@retval ISMD_SUCCESS : The indexing on a pid is successfully enabled or disabled.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_indexing_on_pid(ismd_dev_t        handle,
                                            uint16_t pid,
                                            bool enable_indexing);




/**
Enables or disables index packet counting on a specific filter.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.  This should be a filter exporting TS packets.
@param[in] enable_index_packet_counting : Enable index packet counting if true, false otherwise

@retval ISMD_SUCCESS : Counting successfully enabled on the filter.
@retval ISMD_ERROR_INVALID_HANDLE : handle or filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : filter_handle is not a TS packet filter.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_index_pkt_counting_on_filter(
                                            ismd_dev_t        handle,
                                            ismd_demux_filter_handle_t filter_handle,
                                            bool enable_index_packet_counting);


/**
Enable leaky filters on the demux.  This enables dropping of smd buffers if no
space is available on the filter output port.  It will keep dropping the buffers
till space is available is available on the output port of the filter.  This is
a property on the stream and hence all the filters for that particular stream
will be made leaky.

By default this is turned off.

@param[in] demux_handle : handle for the demux

@retval ISMD_SUCCESS : the mode was changed
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.
*/

ismd_result_t SMDEXPORT ismd_demux_enable_leaky_filters(ismd_dev_t demux_handle);

/**
Disable leaky filters on the demux.  This disables dropping of smd buffers if no
space is available on the filter output port.  This is a property on the stream
and hence all the filters for that particular stream will be made non-leaky.

By default filters on a stream are non-leaky.

@param[in] demux_handle : handle for the demux

@retval ISMD_SUCCESS : the mode was changed
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.

*/

ismd_result_t SMDEXPORT ismd_demux_disable_leaky_filters(ismd_dev_t demux_handle);



/**
Enables pvt data extraction on a single PES PID.  All the "ES Type" filters
mapped to the specified PID will get the pvt_data on the buffer descriptors.
If a PID is invalid, an error is returned.  If the private data extraction is
already enabled on the PID, the function returns no error.

This fuction causes the Demux to send private data on this PID from the stream
's (the handle used to allocate this filter) input to
the all the ES filters (which are enabled) mapped to this PID.  The specified
PID must be mapped to atleast one ES filter.  If it is mapped to multiple ES
filters, all the filters will receive private data associated with this PID
as a buffer descriptor attribute.

Private data extraction  is disabled by default on all PID's.

@param[in] handle : Demux stream handle.
@param[in] pid_val : pid on which private data extraction is to be enabled.

@retval ISMD_SUCCESS : Private data filtering is enabled on the specified PID
on all the ES filters mapped to this pid.
@retval ISMD_ERROR_INVALID_HANDLE : Device handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : If the specified PID is not mapped to
any ES filter or if the specified is not PES pid. OR If the input stream type
is not TS188 or TS192 stream type.
*/

ismd_result_t SMDEXPORT ismd_demux_enable_pes_pvt_data(ismd_dev_t handle,
                                                         uint16_t pid_val);

/**
Disables pvt data extraction on a single PES PID for a particular filter.
All the "ES Type" filters mapped to the specified PID will not get the
pvt_data on the buffer descriptors anymore.  If a PID is invalid, an error is
returned.  If the private data extraction is already disabled on the PID, the
function returns no error.

Private data extraction is disabled by default on all PID's.

@param[in] handle : Demux stream handle.
@param[in] pid_val : pid on which private data extraction is to be disabled.

@retval ISMD_SUCCESS : Private data filtering is disabled on the specified
PID on all the ES filters mapped to this pid.
@retval ISMD_ERROR_INVALID_HANDLE : Device handle is invalid.
*/

ismd_result_t SMDEXPORT ismd_demux_disable_pes_pvt_data(ismd_dev_t handle,
                                                          uint16_t pid_val);


/**
Sets the event the TSIN is to strobe when overrun occurs.  Defaults to invalid.  Can be set to invalid (ISMD_EVENT_HANDLE_INVALID) to disable event strobing.  Only one listener supported - so if the buffering monitor is being used, the client should NOT register its own event here.

@param[in] handle : Demux device handle.
@param[in] overrun_event: the handle of the event to be strobed.

@retval ISMD_SUCCESS : The client's overrun event has been successfully registered.
@retval ISMD_ERROR_INVALID_HANDLE: The handle provided is not valid.
*/

ismd_result_t SMDEXPORT ismd_tsin_set_overrun_event(ismd_dev_t handle,
                                    ismd_event_t overrun_event);




/*! @name Enabling/Disabling functions. */

/**
Enables processing of data by a filter.

NOTE:  All filters are enabled (started) by default when a the filter is
opened using ismd_demux_filter_open().  Therefore, it is not necessary to call
this function for initial configuration.

Pre-conditions required for this function:
* The handle must point to a valid demux stream
* The filter handle must point to a valid filter
* The filter must belong to a stream
* The filter must have previously been stopped using ismd_demux_filter_stop()

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.

@retval ISMD_SUCCESS : The filter was successfully started.
@retval ISMD_ERROR_INVALID_HANDLE : handle or filter_handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_start(ismd_dev_t        handle,
                                         ismd_demux_filter_handle_t filter_handle);


/**
Disables processing of data by a filter.  This will cause the filter to stop
generating output.  Stream processing of any remaining filters will continue.
This function also forces the filter to be flushed.

Pre-conditions required for this function:
* The handle must point to a valid demux stream
* The filter_handle must point to a valid filter
* The filter must belong to a stream

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.

@retval ISMD_SUCCESS : The filter was successfully stopped.
@retval ISMD_ERROR_INVALID_HANDLE : handle or filter_handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_stop(ismd_dev_t        handle,
                                        ismd_demux_filter_handle_t filter_handle);



/**
Force the filter to be flushed.

Pre-conditions required for this function:
* The handle must point to a valid demux stream
* The filter_handle must point to a valid filter
* The filter must belong to a stream

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.

@retval ISMD_SUCCESS : The filter was successfully stopped.
@retval ISMD_ERROR_INVALID_HANDLE : handle or filter_handle is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_flush(ismd_dev_t        handle,
                                        ismd_demux_filter_handle_t filter_handle);


/**
Enables processing of a PID for any filter which this is mapped to.

NOTE:  All PIDs are enabled (started) by default when a filter is mapped to
to the PID.  Therefore, it is not necessary to call this function for initial
configuration.

Pre-conditions required for this function:
* The handle must point to a valid demux stream
* The specified PID exists in the streams list of PIDs.
* This PID filter has previously been stopped by calling ismd_demux_pid_stop()

@param[in] handle : Demux stream handle.
@param[in] pid : Specifes the PID to start.

@retval ISMD_SUCCESS : The PID is successfully started.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The PID does not exist in the stream.
*/
ismd_result_t SMDEXPORT ismd_demux_pid_start(ismd_dev_t        handle,
                                         uint16_t pid);

/**
Disables processing of a PID for any filter which this PID is mapped to.
The demux will continue to process the stream for any other PIDs that have
not been stopped.

Pre-conditions required for this function:
* The handle must point to a valid demux stream
* The specified PID exists in the streams list of PIDs.

@param[in] handle : Demux stream handle.
@param[in] pid : Specifes the PID to stop.

@retval ISMD_SUCCESS : The PID is successfully stopped.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The PID does not exist in the stream.
*/
ismd_result_t SMDEXPORT ismd_demux_pid_stop(ismd_dev_t        handle,
                                        uint16_t pid);


/*! @name Crypto functions */


/**
Loads a key for the specified PID. The keys are used to
decrypt the payload data with the specified PID.  The key may be a key value,
or an 8-bit numeric identifier for a key to load from the security processor.

@param[in] handle : Demux stream handle.
@param[in] pid : Specifes the PID.  The PID should be mapped to a filter before
this function is called. To map a PID use the function
ismd_demux_filter_map_to_pids.
@param[in] algorithm : Crypto algorithm and mode.
@param[in] residual_type : For generic algorithms (DES,3DES, AES), configures how residual bytes (a partial block) are to be handled.  Ignored for Multi2 and DVB-CSA.
@param[in] key_length : Length (in bytes) of the key.
@param[in] odd_or_even : Specified whether the key is odd or even, it is only
                       effective when input stream is transport stream.
@param[in] key_data : Key data.  If the key_length is 1, key_data holds the
                      key identifieer for loading from the security processor
@param[in] iv : Initialization vector if needed, NULL otherwise.
@param[in] iv_length : Specified the Initialization Vector length in bytes.

@retval ISMD_SUCCESS : The key was loaded successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : key_length or iv_length is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The key type or the algorithm specified is not valid.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_load_key(ismd_dev_t        handle,
                                            uint16_t pid,
                                            ismd_demux_crypto_algorithm_t algorithm,
                                            ismd_demux_crypto_residual_mode_t residual_type,
                                            ismd_demux_key_type_t odd_or_even,
                                            ismd_demux_crypto_info_t key_data,
                                            unsigned int key_length,
                                            ismd_demux_crypto_info_t iv,
                                            unsigned int iv_length);


/**
Loads decryption parameters, like algorithm, IV, for the specified PID.
Those parameters along with keys are used to
decrypt the payload data with the specified PID.
Note! This function must be called before calling ismd_demux_ts_set_key
and ismd_demux_ts_set_system_key

@param[in] handle : Demux stream handle.
@param[in] pid : Specifes the PID.  The PID should be mapped to a filter before
this function is called. To map a PID use the function
ismd_demux_filter_map_to_pids.
@param[in] algorithm : Crypto algorithm and mode.
@param[in] residual_type : For generic algorithms (DES,3DES, AES), configures
how residual bytes (a partial block) are to be handled.  Ignored for Multi2 and DVB-CSA.
@param[in] iv : Initialization vector if needed, NULL otherwise.
@param[in] iv_length : Specified the Initialization Vector length in bytes.

@retval ISMD_SUCCESS : The params were loaded successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : iv_length is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The residual type or the algorithm or
iv length specified is not valid.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_crypto_params(ismd_dev_t        handle,
                                            uint16_t pid,
                                            ismd_demux_crypto_algorithm_t algorithm,
                                            ismd_demux_crypto_residual_mode_t residual_type,
                                            ismd_demux_crypto_info_t iv,
                                            unsigned int iv_length);

/**
Set decryption system key for the specified stream.
System key is only used by multi-2 and is shared by sub-elements in one stream
Note! This function must be called after ismd_demux_ts_set_crypto_params

@param[in] handle : Demux stream handle.
@param[in] skey: system key used by multi2.
@param[in] skey_len: system key length

@retval ISMD_SUCCESS : The system key was loaded successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The decrypto algorithm of specified stream
is not multi2. User should first use ismd_demux_ts_set_crypto_paras to
set the decrypto algorithm before calling this.
@retval ISMD_ERROR_INVALID_PARAMETER : skey length is invalid.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_system_key(ismd_dev_t        handle,
                                            ismd_demux_crypto_info_t skey,
                                            unsigned int skey_len);




/**
Loads a key for the specified PID. The keys are used to
decrypt the payload data with the specified PID.  The key may be a key value,
or an 8-bit numeric identifier for a key to load from the security processor.
Note! 1.This function must be called after ismd_demux_ts_set_crypto_params
      2.In case of 3DES, if key length equals to 8 or 16, key will be automatically
		  expanded to 24 bytes by appending the first 8 bytes

@param[in] handle : Demux stream handle.
@param[in] pid : Specifes the PID.  The PID should be mapped to a filter before
this function is called. To map a PID use the function
ismd_demux_filter_map_to_pids.
@param[in] key_length : Length (in bytes) of the key.
@param[in] odd_or_even : Specified whether the key is odd or even, it is only
                       effective when input stream is transport stream.
@param[in] key_data : Key data.  If the key_length is 1, key_data holds the
                      key identifieer for loading from the security processor

@retval ISMD_SUCCESS : The key was loaded successfully.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : key length or key type (odd or even) is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The handle or pid is not valid.
*/
ismd_result_t SMDEXPORT ismd_demux_ts_set_key(ismd_dev_t        handle,
                                            uint16_t pid,
                                            ismd_demux_key_type_t odd_or_even,
                                            ismd_demux_crypto_info_t key_data,
                                            unsigned int key_length);



/**
Enables crypto on the specified filter.  It has no effect if the
transport packets passing through the filter are not encrypted or the PID didn't
have an algorithm and key set for descrambling.  By default, all filters have crypto disabled.
Note: before enable crypto on a filter, the decrytion parameters and key must be set
correctly using ismd_demux_ts_set_key and ismd_demux_ts_set_crypto_params.

@param[in] handle : Demux stream handle.
@param[in] filter_handle : Demux filter handle.
@param[in] enable_decryption : Specifies whether to enable decryption on the filter or not

@retval ISMD_SUCCESS : Crypto was successfully enabled.
@retval ISMD_ERROR_INVALID_HANDLE : filter_handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : the filter_handle is an indexer filter.
*/
ismd_result_t SMDEXPORT ismd_demux_filter_set_ts_crypto(ismd_dev_t        handle,
                                                     ismd_demux_filter_handle_t filter_handle,
                                                     bool enable_decryption);


/**
Returns the value of demux information counters
@param[in] handle : Demux stream handle.
@param[in] pid : The PID, for pid specific statistics
@param[out] stats : the values of the requested information counters

@retval ISMD_SUCCESS : Stat was received.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : pid is not configured.
*/
ismd_result_t SMDEXPORT ismd_demux_get_stats(ismd_dev_t        handle,
                                         unsigned int pid,
                                         ismd_demux_stats_t *stats);

/**
Resets the value of a requested information counter
@param[in] handle : Demux stream handle.
@param[in] type : Specifies what information to reset
@param[in] pid : The PID, if resetting a PID-specific counter (e.g. ISMD_DEMUX_STAT_PES_COUNT)

@retval ISMD_SUCCESS : Stat was reset.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : type is invalid or pid is not configured.
*/
ismd_result_t SMDEXPORT ismd_demux_reset_stat(ismd_dev_t        handle,
                                    ismd_demux_stat_t type,
                                    unsigned int pid);
/*! @name NIM configuration functions. */

/**
This function configures the NIM for the specified handle.
This function is optional and won't need to be called unless the defaults
described below do not match the NIM that the TSI is connected to. <br>

The following default settings will be used if this function is not called: <br>
nim_config->nim_mode = ISMD_DEMUX_PI_NIM_PARALLEL (for all modes except 1394 input)
nim_config->nim_mode = ISMD_DEMUX_PI_NIM_SERIAL (for 1394 input)
nim_config->clk_polarity = ISMD_DEMUX_PI_POS_CLOCK
nim_config->dv_polarity = ISMD_DEMUX_PI_POS_DV
nim_config->err_polarity = ISMD_DEMUX_PI_POS_ERR
nim_config->sync_polarity = ISMD_DEMUX_PI_POS_SYNC

@param[in] handle : Demux stream handle.
@param[in] nim_config : Configuration for the NIM.

@retval ISMD_SUCCESS : The configuration was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : handle is not a valid active handle.
@retval ISMD_ERROR_NULL_POINTER : nim_config is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : This is not a PI handle or nim_config contains an invalid configuration
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_set_nim_config(ismd_dev_t        handle,
                                      ismd_demux_tsin_nim_config_t *nim_config);



/**
This function gets the overall input buffer size and the actual run-time buffer level for
a specified stream.

@param[in] handle : Demux stream handle.
@param[out] buffer_size: pointer to the overall input buffer size in bytes.
@param[out] buffer_level: pointer to the actual buffer level of the input buffer in bytes.

@retval ISMD_SUCCESS : The buffer level and buffer size have been obtained successfully.
@retval ISMD_ERROR_INVALID_HANDLE :  handle is not a valid active handle.
@retval ISMD_ERROR_NULL_POINTER : buffer_size or buffer_level were supplied as NULL.
@retval ISMD_ERROR_INVALID_REQUEST : The DMA has not yet been connected.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_get_buffer_level(ismd_dev_t        handle,
                                     unsigned int *buffer_size,
                                     unsigned int *buffer_level);



/**
Sets the physical input's buffer size for a specified stream.  The size of the buffer determines how much of a stream can be buffered in memory by the physical input in memory before stream loss occurs.

@param[in] handle : Demux stream handle.
@param[in] buffer_size: the input buffer size in bytes.

@retval ISMD_SUCCESS : The buffer size has been set successfully.
@retval ISMD_ERROR_INVALID_HANDLE :  handle is not a valid active handle.
@retval ISMD_ERROR_INVALID_REQUEST : The DMA has not yet been connected.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_set_buffer_size(ismd_dev_t        handle,
                                    unsigned int buffer_size);


/**
This function enables clock recovery based on the specified PCR PID.  If the PID was
not already added to the PID filter then it will be unless all of the
PID filter slots have already been filled, in which case an error will
be reported.  If another PID is currently selected for clock recovery,
clock recovery on that PID will be discontinued and this one will be used.
The old clock recovery PID will still be in the PID filter.

@param[in] handle : Demux stream handle.
@param[in] pcr_pid : PCR PID to use for clock recovery.

@retval ISMD_SUCCESS : The specified PCR PID will be used for clock recovery.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_NO_RESOURCES : the PID filter is full.
@retval ISMD_ERROR_INVALID_REQUEST : The selected use case does not involve the PID filter.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_enable_clock_recovery(ismd_dev_t        handle,
                                                    uint16_t pcr_pid);


/**
This function disables clock recovery.  The PCR PID will remain in the PID filter.

@param[in] handle : Demux stream handle.

@retval ISMD_SUCCESS : Clock recovery is disabled for this hande.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The selected use case does not involve the PID filter.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_disable_clock_recovery(ismd_dev_t        handle);


/**
Gets the clock.

@param[in] handle : Demux stream handle.
@param[out] clock : Pointer to the stream's clock.

@retval ISMD_SUCCESS : Clock recovery is disabled for this hande.
@retval ISMD_ERROR_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The selected use case does not involve the PID filter.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_get_clock(ismd_dev_t        handle,
                                    ismd_clock_t* clock);

/*! @name Section Filter Functions. */

/**
Add a new section filter to the system.  When matches are found, section data
will be written out the port associated with the PSI filter associated with the
section filter.  Multiple section filters for the same pid can be associated
with the same PSI filter.

@param[in] handle : Pointer to the stream handle that the filter will be
    associated with.
@param[in] filter : handle to a PSI filter created prior to this call
@param[in] pid : PID this section filter will operate upon.
@param[out] section_filter_handle : unique ID associated with this section
    filter.  When retrieving a completed section table,  this ID will be
    associated with the section table data so the client knows which section
    filter was used to find the match.

@retval ISMD_SUCCESS : Section filter was successfully set up
@retval ISMD_NULL_POINTER : section_filter structure passed in was NULL
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_open(ismd_dev_handle_t handle,
                                                ismd_demux_filter_handle_t filter,
                                                uint16_t pid,
                                                ismd_demux_section_filter_handle_t* section_filter_handle);

/**
Set the match information for a section filter.  Until called for the first
time, a newly opened section filter is inactive.

Allows configuring a matching TID and a matching byte region of the PSI section.

The sf_params variable has several methods to determine whether or not
a PSI section is allowed or discarded.  The tid field sets the expected TID
value of the PSI section.  The section_offset field controls the byte offset
into the PSI section where pattern matching will be performed. All valid bits
in the positive match must then match, and if there is a non-zero negative mask,
at least one valid bit in the negative match must not match.

As a special case, the tid field in the section params can be ignored by
configuring the section_offset field to be 0.  If at least one bit in
either positive or negative mask fields is set, the tid field is ignored and
instead the TID byte in the PSI section is treated as part of the standard
pattern match.  This enables enables filtering of multiple TIDs with while using
only one section filter.

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] section_filter_handle : unique ID of this section filter.
@param[in] sf_params: new configuration information for the section filter.

@retval ISMD_SUCCESS : Section filter was successfully deleted
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream or
                                    section_filter_handle is not valid
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_set_params(ismd_dev_handle_t handle,
                                                ismd_demux_section_filter_handle_t section_filter_handle,
                                                ismd_demux_section_filter_params_t* sf_params);

/**
Remove an existing section filter from the system.

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] section_filter_handle : unique ID of this section filter.

@retval ISMD_SUCCESS : Section filter was successfully deleted
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream or
                                    section_filter_handle is not valid
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_close(ismd_dev_handle_t handle,
                                                ismd_demux_section_filter_handle_t section_filter_handle);


/**
Start a single section filter

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] section_filter_handle : unique ID of section filter to start

@retval ISMD_SUCCESS : Section filters was successfully started
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream or
                                    section_filter_handle is not valid
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_start(ismd_dev_handle_t handle,
                                                ismd_demux_section_filter_handle_t section_filter_handle);


/**
Stop a single section filter

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] section_filter_handle : unique ID of section filter to stop

@retval ISMD_SUCCESS : Section filter was successfully stopped
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream or
                                    section_filter_handle is not valid
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_stop(ismd_dev_handle_t handle,
                                                ismd_demux_section_filter_handle_t section_filter_handle);


/**
Start all section filters associated with a given PID

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] pid : PID of section filters to start

@retval ISMD_SUCCESS : Section filters were successfully started
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream
@retval ISMD_ERROR_INVALID_PARAMETER : no section filters for this PID
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_start_all_by_pid(ismd_dev_handle_t handle,
                                                uint16_t pid);


/**
Stop all section filters associated with a given PID

@param[in] handle : Pointer to the stream handle that the filter is associated with.
@param[in] pid : PID of section filters to stop

@retval ISMD_SUCCESS : Section filters were successfully stopped
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream
@retval ISMD_ERROR_INVALID_PARAMETER : no section filters for this PID
*/
ismd_result_t SMDEXPORT ismd_demux_section_filter_stop_all_by_pid(ismd_dev_handle_t handle,
                                                uint16_t pid);


/**
Configure a buffer event notification for a demux stream.

The buffering event is triggered when two conditions are met.
1) A PTS and data have reached the demux's output.
2) The localized reference time (PCR or SCR) in the stream is equal or greater
   than buffer_time.

The first condition ensures data is flowing through the demux, and
localization of PTS timestamps has begun.

The second condition can be used to know when a a certain amount of buffering
has arrived and passed through the demux.

A previous, untriggered buffering event request can be cancelled by passing in
an event of ISMD_EVENT_HANDLE_INVALID.

This buffering event is only valid for normal, 1x forward playback.

@param[in] devhandle : Pointer to the stream handle that the filter is associated with.
@param[in] event : Event to be triggered when the buffering condition is reached
@param[in] buffer_time : Amount of stream time to buffer (in 90000 Hz units)

@retval ISMD_SUCCESS : Section filters were successfully stopped
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream
*/
ismd_result_t SMDEXPORT ismd_demux_set_buffering_event(ismd_dev_t devhandle, ismd_event_t event, unsigned buffer_time);





/**
Enable and disable the demux discontinuity hadling based on the arrival time
of the discontinuous PCRs. The newsegments generated have linear time based
on time values taken from local clock.

@param[in] devhandle : Demux stream handle
@param[in] status : true or false

@retval ISMD_SUCCESS : Section filters were successfully stopped
@retval ISMD_ERROR_INVALID_HANDLE : handle does not point to a valid stream
@retval ISMD_ERROR_INVALID_REQUEST : clock should be set in the demux to
                                     calculate arrival times
*/
ismd_result_t SMDEXPORT ismd_demux_stream_use_arrival_time(ismd_dev_t devhandle, bool status);



/**@}*/ // end of annoymous group (look for @{ near line 204)


/*! @name MSPOD Functions. */
/**
Set the LTSID (Local Trasnport Stream IDentifier) in the open cable header
The LTSID value accepted is limited to the values 0-15 as the hardware sets
the upper bits to a unique value.  Use ismd_demux_tsin_get_open_cable_ltsid
to determine the LTSID that the POD will see.  If this API is not called the
driver will automatically assign individual LTSID's for OC enabled streams.
These LTSID's can be read using ismd_demux_tsin_get_open_cable_ltsid.

The operation fails if the open cable connection is single stream.

@param[in] demux_handle : handle for the demux
@param[in] ltsid : value desired for the LTSID associated with this TSIN

@retval ISMD_SUCCESS : LTSID set for the stream
@retval ISMD_ERROR_OPERATION_FAILED : The demux stream is not connected to an
open cable source or the mode of the stream is single stream and at least one
other stream is being routed to the POD.
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid
@retval ISMD_ERROR_INVALID_PARAMETER : The ltsid value is out of range
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_set_open_cable_ltsid(ismd_dev_handle_t demux_handle,
                                                   uint8_t ltsid);


/**
Get the LTSID (Local Trasnport Stream IDentifier) in the open cable header
associated with the stream.

@param[in] demux_handle : handle for the demux
@param[out] ltsid : pointer to storage for ltsid

@retval ISMD_SUCCESS : *ltsid updated with current LTSID for the stream.
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.
@retval ISMD_ERROR_OPERATION_FAILED : The demux stream is not connected to an
open cable source or the mode of the stream is single stream.
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_get_open_cable_ltsid(ismd_dev_handle_t demux_handle,
                                                   uint8_t *ltsid);


/**
Specify the mode of the open cable stream (single stream, multi-stream,
or not an open cable stream).  For the case of not an open cable stream the
data is not sent to the POD.  By default the mode is set to
ISMD_DEMUX_NO_OPEN_CABLE.

@param[in] demux_handle : handle for the demux
@param[in] mode : specifies the mode to set for the stream

@retval ISMD_SUCCESS : the mode was changed
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : An invalid mode value was supplied
@retval ISMD_ERROR_OPERATION_FAILED : The demux stream is not connected to an
open cable source or changing to the requested mode would result in a conflict
(more than one stream in single stream mode or a combination of multi stream
and single stream)

*/
ismd_result_t SMDEXPORT ismd_demux_tsin_set_open_cable_mode(ismd_dev_handle_t demux_handle,
                                                  enum ismd_demux_open_cable_mode mode);


/**
Get the open cable mode for the supplied stream.

param[in] demux_handle : handle for the demux stream
param[out] mode : pointer to store the mode of the stream

@retval ISMD_SUCCESS : the mode was changed
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.
@retval ISMD_ERROR_NULL_POINTER : the mode pointer is NULL
*/
ismd_result_t SMDEXPORT ismd_demux_tsin_get_open_cable_mode(ismd_dev_handle_t demux_handle,
                                                  enum ismd_demux_open_cable_mode *mode);


/* The following two API's only used for testing ms-pod internally */

/* *************************************************************** */
ismd_result_t SMDEXPORT ismd_demux_tsin_enable_internal_bypass(void);
ismd_result_t SMDEXPORT ismd_demux_tsin_disable_internal_bypass(void);
/* *************************************************************** */


/**
Get the handle of clock sync.

param[in] demux_handle : handle for the demux stream
param[out] recovery_clock : pointer to clock syn

@retval ISMD_SUCCESS : clock sync handle is sussessfully gotten.
@retval ISMD_ERROR_INVALID_HANDLE : The demux handle is not valid.
@retval ISMD_ERROR_NULL_POINTER : the recovery_clock pointer is NULL.
@retval ISMD_ERROR_INVALID_REQUEST : clock sync has not been opened.
*/

ismd_result_t SMDEXPORT ismd_demux_tsin_get_recovery_clock(ismd_dev_handle_t handle,
                                    clock_sync_t *recovery_clock);

/**@}*/ // end of weakgroup ismd_demux
/**@}*/

#endif /* !ISMD_DEMUX_DEFINES_ONLY */

#ifdef __cplusplus
}
#endif

#endif
