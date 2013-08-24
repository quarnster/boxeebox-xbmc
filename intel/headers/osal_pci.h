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
 * This file contains OS abstractions for PCI function calls.
 */

#ifndef _OSAL_PCI_H
#define _OSAL_PCI_H

#include "osal_type.h"

/** @defgroup OSAL_PCI OSAL pci support
 * API definitions for the OSAL PCI support
 * @{
 */


#define OS_PCI_FIND_FIRST_DEVICE(v, d, p)           os_pci_find_first_device(v, d, p)
#define OS_PCI_FIND_NEXT_DEVICE(c, n)               os_pci_find_next_device(c, n)
#define OS_PCI_FIND_FIRST_DEVICE_BY_CLASS(s, b, p, d)   os_pci_find_first_device_by_class(s, b, p, d)
#define OS_PCI_FIND_NEXT_DEVICE_BY_CLASS(c, n)      os_pci_find_next_device_by_class(c, n)
#define OS_PCI_DEVICE_FROM_SLOT(p, s)               os_pci_device_from_slot(p, s)
#define OS_PCI_DEVICE_FROM_ADDRESS(p, b, d, f)      os_pci_device_from_address(p, b, d, f)
#define OS_PCI_GET_DEVICE_ADDRESS(p, b, d, f)       os_pci_get_device_address(p, b, d, f)
#define OS_PCI_GET_SLOT_ADDRESS(p, s)               os_pci_get_slot_address(p, s)
#define OS_PCI_READ_CONFIG_8(p, o, v)               os_pci_read_config_8(p, o, v)
#define OS_PCI_READ_CONFIG_16(p, o, v)              os_pci_read_config_16(p, o, v)
#define OS_PCI_READ_CONFIG_32(p, o, v)              os_pci_read_config_32(p, o, v)
#define OS_PCI_WRITE_CONFIG_8(p, o, v)              os_pci_write_config_8(p, o, v)
#define OS_PCI_WRITE_CONFIG_16(p, o, v)             os_pci_write_config_16(p, o, v)
#define OS_PCI_WRITE_CONFIG_32(p, o, v)             os_pci_write_config_32(p, o, v)
#define OS_PCI_READ_CONFIG_HEADER(p, h)             os_pci_read_config_header(p, h)
#define OS_PCI_FREE_DEVICE(p)                       os_pci_free_device(p)

//!  This is a data type that serves as a handle for allocated PCI device.

typedef void* os_pci_dev_t;

#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2
#define PCI_TYPE2_ADDRESSES             1

//! This structure provides easy breakdown of the pci device configuration space
typedef struct _os_pci_dev_header_t {
    unsigned short  vendor_id;          // (ro)
    unsigned short  device_id;          // (ro)
    unsigned short  command;            // Device control
    unsigned short  status;
    unsigned char   revision_id;        // (ro)
    unsigned char   prog_if;            // (ro)
    unsigned char   sub_class;          // (ro)
    unsigned char   base_class;         // (ro)
    unsigned char   cache_line_size;    // (ro+)
    unsigned char   latency_timer;      // (ro+)
    unsigned char   header_type;        // (ro)
    unsigned char   bist;               // Built in self test

    union {
        struct _os_pci_header_type_0 {
            unsigned long   base_addresses[PCI_TYPE0_ADDRESSES];
            unsigned long   cis;
            unsigned short  sub_vendor_id;
            unsigned short  sub_system_id;
            unsigned long   rom_base_address;
            unsigned long   reserved2[2];

            unsigned char   interrupt_line;      //
            unsigned char   interrupt_pin;       // (ro)
            unsigned char   minimum_grant;       // (ro)
            unsigned char   maximum_latency;     // (ro)
        } type0;

        struct _os_pci_header_type_1 {
            unsigned long   base_addresses[PCI_TYPE1_ADDRESSES];
            unsigned char   primary_bus_number;
            unsigned char   secondary_bus_number;
            unsigned char   subordinate_bus_number;
            unsigned char   secondary_latency_timer;
            unsigned char   io_base;
            unsigned char   io_limit;
            unsigned short  secondary_status;
            unsigned short  memory_base;
            unsigned short  memory_limit;
            unsigned short  prefetchable_memory_base;
            unsigned short  prefetchable_memory_limit;
            unsigned long   prefetchable_memory_base_upper32;
            unsigned long   prefetchable_memory_limit_upper32;
            unsigned short  io_base_upper;
            unsigned short  io_limit_upper;
            unsigned long   reserved2;
            unsigned long   expansion_rom_base;
            unsigned char   interrupt_line;
            unsigned char   interrupt_pin;
            unsigned short  bridge_control;
        } type1;

        struct _os_pci_header_type_2{
            unsigned long   base_address;
            unsigned char   capabilities_ptr;
            unsigned char   reserved2;
            unsigned short  secondary_status;
            unsigned char   primary_bus_number;
            unsigned char   cardbus_bus_number;
            unsigned char   subordinate_bus_number;
            unsigned char   cardbus_latency_timer;
            unsigned long   memory_base0;
            unsigned long   memory_limit0;
            unsigned long   memory_base1;
            unsigned long   memory_limit1;
            unsigned short  io_base0_lo;
            unsigned short  io_base0_hi;
            unsigned short  io_limit0_lo;
            unsigned short  io_limit0_hi;
            unsigned short  io_base1_lo;
            unsigned short  io_base1_hi;
            unsigned short  io_limit1_lo;
            unsigned short  io_limit1_hi;
            unsigned char   interrupt_line;
            unsigned char   interrupt_pin;
            unsigned short  bridge_control;
            unsigned short  sub_vendor_id;
            unsigned short  sub_system_id;
            unsigned long   legacy_base_address;
            unsigned char   reserved3[56];
            unsigned long   system_control;
            unsigned char   multi_media_control;
            unsigned char   general_status;
            unsigned char   reserved4[2];
            unsigned char   gpio0_control;
            unsigned char   gpio1_control;
            unsigned char   gpio2_control;
            unsigned char   gpio3_control;
            unsigned long   irq_mux_routing;
            unsigned char   retry_status;
            unsigned char   card_control;
            unsigned char   device_control;
            unsigned char   diagnostic;
        } type2;

    } u;

    unsigned char   device_specific[108];

} os_pci_dev_header_t, *p_os_pci_dev_header_t;


