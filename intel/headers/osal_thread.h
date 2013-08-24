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
 *  This file contains OS abstracted interfaces to common thread related
 *  operations.
 */

/** @file Function prototypes and declarations needed for the OSAL threads */

#ifndef _OSAL_THREAD_H_
#define _OSAL_THREAD_H_

#include "osal_type.h"
#include "osal_sema.h"
#include "os/osal_thread.h"

/** \defgroup OSAL_THREAD OSAL threads
 *
 * <I>This is the API definition for the OSAL thread  support</I>
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_SUSPENDED    0x1001
#define THREAD_ACTIVE       0x1002
#define THREAD_INACTIVE     0x1003

typedef enum {
    // NOTE: These must all be OR-able bit flags

    OS_THREAD_CREATE_SUSPENDED = 1,
        /**<
         * Thread created in suspended state, does not commence execution
         *  until os_thread_resume() is called. Default behavior is for the
         *  thread to begin executing immediately upon creation.
         */

    OS_THREAD_REALTIME       = 2
        /**<
         * Thread is created with realtime scheduling (on Linux, SCHED_RR).
         * Default behavior is to create thread with non-realtime scheduling.
         */
} os_thread_create_flags_t;

/**
 * Create a new thread.
 *
 * @param[out]  thread
 * Thread handle returned here.
 * 
 * @param[in]  priority
 * Priority of thread to be created. Note that on Linux:
 * - When creating a realtime thread (see #flags, below) the valid range of
 *   priorities is [1,99].
 * - When creating a non-realtime thread (see #flags, below) the only valid
 *   priority is 0.
 * - In User mode, priorities are effective only when the parent thread is
 *   running with root (superuser) privilege.
 * 
 * @param[in]  func
 * Pointer to the main function of the new thread.
 * 
 * @param[in]  arg
 * Value to be passed to func() when the thread starts.
 * 
 * @param[in]  flags
 * Thread creation flags: any combination of values from the enumeration
 * #os_thread_create_flags_t OR'd together.  The default (if the value 0 is
 * passed) is to create a non-realtime thread that begins execution immediately.
 *
 * @param[in] name
 * A name that should be assigned to the thread. If NULL is passed, the
 * thread is unnamed.  On Linux, this value is ignored in user space.
 */
osal_result os_thread_create(   os_thread_t *   thread,
                                void *          (*func)(void*),
                                void *          arg,
                                int             priority,
                                unsigned        flags,
                                char *          name
                                );

/**
 * Change the priority of a thread.
 *
 * @param[in]  thread
 * Thread whose priority is to be changed.
 *
 * @param[in]  priority
 * New priority for the thread.  Note that on Linux:
 * - The the valid range of priorities for realtime threads is [1,99].
 * - A non-realtime thread can only have priority 0.
 */
osal_result os_thread_set_priority( os_thread_t *thread,
                                    int          priority);

/**
 * Retrieve the current priority of a thread.
 *
 * @param[in]  thread
 * Thread whose priority is to be changed.
 *
 * @param[out]  priority
 * Current thread priority.  On Linux:
 * - The the valid range of priorities for realtime threads is [1,99].
 * - A non-realtime thread can only have priority 0.
 * 
 * @param[out] flags
 * Priority-related flags. Only currently-defined flag is OS_THREAD_REALTIME,
 * inidicating (on Linux) that the thread is running with a RT scheduling
 * alogorithm. Otherwise, the value returned will be 0.
 * 
 */
osal_result os_thread_get_priority( os_thread_t *thread,
                                    int         *priority,
                                    int         *flags);


/**
 * Resume execution of a thread that is in the SUSPENDED state.
 *
 * @param[in]  thread  Thread to be resumed.
 */
osal_result os_thread_resume(os_thread_t *thread);

/**
 * Exit from a thread. To be called from within the thread function.
 * The parent should release the thread object by calling os_thread_destroy().
 *
 * @param[in]  thread   The thread to be exited.
 * @param[in]  retcode  The exit code returned by the thread to its parent.
 */
void os_thread_exit(os_thread_t * thread, unsigned long retcode);


/** Terminate a thread.
 *
 * @param[in]  thread   The thread to be terminated.
 */
osal_result os_thread_terminate(os_thread_t *thread);


/**
 * Suspend a thread's execution.
 *
 * @param[in\  thread   The thread to be suspended.
 */
osal_result os_thread_suspend(os_thread_t *thread);


/**
 * Close a thread handle and free related resources.
 *
 * @param[in]  thread   The thread to be destroyed.
 */
osal_result os_thread_destroy(os_thread_t *thread);


/**
 * Wait for one or more thread to exit.
 *
 * @param  thread   Arrary of thread(s) to wait on.
 * @param  count    Number of threads in the array.
 */
osal_result os_thread_wait(os_thread_t *thread, int count);


/**
 * Get the state of a thread. The state can be one of:
 *  - THREAD_CREATE_SUSPENDED
 *  - THREAD_SUSPENDED
 *  - THREAD_ACTIVE
 *  - THREAD_INACTIVE
 *
 * @param[in]  thread    The thread whose state is requested.
 * @param[out] state     Thread state returned here.
 */
osal_result os_thread_get_state(os_thread_t *thread, int *state);


/**
 * Suspend execution of the current thread for at least the specified time
 * duration.
 *
 * @param[in]  ms   Number of milliseconds for which thread should be suspended.
 */
void os_sleep(unsigned long ms);


/**
 * Call from within a thread to relinqluish the remainder of its time slice to
 * any other thread which is ready to run.
 */
void os_thread_yield(void);

#ifdef __cplusplus
}
#endif

#endif
