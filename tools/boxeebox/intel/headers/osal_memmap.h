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
 *  This file contains OS abstractions for memory mapping of bus addresses.
 */
#ifndef _OSAL_IO_MEMMAP_H
#define _OSAL_IO_MEMMAP_H

#include <os/osal_memmap.h>

/**
 * @defgroup memmap OSAL Memory Mapping
 * 
 * Provides OS independent interface for mapping/unmapping physical addresses
 * to virtual addresses
 * @{
 */

/**
 * Reserves a range of virtual memory space of "size" and map the that virtual
 * address to the hardware "base_address" provided. This function call will try
 * and enable caching of read write transactions to the region.
 *
 * @param[in] base_address : the base bus/hardware address that will be used as
 *                           the target of the memory mapping.
 * @param[in] size : the size of the virtual memory range requested to be mapped
 *
 * @retval NULL for failure OR valid virtual address casted as a void *
 */
#define OS_MAP_IO_TO_MEM_CACHE(base_address, size) _OS_MAP_IO_TO_MEM_CACHE(base_address, size)

/**
 * Reserves a range of virtual memory space of "size" and map the that virtual
 * address to the hardware "base_address" provided. This function call will NOT
 * cache any read/write transactions to the region and apply the access
 * directly to the hardware bus address that is mapped.
 *
 * @param[in] base_address : the base bus/hardware address that will be used as
 *                           the target of the memory mapping.
 * @param[in] size : the size of the virtual memory range requested to be mapped
 *
 * @retval NULL for failure OR valid virtual address casted as a void *
 */
#define OS_MAP_IO_TO_MEM_NOCACHE(base_address, size)    _OS_MAP_IO_TO_MEM_NOCACHE(base_address, size)
#define OS_MAP_IO_TO_LARGE_MEM_NOCACHE(base_address, size)  _OS_MAP_IO_TO_LARGE_MEM_NOCACHE(base_address, size)

/**
 * Unmaps the range of virtual memory space of "size" that  was previously
 * mapped with any of the above functions.
 *
 * @param[in] virt_address : the base bus or hardware address that will be used
 *                           as the target of the memory  mapping.
 * @param[in] size : the size of the virtual memory range as requested in
 *                          os_map_io*
 */
#define OS_UNMAP_IO_FROM_MEM(virt_address, size)            _OS_UNMAP_IO_FROM_MEM(virt_address, size)

#endif
