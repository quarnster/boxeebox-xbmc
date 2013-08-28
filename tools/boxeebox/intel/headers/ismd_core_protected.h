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

#ifndef __ISMD_CORE_PROTECTED_H__
#define __ISMD_CORE_PROTECTED_H__

#include <stdint.h>
#include "osal.h"
#include "ismd_global_defs.h"
#include "ismd_msg.h"
#include "platform_config.h"
#include "platform_config_paths.h"

/**
This file exposes core interfaces that are not meant to be exposed
directly to user applications, but can be used by other modules
or tests.
*/

////////////////////////////////////////////////////////////////////////////////
// Shortcut macros for accessing core platform configuration properties
////////////////////////////////////////////////////////////////////////////////

#define ISMD_CORE_INT_PROPERTY(name) ({                                         \
   config_result_t icipret = CONFIG_SUCCESS;                                    \
   int icipint = 0;                                                             \
   config_ref_t icip_attr_id = ROOT_NODE;                                       \
                                                                                \
   icipret = config_node_find(ROOT_NODE,                                        \
                              CONFIG_PATH_SMD_CORE,                             \
                              &icip_attr_id);                                   \
   if (icipret == CONFIG_SUCCESS) {                                             \
      icipret = config_get_int(icip_attr_id, name, &icipint);                   \
   }                                                                            \
                                                                                \
   if (icipret != CONFIG_SUCCESS) {                                             \
      OS_INFO("Error! %s undefined!\n", name);                                  \
   }                                                                            \
   icipint;                                                                     \
})

#define SYSTEM_STRIDE             ISMD_CORE_INT_PROPERTY("frame_buffer_properties.stride")
#define FRAME_REGION_BLOCK_HEIGHT ISMD_CORE_INT_PROPERTY("frame_buffer_properties.region_height")
#define TILE_V                    ISMD_CORE_INT_PROPERTY("frame_buffer_properties.tile_height")
#define TILE_H                    ISMD_CORE_INT_PROPERTY("frame_buffer_properties.tile_width")
#define ALLOW_OVERLAP             ISMD_CORE_INT_PROPERTY("allow_memory_overlap")

////////////////////////////////////////////////////////////////////////////////
// Internal defines for the ports.  Since the application is not allocating or
// setting config on them, they are in this file.
////////////////////////////////////////////////////////////////////////////////

#define SMD_PORT_NAME_LENGTH 50         /* Max name length when transferred thru SVEN is 19. */

/**
Port types.  These are used when allocating ports.
*/
typedef enum {
   ISMD_PORT_TYPE_INVALID = 0, /** Cannot be used */
   ISMD_PORT_TYPE_INPUT   = 1, /** Input port (relative to a module).  Callbacks are for reading data out of a port when data enters it (push), uses high waterrmark. */
   ISMD_PORT_TYPE_OUTPUT  = 2  /** Output port (relative to a module).  Callbacks are for writing data into a port when data exits it (pull), uses low waterrmark. */
} ismd_port_type_t;


/**
Port attributes are communicated through flags defined below
*/
typedef uint32_t ismd_port_attributes_t;

#define ISMD_PORT_QUEUE_OFF                (0 << 0)  /** Disable buffer queueing on this port. Not implemented. */
#define ISMD_PORT_QUEUE_ON                 (1 << 0)  /** Enable buffer queueing on this port. */

#define ISMD_PORT_QUEUE_MAX_DEPTH_BYTES    (0 << 1)  /** Not supported: Maximum queue depth and watermarks are specified in number of bytes. */
#define ISMD_PORT_QUEUE_MAX_DEPTH_BUFFERS  (1 << 1)  /** Maximum queue depth and watermarks are specified in number of buffers. */

#define ISMD_PORT_QUEUE_DISCONNECT_BLOCK   (0 << 2)  /** Writes to the output queue fail when queue is full and disconnected from any input port. */
#define ISMD_PORT_QUEUE_DISCONNECT_DISCARD (1 << 2)  /** Writes to the output queue succeed when queue is full and disconnected from any input port.  The oldest data is discarded. */


/**
Used to set the port configuration.  Used by ismd_port_alloc()
and ismd_port_set_config().
*/
typedef struct {
   ismd_event_t           event;             /** Event to set when the conditions in the event mask are met.
                                                 If this is an input port then it's the event for the "output"
                                                 end of the port.  If this is an output port then it's the
                                                 event for the "input" end of the port. */
   ismd_queue_event_t     event_mask;        /** Mask that controls when the event will be triggered. */
   ismd_port_attributes_t attributes;        /** Flags that specify attributes of the port (a bitwise-or of values from ismd_port_attributes_t). */
   int                    max_depth;         /** Maximum depth of the queue, queue will refuse data if depth would become larger than this amount Minimum is 1 */
   int                    watermark;         /** Queue depth when a watermark event is generated.  For input ports,
                                                 this is a high watermark, for output ports, this is a low watermark. The low
                                                 watermark must be between 0 and port_depth-1 (inclusive).  The high watermark
                                                 must be between 1 and port_depth (inclusive). If no watermark is desired,
                                                 use ISMD_QUEUE_WATERMARK_NONE.*/
} ismd_port_config_t;


/**
Used to get the port "descriptor", which is a more advanced status
of the port than the get_status exposed function does.
*/
typedef struct {
   ismd_port_type_t       port_type;         /** Type of port (e.g. input or output) */
   ismd_event_t           event;             /** Event to set when the conditions in the event mask are met.
                                                 If this is an input port then it's the event for the "output"
                                                 end of the port.  If this is an output port then it's the
                                                 event for the "input" end of the port. */
   ismd_queue_event_t     event_mask;        /** Mask that controls when the event will be triggered. */
   ismd_port_attributes_t attributes;        /** Current attributes of the port. */
   int                    max_depth;         /** Maximum depth of the port queue, queue will refuse data if depth would become larger than this amount */
   int                    cur_depth_bytes;   /** Current depth of the port queue in bytes. This may be meaningless if the max depth is specified in buffers. */
   int                    cur_depth_bufs;    /** Current depth of the port queue in buffers. This may be meaningless if the max depth is specified in bytes. */
   int                    watermark;         /** Current watermark setting. */
} ismd_port_descriptor_t;



// The following functions are here and not in the external API because
// the application should not be performing these functions.


