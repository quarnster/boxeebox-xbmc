#ifndef SVEN_PROTOS_H
#define SVEN_PROTOS_H 1
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

#ifndef SVEN_H
#include "sven.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _SVENHeader;
struct SVENReverser;
struct EAS_Register;
struct ModuleReverseDefs;
struct SVEN_FW_Globals;

/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */

/**
 * @brief        Gain access to the System-Visible-Event-Nexus Header
 *
 * @returns     (struct _SVENHeader *) or NULL if unsuccessful
 *
 */
struct _SVENHeader *sven_open_header(void);

/**
 * @brief        Gain access to the System-Visible-Event-Nexus Event Buffer area
 *
 * @returns     (void *) or NULL if unsuccessful
 *
 */
void *sven_get_event_buf( void );

/**
 * @brief        Initialize a FW_Globals struct with physical addresses
 *
 * @param       fw_globals : 
 * @returns     err or 0 on success
 *
 */
int sven_init_fw_globals(
   struct SVEN_FW_Globals  *fw_globals );

/**
 * @brief        Get the current timestamp
 *
 * @returns     current timestamp
 *
 */
unsigned int sven_get_timestamp(void);

/**
 * @brief        Get the current timestamp frequency in ticks per second
 *
 * @returns     timestamp frequency
 *
 */
unsigned int sven_get_timestamp_frequency(void);

/**
 *
 * @brief		Initialize a SVENHandle for an existing shared pool
 *             connect to specific circular queue.
 *
 * @param   svenh  : SVEN Handle structure to initialize
 * @param   hdr  : SVEN Root Shared Memory structure
 * @param   queue_number  : Which Queue within shared area to interact in.
 *
 */
void sven_attach_handle_to_queue(
	struct SVENHandle		*svenh,
	struct _SVENHeader	*hdr,
    int                  queue_number );

/**
 * @deprecated
 * @brief		DEPRECATED: Use sven_attach_handle_to_queue()
 *             Initialize a SVENHandle for an existing shared pool
 *
 * @param   svenh  : SVEN Handle structure to initialize
 * @param   hdr  : SVEN Root Shared Memory structure
 *
 */
void sven_init_writer_handle(
	struct SVENHandle		*svenh,
	struct _SVENHeader	*hdr);

/**
 *
 * @brief		Write an Event into the shared area
 *
 * @param   svenh  : SVEN Handle structure
 * @param   ev  : SVEN Event to write
 *
 */
void sven_write_event(
	struct SVENHandle		*svenh,
	const struct SVENEvent	*ev );

/**
 *
 * @brief		Get position of Latest Event Written to shared area.
 *             Typically used to initialize a reader position when joining
 *             a sven log in progress.
 *
 * @param   svenh  : SVEN Handle structure
 *
 * @returns the latest written position in the log.
 *
 */
int sven_get_write_position(
	struct SVENHandle		*svenh );

/**
 *
 * @brief		Read Next available event into local Event structure. Returns
 *             zero if caught up with latest written event.
 *
 * @param svenh       : SVEN Handle structure
 * @param ev          : SVEN Event to read into
 * @param preader_pos : Pointer to reader position
 *
 * @returns zero if no new event available, non-zero if a new event has been read.
 *
 */
int sven_read_next_event(
	struct SVENHandle		*svenh,
	struct SVENEvent		*ev,
	int						*preader_pos );

/**
 *
 * Write a short SVEN Debug String.
 *   Event Type will ALWAYS == SVEN_event_type_debug_string.
 *   Only 24 characters will fit inside a SVEN DebugString event.
 *
 * @param svenh            : SVEN Handle structure
 * @param module           : enum SVEN_Module (e.g. SVEN_module_VR_VCAP)
 * @param unit             : unit within the module (e.g. 0, 1, 2 .. )
 * @param subtype          : enum SVEN_DEBUGSTR_t (e.g. SVEN_DEBUGSTR_FunctionEntered)
 * @param str              : Debug String to write (not to exceed 24 characters)
 *
 */
void sven_WriteDebugString(
	struct SVENHandle		*svenh,
    int                      module,
    int                      unit,
    int                      subtype,
    const char              *str );


