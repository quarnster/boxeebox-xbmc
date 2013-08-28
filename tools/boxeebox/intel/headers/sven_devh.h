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

/** \defgroup io io
 *
 * <I> Provides OS independent interface for Instrumented Device IO Functions
 * </I>
 *
 *\{*/

#ifndef _SVEN_DEVH_H
#define _SVEN_DEVH_H

#ifndef _OSAL_IO_MEMMAP_H
#include <osal_memmap.h>
#endif

#ifndef _OSAL_SYNC_H
#include <osal_lock.h>
#endif

#ifndef _OSAL_ASSERT_H
#include <osal_assert.h>
#endif

#include <stdint.h>

/* should configure device IO parameters */
//#include <os/osal_devio.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* ----------------------------------------------------------------------- */
/* Instrumented OSAL Configuration detection/defaults area */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/** Compile-time switch, disable SVEN Event logging by uncommenting next line */
//#define SVEN_DEVH_DISABLE_SVEN


/** Compile-time switch, uncomment next line to do Register IO using 
 *  direct-memory accesses instead of through
 */
//#define SVEN_DEVH_FORCE_DIRECT_RW


#ifndef SVEN_DEVH_CHECK_MODULE_DISABLE

    /** Compile-time switch, check "svh_disable_mask" in the shared header.
     * This feature allows for dynamic ramping of what category of events
     * get recorded into the hard-realtime stream.  The operator can modify:
     *
     * _SVENHeader->svh_disable_mask
     *
     * at run-time to throttle RT-recording of events dynamically, even while
     * test applications are running.
     */
    #define SVEN_DEVH_CHECK_MODULE_DISABLE   1

    #ifndef SVEN_DEVH_MODULE_DISABLE_MASK

        /** Compile-time mask
         *  At runtime, AND "svh_disable_mask" with this value to
         *  determine if we're supposed to record events.
         */
        #define SVEN_DEVH_MODULE_DISABLE_MASK  0x00000001

    #endif

#endif

#ifndef SVEN_H
#include "sven.h"
#endif

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**
 *
 */
typedef struct os_devhandle {
    struct os_devh_ops         *devh_ops;             /* Device Operations */
    int                         devh_last_error;      /* Last hard error encountered during IO */

    os_lock_t                   devh_lock;            /* for app/driver writer mutual exclusion */

    void                       *devh_os_data;         /* OS-Specific Register Access info */
    void                       *devh_regs_ptr;        /* Register Pointers in memory, if available */
    unsigned long               devh_regs_phys_addr;  /* TODO: should be prepped for 64 bits */
    unsigned long               devh_regs_size;       /* size of register base */

    const struct ModuleReverseDefs *devh_mrd;         /* "Reverse" definitions */
    struct SVENHandle           devh_svenh;           /* SVEN Handle for recording IOs */
    int                         devh_sven_module;     /* e.g. SVEN_module_VR_VCAP */
    int                         devh_sven_unit;       /* e.g. 0, 1, 2, .... */
    int                         devh_sven_ddis;       /* "dynamic disable" bits */

    unsigned long               devh_reserved[4];     /* reserved, must be NULL */
} os_devhandle_t;

/**
 * Register-map based device operation functions
 */
typedef struct os_devh_ops {

   /** size, in bytes, of this devop structure */
    unsigned long       devh_op_size;

    void     (*Close)(      os_devhandle_t *devh );

    int      (*InjectRead)( os_devhandle_t *devh, const char *param_name, void *pvalue, int len );
    int      (*InjectWrite)(os_devhandle_t *devh, const char *param_name, const void *pvalue, int len );

    uint32_t (*ReadReg32)(  os_devhandle_t *devh, uint32_t reg_offset );
    void     (*WriteReg32)( os_devhandle_t *devh, uint32_t reg_offset, uint32_t value );

    uint16_t (*ReadReg16)(  os_devhandle_t *devh, uint32_t reg_offset );
    void     (*WriteReg16)( os_devhandle_t *devh, uint32_t reg_offset, uint16_t value );

    uint8_t  (*ReadReg8)(   os_devhandle_t *devh, uint32_t reg_offset );
    void     (*WriteReg8)(  os_devhandle_t *devh, uint32_t reg_offset, uint8_t value );

    uint64_t (*ReadReg64)(  os_devhandle_t *devh, uint32_t reg_offset );
    void     (*WriteReg64)( os_devhandle_t *devh, uint32_t reg_offset, uint64_t value );

} os_devh_ops_t;

