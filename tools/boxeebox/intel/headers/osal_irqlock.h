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

#include "osal.h"
#include "os/osal_irqlock.h"


/**
 * irqlock: interrupt-safe locks
 *
 * interrupt safe locks can be used to provide serialization between an
 * interrupt handler and non-interrupt (i.e. process/thread) contexts.
 *
 * Use of an irq-safe lock comes at a cost: It must NOT be held for extended
 * periods of time.  The shorter the better.  While holding an irq-safe lock,
 * code has the same restrictions as an interrupt handler: no sleeping or
 * scheduling, or calling any other function that may do so.
 */


/**
 * Initialize an interrupt safe lock
 * 
 * @param lock: lock structure to initialize to an unlocked state.
 */
osal_result os_irqlock_init(os_irqlock_t *lock);

/**
 * Grab the interrupt-safe lock
 * 
 * Sets the lock to the locked state while preventing interrupts from running
 * REMEMBER: once this lock is acquired, the running code has the same
 * restrictions as an interrupt handler.
 *
 * @param lock:  interrupt safe lock to grab and set locked
 * @param state: context local storage area for saving interrupt states.
 *                  Allocate it on the stack.
 */
osal_result os_irqlock_acquire(os_irqlock_t *lock, os_irqlock_local_t *state);


/**
 * Unlock an interrupt-safe lock
 * 
 * Sets the lock to unlocked and re-enables interrupts.  Must supply the state
 * value configured by the acquire call.
 *
 * @param lock:  Interrupt-safe lock to unlock
 * @param state: state information from the acquire call.
 */
osal_result os_irqlock_release(os_irqlock_t *lock, os_irqlock_local_t *state);

/**
 * Perform any cleanup needed before an interrupt-safe lock can be deallocated
 * 
 * @param lock: interrupt safe lock to clean up.
 */
osal_result os_irqlock_destroy(os_irqlock_t *lock);