/**
Allocate a new smd data port.

@param[in]  port_type : the type of port to allocate (i.e. input or output).
@param[in]  port_config : a port configuration structure that describes the
attributes of the port to allocate.
@param[out] port_handle : a handle to the allocated port.  This value will not
be modified if the allocation is not successful.

@retval ISMD_SUCCESS : The port was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  Not enough resources existed to allocate
the port.
@retval ISMD_ERROR_INVALID_PARAMETER : The port descriptor structure contained
invalid information.  The low watermark must be between 0 and port_depth-1 (inclusive).
The high watermark must be between 1 and port_depth (inclusive). If no watermark is desired,
use ISMD_QUEUE_WATERMARK_NONE.
*/
ismd_result_t SMDCONNECTION ismd_port_alloc( ismd_port_type_t    port_type,
                                             ismd_port_config_t *port_config,
                                             ismd_port_handle_t *port_handle );

/**
Change the port name and or virtual_port_id and associated queue.
@param[in]  port_handle : handle of the port to rename.
@param[in]  name        : New name for the port.
@param[in]  name_length : Number of characters in the previous parameter.
@param[in]  virtual_port_id : Can be zero.
*/
ismd_result_t SMDPRIVATE ismd_port_set_name(
                                  ismd_port_handle_t port_handle,
                                  char *             name,
                                  int                name_length,
                                  int                virtual_port_id );
/**
Helper function that calls ismd_port_alloc() and ismd_port_set_name().
This scannot be used through the marshaller, so do not use this from
an application.
*/
ismd_result_t SMDCONNECTION ismd_port_alloc_named(
                                      ismd_port_type_t    port_type,
                                      ismd_port_config_t *port_config,
                                      char name[SMD_PORT_NAME_LENGTH],
                                      int name_length,
                                      int virtual_id,
                                      ismd_port_handle_t *port_handle );




/**
ismd_port_free frees a data port in the smd_core.  The data port must be
disconnected from other ports before it can be freed.  FIXME: support
auto-disconnect.

@param[in]  port_handle : handle to the port to be freed.

@retval ISMD_SUCCESS : The port was successfully freed.
@retval ISMD_ERROR_PORT_BUSY : The port was not in a state where it could
be freed.
*/
ismd_result_t SMDCONNECTION ismd_port_free( ismd_port_handle_t port_handle );


/**
flushes all buffers in the queue specified
port's internal buffer queue.

@param[in]  port_handle : handle to the port to flush.

@retval ISMD_SUCCESS : The port's queue was successfully flushed.
@retval ISMD_ERROR_INVALID_HANDLE : The port handle was not
valid.
*/
ismd_result_t SMDEXPORT ismd_port_flush( ismd_port_handle_t port );

/**
ismd_port_get_config : retrieves configuration information about the
specfied port.

@param[in]  port_handle : handle to the port retrieve information about.
@param[out]  port_descriptor : pointer to a structure that will hold
the information about the specified port.

@retval ISMD_SUCCESS : The operation was successfull and the port_descriptor
now contains all of the information about the specified port.
@retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.
*/
ismd_result_t SMDEXPORT ismd_port_get_descriptor( ismd_port_handle_t  handle,
                                                  ismd_port_descriptor_t *port_descriptor );


/**
ismd_port_set_config : modifies the configuration information of the
specfied port.

@param[in]  port_handle : handle to the port whose configuration you want
to modify
@param[in]  port_descriptor : pointer to a structure that contains
attributes to change about the specified port.

@retval ISMD_SUCCESS : The operation was successfull and the attributes
of the port have been updated.
@retval ISMD_ERROR_INVALID_PARAMETER : Some attributes specified in the
port_config were not valid, or the port_config itself is NULL.
The low watermark must be between 0 and port_depth-1 (inclusive).
The high watermark must be between 1 and  port_depth (inclusive).
If no watermark is desired, use ISMD_QUEUE_WATERMARK_NONE.
@retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.
*/
ismd_result_t SMDEXPORT ismd_port_set_config( ismd_port_handle_t  handle,
                                              ismd_port_config_t *port_config );


////////////////////////////////////////////////////////////////////////////////
// Queue Manager.
////////////////////////////////////////////////////////////////////////////////

#define ISMD_QUEUE_HANDLE_INVALID -1

typedef enum {
   ISMD_QUEUE_TYPE_INVALID         = 0,
   ISMD_QUEUE_TYPE_VARSIZE_BUFFERS = 1,
   ISMD_QUEUE_TYPE_FIXSIZE_BUFFERS = 2
} ismd_queue_type_t;


typedef int ismd_queue_handle_t;


/**
ismd_producer_func_t is the prototype for the callback function that a queue
will call when the queue input needs servicing.  This is the function
registered using ismd_queue_connect_input.  See the description for
ismd_queue_connect_input for details on when and why the callback function
is called.  Note:  This function must not block and it must not attempt to
perform other queue operations (i.e. calls to ismd_port_XXXXX).  If the
function cannot return a buffer immediately, it should schedule a call to
ismd_queue_enqueue and return immediately.

@param[in]  context : a pointer to user context information.  This value was
set in ismd_queue_connect_input and is passed to the function unmodifed.
@param[in]  queue_event : the event that specifies the reason for calling the
this function.
@param[out] buffer : location that will hold a pointer to a buffer descriptor
to insert into the queue.

@retval ISMD_SUCCESS : The function returned a buffer to insert into the
queue.
@retval ISMD_ERROR_NO_DATA_AVAILABLE :  The function did not return a buffer
to insert into the queue.  Note that if the event was ISMD_QUEUE_EVENT_EMPTY
the function will not be called again until another buffer is inserted into
the queue.  This function should schedule an enqueue operation if it cannot
immediately return a buffer.
*/
typedef ismd_result_t (* ismd_producer_func_t)( void                      *context,
                                                ismd_queue_event_t         queue_event,
                                                ismd_buffer_handle_t      *buffer );

/**
ismd_consumer_func_t is the prototype for the callback function that a queue
will call when the queue output needs servicing.  This is the function
registered using ismd_queue_connect_output.  See the description for
ismd_queue_connect_output for details on when and why the callback function
is called.  Note:  This function must not block and it must not attempt to
perform other queue operations (i.e. calls to ismd_port_XXXXX).  If the
function cannot accept the buffer immediately, it should schedule a call to
ismd_queue_dequeue and return immediately.

@param[in]  context : a pointer to user context information.  This value was
set in ismd_queue_connect_input and is passed to the function unmodifed.
@param[in]  queue_event : the event that specifies the reason for calling the
this function.
@param[out] buffer : pointer to a pointer to a buffer descriptor at the head
of the queue.

@retval ISMD_SUCCESS : The function accepted a buffer.
@retval ISMD_ERROR_NO_SPACE_AVAILABLE :  The function did not accept the
buffer and the buffer will remain at the head of the queue.  Note that if the
event was ISMD_QUEUE_EVENT_FULL the function will not be called again until
another buffer is removed from the queue.  This function should schedule a
dequeue operation if it cannot immediately accept the buffer.
*/
typedef ismd_result_t (* ismd_consumer_func_t)( void                     *context,
                                                ismd_queue_event_t        queue_event,
                                                ismd_buffer_handle_t      buffer );