/**
 * sven_WriteDebugStringEnd() Ensures last "N" chars of debugstring are copied.
 *   Event Type will ALWAYS == SVEN_event_type_debug_string.
 *   Only 24 characters will fit inside a SVEN DebugString event.
 *   This function is usually called from SVEN_AUTO_TRACE().
 *
 * @param svenh            : SVEN Handle structure
 * @param module           : enum SVEN_Module (e.g. SVEN_module_VR_VCAP)
 * @param unit             : unit within the module (e.g. 0, 1, 2 .. )
 * @param subtype          : enum SVEN_DEBUGSTR_t (e.g. SVEN_DEBUGSTR_FunctionEntered)
 * @param str              : string to copy rightmost chars from
 */
void sven_WriteDebugStringEnd(
	struct SVENHandle		*svenh,
    int                      module,
    int                      unit,
    int                      subtype,
    const char              *str );

/**
 *
 * Write a SVEN Event copying an array of ulongs into the payload.
 *
 * @param svenh            : SVEN Handle structure
 * @param module           : enum SVEN_Module (e.g. SVEN_module_VR_VCAP)
 * @param unit             : unit within the module (e.g. 0, 1, 2 .. )
 * @param event_type       : enum SVEN_MajorEventType_t (e.g. SVEN_event_type_port_io)
 * @param event_subtype    : Subtype of Major event type. (e.g. SVEN_EV_PortIo16_Read)
 * @param event_num_ulongs : Number of args to write into the event "payload area"
 * @param pulongs          : array of ulongs to copy into payload area
 *
 */
void sven_WriteEvent_ulongs(
	struct SVENHandle		*svenh,
    int                      module,
    int                      unit,
    int                      event_type,
    int                      event_subtype,
    int                      event_num_ulongs,
    unsigned long           *pulongs );

/**
 *
 * Write a SVEN Event using a variable Argument List, ulongs.
 *
 * @param svenh            : SVEN Handle structure
 * @param module           : enum SVEN_Module (e.g. SVEN_module_VR_VCAP)
 * @param unit             : unit within the module (e.g. 0, 1, 2 .. )
 * @param event_type       : enum SVEN_MajorEventType_t (e.g. SVEN_event_type_port_io)
 * @param event_subtype    : Subtype of Major event type. (e.g. SVEN_EV_PortIo16_Read)
 * @param event_num_ulongs : Number of args to write into the event.
 *
 */
void sven_WriteEventVA_ulong(
	struct SVENHandle		*svenh,
    int                      module,
    int                      unit,
    int                      event_type,
    int                      event_subtype,
    int                      event_num_ulongs,
    ... );

/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */
const struct ModuleReverseDefs *svenreverse_GetModuleTables(
	enum SVEN_Module			 module );

const struct ModuleReverseDefs *svenreverse_GetModuleTables_ByName(
    const char                  *module_name );

int sven_reverse_Lookup(
	const char			               *regname,
    const struct ModuleReverseDefs    **pmrd,
    const struct EAS_Register         **preg,
    const struct EAS_RegBits          **pbits,
    unsigned int                       *poffset );

////////////////////////////////////////////////////////////////////////////////
///	 Reverse-engineers a Register Read/Write SVENEvent to a register-change text string
///     Will break-out register bit definitions if they are defined in the table.
///
/// @param   str  : string to print into
/// @param   ev : SVENEvent containing the Register IO value
/// @param   mrd : ModuleReverseDefs (unit definitions)
/// @param   prev_value : previous value of the register
///
/// @returns number of characters written
///
////////////////////////////////////////////////////////////////////////////////
int sven_reverse_get_register_changes(
	char 				 		    *str,			/* String to build into */
	const struct SVENEvent		    *ev,			/* Register-IO SVENEvent to decode */
	const struct ModuleReverseDefs  *mrd,           /* ModuleReverseDefs */
	unsigned int				     prev_value );	/* previous value of register */

/**
 *
 * @brief		Reverse-engineers a SVENEvent into a text string.
 *
 * @param   str  : string to print into
 * @param   rev : struct SVENReverser
 * @param   ev : SVENEvent containing the Register IO value
 *
 */
int sven_reverse_GetEventTextString(
	char					    *str,
	struct SVENReverser		    *rev,
	const struct SVENEvent	    *ev );

/**
 *
 * @brief		Create a generic SVENReverse handle
 *
 * @param   hdr  : SVEN Root Shared Memory structure
 *
 */
