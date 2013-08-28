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

/** @file ismd_tsout.h
 * This file declares the SMD APIs for controlling TSOut interface
 */

#ifndef __ISMD_TSOUT_H__
    #define __ISMD_TSOUT_H__

    #include "ismd_msg.h"
    #include "osal.h"
    #include "pal.h"
    #include "ismd_global_defs.h"
    #include "sven_devh.h"

    #ifdef __cplusplus
extern "C" {
    #endif

/** @weakgroup ismd_tsout Transport Stream Output
The Transport Stream Output (TSOut) is a timed hardware interface used to transmit a MPEG-2 transport stream.

The Transport Stream Output (tsout) module allows isochronous transmission of
MPEG2 transport streams via physical transport stream interfaces.

\b Operation

The data to be transmitted may be supplied as a standard, MPEG2 transport stream (TS)
containing 188-byte packets or as a timed transport stream (TTS) consisting of 192-byte
packets containing 32-bit timestamp headers.

For timed transport streams containing timestamp headers, each packet is transmitted
when its timestamp matches an internal transmission counter. The internal
transmission counters value is set when the \ref ismd_tsout_set_basetime API is called
to set the basetime for the TSOut module.

For standard streams not containing timestamp headers, packets containing PCR
values will be transmitted at the relative times indicated by the PCRs, with
non-PCR packets being approximately evenly spaced between PCR packets.

The clock to drive the internal transmission counter must be provided using \ref ismd_tsout_set_clock.

 * TSOut characteristics (signal polarity, Transmission rate, etc) are set using platform_config

The following is the typical programming sequence to use tsout:
-# Request access to a specific transport stream interface, using \ref ismd_tsout_open. The stream
type (TS or TTS) is specified at this time.
-# Get a handle to the input port for the newly opened TSOut instance. This handle is used to \ref ismd_port_connect
the TSOut module to other SMD modules to source data to the TSOut interface.
-# Set the clock to be used for controlling transmission timing using \ref
ismd_tsout_set_clock. If the stream to be transmitted is currently being received
via a "push mode", the clock used for TSOut should be the clock recovered from this stream.
-# For TS streams where PCRs need to be used to determine packet transmission times, the PID carrying the
PCRs needs to be set. This is done by calling the \ref ismd_tsout_ts_set_pcr_pid API.
-# Optionally, if required, \ref ismd_tsout_register_event API can be used to register for various interesting events (e.g. End of Stream)
-# Set the basetime for the TSOut module to the same basetime used by the other modules in the pipeline using the \ref ismd_tsout_set_basetime API
-# Place the tsout instance in the "playing" state using \ref ismd_dev_set_state.
-# Begin writing stream data into the input port of the tsout instance.
-# When transmission is complete, close the tsout instance using \ref ismd_dev_close.

This programming sequence is available for reference in the form of 
sample player application ../smd_sample_apps/tsout_streamer/tsout_streamer.c.
*/

/** @ingroup ismd_tsout */
/**@{*/

    /**
     * Supported Stream Types
     */
    typedef enum
    {
        ISMD_TSOUT_DATA_TYPE_INVALID   = 0,
        ISMD_TSOUT_DATA_TYPE_MPEG2_188 = 188, /**< (TS) Standard 188-byte transport stream packets */
        ISMD_TSOUT_DATA_TYPE_MPEG2_192 = 192  /**< (TTS) Transport stream packets prefixed with 4 byte timestamps */
    } ismd_tsout_data_type_t;

    /**
     * Supported Event notifications
     */
    typedef enum
    {
        // One Bit per Event
        TSOUT_EVENT_EOS = 0x1, /**< End of File/Stream event Notification */
    } ismd_tsout_event_type_t;

/**
Opens the specified TSOut interface and returns a handle to it.

Buffers written to the input port of the interface will be transmitted via the corresponding
TS interface. The 'data_type' parameter specifies Stream type. The MPEG2 Transport streams (TS)
use the 188 byte packets. For the TS streams, the embedded PCRs are used for computing the
timestamps for transmitting the packets. The MPEG2 Timed transport streams (TTS) use
the 192 byte packets with an embedded 4 byte timestamp for each packet. For TTS, the embedded
timestamps are used for transmitting the packets.

@param[in] interface : Specifies the TS output interface to open. (e.g. 0)
@param[in] data_type : Specifies the data format of data to be transmitted. (e.g. \ref ISMD_TSOUT_DATA_TYPE_MPEG2_188)
@param[out] dev : On successful return, will hold an open SMD device handle to the requested TS output interface.

@retval ISMD_SUCCESS : Handle was successfully obtained.
@retval ISMD_ERROR_NULL_POINTER : dev was supplied as a NULL pointer.
@retval ISMD_ERROR_INVALID_PARAMETER : stream_type is invalid.
@retval ISMD_ERROR_NO_RESOURCES : Interface is in use.
*/
    ismd_result_t SMDEXPORT ismd_tsout_open( int                    interface,
                                             ismd_tsout_data_type_t data_type,
                                             ismd_dev_t             *dev);


/**
Closes a previously opened TSOut instance.
It will take all the actions required to prepare the instance for being closed down if required.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/
    ismd_result_t SMDEXPORT ismd_tsout_close(ismd_dev_t tsout_handle);


/*! @name Basic Configuration functions. */

/**
For a standard MPEG2 Transport Stream with 188 byte packets, the embedded PCRs are
used to compute the timestamps used for transmitting the packets. In this case,
the PID carrying the PCRs needs to be specified.

@param[in] tsout_handle : TSOut instance handle returned by \ref ismd_tsout_open
@param[in] pcr_pid : Specifies the PCR PID for the stream.

@retval ISMD_SUCCESS : The PID is successfully set.
@retval ISMD_INVALID_HANDLE : handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/
    ismd_result_t SMDEXPORT ismd_tsout_ts_set_pcr_pid(ismd_dev_t    tsout_handle,
                                                      uint16_t      pcr_pid);

/**
Sets the clock source to be used by the TSOut instance. Generally, all stages of
the pipeline should share the same clock source.

@param[in] tsout_handle : TSOut instance handle returned by \ref ismd_tsout_open
@param[out] clock : clock handle to be used.

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/
    ismd_result_t SMDEXPORT ismd_tsout_set_clock(ismd_dev_t        tsout_handle,
                                                 ismd_clock_t      clock);

/**
Sets the basetime on the TSOut instance.

@param[in] tsout_handle : TSOut instance handle returned by \ref ismd_tsout_open
@param[in] basetime : Basetime to be set on the instance

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.

*/
    ismd_result_t SMDEXPORT ismd_tsout_set_basetime(ismd_dev_t      tsout_handle,
                                                    ismd_time_t     basetime);


/**
Gets the handle to the input port for the given TSOut instance.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open
@param[out] port : pointer to the variable to hold the input port handle.

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/

    ismd_result_t SMDEXPORT ismd_tsout_get_input_port(ismd_dev_t            tsout_handle,
                                                      ismd_port_handle_t    *port);


/**
Registers an event for the TSOut driver to set when the specified event occurs.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open
@param[in] event: event to be registered for the tsout instance.
@param[in] type: event mask for the tsout instance. (\ref ismd_tsout_event_type_t)


@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/


    ismd_result_t SMDEXPORT ismd_tsout_register_event(ismd_dev_t                tsout_handle,
                                                      ismd_event_t              event,
                                                      ismd_tsout_event_type_t   type);



/**
Flushes any data buffered in the device.

Devices still accept input after being flushed, so this operation is only truly
meaningful if the application disables the source of input before flushing a
device or sets the state to PAUSE/STOP before flushing.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : tsout handle provided resulted in an invalid instance.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
*/

    ismd_result_t SMDEXPORT ismd_tsout_flush(ismd_dev_t tsout_handle);


/**
Sets the state of the tsout. It is used to play, stop, pause and resume the
tsout instance.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open
@param[in] state : state to transition to.

@retval ISMD_SUCCESS : The state was set successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_REQUEST : The tsout is running.
*/
    ismd_result_t SMDEXPORT ismd_tsout_set_state(ismd_dev_t tsout_handle, ismd_dev_state_t state);


/**
Enables output port support for the TSOut instance. When the output port is enabled,
any output sent to the TSOut physical interface is \a also sent to the output port.
Another module or an application could read the data from the output port to extract
a copy of the data as it is sent to the TSOut hardware interface. This data is in the
Timed Transport Stream (TTS) format and contains a 32bit timestamp for every packet.

@note 
-# <b>This is a DEBUG/TEST interface and Should NOT be used for production code. It imposes serious performance degradation.
-# The Output port is currently supported ONLY for the ISMD_TSOUT_DATA_TYPE_MPEG2_192 stream type.
-# Output Port is disabled by default.</b>

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open
@param[in] port_depth : depth of the output port (should be greater then 0 and less than 256)
@param[out] output_port : output port handle for this instance.

@retval ISMD_SUCCESS : The operation successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : One of the provided parameters has an invalid value.
@retval ISMD_ERROR_INVALID_REQUEST : The request is not valid in current state.
@retval ISMD_ERROR_NULL_POINTER : The output_port pointer is NULL.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : Output port not supported for current configuration.

*/

    ismd_result_t SMDEXPORT ismd_tsout_enable_output_port(ismd_dev_t tsout_handle,
                                                          int port_depth,
                                                          ismd_port_handle_t *output_port);


/**
Disables the output port on TSOut instance.

@param[in] tsout_handle: TSOut instance handle returned by \ref ismd_tsout_open

@retval ISMD_SUCCESS : The state was set successful.
@retval ISMD_ERROR_INVALID_HANDLE : The tsout handle is invalid.

*/

    ismd_result_t SMDEXPORT ismd_tsout_disable_output_port(ismd_dev_t tsout_handle);

/**@}*/ // end of ingroup ismd_tsout
/**@}*/ // end of weakgroup ismd_tsout

    #ifdef __cplusplus
}
    #endif

#endif /* __ISMD_TSOUT_H_ */