typedef struct {
   ismd_queue_type_t         type;             /* Type of queue. */
   ismd_producer_func_t      producer_func;    /* Function to call when the queue need to try to fill itself. */
   void                     *producer_context; /* User-supplied context information for the output callback. */
   ismd_consumer_func_t      consumer_func;    /* Function to call when the queue need to try to empty itself. */
   void                     *consumer_context; /* User-supplied context information for the input callback. */
   int                       max_depth;        /* Maximum depth of the queue, queue will refuse data if depth would become larger than this amount */
   int                       cur_depth_bufs;   /* Current depth of queue in buffers. */
   int                       cur_depth_bytes;  /* Current depth of queue in bytes. */
   int                       high_watermark;   /* Queue depth when a high-watermark event is generated.  The action taken depends on whether the queue is attached to an input port or an output port and wether the port is connected to another port. */
   int                       low_watermark;    /* Queue depth when a low-watermark event is generated.  The action taken depends on whether the queue is attached to an input port or an output port and wether the port is connected to another port. */
} ismd_queue_status_t;


typedef struct {
   ismd_producer_func_t      producer_func;    /* Function to call when the queue need to try to fill itself. */
   void                     *producer_context; /* User-supplied context information for the output callback. */
   ismd_consumer_func_t      consumer_func;    /* Function to call when the queue need to try to empty itself. */
   void                     *consumer_context; /* User-supplied context information for the input callback. */
   int                       max_depth;        /* Maximum depth of the queue, queue will refuse data if depth would become larger than this amount */
   int                       high_watermark;   /* Queue depth when a high-watermark event is generated.  The action taken depends on whether the queue is attached to an input port or an output port and wether the port is connected to another port. */
   int                       low_watermark;    /* Queue depth when a low-watermark event is generated.  The action taken depends on whether the queue is attached to an input port or an output port and wether the port is connected to another port. */
} ismd_queue_config_t;


/**
ismd_queue_alloc allocates a buffer queue.

@param[in]  type : the type of buffer queue.
@param[in]  max_depth : the maximum depth of the queue.  This depth is in
buffers or bytes depending on the type of the queue.
@param[out] queue_handle : a handle to the newly-allocated queue.  This handle
must be used to access the port for all subsequent operations.

@retval ISMD_SUCCESS : The queue was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES :  Resources were not available to allocate
the queue.
@retval ISMD_ERROR_INVALID_PARAMETER : The type or max_depth parameter was not
valid.
*/
ismd_result_t SMDPRIVATE ismd_queue_alloc(
                                ismd_queue_type_t    type,
                                int                  max_depth,
                                ismd_queue_handle_t *queue_handle );



/**
ismd_queue_set_name gives the queue a name

@param[in] handle      : handle to the buffer queue to rename.
@param[in] name        : New name for the queue.
@param[in] name_length : Number of characters in the name.
@param[in] virtual_port_id : Can be zero.
*/
ismd_result_t SMDPRIVATE ismd_queue_set_name(
                                   ismd_queue_handle_t  handle,
                                   char *               name,
                                   int                  name_length,
                                   int                  virtual_queue_id);

/**
Helper function that calls ismd_queue_alloc() and ismd_queue_set_name()
*/
ismd_result_t SMDPRIVATE ismd_queue_alloc_named(
                                ismd_queue_type_t    type,
                                int                  max_depth,
                                char *name,
                                int name_lenght,
                                int virtual_queue_id,
                                ismd_queue_handle_t *queue_handle );

/**
ismd_queue_free frees a buffer queue.

@param[in] handle - handle to the buffer queue to free.

@retval ISMD_SUCCESS : The queue was successfully freed.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
*/
ismd_result_t SMDPRIVATE ismd_queue_free( ismd_queue_handle_t handle );


/**
ismd_queue_connect_input configures the input side of a buffer queue.  This
configuration is not required to write into the queue, but it is required to
receive automatic notifications when the queue can accept more data.

@param[in]  handle - handle to a buffer queue.
@param[in]  producer_func - pointer to a user function that will be called
when any one of the following conditions occur:
   1.  The queue depth goes from full (>= max_depth) to not full.
   2.  Queue depth crosses the low-watermark
   3.  The queue depth goes to zero.
Specify a value of NULL to disable user callbacks.
@param[in]  producer_context - a pointer to user context information that will
be passed through unmodified to the producer function.
@param[in]  low_watermark - the queue depth value that will trigger a call
to the producer function.  The function will be called only when the depth
crosses this value.  Specify ISMD_QUEUE_WATERMARK_NONE to disable callbacks
for the low-watermark condition.

@retval ISMD_SUCCESS : The queue's input connection was successfully
configured.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : The low_watermark value was
outside of the range of valid values for the queue.
*/
ismd_result_t SMDPRIVATE ismd_queue_connect_input(
                                        ismd_queue_handle_t  handle,
                                        ismd_producer_func_t producer_func,
                                        void                *producer_context,
                                        int                  low_watermark );


/**
ismd_queue_connect_output configures the output side of a buffer queue.  This
configuration is not required to read from the queue, but it is required to
receive automatic notifications when the queue has data available.

@param[in]  handle - handle to a buffer queue.
@param[in]  consumer_func - pointer to a user function that will be called
when any one of the following conditions occur:
   1.  The queue depth goes from empty to not-empty.
   2.  Queue depth crosses the high-watermark
   3.  The queue becomes full (>= max_depth).
Specify a value of NULL to disable user callbacks.
@param[in]  consumer_context - a pointer to user context information that will
be passed through unmodified to the consumer function.
@param[in]  high_watermark - the queue depth value that will trigger a call
to the producer function.  The function will be called only when the depth
crosses this value.  Specify ISMD_QUEUE_WATERMARK_NONE to disable callbacks
for the high-watermark condition.

@retval ISMD_SUCCESS : The queue's input connection was successfully
configured.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : The high_watermark value was
outside of the range of valid values for the queue.
*/
ismd_result_t SMDPRIVATE ismd_queue_connect_output(
                                         ismd_queue_handle_t  handle,
                                         ismd_consumer_func_t consumer_func,
                                         void                *consumer_context,
                                         int                  high_watermark );


/**
ismd_queue_disconnect_input disconnects the input side of a buffer queue.
This will disable callbacks for empty, not-full, and low-watermark conditions.

@param[in]  handle - handle to a buffer queue.

@retval ISMD_SUCCESS : The queue's input connection was successfully
disconnected.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
*/
ismd_result_t SMDPRIVATE ismd_queue_disconnect_input( ismd_queue_handle_t handle );


