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
 * This file contains OS abstracted interfaces to common memory operations.
 */

#ifndef _OSAL_MEMORY_H
#define _OSAL_MEMORY_H

#include <os/osal_memory.h>

#define OS_ALLOC(a)             _OS_ALLOC(a)
#define OS_ALLOC_PAGE()         _OS_ALLOC_PAGE()
#define OS_ALLOC_NONBLOCK(a)    _OS_ALLOC_NONBLOCK(a)
#define OS_ALLOC_LARGE(a)       _OS_ALLOC_LARGE(a)

#define OS_FREE(a)              _OS_FREE(a)
#define OS_FREE_NONBLOCK(a)     _OS_FREE_NONBLOCK(a)
#define OS_FREE_LARGE(a)        _OS_FREE_LARGE(a)

#define OS_MEMZERO(a,b)         _OS_MEMZERO(a,b)
#define OS_MEMSET(a,b,c)        _OS_MEMSET(a,b,c)
#define OS_MEMCPY(a,b,c)        _OS_MEMCPY(a,b,c)
#define OS_MEMCMP(a,b,c)        _OS_MEMCMP(a,b,c)

/*
 * These functions may not be used within any code that operates in
 * user-space. They are available in select OSAL implementations only.
 */
#define OS_VIRT_TO_PHYS(a) _OS_VIRT_TO_PHYS(a)

#define OS_ALLOC_CONTIGUOUS(a,b)    _OS_ALLOC_CONTIGUOUS(a,b)
#define OS_FREE_CONTIGUOUS(a)       _OS_FREE_CONTIGUOUS(a)

/*
 * This is a OS independent helper is case the operating environment
 * does not supply a memcmp() function. The OSAL may use this implementation.
 */
static __inline int _os_memcmp(const void *s1, const void *s2, unsigned long n)
{
    const unsigned char *cs1 = (const unsigned char *)s1;
    const unsigned char *cs2 = (const unsigned char *)s2;

    for ( ; n-- > 0; cs1++, cs2++) {
        if (*cs1 != *cs2) {
            return *cs1 - *cs2;
        }
    }
    return 0;
}

#endif