/**
This function will find the first occurrence of a PCI device for the
particular vendor and device.

@param[in]  vendor_id : The vendor_id for the device to be found.
@param[in]  device_id : The device_id for the device to be found.
@param[out] pci_dev : Pointer to a PCI device handle if successful.

@retval OSAL_SUCCESS : PCI device found.
@retval OSAL_NOT_FOUND : No matching PCI device
@retval OSAL_ERROR :  Internal OSAL error
*/
osal_result os_pci_find_first_device(
        unsigned int vendor_id,
        unsigned int device_id,
        os_pci_dev_t* pci_dev);

/**
 * This function will find the next occurrence of a PCI device for the
 * particular vendor and device.
 *
 * @param[in] cur_pci_dev : Handle to a pci device found from a successful call
 *                       to os_pci_find_first_device or os_pci_find_next_device.
 * @param[out] next_pci_dev : Pointer to the next PCI device handle if
 *                                   successful.
 *
 * @retval OSAL_SUCCESS : PCI device found.
 * @retval OSAL_NOT_FOUND : No more matching PCI devices
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_find_next_device(
        os_pci_dev_t cur_pci_dev,
        os_pci_dev_t *next_pci_dev);

/**
 * This function will find the first occurrence of a PCI device for the
 * particular subclass, baseclass, and programming interface.
 *
 * @param[in] subclass : The subclass for the device to be found.
 * @param[in] baseclass : The baseclass for the device to be found.
 * @param[in] pi : The programming interface for the device to be found.
 * @param[out] pci_dev : Pointer to a PCI device handle if successful.
 * 
 * @retval OSAL_SUCCESS : PCI device found.
 * @retval OSAL_NOT_FOUND : No matching PCI devices
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_find_first_device_by_class(
        unsigned char subclass,
        unsigned char baseclass,
        unsigned char pi,
        os_pci_dev_t* pci_dev);

/**
 * This function will find the next occurrence of a PCI device for the
 * particular subclass, baseclass, and programming interface.
 * 
 * @param[in] cur_pci_dev : Handle to a pci device found from a successful call
 *                      to os_pci_find_first_device or os_pci_find_next_device.
 * @param[out] next_pci_dev : Pointer to the next PCI device handle if
 *                      successful.
 *
 * @retval OSAL_SUCCESS : PCI device found.
 * @retval OSAL_NOT_FOUND : No more matching PCI devices
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_find_next_device_by_class(
        os_pci_dev_t cur_pci_dev,
        os_pci_dev_t *next_pci_dev);

/**
 * This function will provide a handle to the pci device found at the specified
 * slot.
 *
 * @param[out] pci_dev : Pointer to a PCI device handle if successful.
 * @param[in] slot : address of the PCI device
 *
 * @retval OSAL_SUCCESS : PCI device found.
 * @retval OSAL_NOT_FOUND : No matching PCI device
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_device_from_slot(
        os_pci_dev_t *pci_dev,
        unsigned int slot);

/**
 * This function will provide a handle to the pci device found at the specified
 * address.
 *
 * @param[out] pci_dev : Pointer to a PCI device handle if successful.
 * @param[in] bus : Bus address of PCI device
 * @param[in] dev : Device address of PCI device
 * @param[in] func : Function address of PCI device
 * 
 * @retval OSAL_SUCCESS : PCI device found.
 * @retval OSAL_NOT_FOUND : No matching PCI device
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_device_from_address(
        os_pci_dev_t* pci_dev,
        unsigned char bus,
        unsigned char dev,
        unsigned char func);

/**
 * Return the slot address of a PCI device.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[out] slot : Pointer to receive the slot address
 *
 * @retval OSAL_SUCCESS : Found slot address.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSA_INVALID_PARAM : slot pointer is invalid
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_get_slot_address(os_pci_dev_t pci_dev, unsigned int *slot);

/**
 * This function will return the bus, device and function address of a PCI
 * device.  Any of the bus/dev/func parameters may be null if the information
 * is not needed.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[out] bus : Pointer to receive the bus address
 * @param[out] dev : Pointer to receive the device address
 * @param[out] func : Pointer to receive the function address
 *
 * @retval OSAL_SUCCESS : Found slot address.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_get_device_address(
        os_pci_dev_t pci_dev,
        unsigned int *bus,
        unsigned int *dev,
        unsigned int *func);

/**
 * Retrieves a byte of information, starting at the specified offset, from the
 * PCI configuration space on a particular PCI device.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to begin reading from
 * @param[out] val : Pointer to receive the read data
 * 
 * @retval OSAL_SUCCESS : Config space read successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : val pointer or offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_read_config_8(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned char* val);

/**
 * Retrieves 2 bytes of information, starting at the specified offset, from the
 * PCI configuration space on a particular PCI device
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to begin reading from
 * @param[out] val : Pointer to receive the read data
 *
 * @retval OSAL_SUCCESS : Config space read successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : val pointer or offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_read_config_16(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned short* val);

/**
 * Retrieves 4 bytes of information, starting at the specified offset, from the
 * PCI configuration space on a particular PCI device
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to begin reading from
 * @param[out] val : Pointer to receive the read data
 *
 * @retval OSAL_SUCCESS : Config space read successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : val pointer or offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_read_config_32(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned int* val);

/**
 * Writes a byte of data, starting at the specified offset, to the PCI
 * configuration space for a particular PCI device.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to write to
 * @param[in] val : Data to write
 *
 * @retval OSAL_SUCCESS : Config space write successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_write_config_8(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned char val);

/**
 * Writes 2 bytes of data, starting at the specified offset, to the PCI
 * configuration space for a particular PCI device.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to write to
 * @param[in] val : Data to write
 *
 * @retval OSAL_SUCCESS : Config space write successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_write_config_16(
        os_pci_dev_t pci_dev,
        unsigned int offset,
        unsigned short val);

/**
 * Writes 4 bytes of data, starting at the specified offset, to the PCI
 * configuration space for a particular PCI device.
 *
 * @param[in] pci_dev : Handle to a PCI device
 * @param[in] offset : Offset to write to
 * @param[in] val : Data to write
 *
 * @retval OSAL_SUCCESS : Config space write successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_INVALID_PARAM : offset is not valid
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_write_config_32(
            os_pci_dev_t pci_dev,
            unsigned int offset,
            unsigned int val);

/**
 * Fills in the os_pci_dev_header data structure which contains verbose
 * breakdown of any pci device's configuration space.
 *
 * @param pci_dev : Handle to a PCI device
 * @param pci_header : Pointer to a pci device header structure to be filled if
 *                      successful
 *
 * @retval OSAL_SUCCESS : Config space write successful.
 * @retval OSAL_INVALID_HANDLE : pci_dev is not a valid PCI device handle
 * @retval OSAL_NOT_FOUND : PCI device not available
 * @retval OSAL_ERROR :  Internal OSAL error
 */
osal_result os_pci_read_config_header(
        os_pci_dev_t pci_dev,
        p_os_pci_dev_header_t pci_header);

/**
 * Determine the interrupt number for a PCI device.
 *
 * @param[in]  pci_device : handle to a pci device
 * @param[out] irq :        interrupt number used by the pci device
 *
 * @retval OSAL_SUCCESS:  returned IRQ is valid
 * @retval OSAL_INVALID_PARAM: pci_device or irq is NULL
 */
osal_result os_pci_get_interrupt(os_pci_dev_t pci_device, unsigned *irq);

/**
 * Free the os_pci_dev_t that was previously allocated by calling
 * os_pci_find_first_device, os_pci_find_next_device, os_pci_device_from_slot
 * or os_pci_device_from_address.
 *
 * @param[in] pci_dev : Handle to a PCI device
 *
 * @retval OSAL_SUCCESS:  PCI device handle is freed
 */
osal_result os_pci_free_device(os_pci_dev_t pci_dev);

#endif
