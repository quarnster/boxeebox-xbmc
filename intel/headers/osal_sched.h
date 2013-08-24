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
 *  This file contains OS abstracted interfaces to common io operations.
 */

#ifndef _OSAL_SCHED_H
#define _OSAL_SCHED_H

typedef void *os_alarm_t;

#include <os/osal_sched.h>

/**
 * Function: OS_SCHEDULE
 *
 * Description:
 *  This function can be called with the caller wished to give up processor
 *  control until the next available timeslice. This will allow other OS tasks
 *  to run before returning to the current context. This function should be
 *  used with caution and observation of reentrant coding principals.
 *
 *  void OS_SCHEDULE( void );
 */
#define OS_SCHEDULE() _OS_SCHEDULE()

/**
 * Function: OS_SLEEP
 *
 * Parameters:
 *  time_val: Unsigned long time value in Micro-seconds (1/1000000 of a second)
 *            for the task to sleep.
 *
 * Description:
 *  This function causes the caller to delay further processing
 *  for the number of micro-seconds ( 1/1000000 or a second ) requested.
 *  This function should only be used with small time values
 *  ( < 1/100 of a second ) as lengthy sleeps could degrade the kernel
 *  response time.
 *
 *  void OS_SLEEP( unsigned long time_val );
 */
#define OS_SLEEP( time_val ) _OS_SLEEP( time_val )


/**
 * Return an opaque alarm to be be used with future calls to OS_TEST_ALARM. The
 * alarm will trigger when the amount of time provided has expired.
 *
 * @param[in] time_val : time in milliseconds from current time that an alarm
 *                       will expire
 *
 * @retval os_alarm_t : alarm handle
 *
 */
#define OS_SET_ALARM( time_val ) _OS_SET_ALARM( time_val )


/**
 * Indicates if an alarm that was previously set with OS_SET_ALARM has expired.
 *
 * @param[in] alarm : An alarm previously acquired by OS_SET_ALARM()
 *
 * @retval 0 : alarm has not expired.
 * @retval >0 : alarm has expired.
 */
#define OS_TEST_ALARM(a) _OS_TEST_ALARM(a)

#endif