/* ----------------------------------------------------------------------- */
/* The user-mode API */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**
 * @brief         Should an event with these flags be written into the Nexus
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         event_flags : flags defining the event to be written
 *
 * @returns       TRUE if event should be written, FALSE if event should not.
 *
 * @see           struct _SVENHeader
 */
static __inline int devh_IsEventHotEnabled(
    os_devhandle_t          *devh,
    int                      event_flags )
{
#ifdef SVEN_DEVH_DISABLE_SVEN
    (void) devh;
    (void) event_flags;
    return(0);
#else
    if ( NULL != devh )
    {
        event_flags |= SVEN_DEVH_MODULE_DISABLE_MASK;
        event_flags |= SVENHeader_DISABLE_ANY;
        event_flags |= devh->devh_sven_ddis;
    
        if ( *devh->devh_svenh.phot & event_flags )
        {
            return(0);
        }
    }
    return(1);
#endif
}

/**
 * @brief         Create a DevHandle
 *
 * @param         desc     : "Creation assist" description, or use NULL.  desc
 *                           will be used to arrange for different connection
 *                           methods to the device, Examples:
 *                              "NULL"         - NULL-implementation (no register IO)
 *                              "ITP:vrlab007" - Use ITP protocol to DUT
 *                              "HCI:host03"   - Use HCI protocol to DUT host
 *                              "PLX:1"        - Addresses off PLX device localbus
 *
 * @returns       FALSE on failure, TRUE on success
 */
os_devhandle_t *devhandle_factory(
    const char *desc );

/**
 * @brief         Connect to a device's registers via PCI bus,dev,function,bar
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         pci_bus     :
 * @param         pci_dev     :
 * @param         pci_fun     :
 * @param         pci_bar     :
 *
 * @returns       FALSE on failure, TRUE on success
 */
int devhandle_connect_pci_bus_dev_fun_bar(
    os_devhandle_t          *devh,
    int                      pci_bus,
    int                      pci_dev,
    int                      pci_fun,
    int                      pci_bar );

/**
 * @brief         Connect to a device's registers via physical address and size
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         phys_addr   : physical address of MMIO registers
 * @param         regs_size   : size of devices address space
 *
 * @returns       FALSE on failure, TRUE on success
 */
int devhandle_connect_physical_address(
    os_devhandle_t          *devh,
    int64_t                  phys_addr,
    int                      regs_size );

/**
 * @brief         Connect to a device's registers via name, (using internal lookup)
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         devname     : device "name" to connect with
 *
 * @returns       FALSE on failure, TRUE on success
 */
int devhandle_connect_name(
    os_devhandle_t          *devh,
    const char              *devname );

int devhandle_connect_sim_name(
    os_devhandle_t          *devh,
    const char              *devname,
    void *                   ptrsimbuffer,
    unsigned long            buffersize);

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/**
 * @brief         Gain Exclusive access to a device handle.
 *
 * @param         devh        : device handle (associated with a MMR device)
 */
void devh_Block(
    os_devhandle_t          *devh );

/**
 * @brief         Release Exclusive access to a device handle.
 *
 * @param         devh        : device handle (associated with a MMR device)
 */
void devh_Unblock(
    os_devhandle_t          *devh );


/**
 * @brief         Delete a DevHandle.  Closing associated resources.
 *
 * @param         devh        : device handle (associated with a MMR device)
 */
