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

//! \file
//! This file contains OS abstracted interfaces to common io operations

/** \defgroup OSAL_IO OSAL I/O
 *
 * <I> Provides OS independent interface for debug output and also memory
 * mapped/io mapped read/write functions.
 * </I>
 *\{*/

#ifndef _OSAL_IO_H
#define _OSAL_IO_H

#include <osal_config.h>

/* Put this here so the OS specific header can get it */
#ifdef CONFIG_DEBUG
extern unsigned long osal_debug;
#endif

#include <os/osal_io.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define os_set_print_to_serial() _os_set_print_to_serial()
/**<
 * Sets the output of OS_PRINT/OS_ERROR/OS_DEBUG to the serial port.
 */

#define os_set_print_to_debugger() _os_set_print_to_debugger()
/**<
 * Sets the output of OS_PRINT/OS_ERROR/OS_DEBUG to the debugger
 * (PlatformBuilder).
 */

/**
 * \todo Linux to convert it to syslog */
#define OS_PRINT _OS_PRINT
/**<
 * Prints a string - uses printf for Linux if CONFIG_PRINT is defined, for
 * xxxxx prints a string using OutputDebugString i.e., the behaviors are not
 * consistent across OSes this macro is controlled by the CONFIG_PRINT
 * preprocessor macro, if this macro is not declared, the _OS_PRINT maps to
 * nothing
 */

#define OS_ERROR _OS_ERROR
/**<  Same as _OS_PRINT, but depends on CONFIG_ERROR */

#define OS_DEBUG _OS_DEBUG
/**<  Same as _OS_PRINT, but depends on CONFIG_DEBUG */

#define OS_DEBUG_S _OS_DEBUG_S
/**<
 * Same as _OS_PRINT, TODO for xxxxx (not implemented for xxxxx), depends on
 * CONFIG_DEBUG
 */

/**
 * Added by Darius: Audio needs this to always print no matter what...do not
 * use this for anything you want turned off
 */
#define OS_INFO _OS_INFO

#define OS_READ32(addr) _OS_READ32(addr)
/**< 32 bit read*/

#define OS_WRITE32(value, addr) _OS_WRITE32(value, addr)
/**< 32 bit write*/

#define OS_READ16(addr) _OS_READ16(addr)
/**< 16 bit read*/

#define OS_WRITE16(value, addr) _OS_WRITE16(value, addr)
/**< 16 bit write*/

#define OS_READ8(addr) _OS_READ8(addr)
/**< 8 bit read*/

#define OS_WRITE8(value, addr) _OS_WRITE8(value, addr)
/**< 8 bit write*/

#define OS_LOG_FILE     _OS_LOG_FILE

#endif
