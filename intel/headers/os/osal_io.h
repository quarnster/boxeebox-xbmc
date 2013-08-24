/*
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

 */

#ifndef _OSAL_LINUX_USER_IO_H
#define _OSAL_LINUX_USER_IO_H

#include <stdio.h>

#include <osal_type.h>
#include "osal_trace.h"

osal_result os_read_port_8(unsigned short port, unsigned char* value);
osal_result os_read_port_16(unsigned short port, unsigned short* value);
osal_result os_write_port_8(unsigned short port, unsigned char value);
osal_result os_write_port_16(unsigned short port, unsigned short value);

/*#ifdef _TRACE*/
#define _OS_DEBUG(...) _os_debug(__VA_ARGS__);
#define _OS_PRINT(...) _os_print(__VA_ARGS__);
#define _OS_ERROR(...) { _os_print( __FILE__":%s:", __func__); _os_error(__VA_ARGS__); }

/*
#else
#define _OS_PRINT(...)
#define _OS_ERROR(...)
#define _OS_DEBUG(...)
#endif
*/

#define _OS_INFO(...) _os_info(__VA_ARGS__);


#define _OS_READ32(addr) *(volatile unsigned int *)(addr)
#define _OS_WRITE32(value, addr) (*(volatile unsigned int *)(addr) = (value))

#define _OS_READ16(addr) *(volatile unsigned short *)(addr)
#define _OS_WRITE16(value, addr) (*(volatile unsigned short *)(addr) = (value))

#define _OS_READ8(addr) *(volatile unsigned char *)(addr)
#define _OS_WRITE8(value, addr) (*(volatile unsigned char *)(addr) = (value))

#ifdef DEBUG_MEM
extern unsigned long global_virt_mmadr;
extern unsigned long global_virt_mmadr_offset;

static __inline unsigned int _OS_READ32(unsigned int addr)
{
    if ((addr < 0x10100000) && (addr > 0x10000000)) {
        return *(volatile unsigned int *)(addr+global_virt_mmadr-global_virt_mmadr_offset);
    }
    return *(volatile unsigned int *)(addr);
}

static __inline void _OS_WRITE32(unsigned int value, unsigned int addr) \
{
    if ((addr < 0x10100000) && (addr > 0x10000000)) {
        *(volatile unsigned int *)(addr+global_virt_mmadr-global_virt_mmadr_offset) = value;
    } else {
        *(volatile unsigned int *)addr = value;
    }
    printf("OS_WRITE32: *0x%x = 0x%x\n", addr, value);
}

static __inline unsigned int _OS_READ8(unsigned int addr)
{
    if ((addr < 0x10100000) && (addr > 0x10000000)) {
        return *(volatile unsigned char *)(addr+global_virt_mmadr-global_virt_mmadr_offset);
    }
    return *(volatile unsigned char *)addr;
}

static __inline void _OS_WRITE8(unsigned char value, unsigned int addr) \
{
    if ((addr < 0x10100000) && (addr > 0x10000000)) {
        (*(volatile unsigned char *)(addr+global_virt_mmadr-global_virt_mmadr_offset) = value);
    } else {
        (*(volatile unsigned char *)(addr) = value);
    }
    printf("OS_WRITE8: *0x%x = 0x%c\n", addr, value);
}
#endif /* DEBUG_MEM */

#endif