void devh_Delete(
    os_devhandle_t          *devh );

/**
 * @brief         Should an event with these flags be written into the Nexus
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         event_flags : flags defining the event to be written
 *
 * @returns       TRUE if event should be written, FALSE if event should not.
 *
 * @see           struct _SVENHeader
 */
int devh_IsEventHotEnabled(
    os_devhandle_t          *devh,
    int                      event_flags );

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**
 * @brief         Read a 32 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 *
 * @returns       register value
 */
uint32_t devh_ReadReg32(
    os_devhandle_t  *devh,
    uint32_t         reg_offset );

/**
 * @brief         Read a 32 bit register at an offset from the device handle
 *                  polling mode, will only write event if the value read.
 *                  is different from "prev_value"
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         prev_value  : byte offset from the device BASE address.
 *
 * @returns       register value
 */
uint32_t devh_ReadReg32_polling_mode(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         prev_value );

/**
 * @brief         Write a 32 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         value       : value to write.
 */
void devh_WriteReg32(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         value );

/**
 * @brief         OR a 32 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         or_value    : value to OR into the register
 */
void devh_OrBitsReg32(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         or_value );

/**
 * @brief         AND a 32 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         and_value   : value to AND into the register
 */
void devh_AndBitsReg32(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         and_value );

/**
 * @brief         AND then OR a 32 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         and_value   : value to AND into the register (performed FIRST)
 * @param         or_value    : value to OR into the register (performed SECOND)
 */
void devh_SetMaskedReg32(
    os_devhandle_t  *devh,
    uint32_t         reg_offset,
    uint32_t         and_value,
    uint32_t         or_value );


/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/**
 * @brief         Read a 64 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 *
 * @returns       register value
 */
uint64_t devh_ReadReg64(
    os_devhandle_t  *devh,
    uint64_t         reg_offset );

/**
 * @brief         Read a 64 bit register at an offset from the device handle
 *                  polling mode, will only write event if the value read.
 *                  is different from "prev_value"
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         prev_value  : byte offset from the device BASE address.
 *
 * @returns       register value
 */
uint64_t devh_ReadReg64_polling_mode(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         prev_value );

/**
 * @brief         Write a 64 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         value       : value to write.
 */
void devh_WriteReg64(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         value );

/**
 * @brief         OR a 64 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         or_value    : value to OR into the register
 */
void devh_OrBitsReg64(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         or_value );

/**
 * @brief         AND a 64 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         and_value   : value to AND into the register
 */
void devh_AndBitsReg64(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         and_value );

/**
 * @brief         AND then OR a 64 bit register at an offset from the device handle
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         reg_offset  : byte offset from the device BASE address.
 * @param         and_value   : value to AND into the register (performed FIRST)
 * @param         or_value    : value to OR into the register (performed SECOND)
 */
void devh_SetMaskedReg64(
    os_devhandle_t  *devh,
    uint64_t         reg_offset,
    uint64_t         and_value,
    uint64_t         or_value );

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**
 * @brief         Read a parameter (scriptable) from the device handle
 *
 *  This function is used to read undocumentented parameters from a device
 *  handle, for example, new registers, configuration information, etc.
 *  This is kind of a catch-all method to allow experimentation and
 *  high-level scripting response to the device handle.
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         param_name  : Parameter name to inject.
 * @param         pvalue      : buffer to read into.
 * @param         len         : number of bytes to read
 *
 *  Note that each unique "param_name" has an expected value type, which
 *  the writer and responder must agree upon ahead of time.  "Inject" only
 *  passes the value pointer and size, it does not enforce data types.
 *
 * @returns       Number of bytes read, or number of bytes required if ( NULL == pvalue )
 *
 */
int devh_InjectRead(
    os_devhandle_t  *devh,
    const char      *param_name,
    void            *pvalue,
    int              len );


