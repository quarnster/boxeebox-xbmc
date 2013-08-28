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

#ifndef __ISMD_CORE_H__
#define __ISMD_CORE_H__

#include <stdint.h>
#include "osal.h"
#include "ismd_global_defs.h"
#include "ismd_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @weakgroup smd_core SMD Core

<H3>Overview</H3>
 
SMD Core is a set of base services used by all SMD applications and modules. It 
includes the following features:
- <b>logging configuration</b>: allows global configuration of logging services 
used by SMD modules.
- <b>device/module control</b>: defines and provides access to standard
functions that every SMD module must support.
- <b>buffer management</b>: allocation and tracking of physically-contiguous 
buffers containing media data.
- <b>ports</b>: data flow of media buffers through pipelines.
- <b>events</b>: notify applications of asynchronous events (e.g. stream errors).
- <b>clocks</b>: used to synchronize media processing elements (i.e. "A/V sync").

\dot
digraph g {
  compound=true;
  [node shape="box"]
  subgraph clusterSmdCore {
    label = "SMD Core";
	 labelloc = "b";
	 logging [label = "Logging"];
	 dev [label = "Device Control"];
	 buffers [label = "Buffer Management"];
	 Ports;
	 Events;
	 Clocks;
  }
  Modules [label = "SMD Modules"];
  Modules -> buffers [lhead=clusterSmdCore];
  Apps    -> Modules;
  Apps    -> buffers [lhead=clusterSmdCore];
}
\enddot

The SMD Core API is accessible to both applications and SMD modules/device 
drivers.

<H3>Devices/Modules</H3>

SMD exposes individual stages of media processing functionality as objects 
called 'SMD modules' or 'SMD devices.' All SMD modules rely heavily on SMD Core
and support a standard set of functions accessed via SMD Core. In addition to 
the standard set of functions, modules also expose their own, specific 
functions. 

Since SMD modules are generally designed for processing continuous streams of 
media data involving multiple stages of processing, they can be connected 
together to form processing pipelines with automatic data flow from one module 
to the next. Data flow between modules occurs via <i>ports,</i> which are 
described in more detail in the 'Ports' section, below. Module-specific 
functions provide access to each module's ports, which may then be controlled 
or connected using SMD Core's port functions.

\dot
digraph g {
  [label="Typical SMD Pipeline"]
  [node shape="box"]
  Application -> demux [label = "write_port", labelfloat = "true"];
  subgraph clusterPipeline {
    label = "Modules in Pipeline";
	 labelloc = "b";
	 viddec [label = "Video Decoder"];
	 demux [label = "TS Demux"];
	 vidpproc [label = "Video Post-processor"];
	 audio [label = "Audio Processor"];
	 vidrend [label = "Video Renderer"];
    demux -> viddec [constraint=false];
    demux -> audio [constraint=false];
	 viddec -> vidpproc [constraint=false];;
	 vidpproc -> vidrend [constraint=false];;
  }
  clock [rank = "max"];
  demux   -> clock ;
  vidrend -> clock [dir=both constraint=false];
  audio   -> clock [dir=both constraint=false];
}
\enddot

