/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2008-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2008-2009 Intel Corporation. All rights reserved.
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

#ifndef _GEN3_CLOCK_H_
#define _GEN3_CLOCK_H_


#include "platform_config.h"
#include "platform_config_paths.h"

////////////////////////////////////////////////////////////////////////////////
// Shortcut macros for accessing core platform configuration properties
////////////////////////////////////////////////////////////////////////////////

#define ISMD_CLOCK_INT_PROPERTY(name) ({                                         \
   config_result_t icipret = CONFIG_SUCCESS;                                    \
   int icipint = 0;                                                             \
   config_ref_t icip_attr_id = ROOT_NODE;                                       \
                                                                                \
   icipret = config_node_find(ROOT_NODE,                                        \
                              CONFIG_PATH_SMD_CLOCK,                             \
                              &icip_attr_id);                                   \
   if (icipret == CONFIG_SUCCESS) {                                             \
      icipret = config_get_int(icip_attr_id, name, &icipint);                   \
   }                                                                            \
                                                                                \
   if (icipret != CONFIG_SUCCESS) {                                             \
      OS_INFO("Error! %s undefined!\n", name);                                  \
   }                                                                            \
   icipint;                                                                     \
})

#define MASTER_CLOCK_SOURCE       ISMD_CLOCK_INT_PROPERTY("clock_properties.master_clock_source")
#define VCXO_LOWER_LIMIT          ISMD_CLOCK_INT_PROPERTY("clock_properties.vcxo_lower_limit")
#define VCXO_UPPER_LIMIT          ISMD_CLOCK_INT_PROPERTY("clock_properties.vcxo_upper_limit")

////////////////////////////////////////////////////////////////////////////////
// Internal defines for the ports.  Since the application is not allocating or
// setting config on them, they are in this file.
////////////////////////////////////////////////////////////////////////////////



/******************************************************************************/
/* Clock-related defines and datatypes */
/******************************************************************************/

//#define VCXO_LOWER_LIMIT ((uint32_t)26998000) // in hz
//#define VCXO_UPPER_LIMIT ((uint32_t)27002000) // in hz

#define VCXO_CENTER_FREQUENCY ((uint32_t)27000000)
#define VCXO_RANGE       (VCXO_UPPER_LIMIT - VCXO_LOWER_LIMIT)


typedef enum {

	ISMD_CLOCK_VSYNC_SOURCE_PIPE_A 	= 0,
	ISMD_CLOCK_VSYNC_SOURCE_PIPE_B  = 1
} ismd_clock_vsync_source_t;


 typedef enum {

	ISMD_CLOCK_TRIGGER_SOURCE_TS_IN_1			= 0,
	ISMD_CLOCK_TRIGGER_SOURCE_TS_IN_2			= 1,	
	ISMD_CLOCK_TRIGGER_SOURCE_TS_IN_3			= 2,
	ISMD_CLOCK_TRIGGER_SOURCE_TS_IN_4			= 3,
	ISMD_CLOCK_TRIGGER_SOURCE_VDC_FLIP_0		= 4,
	ISMD_CLOCK_TRIGGER_SOURCE_VDC_FLIP_1		= 5,
	ISMD_CLOCK_TRIGGER_SOURCE_GPIO_0			= 6,
	ISMD_CLOCK_TRIGGER_SOURCE_GPIO_1			= 7,
	ISMD_CLOCK_TRIGGER_SOURCE_GPIO_2			= 8,
	ISMD_CLOCK_TRIGGER_SOURCE_GPIO_3			= 9,
	ISMD_CLOCK_TRIGGER_SOURCE_SOFTWARE			= 10,
	ISMD_CLOCK_NO_TRIGGER_SOURCE_SET			= 11
	
} ismd_clock_trigger_source_t;

typedef enum {

	ISMD_CLOCK_ROUTE_TS_OUT_1					= 0, 
	ISMD_CLOCK_ROUTE_TS_IN_1					= 1, 
	ISMD_CLOCK_ROUTE_TS_IN_2					= 2, 
	ISMD_CLOCK_ROUTE_TS_IN_3					= 3, 
	ISMD_CLOCK_ROUTE_TS_IN_4					= 4,
	ISMD_CLOCK_NO_DEST_SET						= 5
	
} ismd_clock_dest_t ;

typedef enum {

	ISMD_CLOCK_SOURCE_VCXO						=0,
	ISMD_CLOCK_SOURCE_MASTER_DDS				=1,	
	ISMD_CLOCK_SOURCE_LOCAL_DDS				=2
	
} ismd_clock_source_t;


#endif      