/**
ismd_queue_disconnect_output disconnects the output side of a buffer queue.
This will disable callbacks for not-empty, full, and high-watermark
conditions.

@param[in]  handle - handle to a buffer queue.

@retval ISMD_SUCCESS : The queue's output connection was successfully
disconnected.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
*/
ismd_result_t SMDPRIVATE ismd_queue_disconnect_output( ismd_queue_handle_t handle );


/**
ismd_queue_enqueue writes a buffer into a queue.

@param[in] handle - handle to the queue.
@param[in] buffer - pointer to the buffer descriptor to add to the queue.

@retval ISMD_SUCCESS : The buffer was successfully added to the queue.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_QUEUE_FULL : The queue was full and cannot accept
more data.
*/
ismd_result_t SMDPRIVATE ismd_queue_enqueue(
                                  ismd_queue_handle_t       handle,
                                  ismd_buffer_descriptor_t *buffer );

/**
ismd_queue_enqueue reads a buffer from a queue.

@param[in] handle - handle to the buffer queue.
@param[out] buffer - location to hold the pointer to the buffer
descriptor that will be removed from the queue.

@retval ISMD_SUCCESS : A buffer was successfully read from the queue.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_QUEUE_EMPTY : The queue was empty so no data could be read.
*/
ismd_result_t SMDPRIVATE ismd_queue_dequeue(
                                  ismd_queue_handle_t        handle,
                                  ismd_buffer_descriptor_t **buffer );

/**
ismd_queue_get_status retrieves all information about the current state of
the queue.

@param[in]  handle - handle to the buffer queue.
@param[out] status - pointer to a structure that this function will fill-in
with all of the current queue information.

@retval ISMD_SUCCESS : The queue status was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
*/
ismd_result_t SMDPRIVATE ismd_queue_get_status(
                                     ismd_queue_handle_t  handle,
                                     ismd_queue_status_t *status );

/**
ismd_queue_set_config reconfigures a buffer queue.

@param[in]  handle - handle to the buffer queue.
@param[out] status - pointer to a structure that contains information on
how the buffer queue should be configured.

@retval ISMD_SUCCESS : The queue status was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : The config structure contained
invalid configuration information.
*/
ismd_result_t SMDPRIVATE ismd_queue_set_config(
                                     ismd_queue_handle_t  handle,
                                     ismd_queue_config_t *config );


/**
ismd_queue_get_config gets the queue configuration.

@param[in]  handle - handle to the buffer queue.
@param[out] config - pointer to a structure that contains information on
how the buffer queue should be configured.

@retval ISMD_SUCCESS : The queue config was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : The config structure contained
invalid configuration information.
*/
ismd_result_t SMDPRIVATE ismd_queue_get_config(
                                     ismd_queue_handle_t  handle,
                                     ismd_queue_config_t *config );


/**
ismd_queue_flush empties all of the buffers from the specified queue.  Each
buffer is released either to the SMD Core or by calling the release_function
specified in the buffer descriptor.

@param[in]  handle - handle to the buffer queue.

@retval ISMD_SUCCESS : The queue was successfully flushed.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval Any error codes from ismd_buffer_dereference
*/
ismd_result_t SMDPRIVATE ismd_queue_flush( ismd_queue_handle_t handle );


/**
ismd_queue_lookahead returns a pointer to the buffer descriptor at the
specified position in the buffer queue without removing the buffer from the
queue.

@param[in]  handle - handle to the buffer queue.
@param[in]  buffer_position - the position of the buffer in the queue.
1 is the buffer at the head of the queue, and so on.
@param[out] buffer - pointer to a location where this function can
store the pointer to the buffer descriptor.

@retval ISMD_SUCCESS : The queue was successfully flushed.
@retval ISMD_ERROR_INVALID_HANDLE : The queue handle was not valid.
@retval ISMD_ERROR_INVALID_PARAMETER : The buffer position was invalid.
@retval ISMD_ERROR_?? : The number of buffers in the queue was less than
the position specified.
*/
ismd_result_t SMDPRIVATE ismd_queue_lookahead(
                                    ismd_queue_handle_t        handle,
                                    int                        buffer_position,
                                    ismd_buffer_descriptor_t **buffer );

////////////////////////////////////////////////////////////////////////////////
// Event Manager.
////////////////////////////////////////////////////////////////////////////////

//Same as ismd_event_strobe, except it is meant to be called from an ISR
ismd_result_t SMDPRIVATE ismd_event_strobe_nonblocking(ismd_event_t event);


////////////////////////////////////////////////////////////////////////////////
// Misc stuff.
////////////////////////////////////////////////////////////////////////////////

/*
 * ISMD Device Operations.  All device modules register their device operation
 * functions when they allocate an SMD device handle.  This is typically
 * done in the device's "open" function.
 */

/* See description of ismd_dev_close() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_close_func_t)(ismd_dev_t dev);

/* See description of ismd_dev_flush() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_flush_func_t)(ismd_dev_t dev);//hh remove the two paramters: bool discard_flushed_data, ismd_event_t flush_event

/* See description of ismd_dev_set_state() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_set_state_func_t)(ismd_dev_t dev, ismd_dev_state_t state);

/* See description of ismd_dev_set_clock() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_set_clock_func_t)(ismd_dev_t dev, ismd_clock_t clock);

/* See description of ismd_dev_set_stream_base_time() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_set_base_time_func_t)(ismd_dev_t dev, ismd_time_t base);

/* See description of ismd_dev_set_play_rate() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_set_play_rate_func_t)(ismd_dev_t dev, ismd_pts_t current_linear_time, int rate);

/* See description of ismd_dev_set_underrun_event() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_set_underrun_event_func_t)(ismd_dev_t dev, ismd_event_t underrun_event);

/* See description of ismd_dev_get_underrun_amount() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_get_underrun_amount_func_t)(ismd_dev_t dev, ismd_time_t *underrun_amount);

/* See description of ismd_dev_get_state() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_get_state_func_t)(ismd_dev_t dev, ismd_dev_state_t *state);

/* See description of ismd_dev_get_stream_base_time() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_get_base_time_func_t)(ismd_dev_t dev, ismd_time_t *base);

/* See description of ismd_dev_get_play_rate() in ismd_core.h */
typedef ismd_result_t (* ismd_dev_get_play_rate_func_t)(ismd_dev_t dev, int *rate);

