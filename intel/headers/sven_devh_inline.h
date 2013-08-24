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

*/
 /*
 * Authors:
 *  Pat Brouillette <pat.brouillette@intel.com>
 *
 *----------------------------------------------------------------------------
 */
//! \file 
//! This file contains Inline Implementations of Instrumented OSAL functions.

/** \defgroup devio devio
 *
 * <I> This file contains Inline Implementations of Instrumented OSAL functions.
 * </I>
 * 
 *\{*/

#ifndef _SVEN_DEVH_INLINE_H
#define _SVEN_DEVH_INLINE_H

#ifndef SVEN_DEVH_DISABLE_SVEN
//    #define SVEN_DEVH_WARNING_REG_WRITES
//    #define SVEN_DEVH_WARNING_REG_READS
#endif


#ifndef SVEN_DEVH_DISABLE_SVEN
/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_Write32bitRegIoEvent_inline(
    os_devhandle_t      *devh,
    uint32_t             reg_offset,
    enum SVEN_EV_RegIo_t reg_io_type,
    unsigned int         reg_value,
    unsigned int         reg_mask )
{
    struct SVENEvent        ev;

    if ( ! devh_IsEventHotEnabled(devh,SVENHeader_DISABLE_REGIO) )
        return;

    _sven_initialize_event_top( &ev,
        devh->devh_sven_module,
        devh->devh_sven_unit,
        SVEN_event_type_register_io,
        reg_io_type );

    ev.u.se_reg.phys_addr   = devh->devh_regs_phys_addr + reg_offset;
    ev.u.se_reg.value       = reg_value;
    ev.u.se_reg.mask        = reg_mask;
    ev.u.se_ulong[3]        = 0;
    ev.u.se_ulong[4]        = 0;
    ev.u.se_ulong[5]        = 0;

    sven_write_event( &devh->devh_svenh, &ev );
}
/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_Write64bitRegIoEvent_inline(
    os_devhandle_t      *devh,
    uint32_t             reg_offset,
    enum SVEN_EV_RegIo_t reg_io_type,
    uint64_t             reg_value,
    uint64_t             reg_mask )
{
    struct SVENEvent        ev;

    if ( ! devh_IsEventHotEnabled(devh,SVENHeader_DISABLE_REGIO) )
        return;

    _sven_initialize_event_top( &ev,
        devh->devh_sven_module,
        devh->devh_sven_unit,
        SVEN_event_type_register_io,
        reg_io_type );

    ev.u.se_reg.phys_addr   = devh->devh_regs_phys_addr + reg_offset;
    ev.u.se_reg.value       = (unsigned long) reg_value;
    ev.u.se_reg.mask        = (unsigned long) reg_mask;
    ev.u.se_ulong[3]        = (unsigned long) (reg_value >> 32);
    ev.u.se_ulong[4]        = (unsigned long) (reg_mask >> 32);
    ev.u.se_ulong[5]        = 0;

    sven_write_event( &devh->devh_svenh, &ev );
}
#endif

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline uint32_t devh_ReadReg32_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset )
{
    uint32_t         value;

#ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, -1 );
#endif
    
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    value = *(uint32_t *)((char *)devh->devh_regs_ptr + reg_offset);
#else
    value = (*devh->devh_ops->ReadReg32)( devh, reg_offset );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
#endif

    return( value );
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline int32_t devh_ReadReg32_pollingmode_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         previously_read_value )
{
    uint32_t         value;

#ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, -1 );
#endif

#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    value = *(uint32_t *)((char *)devh->devh_regs_ptr + reg_offset);
#else
    value = (*devh->devh_ops->ReadReg32)( devh, reg_offset );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    if ( value != previously_read_value )
    {
        devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
    }
#endif

    return( value );
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_WriteReg32_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         value )
{
#ifdef SVEN_DEVH_WARNING_REG_WRITES
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, -1, -1 );
#endif

#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    *(volatile uint32_t *)((char *)devh->devh_regs_ptr + reg_offset) = value;
#else
    (*devh->devh_ops->WriteReg32)( devh, reg_offset, value );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, value, 0 );
#endif
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_OrBitsReg32_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         or_value )
{
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_OrBits, -1, -1 );
    #endif
    
    *(volatile uint32_t *)((char *)devh->devh_regs_ptr + reg_offset) |= or_value;

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_OrBits, or_value, 0 );
    #endif
#else
    uint32_t          value;

    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, -1 );
    #endif
    
    value = (*devh->devh_ops->ReadReg32)( devh, reg_offset );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
    #endif

    value |= or_value;

    (*devh->devh_ops->WriteReg32)( devh, reg_offset, value );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, value, 0 );
    #endif
#endif

}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_AndBitsReg32_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         and_value )
{
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_AndBits, -1, -1 );
    #endif
    
    *(volatile uint32_t *)((char *)devh->devh_regs_ptr + reg_offset) &= and_value;

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_AndBits, and_value, 0 );
    #endif
