/* 

This file is provided under a dual BSD/GPLv2 license.  When using or 
redistributing this file, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2009 Intel Corporation. All rights reserved.

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

Copyright(c) 2009 Intel Corporation. All rights reserved.
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

#ifndef CE_PAL_SOC_INFO_H
#define CE_PAL_SOC_INFO_H 1
typedef enum 
{
   SOC_GENERATION_3 = 1,
      SOC_GENERATION_4,
	  SOC_GENERATION_5
} pal_soc_generation_t;

typedef enum
{
   SOC_NAME_CE3100 = 1,
   SOC_NAME_CE4100,
   SOC_NAME_GVL
} pal_soc_name_t;

typedef enum
{
   SOC_STEPPING_A0 = 1,
   SOC_STEPPING_A1,
   SOC_STEPPING_A2,
   SOC_STEPPING_A3,
   SOC_STEPPING_A4,
   SOC_STEPPING_A5,
   SOC_STEPPING_A6,
   SOC_STEPPING_A7,
   SOC_STEPPING_A8,
   SOC_STEPPING_A9,
   SOC_STEPPING_B0,
   SOC_STEPPING_B1,
   SOC_STEPPING_B2,
   SOC_STEPPING_B3,
   SOC_STEPPING_B4,
   SOC_STEPPING_B5,
   SOC_STEPPING_B6,
   SOC_STEPPING_B7,
   SOC_STEPPING_B8,
   SOC_STEPPING_B9,
   SOC_STEPPING_C0,
   SOC_STEPPING_C1,
   SOC_STEPPING_C2,
   SOC_STEPPING_C3,
   SOC_STEPPING_C4,
   SOC_STEPPING_C5,
   SOC_STEPPING_C6,
   SOC_STEPPING_C7,
   SOC_STEPPING_C8,
   SOC_STEPPING_C9,
   SOC_STEPPING_XX
} pal_soc_stepping_t;

typedef struct _pal_soc_info_t
{
   pal_soc_generation_t generation;
   pal_soc_name_t name;
   pal_soc_stepping_t stepping;
} pal_soc_info_t;

typedef struct _soc_user_info
{
   char generation[32];
   int  gen_enum;
   char name[32];
   int  name_enum;
} soc_user_info_t;

int
system_utils_get_soc_info( soc_user_info_t *info );

#define SOC_VERSION(generation, name, stepping) ((generation<<16) | (name<<8) | (stepping)) 

#endif