typedef struct ismd_dev_ops {
   ismd_dev_close_func_t         close;         /**< close the device.  cleanly stop and release resources. */
   ismd_dev_flush_func_t         flush;         /**< syncrhonous discard of all in-flight data including data queued in I/O ports */
   ismd_dev_set_state_func_t     set_state;     /**< change the state of the device (e.g. pause, play, stop, etc.) */
   ismd_dev_set_clock_func_t     set_clock;     /**< (optional) set the clock to be used by the device. */
   ismd_dev_set_base_time_func_t set_base_time; /**< (optional) set the base time of a device. should only be called in pause state. */
   ismd_dev_set_play_rate_func_t set_play_rate; /**< (optional) set the play rate on a device. should only be called in pause state. */
   ismd_dev_set_underrun_event_func_t set_underrun_event;   /**< (optional, for push-mode buffer level monitoring) set an event for the renderers to strobe on underrun. */
   ismd_dev_get_underrun_amount_func_t get_underrun_amount; /**< (optional, for push-mode buffer level monitoring) get the "pts lateness" amount from the renderers. */
   ismd_dev_get_state_func_t    get_state;     /**< (optional) get the state of the device (e.g. pause, play, stop, etc.) */
   ismd_dev_get_base_time_func_t get_base_time; /**< (optional) get the base time of a device. */
   ismd_dev_get_play_rate_func_t get_play_rate; /**< (optional) get the play rate of a device.*/
} ismd_dev_ops_t;


/**
Allocate an SMD core device handle.  This is a systemwide unique handle
that must be used when calling the SMD device common functions such
as ismd_dev_close, ismd_dev_flush, ismd_dev_set_state, ismd_dev_set_clock,
and ismd_dev_set_stream_base_time.  Modules are expected to call
ismd_dev_handle_alloc from their "open" function and return the new
device handle to their client.

@param[in]  dev_ops - structure containing the device-specific functions
that implement the device-common operations.  Any functions not supported
by the module must be filled-in with NULL.
@param[out] handle - the newly-allocated device handle.

@retval ISMD_SUCCESS : The handle was successfully allocated.
@retval ISMD_ERROR_NO_RESOURCES : There were no unallocated
handles in the system to complete the request.

*/
ismd_result_t SMDPRIVATE ismd_dev_handle_alloc(
                                     ismd_dev_ops_t dev_ops,
                                     ismd_dev_t    *handle );

/**
Free an SMD core device handle.  Modules are expected to call
ismd_dev_handle_free from their "close" function.

@param[in] handle - the handle to free.

@retval ISMD_SUCCESS : The handle was successfully freed.
@retval ISMD_ERROR_INVALID_HANDLE : The specified handle
was stale or invalid.
*/
ismd_result_t SMDPRIVATE ismd_dev_handle_free( ismd_dev_t handle );


/**
Assign user-specific data to a device handle.  In general, this function
allows a module to store a tag or pointer so that it can associate some
local context information with the global unique device handle.

@param[in] handle - a device handle.
@param[in] user_data - 32-bit user-specific data.  This value will never be
modified by the SMD core.

@retval ISMD_SUCCESS : The user-data was successfully assigned.
@retval ISMD_ERROR_INVALID_HANDLE : The specified handle was stale or invalid.
*/
ismd_result_t SMDPRIVATE ismd_dev_set_user_data(
                                      ismd_dev_t handle,
                                      uint32_t   user_data );

/**
Retrieve user-specific data from a device handle.  In general, this function
allows a module to retrieve a tag or pointer so that it can look up local
context information based on the global unique device handle.

@param[in] handle - a device handle.
@param[out] user_data - 32-bit user-specific data.  This value is the
last value assigned using ismd_dev_set_user_data.

@retval ISMD_SUCCESS : The user-data was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE : The specified handle was stale or invalid.
*/
ismd_result_t SMDPRIVATE ismd_dev_get_user_data(
                                      ismd_dev_t handle,
                                      uint32_t  *user_data );






/**
Shortcut for buffer allocation that does not require a descriptor copy.
*/
ismd_result_t SMDPRIVATE ismd_buffer_alloc_typed_direct(
                                              ismd_buffer_type_t         type,
                                              size_t                     size,
                                              ismd_buffer_descriptor_t **buffer_descriptor );

/**
Shortcut for getting a pointer to the buffer's real descriptor using the
buffer ID.
*/
ismd_result_t SMDPRIVATE ismd_buffer_find_descriptor(
                                           ismd_buffer_id_t id,
                                           ismd_buffer_descriptor_t **buffer );



/**
THIS SHOULD NOT BE USED.
Allows for the manual reassignment of a buffer's base address.
There is no restriction on the core that it does not use the base address to
help in the reclaiming process, so this could stop working at any time.
This is meant for the test mode where uncompressed video frames are preloaded
into RAM and then those frames are "played" without them being copied into
SMD-allocated frame buffers.
*/
ismd_result_t SMDEXPORT ismd_buffer_reassign_base_address(
                                             ismd_buffer_handle_t handle,
                                             ismd_physical_address_t base_address);


/** @weakgroup ismd_clock_manager_pvt Private SMD Core Clock Manager Functions
*
* @section Private SMD Clock Manager Overview
* The Private SMD clock manager access to physical clocks and counters.  It
* provides a set of APIs that allows other SMD devices to register their
* hardware clocks and counters so that they may be used by SMD core virtual
* clocks.
*/

/*
 *  TODO:
 *    The ultimate goal of the clock manager is to support any subset of
 *    physical clock/counter functionality.  For this we need to be able
 *    to emulate the functionality of all physical device functionality
 *    that we support.  We need to develop a set of emulation APIs that we
 *    can use in place of device APIs when a device registers it's clock
 *    or counter and passes NULL for some of the function pointers.  This
 *    may also imply that we need to define the minimum set of functiality
 *    that a device clock or counter must support.
 */

#define ISMD_CLOCK_DEV_HANDLE_INVALID -1
typedef int ismd_clock_dev_t;



/**
ismd_clock_dev_callback_func_t is the prototype for the function that
will be called by the device when a scheduled clock event occurs.  Note that
this function can be called by the device in the "top-half".

@param[in]  context : opaque pointer to context information.  This value is
the same value passed by the SMD core into the device-specific function
ismd_clock_schedule_alarm_func_t.
*/
typedef void (* ismd_clock_dev_callback_func_t)(void *context);