#else
    uint32_t            value;

    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, -1 );
    #endif
    
    value = (*devh->devh_ops->ReadReg32)( devh, reg_offset );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
    #endif

    value &= and_value;

    (*devh->devh_ops->WriteReg32)( devh, reg_offset, value );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, value, 0 );
    #endif
#endif
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_SetMaskedReg32_inline(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         and_value,
    uint32_t         or_value )
{
    uint32_t         value;
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, 0 );
    #endif
    value = *(uint32_t *)((char *)devh->devh_regs_ptr + reg_offset);
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
    #endif
    value &= and_value; /* clear bits */
    value |= or_value;  /* set bits */
    *(volatile uint32_t *)((char *)devh->devh_regs_ptr + reg_offset) = value;
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, value, 0 );
    #endif
#else
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, -1, 0 );
    #endif
    value = (*devh->devh_ops->ReadReg32)( devh, reg_offset );
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Read, value, 0 );
    #endif
    value &= and_value; /* clear bits */
    value |= or_value;  /* set bits */
    (*devh->devh_ops->WriteReg32)( devh, reg_offset, value );
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write32bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo32_Write, value, 0 );
    #endif
#endif
}

/** -------------------------- 64 bit register IO -------------------------- */

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline uint64_t devh_ReadReg64_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset )
{
    uint64_t         value;

#ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, -1 );
#endif
    
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    value = *(uint64_t *)((char *)devh->devh_regs_ptr + reg_offset);
#else
    value = (*devh->devh_ops->ReadReg64)( devh, reg_offset );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
#endif

    return( value );
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline int64_t devh_ReadReg64_pollingmode_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         previously_read_value )
{
    uint64_t         value;

#ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, -1 );
#endif

#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    value = *(uint64_t *)((char *)devh->devh_regs_ptr + reg_offset);
#else
    value = (*devh->devh_ops->ReadReg64)( devh, reg_offset );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    if ( value != previously_read_value )
    {
        devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
    }
#endif

    return( value );
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_WriteReg64_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         value )
{
#ifdef SVEN_DEVH_WARNING_REG_WRITES
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, -1, -1 );
#endif

#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    *(volatile uint64_t *)((char *)devh->devh_regs_ptr + reg_offset) = value;
#else
    (*devh->devh_ops->WriteReg64)( devh, reg_offset, value );
#endif

#ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, value, 0 );
#endif
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_OrBitsReg64_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         or_value )
{
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_OrBits, -1, -1 );
    #endif
    
    *(volatile uint64_t *)((char *)devh->devh_regs_ptr + reg_offset) |= or_value;

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_OrBits, or_value, 0 );
    #endif
#else
    uint64_t          value;

    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, -1 );
    #endif
    
    value = (*devh->devh_ops->ReadReg64)( devh, reg_offset );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
    #endif

    value |= or_value;

    (*devh->devh_ops->WriteReg64)( devh, reg_offset, value );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, value, 0 );
    #endif
#endif

}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_AndBitsReg64_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         and_value )
{
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_AndBits, -1, -1 );
    #endif
    
    *(volatile uint64_t *)((char *)devh->devh_regs_ptr + reg_offset) &= and_value;

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_AndBits, and_value, 0 );
    #endif
#else
    uint64_t            value;

    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, -1 );
    #endif
    
    value = (*devh->devh_ops->ReadReg64)( devh, reg_offset );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
    #endif

    value &= and_value;

    (*devh->devh_ops->WriteReg64)( devh, reg_offset, value );

    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, value, 0 );
    #endif
#endif
}

/** INLINE Function, do not call directly!  Should only be used sparingly as it inserts code */
static __inline void devh_SetMaskedReg64_inline(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         and_value,
    uint64_t         or_value )
{
    uint64_t         value;
#ifdef SVEN_DEVH_FORCE_DIRECT_RW
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, 0 );
    #endif
    value = *(uint64_t *)((char *)devh->devh_regs_ptr + reg_offset);
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
    #endif
    value &= and_value; /* clear bits */
    value |= or_value;  /* set bits */
    *(volatile uint64_t *)((char *)devh->devh_regs_ptr + reg_offset) = value;
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, value, 0 );
    #endif
#else
    #ifdef SVEN_DEVH_WARNING_REG_READS
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, -1, 0 );
    #endif
    value = (*devh->devh_ops->ReadReg64)( devh, reg_offset );
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Read, value, 0 );
    #endif
    value &= and_value; /* clear bits */
    value |= or_value;  /* set bits */
    (*devh->devh_ops->WriteReg64)( devh, reg_offset, value );
    #ifndef SVEN_DEVH_DISABLE_SVEN
    devh_Write64bitRegIoEvent_inline( devh, reg_offset, SVEN_EV_RegIo64_Write, value, 0 );
    #endif
#endif
}

#endif /* _SVEN_DEVH_INLINE_H */
