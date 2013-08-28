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
#ifndef _OSAL_SEMA_H
#define _OSAL_SEMA_H


#include "osal_type.h"
#include "os/osal_sema.h"

/**
 @defgroup OSAL_SEMA OSAL semaphores

API definitions for the OSAL semaphores

@{
*/

/**
Depreciated.  Used to define a semaphore that can be reinitialized using os_sema_init_pre_inited call
*/
#define OS_DEFINE_INITIALIZED_SEMA(s)  os_sema_t s={0}

#ifdef __cplusplus
extern "C" {
#endif

/**
Depreciated.  Creates (Initializes) a semaphore that has been defined using the OS_INITIALIZED_SEMA() macro. When this call
is passed a semaphore that has already been initialized it is not
re-initialized again. 

@param sema : Pointer to the semaphore to initialize.
@param[in] initial_value : Initial value for the semaphore
*/
osal_result os_sema_init_pre_inited(os_sema_t *sema, int initial_value);

/**
Initializes a semaphore

@param sema : Pointer to the semaphore to initialize.
@param[in] initial_value : Initial value for the semaphore

@retval OSAL_SUCCESS : initialization successful
*/
osal_result os_sema_init(os_sema_t *sema, int initial_value);

/**
destroys a semaphore

@param sema : the semaphore to be destroyed
*/
void os_sema_destroy(os_sema_t *sema);
/**
Acquire a semaphore without blocking


@param sema : the semaphore to acquire
*/
int os_sema_tryget(os_sema_t *sema);
/**
Wait on a semaphore
@param sema : the semaphore to acquire
@retval int : 0 if acquired, non-zero if not acquired
*/
void os_sema_get(os_sema_t *sema);


/**
Release a semaphore

@param sema : the semaphore to release.
*/
void os_sema_put(os_sema_t *sema);

#ifdef __cplusplus
}
#endif
#endif