/**
ismd_clock_dev_alloc_func_t is the prototype for the device-specific
function to allocate a new clock device instance.

@param[in] type : a platform-specific identifier for the type of clock to
allocate.
@param[out] clock : pointer to a location to store the handle to the
newly-allocated clock.

@retval ISMD_SUCCESS : The clock was successfully allocated.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_NO_RESOURCES :  Not enough resources existed to
alocate the clock.
@retval ISMD_ERROR_INVALID_PARAMETER : The type parameter was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_alloc_func_t)( int               type,
                                                       ismd_clock_dev_t *clock);


/**
ismd_clock_dev_free_func_t is the prototype for the device-specific
function to free a previously allocated clock device instance.

@param[in] clock : handle to a device-specific clock instance.

@retval ISMD_SUCCESS : The clock was successfully freed.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_free_func_t)(ismd_clock_dev_t clock);


/**
ismd_clock_dev_get_time_func_t is the prototype for the device-specific
function to get the current time value of the device clock.

@param[in] clock : handle to a device-specific clock.
@param[out] time : pointer to a location to store the current time of the
device clock.

@retval ISMD_SUCCESS : The time was successfully read.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_get_time_func_t)( ismd_clock_dev_t clock,
                                                          ismd_time_t     *time );


/** (Top Half Safe)
Gets the current time value from a clock.(same API as get_time, only top half safe)
Clock handles are NOT locked while getting time when this API is used.
@param[in] handle : a handle to a clock instance.  This handle was
originally returned from ismd_clock_alloc.
@param[out] time : the current time value of the clock in ticks.  Not
modified if the operation is not successful.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_get_time_th_safe(ismd_clock_t clock,
                                            ismd_time_t *time);


/** (Top Half Safe)
Gets the last vsync time value from a clock.

@param[in] handle : a handle to a clock instance.  This handle was
originally returned from ismd_clock_alloc.
@param[out] time : gets the last vsync time value latched in the last
vsync register in clock hardware. Not modified if the operation is not successful.

@retval ISMD_SUCCESS : the time was successfully retrieved.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/
ismd_result_t SMDEXPORT ismd_clock_get_last_vsync_time_th_safe(ismd_clock_t clock,
                                            ismd_time_t *time);



/** (Top Half Safe)
Function to adjust the frequency of the device clock.

@param[in] clock : handle to a device-specific clock instance.
@param[in] adjustment : the amount to adjust the device clock.  This number
is specified in parts-per-million and can be positive or negative.  E.g. an
adjustment of 10000 will make the clock run 1% faster.  An adjustment value
of -2000 will make the clock run slower 0.2% slower.

@retval ISMD_SUCCESS : The frequency was successfully adjusted.
@retval ISMD_ERROR_INVALID_PARAMETER : The context pointer was not valid or the
adjustment value was out of range.
@reval ISMD_ERROR_OUT_OF_RANGE : If the adjustment requested is out of range of
what clock driver can support, it will return this return code.  The client can
then send a different adjustment.

*/
ismd_result_t SMDEXPORT ismd_clock_adjust_freq_th_safe(ismd_clock_dev_t clock,
                                                                 int adjustment );

/**
ismd_clock_dev_set_time_func_t is the prototype for the device-specific
function to set the current time value of the device clock.

@param[in] clock : Handle to a device-specific clock.
@param[in] time : The time value that the device should set as the current
time of the device clock.

@retval ISMD_SUCCESS : The time was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_set_time_func_t)( ismd_clock_dev_t clock,
                                                          ismd_time_t      time );


/**
ismd_clock_dev_adjust_time_func_t is the prototype for the device-specific
function to adjust the current time value of the device clock.

@param[in] clock : Handle to a device-specific clock.
@param[in] adjustment : How much to add to the counter (can be negative).

@retval ISMD_SUCCESS : The time was successfully adjusted.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_adjust_time_func_t)( ismd_clock_dev_t clock,
                                                             int64_t      adjustment );

/**
ismd_clock_dev_set_alarm_handler_func_t is the prototype for the device-
specific function to setup the callback function that the device will
call when scheduled events trigger.

@param[in] clock : handle to a device-specific clock.
@param[in] callback : function to call when an alarm triggers.
@param[in] context : an opaque pointer that will be passed unmodified to
the callback function when the alarm triggeres..

@retval ISMD_SUCCESS : The alarm handler function was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_set_alarm_handler_func_t)( ismd_clock_dev_t               clock,
                                                                   ismd_clock_dev_callback_func_t callback,
                                                                   void                          *context );


/**
ismd_clock_dev_schedule_alarm_func_t is the prototype for the device-
specific function to schedule a one-time callback based on the device
clock time.

@param[in] clock : handle to a device-specific clock.
@param[in] time : the time to trigger the alarm.  When the clock time
reaches this time, the callback function will be called.

retval ISMD_SUCCESS : The alarm was successfully scheduled.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid..
*/
typedef ismd_result_t (* ismd_clock_dev_schedule_alarm_func_t)( ismd_clock_dev_t clock,
                                                                void *clock_alarm,
                                                                ismd_time_t      time);


/**
ismd_clock_dev_cancel_alarm_func_t is the prototype for the device-
specific function to cancel the currently scheduled one-time alarm
callback.

@param[in] clock : handle to a device-specific clock.

retval ISMD_SUCCESS : The alarm was successfully cancelled or there
was no alarm scheduled at the time this function was called.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid..
*/
typedef ismd_result_t (* ismd_clock_dev_cancel_alarm_func_t)(ismd_clock_dev_t clock);


/**
ismd_clock_dev_set_source_func_t is the prototype for the device-specific
function to route a physical clock source to drive a specific counter.

@param[in] clock : handle to a device-specific clock instance.
@param[in] type :  A platform-specific identifier for the type of source
to route to the clock instance.

@retval ISMD_SUCCESS : The clock source was successfully routed .
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The device clock currently registered
with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_PARAMETER : The specifed type was invalid.
@retval ISMD_ERROR_NO_RESOURCES : The hardware cannot route the specified clock
type to the clock instance.
*/
typedef ismd_result_t (* ismd_clock_dev_set_source_func_t)( ismd_clock_dev_t clock,
                                                            int              type );


/**
ismd_clock_dev_adjust_freq_func_t is the prototype for the device-specific
function to adjust the frequency of the device clock.

@param[in] clock : handle to a device-specific clock instance.
@param[in] adjustment : the amount to adjust the device clock.  This number
is specified in parts-per-million and can be positive or negative.  E.g. an
adjustment of 10000 will make the clock run 1% faster.  An adjustment value
of -2000 will make the clock run slower 0.2% slower.

@retval ISMD_SUCCESS : The frequency was successfully adjusted.
@retval ISMD_ERROR_INVALID_PARAMETER : The context pointer was not valid or the
adjustment value was out of range.
*/
typedef ismd_result_t (* ismd_clock_dev_adjust_frequency_func_t)( ismd_clock_dev_t clock,
                                                                 int adjustment );

