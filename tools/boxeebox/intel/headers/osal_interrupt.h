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

#ifndef _OSAL_INTERRUPT_H
#define _OSAL_INTERRUPT_H

/**
@defgroup interrupt OSAL interrupts

Provides OS independent interface for registering bottom half interrupt handlers
@{
*/


/**
 * This is an opaque data type that serves as a handle for an allocated
 * interrupt. It should not be modified or used for any purpose other than to
 * free the interrupt at a later time.
 */
typedef void * os_interrupt_t;

/**
 * This function pointer is used as an interrupt handler. When the interrupt is
 * triggered the function is called. The IRQ parameter is the integer number of
 * the irq that has been triggered, and the data pointer is the data provided
 * during the allocation of the interrupt.  
 *
 * These handlers run in the context of user level thread (pthreads) in Linux
 * user mode.
 *
 * The implementation for Linux kernel mode runs this code as an interrupt
 * handler (ISR) in kernel mode
 *
 * The following documentation exists in the orignal EID code:
 *
 * Interrupt handlers MUST meet these criteria
 * - They MUST NOT sleep.
 * - Interrupts are called with interrupts disabled or at least interrupts of
 *   equal or lower priority disabled.
 * - Any memory allocation, timers etc must not sleep.
 * - They MUST allow for interrupt sharing with other devices.
 * - They MUST return in a very short amount of time.
 *
 * @param data
 *      A void pointer to what ever data is needed by the handler.
 */
typedef void (os_interrupt_handler_t)(void *data);


/**
 * Used to Register a hard interrupt handler.
 *
 * Allows configuring a hard interrupt handler (a.k.a. a top half handler),
 * with all of the good and bad that comes with it.  Advantages include
 * generally lower latencies and less overhead.  The cost, is greatly reduced
 * access to OS functionality.  The handler may not do anything that would
 * sleep or schedule.  No calling sleep functions, memory mapping or allocation
 * functions, etc.  Assume anything other than register access and event wake
 * up functions are not safe unless their safety has been verified.
 *
 * There may also be shared interrupts.  An interrupt handler cannot assume
 * that because it was called, its device raised an interrupt.
 *
 * @param[in]  irqnum
 *      Number of the interrupt to register for.
 * @param[out] irqhandle
 *      If registration is successful, returns with a value used for
 *      unregistration later.
 * @param[in]  irqhandler
 *      Function to invoke when an interrupt occurs.
 * @param[in]  data
 *      User supplied pointer that will be passed to the irqhandler when when
 *      an interrupt occurs.
 * @param[in]  name
 *      String that may be used by the OS for information display purposes.
 */
osal_result os_register_top_half(
        int                     irqnum,
        os_interrupt_t *        irqhandle,
        os_interrupt_handler_t *irqhandler,
        void *                  data,
        const char *            name);

/**
 * Unregister a hard interrupt handler
 *
 * Removes an interrupt handler.  After this function returns, the interrupt
 * handler will not be called again.
 *
 * @param[in]  irqhandle
 *      Handle returned by registration to determine what handler to remove.
*/
osal_result os_unregister_top_half(os_interrupt_t *irqhandle);

// DEFINE TO HELP THE TRANSITION TO PAL 
#define os_acquire_interrupt(a,b,c,d,e) pal_acquire_interrupt(a,b,c,d,e)
#define os_release_interrupt(x) pal_release_interrupt(x)

#define OS_DISABLE_INTERRUPTS _OS_DISABLE_INTERRUPTS
#define OS_ENABLE_INTERRUPTS  _OS_ENABLE_INTERRUPTS

#endif
