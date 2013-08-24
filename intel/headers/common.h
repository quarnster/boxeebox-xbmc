//-----------------------------------------------------------------------------
// This file is provided under a dual BSD/GPLv2 license.  When using or 
// redistributing this file, you may do so under either license.
//
// GPL LICENSE SUMMARY
//
// Copyright(c) 2007-2010 Intel Corporation. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify 
// it under the terms of version 2 of the GNU General Public License as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but 
// WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
// The full GNU General Public License is included in this distribution 
// in the file called LICENSE.GPL.
//
// Contact Information:
//      Intel Corporation
//      2200 Mission College Blvd.
//      Santa Clara, CA  97052
//
// BSD LICENSE 
//
// Copyright(c) 2007-2010 Intel Corporation. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
//
//   - Redistributions of source code must retain the above copyright 
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in 
//     the documentation and/or other materials provided with the 
//     distribution.
//   - Neither the name of Intel Corporation nor the names of its 
//     contributors may be used to endorse or promote products derived 
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// This file contains items that can be used in both kernel and user space.
//----------------------------------------------------------------------------

#ifndef _COMMON_H_
#define _COMMON_H_

#define CONFIG_ERROR
#define CONFIG_PRINT

//----------------------------------------------------------------------------
// Message printing
//----------------------------------------------------------------------------
#ifdef __KERNEL__
    #include <linux/kernel.h> /* for printk */
    #define IO_TAG KERN_ERR
    #define PRT    printk
#else
    #include <stdio.h>
    #define IO_TAG __BASE_FILE__ ":"
    #define PRT    printf
#endif

/* Error Printing  */
#ifdef CONFIG_ERROR
#define GDL_ERROR(arg...){ PRT(IO_TAG"%s:ERROR: ", __func__); PRT(arg); }
#define GDL_WARNING(arg...) { PRT("\nWARNING: "); PRT(arg); }
#else
#define GDL_ERROR(arg...)
#define GDL_WARNING(arg...)
#endif

/* Debug printing  */
#ifdef DEBUG
#define GDL_DEBUG(arg...){ PRT(IO_TAG"%s: ", __func__); PRT(arg); }
#else
#define GDL_DEBUG(arg...)
#endif

#ifdef CONFIG_PRINT
#define GDL_PRINT(arg...){ PRT(IO_TAG"%s: ", __func__); PRT(arg); }
#else
#define GDL_PRINT(arg...)
#endif


//----------------------------------------------------------------------------
// Useful macros
//----------------------------------------------------------------------------
#define ROUND_TO_128(x) (((x) + 127) & ~0x7f)

#define VERIFY(exp, rc, error, label) \
        if (!(exp))                   \
        {                             \
            rc = error;               \
            goto label;               \
        }

#define VERIFY_QUICK(exp, label) \
        if (!(exp))              \
        {                        \
            goto label;          \
        }


#define BIT0  0x00000001
#define BIT1  0x00000002
#define BIT2  0x00000004
#define BIT3  0x00000008
#define BIT4  0x00000010
#define BIT5  0x00000020
#define BIT6  0x00000040
#define BIT7  0x00000080
#define BIT8  0x00000100
#define BIT9  0x00000200
#define BIT10 0x00000400
#define BIT11 0x00000800
#define BIT12 0x00001000
#define BIT13 0x00002000
#define BIT14 0x00004000
#define BIT15 0x00008000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

//----------------------------------------------------------------------------
// Timeouts
//----------------------------------------------------------------------------
// Timeout on interrupts in minimal module (in milliseconds)
#define MM_INTERRUPT_TIMEOUT 48

// Timeout value for framestart in microseconds
#define TIMEOUT_FRAMESTART 48000

// __get_thread_id function for user-space 
#ifndef __KERNEL__
    #include <sys/types.h>
    #include <sys/syscall.h>
    #include <unistd.h>
    #define __get_thread_id() syscall(__NR_gettid)
#endif

#endif // _COMMON_H_