/**
ismd_clock_dev_adjust_primary_func_t is the prototype for the device-specific
function to adjust the frequency of the primary/ master device clock.

@param[in] clock : handle to a device-specific clock instance.  This is
required to keep the clock associated with the primary clock in sync with the
master clock.  The local dds associated to the master dds / vcxo will be
adjusted in similar amount as the master clock adjustment.

@param[in] adjustment : the amount to adjust the device clock.  This number
is specified in parts-per-million and can be positive or negative.  E.g. an
adjustment of 10000 will make the clock run 1% faster.  An adjustment value
of -2000 will make the clock run slower 0.2% slower.

@retval ISMD_SUCCESS : The frequency was successfully adjusted.
@retval ISMD_ERROR_INVALID_PARAMETER : The context pointer was not valid or the
adjustment value was out of range.
@retval ISMD_ERROR_OUT_OF_RANGE : If the client/ module hits the
lower/upper bound in the tuning range (when VCXO is selected as the clock
source), the freq at the master vcxo would be saturated to the upper/lower
limit depending on the +ve or -ve adjustment and this code would be returned.
The tunable frequency range of the vcxo is platform specific and can change
from board to board.  Its specified in the platform specific header file for
clock.

*/

typedef ismd_result_t (* ismd_clock_dev_adjust_primary_func_t)( 	ismd_clock_dev_t clock,
																            		int adjustment );


typedef ismd_result_t (* ismd_clock_dev_make_primary_func_t)( ismd_clock_dev_t clock );

typedef ismd_result_t (* ismd_clock_dev_reset_primary_func_t)( ismd_clock_dev_t clock );


typedef ismd_result_t (* ismd_clock_dev_get_last_trigger_time_funct_t)(ismd_clock_dev_t clock, ismd_time_t *time);


typedef ismd_result_t (* ismd_clock_dev_trigger_software_event_func_t)(ismd_clock_dev_t clock);



/**
ismd_clock_dev_set_vsync_pipe_func_t is the prototype for the device-specific
function to set the pipe driving the last vsync time capture on the clock hardware

@param[in] clock : handle to a device-specific clock.
@param[out] int : select from pipe A or pipe B.
"0" for A
"1" for B

@retval ISMD_SUCCESS : The pipe was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/
typedef ismd_result_t (* ismd_clock_dev_set_vsync_pipe_func_t)( ismd_clock_dev_t clock,
                                                                int pipe );

/**
ismd_clock_dev_get_last_vsync_time is the prototype for the device-specific
function to get the last latched value in the last vsync timestamp register.
Whenever a frame is flipped an interrupt is generated and current value of the
wall clock is latched in the vsync register.

@param[in] clock : handle to a device-specific clock.
@param[out] time  : pointer to a location to store the current time of the
device clock.

@retval ISMD_SUCCESS : The pipe was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/


typedef ismd_result_t (* ismd_clock_dev_get_last_vsync_time_func_t)( ismd_clock_dev_t clock,
                                                                     ismd_time_t     *time );




/**
ismd_clock_dev_set_timestamp_trigger_source is the prototype for the device-specific
function to set the timestamping source for a particular clock.  Timestamps
would be generated on timestamp events cause by the source set (software OR one
of the hardware events supported with 'ismd_clock_trigger_source_t" enum in
gen3_clock.h, whch is platform specific)

@param[in] clock : handle to a device-specific clock.
@param[in] source  : timestamp trigger source to be set.

@retval ISMD_SUCCESS : The pipe was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/


typedef ismd_result_t (* ismd_clock_dev_set_timestamp_trigger_source_func_t)( ismd_clock_dev_t clock,
                                                          int     source );

/**
ismd_clock_dev_route_func_t is the prototype for the device-specific
function to set destination driven by a particular clock.

@param[in] clock : handle to a device-specific clock.
@param[in] destination : TS_IN or TS_OUT driven by a particular clock.

@retval ISMD_SUCCESS : The pipe was successfully set.
@retval ISMD_ERROR_FEATURE_NOT_SUPPORTED: The clock device currently
registered with the SMD core does not support this feature.
@retval ISMD_ERROR_INVALID_HANDLE : The clock device handle was invalid.
*/

typedef ismd_result_t (* ismd_clock_dev_route_func_t)( ismd_clock_dev_t clock,
                                                       int destination);

/**
ismd_clock_dev_ops_t defines the device-specific interface to a clock. The
interface is a set of function pointers plus a context pointer.  The function
pointers point to a standardized set of functions in the device driver to
access the clock features.  The context pointer is a pointer to a device-
specific context information for the clock.  This pointer is set by the device
and is passed through unmodified to each of the device functions.  The pointer
allows for greater flexibility, such as allowing the device to use the same
functions for multiple physical clocks.

Device drivers are not required to support all of the functions.  In cases
where a function is not supported, the device driver can insert a NULL pointer
into the ismd_clock_dev_ops_t structure when calling ismd_clock_register.
*/
typedef struct {
   ismd_clock_dev_alloc_func_t                        allocate;          /**< Device-specific function to allocate a clock device resource. */
   ismd_clock_dev_free_func_t                         free;              /**< Device-specific function to free a clock device resource. */
   ismd_clock_dev_get_time_func_t                     get_time;          /**< Device-specific function to get the current time of the clock. */
   ismd_clock_dev_set_time_func_t                     set_time;          /**< Device-specific function to set the current time of the clock. */
   ismd_clock_dev_adjust_time_func_t                  adjust_time;       /**< Device-specific function to adjust the current time of the clock. */
   ismd_clock_dev_get_last_trigger_time_funct_t       get_last_trigger_time;
   ismd_clock_dev_schedule_alarm_func_t               schedule_alarm;    /**< Device-specific function to schedule a one-time clock alarm. */
   ismd_clock_dev_cancel_alarm_func_t                 cancel_alarm;      /**< Device-specific function to cancel the currently-scheduled alarm. */
	ismd_clock_dev_set_alarm_handler_func_t            set_alarm_handler; /**< Device-specific function to set an alarm handler function for future alarms. */
	ismd_clock_dev_set_source_func_t                   set_source;        /**< Device-specific function to set the source that drives a clock device. */
	ismd_clock_dev_adjust_frequency_func_t             adjust_frequency;  /**< Device-specific function to adjust the frequency of a clock device's source. */
	ismd_clock_dev_adjust_primary_func_t	            adjust_primary;
	ismd_clock_dev_make_primary_func_t                 make_primary;      /**< Device-specific function to make a clock device the primary clock in the system. */
	ismd_clock_dev_reset_primary_func_t		            reset_primary;
	ismd_clock_dev_trigger_software_event_func_t		   trigger_software_event;
   ismd_clock_dev_set_vsync_pipe_func_t	            set_vsync_pipe;
   ismd_clock_dev_get_last_vsync_time_func_t          get_last_vsync_time;
   ismd_clock_dev_set_timestamp_trigger_source_func_t set_timestamp_trigger_source;
   ismd_clock_dev_route_func_t                        clock_route;
} ismd_clock_dev_ops_t;



