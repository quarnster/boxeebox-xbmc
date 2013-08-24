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

/**
\mainpage Platform Configutation Strings
*/

/** \file platform_config.h */
#ifndef _PLATFORM_CONFIG_STRINGS_H_
#define _PLATFORM_CONFIG_STRINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**  Example Configuration file

    platform
    {
        startup     // startup actions to perform immediately after config file is loaded
        {
            {   action      "load"
                location    "platform.memory.layout"
                filename    "memory_layout_128M.hcfg"
            }
            { ... }
            { ... }
            { ... }
        }

        memory
        {
            layout      // traversible directory of memory regions
            {
                smd_2mb_buffers     { base = 0x08C00000 size = 0x00400000 }
                sven_hdr            { base = 0x0987e000 size = 0x00001000 }
                smd_16mb_buffers    { base = 0x0a000000 size = 0x02000000 }
                display             { base = 0x91000000 size = 0x08000000 }
            }
        }

	    software 
	    {
		    drivers     // device driver parameters
		    {
			    osal
			    {
			    }
			    pal
			    {
			    }
			    sven
			    {
				    dismask = 0xffffffff
				    debug_level = 0
				    num_bufs = 2
			    }
			    smd
			    {
				    core
				    {
					    allow_memory_overlap = 1
				    }
				    demux
				    {
				    }
				    viddec
				    {
				    }
				    vidpproc
				    {
				    }
				    vidrend
				    {
					    max_hold_time = 6000
				    }
				    tsout
				    {
				    }					
				    audrend
				    {
					    num_channels = 6
					    spdif_userdata = "Intel CE Device"
				    }
			    }
		    }
	    }
    }
 *
 */

/** "platform" */
#define CONFIG_PATH_PLATFORM_ROOT           "platform"

/** "platform.startup" */
#define CONFIG_PATH_PLATFORM_STARTUP        CONFIG_PATH_PLATFORM_ROOT ".startup"

/** "platform.memory.layout" */
#define CONFIG_PATH_PLATFORM_MEMORY_LAYOUT  CONFIG_PATH_PLATFORM_ROOT ".memory.layout"

/** "platform.software.drivers" */
#define CONFIG_PATH_PLATFORM_DRIVERS        CONFIG_PATH_PLATFORM_ROOT ".software.drivers"

/** "platform.software.drivers.smd" */
#define CONFIG_PATH_PLATFORM_SMD_DRIVERS    CONFIG_PATH_PLATFORM_DRIVERS ".smd"


/*--- SPECIFIC Device Driver PATHS ---*/

/** "platform.software.drivers.osal" */
#define CONFIG_PATH_DRIVER_OSAL             CONFIG_PATH_PLATFORM_DRIVERS ".osal"
/** "platform.software.drivers.pal" */
#define CONFIG_PATH_DRIVER_PAL              CONFIG_PATH_PLATFORM_DRIVERS ".pal"
/** "platform.software.drivers.sven" */
#define CONFIG_PATH_DRIVER_SVEN             CONFIG_PATH_PLATFORM_DRIVERS ".sven"
/** "platform.software.drivers.display" */
#define CONFIG_PATH_DRIVER_DISPLAY          CONFIG_PATH_PLATFORM_DRIVERS ".display"


/** "platform.software.drivers.smd.core" */
#define CONFIG_PATH_SMD_CORE                CONFIG_PATH_PLATFORM_SMD_DRIVERS ".core"
/** "platform.software.drivers.smd.clock" */
#define CONFIG_PATH_SMD_CLOCK               CONFIG_PATH_PLATFORM_SMD_DRIVERS ".clock"
/** "platform.software.drivers.smd.demux" */
#define CONFIG_PATH_SMD_DEMUX               CONFIG_PATH_PLATFORM_SMD_DRIVERS ".demux"
/** "platform.software.drivers.smd.audio" */
#define CONFIG_PATH_SMD_AUDIO               CONFIG_PATH_PLATFORM_SMD_DRIVERS ".audio"
/** "platform.software.drivers.smd.viddec" */
#define CONFIG_PATH_SMD_VIDDEC              CONFIG_PATH_PLATFORM_SMD_DRIVERS ".viddec"
/** "platform.software.drivers.smd.vidpproc" */
#define CONFIG_PATH_SMD_VIDPPROC            CONFIG_PATH_PLATFORM_SMD_DRIVERS ".vidpproc"
/** "platform.software.drivers.smd.vidrend" */
#define CONFIG_PATH_SMD_VIDREND             CONFIG_PATH_PLATFORM_SMD_DRIVERS ".vidrend"
/** "platform.software.drivers.smd.tsout" */
#define CONFIG_PATH_SMD_TSOUT               CONFIG_PATH_PLATFORM_SMD_DRIVERS ".tsout"
/** "platform.software.drivers.smd.bufmon" */
#define CONFIG_PATH_SMD_BUFMON              CONFIG_PATH_PLATFORM_SMD_DRIVERS ".bufmon"

#ifdef __cplusplus
}
#endif
#endif /* _PLATFORM_CONFIG_STRINGS_H_ */
