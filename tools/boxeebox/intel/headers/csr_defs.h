#ifndef _CSR_DEFS_H
#define _CSR_DEFS_H 1
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

*/

/** Extension List, User-Defined "decorators" in a singly-linked list
 * that allow extended parameters for each entry in the tables.
 * Please see "csr_example.c" for how extension lists are implemented.
 */
struct EAS_Extension
{
    struct EAS_Extension        *eas_ext_next;
    const char                  *eas_ext_name;
    int                          eas_ext_type;
};

/** Bit Breakout for register definitions */
struct EAS_RegBits
{
    const char                  *regbits_name;
    int                          regbits_lsb_pos;
    int                          regbits_width;
    const char                  *regbits_comment;
    const struct EAS_Extension  *regbits_ext;
};

/** Register definition for a functional unit */
struct EAS_Register
{
    const char                  *reg_name;
    int                          reg_offset;
    const struct EAS_RegBits	*reg_bits;
    const char                  *reg_comment;
    const struct EAS_Extension  *reg_ext;
};

/** Unit Specific events */
struct SVEN_Module_EventSpecific
{
    const char                  *mes_name;      /* Module specifc event string.  This string is used in the mesage #define so don't use any funky charactes here. */
    unsigned int                 mes_subtype;   /* event subtype */
    const char                  *mes_comment;   /* comment regarding event */
    const struct EAS_Extension  *mes_ext;       /* Extension */
};

/** Reverse-engineering table for functional unit */
struct ModuleReverseDefs
{
    const char                              *mrd_name;
    unsigned int                             mrd_module;    /* MODULE */
    unsigned int                             mrd_size;
    const struct EAS_Register               *mrd_regdefs;
    const char                              *mrd_comment;
    const struct SVEN_Module_EventSpecific  *mrd_event_specific;
    const struct EAS_Extension              *mrd_ext;
};

#ifndef NULL
#define NULL ((void *)0)
#endif

#define CSR_REG(name, offset, desc)            {name, offset, NULL, desc, NULL},
#define CSR_REG_W_BB(name, offset, BB, desc)   {name, offset, BB, desc, NULL},
#define	CSR_NULL_TERM()                        {NULL,0,NULL,"",NULL}
#define CSR_BB(name, lsb, width)               {name, lsb, width, NULL, NULL}
#define	CSR_BB_NULL_TERM()                     {NULL,0,0,NULL,NULL}


#endif /* _CSR_DEFS_H */