/**
 * @brief         Write a parameter (scriptable) into the device handle
 *
 *  This function is used to inject undocumentented parameters to a device
 *  handle, for example, new registers, configuration information, etc.
 *  This is kind of a catch-all method to allow experimentation and
 *  high-level scripting response to the device handle.
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         param_name  : Parameter name to inject.
 * @param         pvalue      : buffer to read from.
 * @param         len         : number of bytes to write
 *
 *  Note that each unique "param_name" has an expected value type, which
 *  the writer and responder must agree upon ahead of time.  "Inject" only
 *  passes the value pointer and size, it does not enforce data types.
 *
 * @returns       Number of bytes written.
 */
int devh_InjectWrite(
    os_devhandle_t  *devh,
    const char      *param_name,
    const void      *pvalue,
    int              len );

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/**
 * @brief         Default Handler for DeviceHandle Inject.  Performs Register IO Only.
 *
 *  This function is used to read undocumentented parameters from a device
 *  handle, for example, new registers, configuration information, etc.
 *  This is kind of a catch-all method to allow experimentation and
 *  high-level scripting response to the device handle.
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         param_name  : Parameter name to inject.
 * @param         pvalue      : buffer to read into.
 * @param         len         : number of bytes to read.
 *
 *  Note that each unique "param_name" has an expected value type, which
 *  the writer and responder must agree upon ahead of time.  "Inject" only
 *  passes the value pointer and size, it does not enforce data types.
 *
 * @returns       Number of bytes read, or number of bytes required if ( NULL == pvalue )
 *
 */
int devh_default_InjectRead(
    os_devhandle_t  *devh,
    const char      *param_name,
    void            *pvalue,
    int              len );

/**
 * @brief         Default Handler for DeviceHandle Inject.  Performs Register IO Only.
 *
 *  This function is used to inject undocumentented parameters to a device
 *  handle, for example, new registers, configuration information, etc.
 *  This is kind of a catch-all method to allow experimentation and
 *  passes the value pointer and size, it does not enforce data types.
 *
 * @param         devh        : device handle (associated with a MMR device)
 * @param         param_name  : Parameter name to inject.
 * @param         pvalue      : buffer to read from.
 * @param         len         : number of bytes to write
 *
 *  Note that each unique "param_name" has an expected value type, which
 *  the writer and responder must agree upon ahead of time.  "Inject" only
 *  passes the value pointer and does not enforce data types.
 *
 * @returns       Number of bytes written.
 */
int devh_default_InjectWrite(
    os_devhandle_t  *devh,
    const char      *param_name,
    const void      *pvalue,
    int              len );

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/** Get SVEN Handle for device
 */
struct SVENHandle *devh_SVEN_GetHandle(
    os_devhandle_t          *devh );

/** Permanently Set DeviceHandles Module and Unit for sven event writing
 */
void devh_SVEN_SetModuleUnit(
    os_devhandle_t          *devh,
    int                      sven_module,
    int                      sven_unit );

/** Write a hand-rolled event to the queue.
 */
void devh_SVEN_WriteEvent(
    os_devhandle_t          *devh,
    const struct SVENEvent  *ev );

/** Write a short debug string to SVEN, truncate end of string */
void devh_SVEN_WriteDebugString(
    os_devhandle_t          *devh,
    int                      subtype,
    const char              *str );

/** Write a short debug string to SVEN, ensure end of string shows */
void devh_SVEN_WriteDebugStringEnd(
    os_devhandle_t          *devh,
    int                      subtype,
    const char              *str );

/** Set Dynamic Disable mask.  Allows this module to have event categories
 * more granularly disabled at runtime.
 */
void devh_SVEN_SetDynamicDisable(
    os_devhandle_t          *devh,
    unsigned int             dyn_disable_mask );

/**
 * @brief         Write a Module-Specific Debug Event to SVEN Nexus.
 *
 *  This is used to throw out a module-specific event to the debug stream
 *
 * @param         devh                  : device handle (associated with a MMR device)
 * @param         module_event_subtype  : Module-Specific event type
 * @param         payload0              : Module-Specific event payload 0
 * @param         payload1              : Module-Specific event payload 1
 * @param         payload2              : Module-Specific event payload 2
 * @param         payload3              : Module-Specific event payload 3
 * @param         payload4              : Module-Specific event payload 4
 * @param         payload5              : Module-Specific event payload 5
 *
 */