/**
ismd_clock_register_clock allows a device driver to expose its clock to the
SMD Core, which in turn allows other software componets to access the clock.
A device driver exposes a clock by providing a set of functions that the SMD
Core can call when it needs to access the clock.  The function are defined in
the ismd_clock_dev_ops_t structure.

@param[in]  clock_descriptor : a pointer to a structure that describes how the
SMD Core can access the device's clock.

@retval ISMD_SUCCESS : The clock was successfully registered.
@retval ISMD_ERROR_NO_RESOURCES :  Not enough resources existed to register the
clock.
@retval ISMD_ERROR_INVALID_PARAMETER : The clock_descriptor structure did not
provide sufficient functionality to support an SMD clock.
*/
ismd_result_t SMDPRIVATE ismd_clock_register_clock( ismd_clock_dev_ops_t *clock_descriptor );


/**
ismd_clock_get_primary returns the handle of the clock which has been made
primary with ismd_clock_make_primary API.  If no clock has been made primary,
it returns ISMD_ERROR_NO_RESOURCES.

@param[out]  handle: handle to the primary clock.  This value is only valid if
the return code is ISMD_SUCCESS.  So make sure to check the return code.

@retval ISMD_SUCCESS : This API returns ISMD_SUCCESS if any clock has been made
primary and in that case, the "handle" holds a valid clock instance value.
@retval ISMD_ERROR_NO_RESOURCES : No clock has been made primary currently and
"handle" will hold an invalid value.
*/
ismd_result_t SMDPRIVATE ismd_clock_get_primary(ismd_clock_t * handle);

/**
ismd_convert_segment_time_to_stream_position is used to convert a presentation
timestamp in original content time domain into stream position.

@param[in]  segment_time : the presentation timestamp in original media time.
                        The value to be converted to linear time.
@param[in]  segment_start : the basetime in original media time to be used in conversion for forward modes.
                        This is the 'start' field in in-band newsegment tags in original media time.
@param[in]  segment_stop : the basetime in original media time to be used in conversion for reverse modes.
                        This is the 'stop' field in in-band newsegment tags in original media time.
@param[in]  rate : the playback rate for the stream normalized to ISMD_NORMAL_PLAY_RATE.
@param[in]  applied_rate : Play rate achieved for this segment, by earlier elements in the pipeline.
                        This is normalized to ISMD_NORMAL_PLAY_RATE.
@param[in]  segment_position: segment's stream position 
@param[out] stream_position : Used to return the converted stream position.

@retval ISMD_SUCCESS : The value was successfully converted.
*/
ismd_result_t SMDPRIVATE ismd_convert_segment_time_to_stream_position(ismd_pts_t  segment_time, 
                                                       ismd_pts_t  segment_start, 
                                                       ismd_pts_t  segment_stop, 
                                                       int  	   rate, 
                                                       int  	   applied_rate, 
                                                       ismd_pts_t  segment_position,
                                                       ismd_pts_t  *stream_position);

/**
ismd_convert_media_time_to_stream_time is used to convert a presentation
timestamp in original media time into local stream time.
The renderers use the stream time to translate to SMD clock time and decide
when frames or samples are rendered.

@param[in]  media_time : the presentation timestamp in original media time.
                        The value to be converted to stream time.
@param[in]  rebasing_info : all rebasing parameters required for the conversion.
@param[out] unscaled_stream_time : the unscaled presentation timestamp in local stream time.
@param[out] scaled_stream_time : the scaled presentation timestamp in local stream time.

@retval ISMD_SUCCESS : The value was successfully converted.
*/
ismd_result_t SMDPRIVATE ismd_convert_media_time_to_stream_time(
                                                               ismd_pts_t  media_time,
                                                               ismd_pts_t  media_base_time,
                                                               ismd_pts_t  local_presentation_base_time,
                                                               ismd_pts_t  local_rate_change_time,
                                                               ismd_pts_t  play_rate_change_time,
                                                               int         rate,
                                                               ismd_pts_t  *unscaled_stream_time,
                                                               ismd_pts_t  *stream_time);

/**
ismd_convert_segment_time_to_linear_time is used to convert a presentation
timestamp in original content time domain into local linear time.

@param[in]  segment_time : the presentation timestamp in original media time.
                        The value to be converted to linear time.
@param[in]  segment_start : the basetime in original media time to be used in conversion for forward modes.
                        This is the 'start' field in in-band newsegment tags in original media time.
@param[in]  segment_stop : the basetime in original media time to be used in conversion for reverse modes.
                        This is the 'stop' field in in-band newsegment tags in original media time.
@param[in]  rate : the playback rate for the stream normalized to ISMD_NORMAL_PLAY_RATE.
@param[in]  segment_linear_start : the basetime in local linear time to be used in conversion.
                        This is the 'linear_start' field in in-band newsegment tags.
@param[out] linear_time : the unscaled presentation timestamp in local 1X time.

@retval ISMD_SUCCESS : The value was successfully converted.
*/
ismd_result_t SMDPRIVATE ismd_convert_segment_time_to_linear_time(
                                                               ismd_pts_t  segment_time,
                                                               ismd_pts_t  segment_start,
                                                               ismd_pts_t  segment_stop,
                                                               int  	   rate,
                                                               ismd_pts_t  segment_linear_start,
                                                               ismd_pts_t  *linear_time);

/**
ismd_convert_linear_time_to_scaled_time is used to convert a presentation
timestamp in local linear (1x) time domain into local scaled time after accounting
for playback rate.

@param[in]  linear_time : the unscaled presentation timestamp in local 1X time.
@param[in]  linear_rate_change_time : the linear local unscaled time when the current rate change occured.
@param[in]  scaled_rate_change_time : the scaled local time when the current rate change occured.
@param[in]  rate : the playback rate for the stream normalized to ISMD_NORMAL_PLAY_RATE.
@param[out] scaled_time : the scaled presentation timestamp in local time.

@retval ISMD_SUCCESS : The value was successfully converted.
*/
ismd_result_t SMDPRIVATE ismd_convert_linear_time_to_scaled_time(
                                                               ismd_pts_t  linear_time,
                                                               ismd_pts_t  linear_rate_change_time,
                                                               ismd_pts_t  scaled_rate_change_time,
                                                               int         rate,
                                                               ismd_pts_t  *scaled_time);
/**
Decrements the reference count for a buffer.  When the count reaches zero,
the buffer's release_function will be called to notify the buffer's owner
that the buffer is no longer being accessed. It also updates the output param reference count.

@param[in] buffer : handle to an SMD buffer.
@param[out] ref_count : reference count of the buffer.

@retval ISMD_SUCCESS : the buffer was successfully dereferenced.
@retval ISMD_ERROR_INVALID_HANDLE :  the buffer handle was stale or invalid.
*/
ismd_result_t SMDPRIVATE ismd_buffer_dereference_pvt(
                                 ismd_buffer_handle_t buffer,
                                 int *ref_count );

#endif /* __ISMD_CORE_PROTECTED_H__ */
