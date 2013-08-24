#ifndef SVEN_H
#define SVEN_H 1
/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007-2009 Intel Corporation. All rights reserved.
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

Pat Brouillette
*/

#ifndef SVEN_EVENT_H
#include "sven_event.h"
#endif

#ifndef SVEN_EVENT_TYPE_H
#include "sven_event_type.h"
#endif

#ifndef SVEN_MODULE_H
#include "sven_module.h"
#endif


#ifndef SVEN_FIRMWARE_TX
   #include "svenreverse.h"
   #include "csr_defs.h"
#endif /* SVEN_FIRMWARE_TX */

struct SVENCircBuffer
{
	unsigned int svc_physaddr;
	unsigned int svc_size;
	unsigned int svc_pos;
	unsigned int svc_id;
	char pad[112]; // pad to 128 bytes
};

#if defined(__XSCALE__) || defined(__LINUX_ARM_ARCH__)
    #define SVEN_CIRCBUFFER_ID_CPU_KERNEL       0
    #define SVEN_CIRCBUFFER_ID_CPU_USER         1
    #define SVEN_CIRCBUFFER_ID_FIRST_FIRMWARE   2
#else
    #define SVEN_CIRCBUFFER_ID_CPU_KERNEL       0
    #define SVEN_CIRCBUFFER_ID_CPU_USER         0
    #define SVEN_CIRCBUFFER_ID_FIRST_FIRMWARE   1
#endif


/** SVEN Header (version 2) structure in SHARED Memory.
 *  Software connects.  Added Physical Address and total
 *  memory pool size, not just offset/size of the event
 *  buffer.  This should become the next header size
 *  once the kernel driver is updated.
 */
struct _SVENHeader
{
    unsigned int           svh_version;
        #define SVENHeader_VERSION_2        (('S'<<24)|('V'<<16)|('E'<<8)|('2'))

    /* TODDO: Heavily document operation of disable mask */
    unsigned int           svh_disable_mask_deprecated;  /* for system HALT and Security */
        #define SVENHeader_DISABLE_ANY          (1<< 0)  /* All writers should check for this */
        #define SVENHeader_DISABLE_STRINGS      (1<< 1)  /* String Writers should check for this */
        #define SVENHeader_DISABLE_REGIO        (1<< 2)  /* Register IO functions check */
        #define SVENHeader_DISABLE_FUNCTION     (1<< 3)  /* Function Entered / Exited / AutoTrace */
        #define SVENHeader_DISABLE_SECURITY     (1<< 4)  /* security data should not pass */
        #define SVENHeader_DISABLE_KERNEL_HI    (1<< 5)  /* Kernel macro activity (ISR,Context,..) */
        #define SVENHeader_DISABLE_KERNEL_LO    (1<< 6)  /* Kernel micro activity (WARNING: VERBOSE) */
        #define SVENHeader_DISABLE_SMD          (1<< 7)  /* Streaming Media Driver Activity */
        #define SVENHeader_DISABLE_MODULE       (1<< 8)  /* Module Specific Events */
        #define SVENHeader_DISABLE_PERFORMANCE  (1<< 9)  /* Performance Measurement Events */
        #define SVENHeader_DISABLE_FW           (1<<10)  /* Performance Measurement Events */
        #define SVENHeader_DISABLE_API          (1<<11)  /* API Events */
         /* host fills up from LSB */
         /* FW fills down from MSB */
        #define SVENHeader_DISABLE_FW_DSP       (1<<26)  /* FW TX from DSPs */
        #define SVENHeader_DISABLE_FW_SEC       (1<<27)  /* FW TX from Security */
        #define SVENHeader_DISABLE_FW_DPE       (1<<28)  /* FW TX from Display Proc */
        #define SVENHeader_DISABLE_FW_MFD       (1<<29)  /* FW TX from Video Decoder  */
        #define SVENHeader_DISABLE_FW_GV        (1<<30)  /* FW TX from Global Vsparc */
        #define SVENHeader_DISABLE_FW_DEMUX     (1<<31)  /* FW TX from DEMUX */

    unsigned int           svh_debug_flags;

   unsigned int            svh_hdr_physaddr;
   unsigned int            svh_hdr_size;
   unsigned int            svh_buff_physaddr;
   unsigned int            svh_buff_size;
   unsigned int            svh_circbuffer_count;
   unsigned int            svh_dfx_physaddr;   /* Added 2007-12-27 pbrouill */
   unsigned int            svh_dfx_size;       /* Added 2007-12-27 pbrouill */

   char                    pad[88]; // pad to 128 bytes

   struct SVENCircBuffer   buffers[0];
};

/** Generic "White Box" SvenHandle.
 *  This structure is often embedded in larger
 *  user-defined structs.
 */
struct SVENHandle
{
	struct _SVENHeader      *hdr;	            /* header area */
	struct SVENEvent        *en;	            /* circular list of events */
	unsigned int             event_pos_mask;  /* mask */
	short                    buffnum;         /* which circbuf */
	short                    event_pos_lsb;   /* which lsb */
	volatile unsigned int   *phot;            /* TX hot mask */
	volatile unsigned int   *ptime;           /* TX timestamp */
	volatile unsigned int   *pinc;            /* TX increment */
	volatile unsigned int   *pcur;            /* TX current writer position */
};

#ifndef SVEN_FIRMWARE_TX
   #ifndef SVEN_INLINE_H
   #include "sven_inline.h"
   #endif
#endif /* SVEN_FIRMWARE_TX */

#endif /* SVEN_H */





