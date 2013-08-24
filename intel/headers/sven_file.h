#ifndef SVEN_FILE_H
#define SVEN_FILE_H 1
/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2008 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007-2008 Intel Corporation. All rights reserved.
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

#include <stdint.h>

struct SVENFile_TraceHeader
{
    uint32_t            trace_type;         /* SVEN,OMAR */
      #define BLACKBOX_RECORDING_TRACE_SVEN  (('S'<<24)|('V'<<16)|('E'<<8)|('N'))
      #define BLACKBOX_RECORDING_TRACE_OMAR  (('O'<<24)|('M'<<16)|('A'<<8)|('R'))
    uint32_t            hclk_frequency;     /* frequency of HCLK, cycles-per-second */
    uint32_t            timestamp_divider;  /* Divide HCLK by this to get counter freq */
    uint32_t            pad0;               /* spectromangulation */

    /* 64-bit file offsets to be used with:
     * int fgetpos(FILE *stream, fpos_t *pos);
     * int fsetpos(FILE *stream, fpos_t *pos);
     */
    uint64_t            file_offset;        /* where does the trace start (within file) */
    uint64_t            file_size;          /* how big is the trace */

    uint64_t            capture_start;      /* time (hclk) capture was initiated */

    uint64_t            capture_stop;       /* time (hclk) capture was stopped */

    uint32_t            pad1[4];            /* pad to a power of two */
};

struct _SVENFileHeader
{
    uint32_t            svenfile_version;           /* should be able to determine endian-ness */
        #define SVENFileHeader_VERSION_1        (('S'<<24)|('V'<<16)|('F'<<8)|('1'))

    uint32_t            svenfile_hdr_size;          /* size of the header */

    uint32_t            svenfile_num_traces;

    uint32_t            svenfile_hdr_continue;

    struct SVENFile_TraceHeader svenfile_trace[1];  /* more than this if necessary */
};

#endif /* SVEN_FILE_H */
