/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2006-2009 Intel Corporation. All rights reserved.

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

  Copyright(c) 2006-2009 Intel Corporation. All rights reserved.
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

#ifndef __ISMD_CORE_AUTOAPI_DEFS_H__
#define __ISMD_CORE_AUTOAPI_DEFS_H__

#include "ismd_core.h"
#include "ismd_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISMD_MODULE_INIT ismd_core_init

#define ISMD_MODULE_EXIT ismd_core_deinit

ismd_result_t ismd_buffer_alloc_track(unsigned long connection, size_t size, ismd_buffer_handle_t *buffer);
ismd_result_t ismd_frame_buffer_alloc_track(unsigned long connection, size_t width, size_t height, ismd_buffer_handle_t *buffer);
ismd_result_t ismd_frame_buffer_alloc_with_context_track(unsigned long connection, size_t width, size_t height, int context, ismd_buffer_handle_t *buffer);
ismd_result_t ismd_buffer_alias_track(unsigned long connection, ismd_buffer_handle_t buffer_to_alias, ismd_buffer_handle_t *alias_buffer);
ismd_result_t ismd_buffer_alias_subset_track(unsigned long connection, ismd_buffer_handle_t buffer_to_alias, size_t start_offset, size_t size, ismd_buffer_handle_t *alias_buffer);
ismd_result_t ismd_port_read_track(unsigned long connection, ismd_port_handle_t port, ismd_buffer_handle_t *buffer);
ismd_result_t ismd_buffer_add_reference_track(unsigned long connection, ismd_buffer_handle_t buffer);
ismd_result_t ismd_port_alloc_track(unsigned long connection, ismd_port_type_t port_type, ismd_port_config_t *port_config, ismd_port_handle_t *port_handle);
ismd_result_t ismd_port_alloc_named_track(unsigned long connection, ismd_port_type_t port_type, ismd_port_config_t *port_config, char name[SMD_PORT_NAME_LENGTH], int name_length, int virtual_id, ismd_port_handle_t *port_handle);
ismd_result_t ismd_event_alloc_track(unsigned long connection, ismd_event_t *event);
ismd_result_t ismd_clock_alloc_track(unsigned long connection, int clock_type, ismd_clock_t *clock);

ismd_result_t ismd_buffer_free_track( unsigned long connection, ismd_buffer_handle_t buffer );
ismd_result_t ismd_buffer_dereference_track( unsigned long connection, ismd_buffer_handle_t buffer );
ismd_result_t ismd_port_write_track(unsigned long connection, ismd_port_handle_t port, ismd_buffer_handle_t buffer);
ismd_result_t ismd_port_free_track( unsigned long connection, ismd_port_handle_t port_handle );
ismd_result_t ismd_clock_free_track( unsigned long connection, ismd_clock_t handle );
ismd_result_t ismd_event_free_track( unsigned long connection, ismd_event_t event );

ismd_result_t ismd_message_context_alloc_track(unsigned long connection, ismd_message_context_t *context);
ismd_result_t ismd_message_context_free_track( unsigned long connection, ismd_message_context_t context);


#ifdef __cplusplus
}
#endif

#endif  /* __ISMD_CORE_AUTOAPI_DEFS_H__ */