struct SVENReverser *svenreverse_Create(
	struct _SVENHeader		    *hdr );

/**
 *
 * @brief		Delete a generic SVENReverse handle
 *
 * @param   rev  : SVENReverser
 *
 */
void svenreverse_Delete(
	struct SVENReverser		    *rev );
/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */
/* ============================================================================= */
struct SVENLog;
struct SVENEventFilter;
struct SVENUserFilter;

/**
 * @brief         Create and initialize a new SVENLog handle
 *
 * @param         hdr                   : struct _SVENHeader
 * @param         local_eventbuf_size   : how much memory to allocate for local event storage
 *
 * @returns       (struct SVENLog *) or NULL
 */
struct SVENLog *svenlog_Create(
	struct _SVENHeader		*hdr,
	unsigned int			 local_eventbuf_size );

/**
 * @brief         Delete a SVENLog
 *
 * @param         svenlog     : SVENLog to delete
 */
void svenlog_Delete(
	struct SVENLog	*svenlog );


/**
 * @brief         Write a text string describing the event
 *
 * @param         svenlog     : struct SVENLog
 * @param         ev          : SVENEvent to check
 * @param         str         : Text String for the event
 *
 * @returns       number of characters written to string
 */
int svenlog_GetEventTextString(
	struct SVENLog		*svenlog,
	struct SVENEvent	*ev,
    char                *str );

/**
 * @brief         For Debug of "field recorded events", Binary files captured off-site
 *
 * @param         svenlog           : struct SVENLog
 * @param         ev                : SVENEvent to load
 * @param         ignore_filters    : if 1: do not pass event through any active filters
 *
 * @returns       number of new events read.  Note it is possible
 *                that all events were rejected by filtering, and
 *                there would be no new events in the local buffer.
 */
int svenlog_InsertEvent_Locally(
	struct SVENLog		*svenlog,
	struct SVENEvent	*ev,
    int                  ignore_filters );

/**
 * @brief         Read all available RT events into local queue.  This
 *                function is usually periodically called on a dedicated thread.
 *
 * @param         svenlog     : struct SVENLog
 *
 * @returns       number of new events read.  Note it is possible
 *                that all events were rejected by filtering, and
 *                there would be no new events in the local buffer.
 */
int svenlog_CaptureRealTimeEvents_Locally(
	struct SVENLog		*svenlog );


/**
 * @brief         Read the next event out of the "local capture" queue.
 *
 * @param         svenlog     : struct SVENLog
 * @param         ev          : SVENEvent to check
 *
 * @returns       0 if event ready, 1 otherwise
 */
int svenlog_ReadNextLocalEvent( 
	struct SVENLog		*svenlog,
	struct SVENEvent	*ev );

/**
 * @brief         Pause capture of all Realtime events into local queue
 *
 * @param         svenlog     : struct SVENLog
 */
void svenlog_Pause(
	struct SVENLog	*svenlog );

/**
 * @brief         Get a view of the event buffer in increasing time order
 *
 *    Get a view of the internal SVENLog Buffer in increasing time order.
 *
 *    SVENLog captures events in a circular fashion.  This function gives
 *    the application a view into the events before and after the "split"
 *    created when capturing events.
 *
 *    Warning: behavior is unpredictable if the svenlog is not "paused"
 *
 * @param         svenlog              : struct SVENLog *
 * @param         pev0                 : pointer to first block of sven events
 * @param         nev0                 : number of SVEN Events in the first block
 * @param         pev1                 : pointer to second block of sven events
 * @param         nev1                 : number of SVEN Events in the second block
 *
 * @returns       0 for success, error otherwise
 */
int svenlog_view_event_buffer(
	struct SVENLog    *svenlog,
   struct SVENEvent  **pev0,
   int               *nev0,
   struct SVENEvent  **pev1,
   int               *nev2 );

/**
 * @brief         Set the number of events to record after the trigger fires
 *
 * @param         svenlog     : struct SVENLog
 */
void svenlog_SetTriggerDelay(
    struct SVENLog		*svenlog,
    unsigned int         svenlog_trigger_delay );

/**
 * @brief         Set the number of events to record after the trigger fires
 *
 * @param         svenlog     : struct SVENLog
 *
 * @returns       0 if capture is running, 1 if capture has been paused (by a trigger firing)
 */
int svenlog_IsCapturePaused(
    struct SVENLog		*svenlog );

