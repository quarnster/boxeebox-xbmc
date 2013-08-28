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

/** @weakgroup config_doc CE31xx Platform Config documentation

\brief High level usage of platform config application for CE31xx SDK target OS

\anchor Contents
<H2>Contents</H2>
- <A HREF="#Related_Documents">Related Documents</A>
- <A HREF="#platform_config">Platform Config</A>
 - <A HREF="#platform_config_app">platform_config_app</A>
  - <A HREF="#app_usage">Usage</A>
  - <A HREF="#memory_dump">Memory Dump</A>
  .
 - <A HREF="#memory_layout">Memory Layout</A>
 - <A HREF="#configuration">Configuration</A>
 .
.

\anchor Related_Documents
<H2>See Also (Related_Documents)</H2>
-# tbd:

\anchor platform_config
<H2>Platform Config</H2>

The Intel Platform config uses a kernel mode driver platform_config.ko with a user
space interface called platform_config_app during the boot of the Intel CE31xx or CE41xx platforms to
load and store configuration options for the allocation of memory beyond the OS range of memory
and configuration options for various Intel drivers.   This is accomplished by
the System V boot script /etc/init.d/platform_config which insmod the driver and calls the
/bin/platform_config_app with load parameters for various configurations. Example:

\code
start_function() {
    # Install the platform config module
    try_command insmod /lib/modules/platform_config.ko

    platform_config_app load /etc/platform_config/ce3100/platform_config.hcfg
    platform_config_app load /etc/platform_config/ce3100/memory_layout_512M.hcfg platform.memory.layout

    platform_config_app memshift platform.memory.media_base_address
    platform_config_app execute platform.startup
}
\endcode
 
In the above script the platform_config.hcfg file contains various configuration
options for the Intel drivers. The second file loaded has a memory layout for the
Intel SMD and GDL Display drivers that is managed by the SMD Core Buffer Management
(see ismd_buffer_alloc()).
This layout is then shifted up just beyond OS memory allocation range by the config option
from the first file called media_base_address.

Finally the platform_config_app executes startup instructions from the startup section of
the platform_config.hcfg using the execute option.

\anchor platform_config_app
<H3>platform_config_app</H3>

\anchor app_usage
<H4>usage</H4>
<pre>
usage for platform_config_app:
  platform_config_app load [filename] [location]
  platform_config_app dump [location]
  platform_config_app execute [location]
  platform_config_app remove [location]
  platform_config_app memshift [offset_in_MB]
  platform_config_app memory
  * [location] optional parameter (default is root_node)
</pre>

\anchor memory_dump
<H4>Memory dump</H4>
Use <code>platform_config_app memory</code> to dump the new offsets for the memory_layout after the
memshift operation has been executed.  Example:

<pre>
# platform_config_app memory
          smd_buffers_ADSPFW: base 10000000 size 00400000
            smd_buffers_ADEC: base 10400000 size 00240000
            smd_buffers_ALSA: base 10640000 size 00040000
...
            display:          base 14040000 size 04600000
           smd_frame_buffers: base 18640000 size 09f60000
#
</pre>

\anchor memory_layout
<H3>Memory Layout</H3>

Platform config loads a memory layout to allocate memory for Intel drivers outside of
OS memory allocation range.  The default example is located in fsroot under
/etc/platform_config/ce3100/memory_layout_512M.hcfg which is stored in a location
called platform.memory.layout by the /etc/init.d/platform_config System V script.

The layout starts at address 0, as it is assumed that the memshift option in
platform_config_app is used to shift the layout above the OS memory.

Each line starts with a unique identifier and contains a size for the buffers
being allocated (buf_size), a base address (base), and a total size (size).  The
total size should be a multiple of the buf_size.   The base address plus the total size
is usually the base address of the next entry.  When editing this file, be very
careful not to overlap memory ranges.

Example:
\code
...
smd_buffers_VFWQ2      { buf_size =   0x100000    base = 0x01200000    size = 0x00100000 } // (  1MB) 1x1MB VidDec Firmware Stream Queue Stream 2
smd_buffers_VID_ES1    { buf_size =     0x8000    base = 0x01300000    size = 0x00800000 } // (  8MB) 256x32KB Video ES Data from Demux to VidDec Stream 1
...
\endcode

\anchor configuration
<H3>Configuration</H3>

This platform_config_app loads a configuration file called platform_config.hcfg.
This file contains options for various Intel Drivers to specify default behavior
at boot time.

*/

