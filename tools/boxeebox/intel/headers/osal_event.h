/*==========================================================================
  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.
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
 =========================================================================*/

/*
 *  This file contains OS abstracted interfaces to common event related
 *  operations.
 */

#ifndef _OSAL_EVENT_H_
#define _OSAL_EVENT_H_

#include "os/osal_event.h"
#include "osal_type.h"

/**
@defgroup event OSAL events

API definitions of OSAL event functions

@{
*/

/**
@def EVENT_NO_TIMEOUT
A constant defined to be in os_event_wait() to indicate that no wait time out
*/
#define EVENT_NO_TIMEOUT 0xFFFFFFFF


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates an event in the non-signaled state.
 *
 * @param[out]  p_event : created event
 * @param[in]  manual_reset : Indicates if the event is a manual-reset event or
 *                      not, 1 for manual reset event, 0 for auto_reset event.
 *
 * @retval OSAL_SUCCESS : event creation successful
 * @retval OSAL_ERROR : internal OSAL error
 */
osal_result
os_event_create( os_event_t* p_event,int manual_reset );

/**
 * Destroys an event object
 *
 * @param p_event : event to be destroyed
 *
 * @retval OSAL_SUCCESS : event resources destroyed
 */
osal_result
os_event_destroy(os_event_t* p_event );

/**
 * Set an event to the signaled state.
 *
 * @param p_event : Event to be set
 *
 * @retval OSAL_SUCCESS : event now set
 */
osal_result
os_event_set(os_event_t* p_event );

/**
 * Clears an event.
 *
 * @param p_event : Event to be reset
 *
 * @retval OSAL_SUCCESS : event now is now cleared
 */
osal_result
os_event_reset( os_event_t* p_event );

/**
 * Wait for an event to occur or timeout.  May also return if the process
 * receives a signal.
 *
 * @param  p_event : The handle of the event to wait for.
 * @param  wait_ms : The timeout in milliseconds to wait
 *
 * @retval OSAL_SUCCESS : an event occurred
 * @retval OSAL_TIMEOUT : timeout reached before
 * @retval OSAL_NOT_DONE: interrupted by a signal
 */
osal_result
os_event_wait(os_event_t*   p_event,unsigned long   wait_ms);


/**
 * Wait for an event to occur or timeout.  Not interrupted by signals.
 * The hard wait will only complete on timeout or an event occuring
 *
 * @param  p_event : The handle of the event to wait for.
 * @param  wait_ms : The timeout in milliseconds to wait
 *
 * @retval OSAL_SUCCESS : an event occurred
 * @retval OSAL_TIMEOUT : timeout reached before
 */
osal_result
os_event_hardwait(os_event_t*       p_event,unsigned long   wait_ms);


#ifdef __cplusplus
}
#endif


#endif  /*_OSAL_EVENT_H_ */