/**
 * @brief         Resume capture of all Realtime events into local queue.
 *                Note this will update the internal "read" pointer with
 *                the "last written" pointer in the RT Queue, so it is
 *                likely events will have been lost between Paus and Run.
 *
 * @param         svenlog     : struct SVENLog
 */
void svenlog_Run(
	struct SVENLog	*svenlog );

/**
 * @brief         Recover all events from the event queues (typically after reset)
 *
 * @param         svenlog     : struct SVENLog
 */
void svenlog_Recover(
	struct SVENLog	*svenlog );

/**
 * @brief         Place the local event reader one full buffer behind the writer.
 *
 * @param         svenlog     : struct SVENLog
 * @param         num_events  : how many events to rewind (0 if all available)
 *
 *    This function should only be called while local capture is Paused.
 *    Behavior is undefined if this is called while capture is running.
 *
 */
void svenlog_RewindLocalEventReader( 
	struct SVENLog		*svenlog,
   int                num_events );

/**
 * @brief         Set the default filter behavior for events that
 *                are not force-accepted or force-rejected by filters
 *
 * @param         svenlog        : struct SVENLog
 * @param         filter_default : 0 to record all, 1 to reject all
 */
void svenlog_set_filter_default(
	struct SVENLog		*svenlog,
	int					 filter_default );

/**
 * @brief         Add a rudimentary filter into the White (always record) list.
 *
 * @param         svenlog        : struct SVENLog
 * @param         event_offset   : byte offset within single SVENEvent
 * @param         event_and_mask : mask of bits to compare to equ_mask
 * @param         event_equ_mask : "positive" result of AND operation
 *
 * @returns       pointer to struct SVENEventFilter added
 */
struct SVENEventFilter *svenlog_add_whitelist_filter(
	struct SVENLog		*svenlog,
	int					 event_offset,
	unsigned int		 event_and_mask,
	unsigned int		 event_equ_mask );

/**
 * @brief         Remove a whitelist filter from internal filter queue.
 *
 * @param         svenlog     : struct SVENLog
 * @param         ef          : filter to remove.
 *
 * @returns       1 if filter found, 0 if filter not found
 */
int svenlog_remove_whitelist_filter(
	struct SVENLog		*svenlog,
	struct SVENEventFilter *ef );

/**
 * @brief         Add a rudimentary filter into the Black (always reject) list.
 *
 * @param         svenlog        : struct SVENLog
 * @param         event_offset   : byte offset within single SVENEvent
 * @param         event_and_mask : mask of bits to compare to equ_mask
 * @param         event_equ_mask : "positive" result of AND operation
 *
 * @returns       pointer to struct SVENEventFilter added
 */
struct SVENEventFilter *svenlog_add_blacklist_filtery(
	struct SVENLog		*svenlog,
	int					 event_offset,
	unsigned int		 event_and_mask,
	unsigned int		 event_equ_mask );

/**
 * @brief         Remove a blacklist filter from internal filter queue.
 *
 * @param         svenlog     : struct SVENLog
 * @param         ef          : filter to remove.
 *
 * @returns       1 if filter found, 0 if filter not found
 */
int svenlog_remove_blacklist_filter(
	struct SVENLog		*svenlog,
	struct SVENEventFilter *ef );

/**
 * @brief         Add a rudimentary filter into the White (always record) list.
 *
 * @param         svenlog        : struct SVENLog
 * @param         event_offset   : byte offset within single SVENEvent
 * @param         event_and_mask : mask of bits to compare to equ_mask
 * @param         event_equ_mask : "positive" result of AND operation
 *
 * @returns       pointer to struct SVENEventFilter added
 */
struct SVENEventFilter *svenlog_add_whitelist_entry(
	struct SVENLog		*svenlog,
	int					 event_offset,
	unsigned int		 event_and_mask,
	unsigned int		 event_equ_mask );