void devh_SVEN_WriteModuleEvent(
    os_devhandle_t  *devh,
    int              module_event_subtype,
    unsigned int     payload0,
    unsigned int     payload1,
    unsigned int     payload2,
    unsigned int     payload3,
    unsigned int     payload4,
    unsigned int     payload5 );

/* ========================================================================== */
/* ========================================================================== */
/**
 * @brief         Write A constant-defined bitfield
 *
 * @param         devh                  : device handle (associated with a MMR device)
 * @param         roff                  : Register offset from base
 * @param         lsb                   : LSB Position of bitfield
 * @param         bwidth                : Width of bitfield, in bits
 * @param         mask                  : 32-bit Mask of bitfield
 * @param         value                 : Value
 *
 */
#define DEVH_WRITE_BITFIELD_UNWRAPPED( devh, roff, lsb, bwidth, mask, value ) \
    { if ( 0xffffffff == ((unsigned int)mask) )                             \
        devh_WriteReg32( devh, roff, value );               \
      else if ( (1 == bwidth) && (1 & (value)) )            \
        devh_OrBitsReg32( devh, roff, (1 << (lsb)) );       \
      else if ( (1 == bwidth) && !(1 & (value)) )           \
        devh_AndBitsReg32( devh, roff, ~(1 << (lsb)) );     \
      else                                                  \
        devh_SetMaskedReg32( devh, roff,                    \
            ~(((1<<((bwidth)&0x1f))-1)<<(lsb)),             \
            ((value)<<(lsb)) );                             \
    }

/**
 * @brief         Write A constant-defined bitfield
 *
 * @param         devh                  : device handle (associated with a MMR device)
 * @param         roff                  : Register offset from base
 * @param         lsb                   : LSB Position of bitfield
 * @param         bwidth                : Width of bitfield, in bits
 * @param         mask                  : 32-bit Mask of bitfield
 * @param         value                 : Value
 *
 */
#define DEVH_WRITE_BITFIELD( devh, BITFDEF, value )         \
    DEVH_WRITE_BITFIELD_UNWRAPPED( devh, BITFDEF, value )



/**
 * @brief         Read a constant-defined bitfield
 *
 * @param         devh                  : device handle (associated with a MMR device)
 * @param         roff                  : Register offset from base
 * @param         lsb                   : LSB Position of bitfield
 * @param         bwidth                : Width of bitfield, in bits
 * @param         mask                  : 32-bit Mask of bitfield
 *
 */
#define DEVH_READ_BITFIELD_UNWRAPPED( devh, roff, lsb, bwidth, mask ) \
    (uint32_t) ( (0xffffffff == ((unsigned int)mask) ) ?               \
         (unsigned int) devh_ReadReg32( devh, (roff) ) :                   \
        ((unsigned int)(devh_ReadReg32( devh, (roff) ) & (mask)) >> (lsb)) \
    )

/**
 * @brief         Read a constant-defined bitfield
 *
 * @param         devh                  : device handle (associated with a MMR device)
 * @param         roff                  : Register offset from base
 * @param         lsb                   : LSB Position of bitfield
 * @param         bwidth                : Width of bitfield, in bits
 * @param         mask                  : 32-bit Mask of bitfield
 *
 */
#define DEVH_READ_BITFIELD( devh, BITFDEF )                  \
    DEVH_READ_BITFIELD_UNWRAPPED( devh, BITFDEF )