The function \ref ismd_dev_set_state allows clients to place modules in any of 
the following, standardized processing states:
- <b>stop</b>: Prevents a module from processing any input.
- <b>play</b>: Input data is processed and resulting data is output, normally.
- <b>pause</b>: Input is processed; however, if the module is a 'renderer' 
(i.e. designed to play output at precise times, the module will not output or 
present any data. For non-renderer modules, the pause and play states result in
identical behavior.

Sometimes it is necessary to clear any data currently flowing within a module 
or pipeline or to force output of any buffered data. Both actions are supported
using \ref ismd_dev_flush.

When a client no longer requires a module, it must release it using \ref 
ismd_dev_close.

Standard functions related to media timing are also defined and are described 
in the 'Clocks and Timing' section, below.


<H3>Buffers</H3>

Many SMD devices require media data to be stored in physically contiguous 
buffers. Therefore, some system memory must be reserved for use by SMD rather 
than the operating system. SMD manages buffer allocations with those 
physically-contiguous regions of memory, which are hidden from OS. 

Configuration files detailing which address ranges are assigned to SMD must be 
created by the system integrator and loaded during system boot prior to loading
the SMD Core driver. See documentation on the "Platform Configuration Database"
for information on how to create or update the configuration files. 

SMD supports 2 classes of memory regions:
- <b>linear buffer regions</b>: Linear buffer regions include a "buffer size" 
attribute. SMD divides each linear buffer region into a set of buffers of the 
specified size, similar to memory pages. Linear buffer regions are used for 
storing all SMD data other than video frame buffers and are allocated using 
\ref ismd_buffer_alloc.
- <b>frame buffer regions</b>: Frame buffer regions are used for storing video 
frame buffers. They are first sub-divided into fixed-size sub-regions. As 
clients allocate frame buffers using \ref ismd_frame_buffer_alloc, SMD divides 
the sub-regions into fixed size frame buffers, based on the requested buffer 
resolutions (i.e. widths and heights). All of the buffers stored in a 
sub-region must be the same resolution.

SMD Core maintains a reference count for each allocated buffer. Reference 
counts are decremented using \ref ismd_buffer_dereference. SMD Core returns a 
buffer to a pool when its reference count reaches zero.

\todo Document how buffer attributes are used.

<H3>Events</H3>

SMD Core supports asynchronous event notifications. Events are logical objects 
that can wake up waiting threads when triggered. SMD modules typically support 
some module-specific events, and the most common usage of events is to request 
a handle to a module event using a module-specific function and then call \ref 
ismd_event_wait or \ref ismd_event_wait_multiple to block until the event 
triggers.

<H3>Messages</H3>

Message manager offer functions to facilitate message transfer between SMD modules 
as well as SMD module and application. Message source can generate message and send
message to message manager. Message client can subscribe message from message manager.
Message manager relies on Event manager to wake up message clients.

<H3>Ports</H3>

Media data flows between SMD modules and applications via 'ports.' Module ports
may be connected together using \ref ismd_port_connect, and then SMD Core will 
automatically manage data flow between the connected ports. 

Applications may also write input data to ports or read output data from ports 
using \ref ismd_port_write and \ref ismd_port_read.

Ports can trigger data flow-related events which applications can use to 
determine when to write or read data. Clients call \ref ismd_port_attach to 
register for and configure port events.

\note Ports may not be 'attached' and 'connected' at the same time. Use 
\ref ismd_port_disconnect or \ref ismd_port_detach before changing the data 
flow mode of a port.

<H3>Clocks and Timing</H3>
 
Some SMD modules (e.g. audio and video renderers) require a time reference to 
perform their processing. <i>SMD clock</i> objects are used to coordinate 
timing between modules. SMD clocks are essentially counters. Modules or 
applications may query the counter value of a clock using \ref 
ismd_clock_get_time or use \ref ismd_clock_alarm_schedule to cause an event to 
trigger when the counter reaches a specified time.

There are two different ways to gain access to a clock:
- A new clock may be allocated/instantiated using \ref ismd_clock_alloc. Minor 
adjustments to the frequency of the clock can later be made using \ref 
ismd_clock_adjust_freq.
- Some modules expose clocks that are related to the module's operation or to a
stream flowing within a module. A module that exposes a clock must provide a 
module-specific function for requesting a handle to its clock.

The function \ref ismd_dev_set_clock sets the clock that a module should 
reference for time-related actions (e.g. output timing of video frames), and 
all modules in a pipeline should typically reference the same clock. Since the 
phase of time values within a stream are independent of the clock's phase, \ref
ismd_dev_set_stream_base_time sets a value that a module should use to 
correlate/translate timestamps within a stream to clock time.

The function \ref ismd_clock_make_primary causes the audio and display outputs 
to be synchronized with the specified clock.

*/

/** @ingroup smd_core */
/** @{ */

/******************************************************************************/
/*! @name General */

/**
Internal Function. Should not be called by clients.

@todo Move to internal header file.

@retval ISMD_SUCCESS : initialization succeeded.
@retval ISMD_ERROR_OPERATION_FAILED : initialization was not successful.
*/
ismd_result_t SMDEXPORT ismd_core_init(void);


/**
Internal Function. Should not be called by clients.

@todo Move to internal header file.

@retval ISMD_SUCCESS : SMD deinitialization succeeded.
*/
ismd_result_t SMDEXPORT ismd_core_deinit(void);


/**
Sets the verbosity level for SMD event (non-console)logging.  See the
documentation for ismd_verbosity_level_t.

@retval ISMD_SUCCESS : the level was successfully set.
*/
ismd_result_t SMDEXPORT ismd_set_logging_verbosity_level( 
                                                 ismd_verbosity_level_t level);


/**
Sets the level of debug (console) messages output from the SMD core.

 Currently:  0 = no debug output
             1 = critical error messages only
             2 = "important" info
             4 = all messages

@retval ISMD_SUCCESS : the level was successfully set.
*/
ismd_result_t SMDEXPORT ismd_set_debug_message_level(int debug_level);


/** This is used in ismd_core_statistics_t to return frame buffer statistics */
typedef struct {
   unsigned long total_fb_regions;     /**< Total number of frame buffer regions*/
   unsigned long allocated_fb_regions; /**< Total number of frame buffer regions allocated */
   unsigned long fb_regions_size;      /**< Size of a fb region*/
} ismd_core_fb_statistics_t;

/** 
Used by ismd_core_get_statistics() to return core statistical information.

Currently returns frame buffer stats, but is planned to be expanded for future statictics.
*/
typedef struct {
   ismd_core_fb_statistics_t fb_stats; /**< frame buffer statistics*/
} ismd_core_statistics_t;

/**
Get statistics of the memory

   @param[out] stat: Pointer to structure to receive the statistics

   @retval ISMD_SUCCESS: Successfully read stream statistics.
 */
ismd_result_t SMDEXPORT ismd_core_get_statistics(ismd_core_statistics_t *stat);




/******************************************************************************/
/*! @name Devices/Modules */


/**
Closes an instance of an SMD device.

Note that there is no common ismd_dev_open.  Each device is opened by calling
its open API separately, at which time the modules register themselves with the
core so that these device functions can work.

@param[in] dev : a handle to the device to close. 

@retval ISMD_SUCCESS : the device was closed successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid. 
*/
ismd_result_t SMDEXPORT ismd_dev_close(ismd_dev_t dev);


/** 
Flushes any data buffered in the device.

Devices still accept input after being flushed, so this operation is only truly
meaningful if the application disables the source of input before flushing a
device.

@param[in] dev : a handle to the device to flush. 

@retval ISMD_SUCCESS : the device was flushed successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid. 
*/
ismd_result_t SMDEXPORT ismd_dev_flush(ismd_dev_t dev);

/** 
Changes the state of an SMD device.  Note that the device might not be able to
transition to certain states (e.g. paused) unless certain pre-requisites 
have been met.

states:
   ISMD_DEV_STATE_STOP
            |
   ISMD_DEV_STATE_PAUSE
            |
   ISMD_DEV_STATE_PLAY


@param[in] dev : a handle to the device to change state. 
@param[in] state : the state the device should move to.

@retval ISMD_SUCCESS : the device successfully transitioned into the 
requested state.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid. 
@retval ISMD_ERROR_DEVICE_BUSY : the device cannot transition into the 
requested state at this time.
*/
ismd_result_t SMDEXPORT ismd_dev_set_state(ismd_dev_t       dev, 
                                           ismd_dev_state_t state); 


/** 
Gets the state of an SMD device. 

states:
   ISMD_DEV_STATE_STOP
            |
   ISMD_DEV_STATE_PAUSE
            |
   ISMD_DEV_STATE_PLAY


@param[in] dev : a handle to the device to get state. 
@param[out] state : the current state of the device.

@retval ISMD_SUCCESS : the state was successfully read from the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_get_state(ismd_dev_t       dev, 
                                           ismd_dev_state_t *state); 



/** 
Sets the reference clock for a device to use.  The device will use the 
specified clock for all internal timing functions.  The time reported by this 
clock is called the reference time for the device.  Devices that don't care 
about time will ignore the clock.

@param[in] dev : handle to a device instance. 
@param[in] clock : handle to a clock instance.

@retval ISMD_SUCCESS : the clock was successfully set for the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_set_clock(ismd_dev_t   dev,
                                           ismd_clock_t clock);



/** 
Sets the base time for the current stream.  The base time is typically the
value of the clock (which was assigned to the device) when the "play" state 
was entered.  Devices calculate time by subtracting this
value from their clock's value - when playback starts the time is 0 and it
counts up from there.
While the device is in the "PLAY" state, the stream (local) time will equal
the reference (clock) time minus the base time.  
For physical input base time should be set to the buffering time as the capture driver sets the
new segment start to 0. 

@param[in] dev : handle to a device instance. 
@param[in] base_time : time value to use as the stream base time.

@retval ISMD_SUCCESS : the base time was successfully set for the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_set_stream_base_time(ismd_dev_t  dev, 
                                                      ismd_time_t base_time);

/** 
Gets the base time for the current stream.  The base time is typically the
value of the clock (which was assigned to the device) when the "play" state 
was entered.  Devices calculate time by subtracting this
value from their clock's value - when playback starts the time is 0 and it
counts up from there.
While the device is in the "PLAY" state, the stream (local) time will equal
the reference (clock) time minus the base time.  

@param[in] dev : handle to a device instance. 
@param[out] base_time : time value of the stream base time.

@retval ISMD_SUCCESS : the base time was successfully read from the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_get_stream_base_time(ismd_dev_t  dev, 
                                                      ismd_time_t *base_time);

/**
Sets the play rate for the current stream.
The rate must be expressed as a fixed point value (when expressed in decimal) 
with 4 digits of fractional part. For example, 10000 indicates 1.0X playback;
5000 -> 0.5X, -25000 -> -2.5X, etc.

@param[in] dev : handle to a device instance. 
@param[in] current_linear_time : current linear time value.
@param[in] rate : play rate that the device is to be changed to.

@retval ISMD_SUCCESS : the play rate was successfully set for the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_set_play_rate(ismd_dev_t dev,
                                               ismd_pts_t current_linear_time,
                                               int        rate);

/**
Gets the play rate of the current stream.
The rate is expressed as a fixed point value (when expressed in decimal) 
with 4 digits of fractional part. For example, 10000 indicates 1.0X playback;
5000 -> 0.5X, -25000 -> -2.5X, etc.

@param[in] dev : handle to a device instance. 
@param[out] rate : current play rate of the device.

@retval ISMD_SUCCESS : the play rate was successfully read from the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality.
*/
ismd_result_t SMDEXPORT ismd_dev_get_play_rate(ismd_dev_t dev,
                                               int        *rate);

/**
Sets the underrun event for the specified device.  This event is meant to
be used with ISMD renderers.  It is used to signal that an underrun has 
occurred.  

The event is meant to be supplied by the ISMD buffering monitor
and only listened to by that device, but if the ISMD buffering monitor is not
being used this can be used directly by clients.  

If the ISMD bufering monitor
is being used the client should obtain the event by adding this renderer's 
device handle to the monitor, and then passing the event returned from the 
monitor into this function using the renderer device handle.  
Only one listener is supported, so if the ISMD buffering monitor is in use
the client should not register any other events here or wait on the event.  If
the client wishes to be notified of underruns, the ISMD buffering monitor
exposes a separate "underrun notification event" that forwards the notice.

The event used here is strobed by the specified device if there is an underrun,
no other oprtations are done with the event.  if this is set to 
ISMD_EVENT_HANDLE_INVALID then it effectively disables underrun eventing.
By default it is set to ISMD_EVENT_HANDLE_INVALID.

@param[in] dev : handle to a device instance. 
@param[in] underrun_event : underrun event to strobe, or 
ISMD_EVENT_HANDLE_INVALID to disable underrun event strobing.

@retval ISMD_SUCCESS : the underrun event was successfully set for the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality (non-renderer devices will return this).
*/
ismd_result_t SMDEXPORT ismd_dev_set_underrun_event(ismd_dev_t dev,
                                                    ismd_event_t underrun_event); 


/**
Gets the current underrun amount for the specified ISMD renderer.  The underrun
amount is how "late" the most recent sample was.  For on-time samples 
(non-underrun situation) this will be set to 0.  For late samples this will
be a positive amount describing how late the sample was.

@param[in] dev : handle to a device instance. 
@param[out] underrun_amount : how late the most recent sample was.  If the most
recent sample was on-time this will be 0.  it will also be 0 on startup.
Otherwise this is a positive quantity describing how late the sample was.


@retval ISMD_SUCCESS : the underrun amount was successfully read from the device.
@retval ISMD_ERROR_INVALID_HANDLE : the device handle was stale or invalid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED : the device does not support this 
functionality (non-renderer devices will return this).
*/
ismd_result_t SMDEXPORT ismd_dev_get_underrun_amount(ismd_dev_t dev,
                                                     ismd_time_t *underrun_amount); 


/******************************************************************************/
/*! @name Events */

/**
The ISMD event manager provides edge and level-sensitive events that can operate
across process boundaries.
The event manager interface includes set and reset operations for 
level-sensitive events and strobe for edge-sensitive events.  Clients can wait 
for single events or a list of events, and can acknowledge events when the 
condition causing the event has been serviced.

In general, set, reset, and strobe are used only be the source of the event
to reflect the status of the condition, and acknowledge is used only by the
event client to "clear" an event after it has been serviced.

Note that acknowledge has no effect if called between after a set with no reset,
the event remains in the active state and all waits will immediately return.
*/


/**
Allocates a new event.

@param[out] event : handle to the newly-created event. This value will not 
be modified if an event was not successfully allocated.

@retval ISMD_SUCCESS : an event was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES : there were insufficient resources available 
to allocate an event.
@retval ISMD_ERROR_INVALID_PARAMETER : event address is null
*/
ismd_result_t SMDCONNECTION ismd_event_alloc(ismd_event_t *event);


/** 
Frees a previously-allocated event.  The handle to the event should not be 
re-used after this operation completes.

@param[in] event : handle to an event. 

@retval ISMD_SUCCESS : the event was successfully freed.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
*/
ismd_result_t SMDCONNECTION ismd_event_free(ismd_event_t event);;


/** 
Waits until the specified event has been triggered.  The thread calling 
ismd_event_wait will be suspended until the event triggers or until 
the timeout elapses, whichever occurs first.  Note that the act of waking
up from a wait does not automatically clear the event, 
ismd_event_acknowledge must be used to do that once the condition that
caused the event is serviced.


@param[in] event : handle to an event.
@param[in] timeout : the maximum amount of time in milliseconds to wait for 
an event to trigger.  Specify the value ISMD_TIMEOUT_NONE to wait forever.

@retval ISMD_SUCCESS : the event triggered.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
@retval ISMD_ERROR_EVENT_BUSY : another thread is currently waiting 
on the event.  Only one thread is allowed to wait on an event at any time.
@retval ISMD_ERROR_OBJECT_DELETED : the event was deleted (freed) while 
the thread was waiting on the event.
@retval ISMD_ERROR_UNSPECIFIED : an error occured in the operating system 
infrastructure that supports ISMD events.
@retval ISMD_ERROR_TIMEOUT : the timeout period expired before the event 
triggered.
*/
ismd_result_t SMDEXPORT ismd_event_wait(ismd_event_t event, 
                                        unsigned int timeout);


/** 
Waits until any one of the specified events has triggered.  The thread calling 
ismd_event_wait will be suspended until one of the events triggers or until 
the timeout elapses, whichever occurs first.  Note that the act of waking
up from a wait does not automatically clear the event, 
ismd_event_acknowledge must be used to do that once the condition that
caused the event is serviced.

@param[in] events : a list (array) of event handles.  If multiple events 
trigger before the function returns, the first triggered event in the array 
will be notified. Thus, events are essentially prioritized from highest to 
lowest priority by position in the array.
@param[in] num_events : the number of event handles in the array.
@param[in] timeout : the maximum amount of time in milliseconds to wait for 
any one of the events to trigger.  Specify the value ISMD_TIMEOUT_NONE to wait 
forever.
@param[out] triggered_event : the handle of the highest-priority event in 
the list that actually triggered.  This handle may be used to acknowledge
the event.  Note that it is possible that more than one event in the list 
is triggered.  Modified only when the function return  value is ISMD_SUCCESS.

@retval ISMD_SUCCESS : one of the events triggered.
@retval ISMD_ERROR_INVALID_HANDLE : one of the event handles was stale or 
invalid.
@retval ISMD_ERROR_EVENT_BUSY : another thread is currently waiting  on one of
the events.  Only one thread is allowed to wait on an event at any time.
@retval ISMD_ERROR_OBJECT_DELETED : one of the events was deleted (freed) 
while the thread was waiting on the event.
@retval ISMD_ERROR_UNSPECIFIED : an error occured in the operating system 
infrastructure that supports ISMD events.
@retval ISME_ERROR_TIMEOUT_EXPIRED : the timeout period expired before an 
event triggered.
*/
ismd_result_t SMDEXPORT ismd_event_wait_multiple(
                                       ismd_event_t       events[ISMD_EVENT_LIST_MAX], 
                                       int                num_events, 
                                       unsigned int       timeout,
                                       ismd_event_t      *triggered_event);


/** 
Sets an event to its active (or triggered) state.  Any thread waiting on the 
specified event via ismd_event_wait or ismd_event_wait_multiple will be woken 
up.  Generally, there should be a single module that will be the "source" of an
event and be the only module to trigger it.

@param[in] event : a handle to the event to trigger.

@retval ISMD_SUCCESS : the event triggered successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_event_set(ismd_event_t event);


/** 
Resets an event to it's inactive (or un-triggered) state.  Generally, there 
should be a single module that will be the source of an event and be the only
module to reset it.  ismd_event_reset may be called regardless of whether the 
event is currently in the set or the reset state.

Note that ismd_event_reset will not reset any previous strobe.


@param[in] event : a handle to the event to reset.

@retval ISMD_SUCCESS : the event was reset successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_event_reset(ismd_event_t event);


/** 
Automatically strobes the state of an event to the active state and back to 
inactive state again.  This will wake up a thread waiting on the event via 
ismd_event_wait  or ismd_event_wait_multiple.  If no thread is currently 
waiting, then the very next thread to call ismd_event_wait or 
ismd_event_wait_multiple will immediately return.

Note that if the event is currently in the active state, a call to 
ismd_event_strobe will have no effect.

Limitations: This function uses semaphores and cannot be called from a
             Top-Half handler.

@param[in] event : a handle to the event to strobe.

@retval ISMD_SUCCESS : the event was strobed successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_event_strobe(ismd_event_t event);


/**
Acknowledges an event.  After a thread returns from a call to ismd_event_wait 
or ismd_event_wait_multiple, the thread should process the event and call 
ismd_event_acknowledge to reset the event notification mechanism before it 
calls ismd_event_wait or ismd_event_wait_multiple again.

@param[in] event : a handle to the event to acknowledge.

@retval ISMD_SUCCESS : the event was acknowledged successfully.
@retval ISMD_ERROR_INVALID_HANDLE : the event handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_event_acknowledge(ismd_event_t event);


/******************************************************************************/
/*! @name Messages */

/**
The ISMD Message manager offer functions to facilitate message transfer between SMD modules 
as well as SMD module and application. Message source can generate message and send
message to message manager. Message client can subscribe message from message manager.
Message manager relies on Event manager to wake up message clients.
The message manager interface for message source is message add operation to send a 
message to message manager. 
The interface for client is operation to set message filter and read message info.
*/

/**
Alloc message context

@param[out] context_id : used to return to alloced message context id, see definition in ismd_message_context_t

@retval ISMD_SUCCESS : message context was successfully allocated.
@retval ISMD_ERROR_INVALID_PARAMETER : bad parameters 
*/
ismd_result_t SMDCONNECTION ismd_message_context_alloc(ismd_message_context_t *context_id);


/**
Free message context

@param[in] context_id : message context id, see definition in ismd_message_context_t

@retval ISMD_SUCCESS : message context was successfully freed.
@retval ISMD_ERROR_INVALID_PARAMETER : bad parameters 
*/
ismd_result_t SMDCONNECTION ismd_message_context_free(ismd_message_context_t context_id);

/**
Add message

@param[in] context_id : message context id, see definition in ismd_message_context_t
@param[in] type : message type in the context
@param[in] count : how many times the message has happened 
@param[in] data : message specific data 

@retval ISMD_SUCCESS : a message was successfully added.
@retval ISMD_ERROR_INVALID_PARAMETER : bad parameters 
*/
ismd_result_t SMDEXPORT ismd_message_write(ismd_message_context_t context_id, 
                                           uint32_t type, 
                                           uint32_t count,
                                           uint64_t data);

/**
set message mask for a message context
Note: It is client's responsibility to unsubscribe a event(call this function with mask==0)
before this event is freed. Otherwise the consequence is unknow. 

@param[in] context_id : message context id, see definition in ismd_message_context_t
@param[in] event : event handler passed to message manager. This event will be notified  
when the user subscribed message arrived
@param[in] mask : mask/filter for a message context. This is a 32 bits int, specific
message will be subscribed if the corresponding bit of this mask is 1. 
If the mask is set to zero, this function is used to unsbscribe all the message
type of the message context.

@retval ISMD_SUCCESS : set event mask successfully
@retval ISMD_ERROR_INVALID_PARAMETER : bad parameters 
@retval ISMD_ERROR_NO_RESOURCES: message manager running out of resources. Each message type 
can support ISMD_MESSAGE_MAX_SUBSCRIBERS_PER_MESSAGE subscribers.      
*/
ismd_result_t SMDEXPORT ismd_message_set_mask(ismd_message_context_t context_id, 
                                              ismd_event_t event, 
                                              uint32_t mask);


/**
Read messages from a message context

@param[in] context_id : message context id, see definition in ismd_message_context_t
@param[in] event : an event handler to specify which client is reading message. Massage 
manager will use this parameter to tell and return message client is asking for. For client
this should be the triggered event returned by ismd_event_wait[multiple].
if ISMD_EVENT_HANDLE_INVALID is passed, this function will return all the available messages
of the context. This is useful in polling mode usage of message manager(Message client will
not subscribe specific message but keep polling available messages.)
@param[in] reset_flag : message count will be reset to zero if this flag is 1 
@param[out] message : used to return message info. message->num_of_items is used to return
how many items have been filled. In case of available items is more than size of info array, 
only as many of items returned to fill the array.

@retval ISMD_SUCCESS : a message was successfully added.
@retval ISMD_ERROR_INVALID_PARAMETER : bad parameters 
*/
ismd_result_t SMDEXPORT ismd_message_read(ismd_message_context_t context_id, 
                                          ismd_event_t event, 
                                          int reset_flag, 
                                          ismd_message_array_t *message);
/******************************************************************************/
/*! @name Ports */

/**
The port manager exposes APIs to connect ports to one another, or perform I/O
on ports directly.  Note that the ports themselves are only allocated by the
SMD modules and cannot be allocated or freed by the application.
*/


/**
Connects two ports together.  This operation directly connects the output of 
one port to the input of the other. One of the specified ports must be an output
port and one must be an input port. When this connection is made, data pushed 
into the output port will be seemlessly transferred into the input port.
Note that if either port had an event attached using ismd_port_attach() then
that event will be detached automatically.

@param[in]  port0 : handle to the first port.
@param[in]  port1 : handle to second port.

@retval ISMD_SUCCESS : the ports were successfully connected.
@retval ISMD_ERROR_PORT_BUSY : one or both of the ports were in a 
state where they could not be connected (e.g., they were already 
connected to other ports).
@retval ISMD_ERROR_INVALID_HANDLE : one or both of the port handles 
were not valid.
@retval ISMD_ERROR_INVALID_REQUEST: port0 and port1 were same one,
or both ports currently have a max_depth of 0.
*/
ismd_result_t SMDEXPORT ismd_port_connect(ismd_port_handle_t port0, 
                                          ismd_port_handle_t port1);

/**
Disconnects a port from an existing port connection.

@param[in]  port : handle to one of the ports in the connection.  Both
ports in the connection will be disconnected from each other.

@retval ISMD_SUCCESS : the port was successfully disconnected.
@retval ISMD_ERROR_PORT_BUSY : one or both of the ports were in a 
state where they could not be disconnected (e.g., they were not 
in a connected state).
@retval ISMD_ERROR_INVALID_HANDLE : one of the port handles was not 
valid.
*/
ismd_result_t SMDEXPORT ismd_port_disconnect(ismd_port_handle_t port);

/**
Reads an SMD buffer from a port.

@param[in]  port : handle to the port to read. 
@param[out]  buffer : a buffer handle read from the port.  This handle 
will not be updated if the read operation is not successful.


@retval ISMD_SUCCESS : a buffer was successfully read from the port.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : the port was empty and no data was 
read from the port.
@retval ISMD_ERROR_PORT_BUSY : the port's output is currently connected to 
another port; therefor, direct read operations from the port are not 
permitted.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was stale or invalid.
@retval ISMD_ERROR_INVALID_REQUEST : the port currently has a max_depth of 0.
*/
ismd_result_t SMDCONNECTION ismd_port_read(ismd_port_handle_t        port, 
                                           ismd_buffer_handle_t     *buffer);

/**
Writes an SMD buffer into a port.

@param[in]  port : handle to the port to write the buffer into. 
@param[in]  buffer : buffer handle to be written to the port.  The buffer
should have been created by ismd_buffer_alloc() or read from another port.

If changes  were made to the buffer descriptor, ismd_buffer_update_desc() should
be used to commit the changes before this is called.

@retval ISMD_SUCCESS : the buffer was successfully written into the port.
@retval ISMD_ERROR_NO_SPACE_AVAILABLE : the port was full and the buffer 
could not be written.
@retval ISMD_ERROR_PORT_BUSY : the port's input is currently connected to 
another port; therefor, direct write operations to the port are not permitted.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was stale or invalid.
@retval ISMD_ERROR_INVALID_REQUEST : the port currently has a max_depth of 0.
*/
ismd_result_t SMDCONNECTION ismd_port_write(ismd_port_handle_t   port, 
                                            ismd_buffer_handle_t buffer);


/**
Peeks into the specified port's internal queue 
without actually removing any buffers from the queue.

@param[in]  port : handle to the port to peek into. 
@param[in]  position : position of the buffer within the queue.
1=head of queue, 2=next, etc.
@param[out]  buffer : pointer to a buffer handle at the specified 
position.

@retval ISMD_SUCCESS : the buffer parameter contains a pointer to the 
buffer descriptor at the specified position in the queue.
@retval ISMD_ERROR_PORT_EMPTY : the position specified was greater than 
the number of buffers in the queue.
@retval ISMD_ERROR_INVALID_PARAMETER : the position specified was not 
valid.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was not 
valid.
*/
ismd_result_t SMDEXPORT ismd_port_lookahead( 
                                   ismd_port_handle_t          port, 
                                   int                         position,
                                   ismd_buffer_handle_t       *buffer);

/**
Sets the port's watermark.  For input ports, 
this is a low watermark, for output ports, this is a high watermark.

@param[in]  port : handle to the port to set the watermark on. 
@param[out]  watermark : watermark value to set, in buffers.

@retval ISMD_SUCCESS : the watermark was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : the watermark specified was not valid.
*/
ismd_result_t SMDEXPORT ismd_port_set_watermark( 
                                       ismd_port_handle_t  port, 
                                       int                 watermark);

/**
Retrieves configuration information about the specfied port.

@param[in]  port : handle to the port retrieve information about. 
@param[out]  status : pointer to a structure that will hold
the information about the specified port.

@retval ISMD_SUCCESS : the operation was successfull and the status structure
now contains all of the information about the specified port.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was not valid.
*/
ismd_result_t SMDEXPORT ismd_port_get_status( 
                                    ismd_port_handle_t  port, 
                                    ismd_port_status_t  *status);


/**
Registers an event to the "open" end of the port - that is, the end of the port
not being accessed by the owning SMD module.  For input ports, this is an event
for the input to the port and for output ports it is an event for the port's
output.  Only one event can be attached to a port.

This allows for event-driven I/O between the port owners and clients can take 
place.

@param[in]  port : handle to the port to which the event will be attached.
The port must not be connected to another port.
@param[in]  event : event to set when the conditions in the event mask are met.  
If this is an input port then it's the event for the "input"  end of the port.  
If this is an output port then it's the event for the "output" end of the port.
Setting the event to ISMD_EVENT_HANDLE_INVALID is functionally the same as 
calling ismd_port_detach(),
and in that case the other parameters do not matter.
@param[in]  event_mask : bitmask that controls when the event will be triggered.
See the documentation for ismd_queue_event_t for an explanation of which options
are available.
@param[in]  watermark : if a watermark event is selected, the watermark that 
triggers the event.  

For output ports, the watermark is for the 
port to tell the receiver when the port is getting full
For input ports the watermark is for telling the transmitter when the port needs
more data.

The watermark refers to the buffering level (in units of buffer count) in 
the port.  If this is an output port, this is a high watermark.  For high 
watermarks, the event is triggered when the number of buffers in the port goes 
from being less than the high watermark to being greater than or equal to the 
high watermark.  If this is an input port, this is a low watermark.  For low 
watermarks, the event is triggered when the number of buffers in the port goes 
from being greater than the low watermark to being less than or equal to the 
low watermark.
Specifying ISMD_QUEUE_WATERMARK_NONE disables watermark-based events.

@retval ISMD_SUCCESS : the operation was successfull.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was not valid.
@retval ISMD_ERROR_PORT_BUSY : the port is currently connected to another port.
@retval ISMD_ERROR_INVALID_PARAMETER : the watermark is not valid.  The low
watermark must be between 0 and port_depth-1 (inclusive).  The high watermark
must be between 1 and port_depth (inclusive).  If no watermark is desired,
use ISMD_QUEUE_WATERMARK_NONE.
@retval ISMD_ERROR_QUEUE_BUSY : the port already has an event attached.  Use
ismd_port_detach to detach it before attaching a new one.
@retval ISMD_ERROR_INVALID_REQUEST : the port currently has a max_depth of 0.
*/
ismd_result_t SMDEXPORT ismd_port_attach( 
                                ismd_port_handle_t   port, 
                                ismd_event_t         event,
                                ismd_queue_event_t   event_mask,
                                int                  watermark);

/**
Unregisters a previously registered event from this port.

@param[in]  port : handle to the port to detach an event from.  The port must
not be connected to another port.

@retval ISMD_SUCCESS : the operation was successfull.
@retval ISMD_ERROR_INVALID_HANDLE : the port handle was not valid.
@retval ISMD_ERROR_PORT_BUSY : the port is currently connected to another port.
ismd_port_disconnect() can disconnect it.
*/
ismd_result_t SMDEXPORT ismd_port_detach( ismd_port_handle_t   port);


/******************************************************************************/
/*! @name Clocking */


/** 
Allocates a new clock instance of a specified type.

@param[in] clock_type : the platform-specific clock type to allocate.
This can be ISMD_CLOCK_TYPE_FIXED or ISMD_CLOCK_TYPE_SW_CONTROLLED.
@param[out] clock : a handle to the newly-allocated clock.  Not modified if 
the operation is not successful.

@retval ISMD_SUCCESS : the clock was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to open the 
clock.
@retval ISMD_ERROR_INVALID_PARAMETER : the clock_type parameter is invalid.
*/
ismd_result_t SMDCONNECTION ismd_clock_alloc(int           clock_type, 
                                             ismd_clock_t *clock);

/**
Frees a currently allocated clock instance.

@param[in] handle : handle to the clock to free.

@retval ISMD_SUCCESS : the clock was successfully closed.
@retval ISMD_ERROR_INVALID_HANDLE : the specified clock handle was stale 
or invalid.
*/
ismd_result_t SMDCONNECTION ismd_clock_free(ismd_clock_t handle);


/**
Gets the current time value from a clock.

@param[in] clock : a handle to a clock instance.  This handle was 
originally returned from ismd_clock_alloc.
@param[out] time : the current time value of the clock in ticks.  Not 
modified if the operation is not successful.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_get_time(ismd_clock_t clock, 
                                            ismd_time_t *time);



/**
Sets the timestamp trigger source for the request clock.  This API is primarily used by
the app / modules using clock recovery.  The client module sets the timestamp 
trigger source to one of the sources specified in enum 
ismd_clock_trigger_source_t specified in the platform specific clock header 
files.  Client module can call this API multiple times to set different 
timestamp trigger sources on the same clock during the course of operation.  Of 
all the latest source would be applicable.  


@param[in] clock : a handle to a clock instance.  This handle was 
originally returned from ismd_clock_alloc.
@param[in] source : timestamp trigger source to set.  
@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_set_timestamp_trigger_source(ismd_clock_t clock, 
                                            int source);




/**
Gets the last trigger time value from a clock.  
Last trigger time value is updated on every hardware event (PCR arrival)
or it can also be updated by a software event by any module.  But for a 
particular clock only one source of trigger/update is allowed per clock.  
The client can set it to TS_IN_1/TS_IN_2/TS_IN_3/TS_IN_4 (PCR event) or 
can set it to a software trigger using ismd_clock_set_timestamp_trigger_source()

By default every clock is hooked up to TS_IN_1(PCR).

@param[in] clock : a handle to a clock instance.  This handle was 
originally returned from ismd_clock_alloc.
@param[out] time : the last trigger time value of the clock in ticks.  Not 
modified if the operation is not successful.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/


ismd_result_t SMDEXPORT ismd_clock_get_last_trigger_time(ismd_clock_t clock, 
                                            ismd_time_t *time);


/**
Triggers a software event for the requested clock.  The software trigger will 
capture the free running wall clock value to the timestamp register.  The 
timestamp value can then be read using the get_time API in clock.  There can 
be only one trigger source per clock i.e. software / other hardware sources 
which can be selected with ismd_clock_set_trigger_source().  This API will 
return ISMD_SUCCESS only if the trigger source is set to software.

@param[in] clock : a handle to a clock instance.  This handle was 
originally returned from ismd_clock_alloc.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/

ismd_result_t SMDEXPORT ismd_clock_trigger_software_event(ismd_clock_t clock);

/**
Gets the last vsync trigger time.  VSYNC register is updated with the current 
time on the vsync interrupt.  VSYNC interrupt happens at the display frame rate.

The client should set the display pipe (A/B) from which it wants to get the 
VSYNC interrupt in the clock unit.  By default each clock is initialized to 
display pipe A.

@param[in] clock : a handle to a clock instance.  This handle was 
originally returned from ismd_clock_alloc.
@param[out] time : the last vsync time value of the clock in ticks.  Not 
modified if the operation is not successful.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/

ismd_result_t SMDEXPORT ismd_clock_get_last_vsync_time(ismd_clock_t clock, 
                                                       ismd_time_t *time);



/**
Sets the current time value to a clock.

@param[in] clock_handle : a handle to a clock instance.   This handle was 
originally returned from ismd_clock_alloc.
@param[in] time : time to set the clock to, in ticks.  The clock will 
immediately  begin incrementing normally starting from this time value.

@retval ISMD_SUCCESS : the clock value was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_set_time(ismd_clock_t clock_handle, 
                                            ismd_time_t  time);

/**
Adjusts the current time value to a clock.

@param[in] clock_handle : a handle to a clock instance.   This handle was 
originally returned from ismd_clock_alloc.
@param[in] adjustment : signed amount to add to the current clock time.
If negative, the clock counter is moved backward, it is moved forward
otherwise.  The clock will immediately  begin incrementing normally 
starting from the new time.
Clock alarms scheduled during any time that gets skipped will get triggered
immediately on a forward adjustment.  For a backward adjustment, the clock
alarms will be delayed as expected.

@retval ISMD_SUCCESS : the clock value was successfully adjusted.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_adjust_time(ismd_clock_t clock_handle, 
                                               int64_t  adjustment);


ismd_result_t SMDEXPORT ismd_clock_get_time_adjust_value( ismd_clock_t handle, 
                                               int64_t  *adjustment );

/**
Sets the current time value to a clock.

@param[in] clock_handle : a handle to a clock instance.   This handle was 
originally returned from ismd_clock_alloc.
@param[in] pipe  : select from display_pipe_a (0) or display_pipe_b (1).  By default
all clocks are hooked to display pipe a.

@retval ISMD_SUCCESS : the clock value was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/

ismd_result_t SMDEXPORT ismd_clock_set_vsync_pipe(ismd_clock_t clock_handle, 
                                                  int  pipe);

/**
Schedules a one-time or repeating alarm that will occur once for each specified 
time interval until the alarm is cancelled.

@param[in] clock : a handle to clock instance.  This handle was 
originally returned from ismd_clock_alloc.
@param[in] first_time : the time in ticks the first alarm should trigger.  
If this time  is lower than the current value of the clock, the alarm will 
trigger  immediately.
@param[in] interval : the time interval in ticks between alarms.  The first 
alarm will occur at start_time, the 2nd alarm will occur at (start_time + 
interval), etc. To schedule a one-time alarm, specify an interval of 0.
@param[in] event : a handle to an ISMD event that will be triggered by the 
alarm.  This event must have been previously allocated with ismd_event_alloc().
@param[out] alarm : a pointer to a location where this function will store 
the handle to the newly-scheduled alarm.  This handle may later be used 
to cancel the alarm.

@retval ISMD_SUCCESS : the alarm was successfully scheduled.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle or event handle was stale 
or invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : the start_time and interval were 
specified such that multiple alarms occur in the past.
*/
ismd_result_t SMDEXPORT ismd_clock_alarm_schedule(ismd_clock_t        clock, 
                                                  ismd_time_t         first_time, 
                                                  ismd_time_t         interval, 
                                                  ismd_event_t        event, 
                                                  ismd_clock_alarm_t *alarm);




/**
Sets specified clock as the primary clock (clock that drives video and audio 
PLLs).  This is used in a multi-stream playback environment to select a stream
as "primary" by making its clock the dominant clock.  This has no effect
in a single-stream environment.

The reason for this is that when using multiple streams, each stream could have
its own independent clock.  Consider the case where both streams are from a
hardware source and both have independently recovered clocks.  A single clock
must be selected to drive the renderer logic, and this gives control over which
clock is selected.

No harm in calling make primary multiple times on a particular clock 
consecutively multiple times.  It would make it primary on the first call and 
not have any effect of make primary on this clock or other clocks till the 
primary clock is reset using ismd_clock_reset_primary. i.e. ismd_clock_reset_primary()
has to be called on the primary clock if another clock is to be made primary.  If 
ismd_clock_make_primary() is called on a clock, and another clock is already 
primary, the make_primary() call will return an error with "No Resources" as the 
previous primary clock is not reset yet using ismd_clock_reset_primary and still holds
the primary flag.

@param[in] clock : a handle to clock instance.  This handle was 
originally returned from ismd_clock_alloc.

@retval ISMD_SUCCESS : the clock was successfully made primary.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_make_primary(ismd_clock_t clock);



/**
Resets specified clock as the normal DDS clock .  

If the clock for which this API is called is not a primary clock it wont do
anything.  If it is primary then it would reset it to a normal DDS clock. 
No harm in calling reset on a particular clock consecutively smultiple times.

@param[in] clock : a handle to clock instance.  This handle was 
originally returned from ismd_clock_alloc.

@retval ISMD_SUCCESS : the clock was successfully reset to normal dds clock.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_reset_primary(ismd_clock_t clock);

/**
Cancels a previously-scheduled clock alarm.  When this function returns, the 
alarm is guaranteed not to occur again.  However, the alarm may still occur 
before the function returns.  The client must be capable of handling the alarm 
until this functions returns.  Also note that this function may return an 
invalid handle error if the alarm was a one-time event that already triggered.

@param[in] clock : handle to an SMD Core clock.  This handle
was originally returned from ismd_clock_alloc.
@param[in] alarm : handle to the clock alarm to cancel.  This handle was 
originally returned from ismd_clock_alarm_schedule().

@retval ISMD_SUCCESS : the alarm was successfully cancelled.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle or the alarm handle 
were stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_alarm_cancel(ismd_clock_t       clock, 
                                                ismd_clock_alarm_t alarm);


/**
Adjusts the frequency of a clock instance.

Generally this is done by hardware but in some cases where hardware clock 
recovery is not available this can be used to implement software clock 
recovery.

@param[in] clock : a handle to a clock instance.  This handle
was originally returned from ismd_clock_alloc.
@param[in] adjustment : the amount to adjust the clock frequency.  This value
is specified in parts per million.  E.g. an adjustment value of 1 makes the 
clock go faster by 0.00001%, and an adjustment value of -10000 makes the clock 
go 1% slower.  Frequency adjustments are cumulative.

@retval ISMD_SUCCESS : the clock frequency was successfully adjusted.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock_handle was not valid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED :  the clock associated with 
the specified clock_handle does not support frequency adjustments.
*/
ismd_result_t SMDEXPORT ismd_clock_adjust_freq(ismd_clock_t clock, 
                                               int          adjustment);


/**
Routes a clock to a particular TS-OUT or TS-IN.

@param[in] clock : a handle to a clock instance.  This handle
was originally returned from ismd_clock_alloc.
@param[in] destination : TS-IN or TS-OUT to be driven by the clock.

@retval ISMD_SUCCESS : the clock was successfully routed.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock_handle was not valid.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED :  the clock associated with 
the specified clock_handle does not support frequency adjustments.
*/
ismd_result_t SMDEXPORT ismd_clock_route(ismd_clock_t clock, 
                                         int          destination);



/******************************************************************************/
/*! @name Buffer Management */


/**
Allocates an SMD buffer from the media buffer address (specified in the 
\ref platform_config memory_layout)

@param[in] size : the size of the buffer to allocate.  Note that the actual 
size of the buffer allocated may be larger than the requested size.  Users 
may specify a size of 0 (zero) to allocate a buffer descriptor with no 
associated data buffer memory.
@param[out] buffer : A handle to the newly allocated buffer.  This value will 
not be modified if the operation is not successful.

@retval ISMD_SUCCESS : the buffer was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to allocate 
the buffer.
*/
ismd_result_t SMDCONNECTION ismd_buffer_alloc( 
                                 size_t                size, 
                                 ismd_buffer_handle_t *buffer);


/**
Allocates an SMD frame buffer from the media buffer address (specified in the 
\ref platform_config memory_layout).
This is a special variant of ismd_buffer_alloc() that is to be used
exclusively for allocating hardware frame buffers that have a fixed stride
(defined as a system parameter), as for example, to be used by video decoders.

@param[in]  width : the width of the frame buffer in bytes.  Note that this 
is bytes and not pixels, the client must use the pixel dimensions and the data
format to calculate the actual size in bytes. A value of 0 is not allowed.

@param[in]  height : the height of the frame buffer in bytes.  Note that this 
will probably not equal the frame height in pixels, since the UV data 
generally requires extra lines below the lowest Y line.  A value of 0 is not 
allowed.

@param[out] buffer : a handle to the newly allocated buffer.  This value will 
not be modified if the operation is not successful.

@retval ISMD_SUCCESS : the buffer was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to allocate 
the buffer.
@retval ISMD_ERROR_OPERATION_FAILED : the frame buffer manager was not yet
initialized - ismd_core_init did not complete.
*/
ismd_result_t SMDCONNECTION ismd_frame_buffer_alloc(   
                                       size_t                     width,
                                       size_t                     height,
                                       ismd_buffer_handle_t      *buffer);

/**
Allocates an SMD frame buffer from the media buffer address (specified in the 
\ref platform_config memory_layout).
This is a special variant of ismd_frame_buffer_alloc() that is
to be used if a frame buffer needs to be associated with a context.  A context
is a way to indicate that buffers from another context can't share the same
area of memory as buffers from this context.
The reason for doing this is to keep one context's buffers from mixing with 
another stream's buffers, in order to avoid fragmentation.  Calling 
ismd_frame_buffer_alloc is equivalent to calling this and specifying the context
as ISMD_FRAME_BUFFER_MANAGER_NO_CONTEXT.

@param[in]  width : the width of the frame buffer in bytes.  Note that this 
is bytes and not pixels, the client must use the pixel dimensions and the data
format to calculate the actual size in bytes. A value of 0 is not allowed.

@param[in]  height : the height of the frame buffer in bytes.  Note that this 
will probably not equal the frame height in pixels, since the UV data 
generally requires extra lines below the lowest Y line.  A value of 0 is not 
allowed.

@param[in]  context : the context to associate this buffer to.  The memory
region that this buffer resides in can only be used to allocate other buffers
from the same context.

@param[out] buffer : A handle to the newly allocated buffer.  This value will 
not be modified if the operation is not successful.

@retval ISMD_SUCCESS : the buffer was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to allocate 
the buffer.
@retval ISMD_ERROR_OPERATION_FAILED : the frame buffer manager was not yet
initialized - ismd_core_init did not complete.
@retval ISMD_ERROR_INVALID_PARAMETER
*/
ismd_result_t SMDCONNECTION ismd_frame_buffer_alloc_with_context(   
                                       size_t                     width,
                                       size_t                     height,
                                       int                        context,
                                       ismd_buffer_handle_t      *buffer);

/**
Makes an alias (another buffer pointing to the same physical data) to an 
already existing buffer.  The alias looks exactly like
the buffer_to_alias except that it has a different unique_id field.
The buffer_to_alias's reference count is increased, and when the alias is
freed the buffer_to_alias's reference count is decreased.
NOTE: If the buffer_to_alias is freed before its aliases are, the aliases are
not automatically freed.  This should not be an issue because the 
buffer_to_alias had its reference count incremented when the alias was created.

@param[in] buffer_to_alias : Original buffer that the alias is being made to.
This is allowed to be an aliased buffer (you can alias an alias).
@param[out] alias_buffer : alias to the original buffer, identical in every way
except for the unique_id field.

@retval ISMD_ERROR_INVALID_HANDLE :  buffer_to_alias was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to allocate 
the buffer.
*/
ismd_result_t SMDCONNECTION ismd_buffer_alias( 
                                       ismd_buffer_handle_t    buffer_to_alias,
                                       ismd_buffer_handle_t   *alias_buffer);

/**
Creates an alias just as ismd_buffer_alias() does, with extra control over
how the alias is created.  The start_offset and size parameters are used to
specify a subset of the buffer that the alias represents.  The base address
of the new buffer will be offset by the original buffer's base address, 
and the size of th enew buffer will be equal to the size parameter.  Alias
buffers obtained from this function behave the same way as aliases created with
ismd_buffer_alias().

@param[in] buffer_to_alias : Original buffer that the alias is being made to.
This is allowed to be an aliased buffer (you can alias an alias).
@param[in] start_offset : offset from the original buffer's phys.base that the
alias buffer's phys.base will be at.  Is allowed to be 0, 
the original buffer's phys.size, and everything in between.
@param[in] size : the data size of the alias buffer.  Is allowed to be 0, 
the original buffer's phys.size - start_offset, and everything in between.
@param[out] alias_buffer : alias to the original buffer, identical in every way
except for the unique_id field, and possibly the phys.base and phys.size fields.

@retval ISMD_ERROR_INVALID_HANDLE :  buffer_to_alias was stale or invalid.
@retval ISMD_ERROR_INVALID_REQUEST :  start_offset or start_offset + size was 
greater than the original buffer's phys.size field.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to allocate 
the buffer.
*/
ismd_result_t SMDCONNECTION ismd_buffer_alias_subset( 
                                       ismd_buffer_handle_t    buffer_to_alias,
                                       size_t                  start_offset,
                                       size_t                  size,
                                       ismd_buffer_handle_t   *alias_buffer);

/**
Increments the reference count for a buffer.  As long as the count is non-zero
the buffer will not be recyled/overwritten etc.

@param[in] buffer : handle to an SMD buffer.

@retval ISMD_SUCCESS : the buffer reference count was successfully incremented
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
*/
ismd_result_t SMDCONNECTION ismd_buffer_add_reference( 
                                          ismd_buffer_handle_t buffer);

/**
Decrements the reference count for a buffer.  When the count reaches zero, 
the buffer's release_function will be called to notify the buffer's owner 
that the buffer is no longer being accessed.

@param[in] buffer : handle to an SMD buffer.

@retval ISMD_SUCCESS : the buffer was successfully dereferenced.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
*/
ismd_result_t SMDCONNECTION ismd_buffer_dereference(ismd_buffer_handle_t buffer);


/**
Releases a buffer back to SMD Core regardless of the 
buffer's reference_count or release_function values.  Users should apply 
caution when calling this function as it could cause negative side 
effects if another units is keeping state information for a freed buffer.

@param[in] buffer : handle to an SMD buffer.

@retval ISMD_SUCCESS : the buffer was successfully freed.
*/
ismd_result_t SMDCONNECTION ismd_buffer_free(ismd_buffer_handle_t buffer);



/**
Gets the level of the data in the buffer.

@param[in] buffer : handle to an SMD buffer.
@param[out] level : data level (in bytes) in the buffer.

@retval ISMD_SUCCESS : the level was read successfully.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_buffer_get_level(ismd_buffer_handle_t buffer, 
                                              int                 *level);


/**
Reads the descriptor data of an SMD buffer.  This allows a client to access 
all of the buffers attributes (e.g. metadata).

@param[in] buffer : handle to an SMD buffer.
@param[out] desc : the buffer descriptor information.  This information will 
not be modified if the operation is not successful.
Note that this is a copy of the actual buffer descriptor.  If changes are
made to it, use the ismd_buffer_update_desc() function to commit those
changes for other entities.

@retval ISMD_SUCCESS : the buffer information was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_buffer_read_desc( 
                                     ismd_buffer_handle_t      buffer, 
                                     ismd_buffer_descriptor_t *desc);

/**
Updates the internal SMD buffer descriptor with the local modifications.

@param[in] buffer : handle to an SMD buffer.
@param[in] desc : pointer to local descriptor containing updated data that
is to be committed.
The following sections of the descriptor are not allowed to change and therefore
are not updated with this function:
 * buffer type
 * physical base address
 * virtual base address
 * size
 * unique id
 * next
 * reference count - use ismd_buffer_add_reference and ismd_buffer_dereference
 * state
 * tags_present

@retval ISMD_SUCCESS : the buffer information was successfully updated.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NULL_POINTER : the desc parameter was NULL.
@retval ISMD_ERROR_INVALID_REQUEST:  the buffer level specified in the desc 
structure was greater than the physical size of the buffer.
*/
ismd_result_t SMDEXPORT ismd_buffer_update_desc( 
                                       ismd_buffer_handle_t      buffer,
                                       ismd_buffer_descriptor_t *desc);


/**
This is a deprecated API and available only for backward compatibility. The 
ismd_tag_set_newsegment_position should be used.

Sets a newsegment tag on the specified buffer.  The first data in this buffer
and data in subsequent buffers will use the new segment information.
If this buffer already has a newsegment tag, it will be overwritten with the
new one. 
Please note that, the newsegment_data.segment_position field
will be set to invalid automatically. Application writers are encouraged to
use ismd_tag_set_newsegment_position to set the newsegment tag.

@param[in] buffer : handle to an SMD buffer.
@param[in] newsegment_data : the newsegment parameters.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_newsegment(
                        ismd_buffer_handle_t       buffer,
                        ismd_newsegment_tag_t      newsegment_data);

/**
Sets a newsegment tag on the specified buffer.  The first data in this buffer
and data in subsequent buffers will use the new segment information.
If this buffer already has a newsegment tag, it will be overwritten with the
new one. 
The difference between ismd_tag_set_newsegment_position and ismd_tag_set_newsegment is that, the
latter ignores the segment_position field by replacing any value passed in with ISMD_NO_PTS. We 
encourage applications use this function instead of ismd_tag_set_newsegment.

@param[in] buffer : handle to an SMD buffer.
@param[in] newsegment_data : the newsegment parameters.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_newsegment_position(
                        ismd_buffer_handle_t       buffer,
                        ismd_newsegment_tag_t      newsegment_data);

/**
Reads the newsegment tag from the specified buffer, if one exists.

@param[in] buffer : handle to an SMD buffer.
@param[out] newsegment_data : the read newsegment parameters.

@retval ISMD_SUCCESS : the buffer tag was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : this buffer does not have a newsegment tag.
*/
ismd_result_t SMDEXPORT ismd_tag_get_newsegment(
                        ismd_buffer_handle_t       buffer,
                        ismd_newsegment_tag_t     *newsegment_data);


/**
Sets an EOS tag on the specified buffer.  The actual end of the stream is defined
as the last byte in the buffer.
If this buffer already has an EOS tag, it will be overwritten with the
new one.

@param[in] buffer : handle to an SMD buffer.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_eos(
                        ismd_buffer_handle_t       buffer);


/**
Reads the EOS tag from the specified buffer, if one exists.  Note that there
is no data associated with the EOS tag.

@param[in] buffer : handle to an SMD buffer.

@retval ISMD_SUCCESS : the buffer tag was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : this buffer does not have an EOS tag.
*/
ismd_result_t SMDEXPORT ismd_tag_get_eos(
                        ismd_buffer_handle_t       buffer);


/**
Sets an attribute change tag on the specified buffer.  
The first data in this buffer and data in subsequent buffers will use the new
attributes.
If this buffer already has a dynamic attributes tag, it will be overwritten with
the new one.

@param[in] buffer : handle to an SMD buffer.
@param[in] attrs : the new attributes.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_dyn_attrs(
                        ismd_buffer_handle_t       buffer,
                        ismd_dyn_input_buf_tag_t   attrs);


/**
Reads the attribute change tag from the specified buffer, if one exists.

@param[in] buffer : handle to an SMD buffer.
@param[out] attrs : the read attribute change parameters.

@retval ISMD_SUCCESS : the buffer tag was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : this buffer does not have an attributes
change tag.
*/
ismd_result_t SMDEXPORT ismd_tag_get_dyn_attrs(
                        ismd_buffer_handle_t       buffer,
                        ismd_dyn_input_buf_tag_t  *attrs );


/**
Sets a client ID tag on the specified buffer.  This ID is not interpreted by the
SMD core and is intended for the application to use for tracking data through
the pipeline.
If this buffer already has an application ID tag, it will be overwritten with
the new one.

@param[in] buffer : handle to an SMD buffer.
@param[in] id : the ID.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_client_id(
                        ismd_buffer_handle_t       buffer,
                        int                        id);


/**
Reads the client ID tag from the specified buffer, if one exists.

@param[in] buffer : handle to an SMD buffer.
@param[out] id : the read ID.

@retval ISMD_SUCCESS : the buffer tag was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : this buffer does not have an ID tag
*/
ismd_result_t SMDEXPORT ismd_tag_get_client_id(
                        ismd_buffer_handle_t       buffer,
                        int                       *id);


/**
Sets reclaim event tag on the specified buffer.  This is an event that will be
set when the buffer is reclaimed back to the SMD core (it is no longer used).

If this buffer already has a reclaim event tag, it will be overwritten with
the new one.

@param[in] buffer : handle to an SMD buffer.
@param[in] event : the event to set on buffer reclaim.

@retval ISMD_SUCCESS : the buffer tag was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_RESOURCES : this buffer already has the maximum number of 
tags, no more can be added.
*/
ismd_result_t SMDEXPORT ismd_tag_set_reclaim_event(
                        ismd_buffer_handle_t       buffer,
                        ismd_event_t               event);


/**
Reads the buffer reclaim tag from the specified buffer, if one exists.

@param[in] buffer : handle to an SMD buffer.
@param[out] event : the read event.

@retval ISMD_SUCCESS : the buffer tag was successfully read.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
@retval ISMD_ERROR_NO_DATA_AVAILABLE : this buffer does not have a buffer 
reclaim tag.
*/
ismd_result_t SMDEXPORT ismd_tag_get_reclaim_event(
                        ismd_buffer_handle_t       buffer,
                        ismd_event_t              *event);


/**
Copies all tags from one buffer to another.  This is useful for devices that need
to forward events to new buffers without knowing about all possible events that
need to be forwarded.

If the destination buffer already has
tags of the same type as the tags in the source buffer, they will be 
overwritten.  Tags on the destination buffer that do not exist on the source
buffer are not overwritten.


@param[in] from_buffer : handle to the source SMD buffer.
@param[in] to_buffer : handle to the destination SMD buffer.

@retval ISMD_SUCCESS : the buffer tags were successfully copied.
@retval ISMD_ERROR_INVALID_HANDLE :  one of the buffer handles was stale or 
invalid.
@retval ISMD_ERROR_NO_RESOURCES : the destination buffer already has the 
maximum number of tags, some of the tags could not be added to it.
*/
ismd_result_t SMDEXPORT ismd_tag_copy_all(
                        ismd_buffer_handle_t  from_buffer,
                        ismd_buffer_handle_t  to_buffer);


/**
Copies all tags from one buffer to another, except for EOS tags.  
This is useful for devices that need to forward events to new buffers 
without knowing about all possible events that need to be forwarded, but want
to handle EOS separately (dur to potential buffer re-ordering issues).

If the destination buffer already has
tags of the same type as the tags in the source buffer, they will be 
overwritten.  Tags on the destination buffer that do not exist on the source
buffer are not overwritten.


@param[in] from_buffer : handle to the source SMD buffer.
@param[in] to_buffer : handle to the destination SMD buffer.
@param[out] copied_any_tags : Indicate whether any tags were actually copied.


@retval ISMD_SUCCESS : the buffer tags were successfully copied.
@retval ISMD_ERROR_INVALID_HANDLE :  one of the buffer handles was stale or
invalid.
@retval ISMD_ERROR_NO_RESOURCES : the destination buffer already has the
maximum number of tags, some of the tags could not be added to it.
*/
ismd_result_t SMDEXPORT ismd_tag_copy_all_except_eos(
                        ismd_buffer_handle_t  from_buffer,
                        ismd_buffer_handle_t  to_buffer,
                        bool                 *copied_any_tags);


/**
Query the largest available continuous block of DRAM avaiable to the OS

   @retval size of the largest available continuous block of DRAM avaiable to the OS
 */
size_t SMDEXPORT ismd_core_query_largest_available_block(void);

/**@}*/ // end of ingroup smd_core
/**@}*/ // end of weakgroup smd_core


#ifdef __cplusplus
}
#endif

#endif  /* __ISMD_CORE_H__ */