/**
 * @brief         Add a user programmable filter to the logger.
 *
 * @param         svenlog        : struct SVENLog
 * @param         filter         : filter function to call
 * @param         userdata0      : userdata0 to pass to the filter
 * @param         userdata1      : userdata1 to pass to the filter
 *
 * @returns       pointer to struct SVENUserFilter added
 *
 *  Note the filter function gets passed these two userdata pointers
 *  and an event to "sniff".  The return value of this callback indicates
 *  how the event should be handled.  Note your filter is expected to
 *  cooperate with all other added filters in the system.  This is a
 *  quick accept-or-reject filter.  Do NOT do prints, sleeps() or any
 *  other potentially long-executing operation during this callback.
 *
 *      Your filter callback return values:
 *
 *      retval > 0      - record the event immediately.
 *                          do not check subsequent user filters.
 *
 *      retval == 0     - indifferent, pass to next user filter.
 *
 *      retval < 0      - reject (don't record) the event immediately.
 *                          do not check subsequent user filters.
 *
 *
 *  User-filters are called in the order they were added, after
 *  the black and white list filters have run.  Events that are
 *  accepted by the whitelist are seen by the user filters in sequence,
 *  until one of them rejects it by returning (retval < 0).
 *
 */
struct SVENUserFilter *svenlog_add_user_filter(
	struct SVENLog		*svenlog,
    int                (*filter)(
                          struct SVENEvent    *ev,
                          void                *userdata0,
                          void                *userdata1 ),
    void                *userdata0,
    void                *userdata1 );

/**
 * @brief         Remove a user filter from the logger
 *
 * @param         svenlog        : struct SVENLog
 * @param         filter         : filter created by svenlog_add_user_filter()
 *
 * @returns       pointer to struct SVENUserFilter added
 */
int svenlog_remove_user_filter(
	struct SVENLog		*svenlog,
    struct SVENUserFilter   *filter );

struct SVENUserFilter *svenlog_add_template_filter(
	struct SVENLog		      *svenlog,
    const struct SVENEvent    *and_ev,
    const struct SVENEvent    *or_ev,
    int                        hit_returnval );

struct SVENUserFilter *svenlog_add_template_trigger(
	struct SVENLog		      *svenlog,
    const struct SVENEvent    *mask_ev,
    const struct SVENEvent    *eq_ev );

void svenlog_SetTrigger( 
	struct SVENLog		      *svenlog );

/* ============================================================================= */
/* ============================================================================= */

/** mark a function entered into sven debug log */
#define SVEN_FUNC_ENTER( svenh, module, unit ) \
    sven_WriteDebugString( svenh, module, unit, \
        SVEN_DEBUGSTR_FunctionEntered, \
        __FUNCTION__ )

/** mark a function exit into sven debug log */
#define SVEN_FUNC_EXIT( svenh, module, unit ) \
    sven_WriteDebugString( svenh, module, unit, \
        SVEN_DEBUGSTR_FunctionExited, \
        __FUNCTION__ )

/** Macros required to trick compiler to convert __LINE__ to a string */
#define SVEN_STRINGIFY(x)   #x
/** Macros required to trick compiler to convert __LINE__ to a string */
#define SVEN_TOSTRING(x)    SVEN_STRINGIFY(x)

/** Inserts __LINE__:__FILE__  into sven debug log */
#define SVEN_AUTO_TRACE( svenh, module, unit ) \
    sven_WriteDebugStringEnd( svenh, module, unit, \
        SVEN_DEBUGSTR_AutoTrace, \
        __FILE__ ":" SVEN_TOSTRING(__LINE__) )

#define SVEN_INVALID_PARAM( svenh, module, unit ) \
    sven_WriteDebugStringEnd( svenh, module, unit, SVEN_DEBUGSTR_InvalidParam, __FUNCTION__ )

#ifdef NDEBUG
    #define SVEN_ASSERT( svenh, module, unit, cond )
#else
    #define SVEN_ASSERT( svenh, module, unit, cond ) \
        if ( ! (cond) ) sven_WriteDebugStringEnd( svenh, module, unit, \
            SVEN_DEBUGSTR_Assert, __FUNCTION__ ":" #cond  )
#endif

#define SVEN_WARN( svenh, module, unit, str ) \
    sven_WriteDebugString( svenh, module, unit, SVEN_DEBUGSTR_Warning, str )

#define SVEN_FATAL_ERROR( svenh, module, unit, str ) \
    sven_WriteDebugString( svenh, module, unit, SVEN_DEBUGSTR_FatalError, str )


/** Makes sven log come out locally.
 */
void Set_Output_SVEN_log_locally(int logLocal_bool);


#ifdef __cplusplus
}
#endif

#endif /* SVEN_PROTOS_H */