#ifdef SVEN_DEVH_DISABLE_SVEN

    /** mark a function entered into sven debug log */
    #define DEVH_FUNC_ENTERED( devh )

    /** mark a function entered into sven debug log */
    #define DEVH_FUNC_ENTER( devh )

    /** mark a function exit into sven debug log */
    #define DEVH_FUNC_EXITED( devh )

    /** mark a function exit into sven debug log */
    #define DEVH_FUNC_EXIT( devh )

    /** Inserts __FILE__:__LINE__  into sven debug log */
    #define DEVH_AUTO_TRACE( devh )

    /** Inserts "Invalid param" and function name into debugstring */
    #define DEVH_INVALID_PARAM( devh )


    /** Inserts assertion if condition fail into debugstring */
    #define DEVH_ASSERT( devh, cond )   OS_ASSERT(cond)

    #define DEVH_DEBUG( devh, str )

    /** Throw a warning string */
    #define DEVH_WARN( devh, str )

    /** Throw a Fatal Error string */
    #define DEVH_FATAL_ERROR( devh, str )

    #define dump_devh_os_devhandle_t( devh )

#else

    /** mark a function entered into sven debug log */
    #define DEVH_FUNC_ENTERED( devh ) \
        devh_SVEN_WriteDebugStringEnd( devh, \
            SVEN_DEBUGSTR_FunctionEntered, \
            __FUNCTION__ )

    /** mark a function entered into sven debug log */
    #define DEVH_FUNC_ENTER( devh ) \
        devh_SVEN_WriteDebugStringEnd( devh, \
            SVEN_DEBUGSTR_FunctionEntered, \
            __FUNCTION__ )

    /** mark a function exit into sven debug log */
    #define DEVH_FUNC_EXITED( devh ) \
        devh_SVEN_WriteDebugStringEnd( devh, \
            SVEN_DEBUGSTR_FunctionExited, \
            __FUNCTION__ )

    /** mark a function exit into sven debug log */
    #define DEVH_FUNC_EXIT( devh ) \
        devh_SVEN_WriteDebugStringEnd( devh, \
            SVEN_DEBUGSTR_FunctionExited, \
            __FUNCTION__ )

    /** Inserts "Invalid param" and function name into debugstring */
    #define DEVH_INVALID_PARAM( devh ) \
        devh_SVEN_WriteDebugString( devh, SVEN_DEBUGSTR_InvalidParam, __FUNCTION__ )

    /** Macros required to trick compiler to convert __LINE__ to a string */
    #define DEVH_STRINGIFY(x)   #x
    /** Macros required to trick compiler to convert __LINE__ to a string */
    #define DEVH_TOSTRING(x)    DEVH_STRINGIFY(x)

    /** Inserts __FILE__:__LINE__  into sven debug log */
    #define DEVH_AUTO_TRACE( devh ) \
        devh_SVEN_WriteDebugStringEnd( devh, \
            SVEN_DEBUGSTR_AutoTrace, \
            __FILE__ ":" DEVH_TOSTRING(__LINE__) )

    #ifdef NDEBUG
        #define DEVH_ASSERT( devh, cond )
    #else
        /** Inserts assertion if condition fail into debugstring */
        #define DEVH_ASSERT( devh, cond ) \
            if ( !(cond) ) { devh_SVEN_WriteDebugStringEnd( devh, \
                               SVEN_DEBUGSTR_Assert, \
                               ":" __FILE__ ":" DEVH_TOSTRING(__LINE__)  ); \
                               OS_ASSERT( cond ); \
                           }
    #endif

    #define DEVH_DEBUG( devh, str ) \
        devh_SVEN_WriteDebugString( devh, SVEN_DEBUGSTR_Generic, str )

    #define DEVH_WARN( devh, str ) \
        devh_SVEN_WriteDebugString( devh, SVEN_DEBUGSTR_Warning, str )

    #define DEVH_FATAL_ERROR( devh, str ) \
        devh_SVEN_WriteDebugString( devh, SVEN_DEBUGSTR_FatalError, str )

    void dump_devh_os_devhandle_t ( os_devhandle_t  *devh );

#endif

#endif /* _SVEN_DEVH_H */
