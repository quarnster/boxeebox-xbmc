/*
  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007, 2008 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007, 2008 Intel Corporation. All rights reserved.
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

#ifndef __ISMD_MSG_H__
#define __ISMD_MSG_H__

/**
@defgroup AUTO_API Auto API

Autoapi is a set of scripts and source templates for making an API "location independent".  What this means is code can be written to a local, standard C API, and the real implementation of the API can be located in a completely different address space or even a different device.  It can be in a shared library, in a kernel driver, even across the network.  And which version an app uses can be changed simply by swapping a shared library.

@{
*/


/**
@name Driver Usable Functions

@{
*/

/* Defines which control the auto api script.  Only functions marked with these
 * defines will be exported by Auto API */
#define SMD_API_FUNC(x) x
#define SMDEXPORT
#define SMDCONNECTION
#define SMDPRIVATE

/**
@brief add a connection release callback

Adds a new callback function that will be called if the connection dies (e.g. the app exits).  Allows resources to be released gracefully if the app dies or does not clean up properly.

@param [in] connection ID number for the connection to bind the callback to.
@param [in] callback Function to call if the connection ends.
@param [in] data Pointer to pass to the callback function.
@retval 0 Registration was successful.
@retval ISMD_ERROR_NO_RESOURCES Out of memory.  Unable to complete registration
@retval ISMD_ERROR_INVALID_PARAMETER Invalid parameter.  Callback is NULL or connection value is invalid.
*/
int ismd_connection_register(unsigned long connection, void (*callback)(void *), void *data);

/**
@brief remove a connection release callback

Remove the callback and data from the connection manager.  On success, the
callback request is removed and not called. An error means the callback has already been called and removed.

WARNING! ismd_connection_unregister must NOT be called from the callback function
registered with ismd_connect_register(). Doing so will result in a deadlock.
@param [in] callback Callback function pointer passed in to ismd_connect_register().
@param [in] data The data matching the callback to be removed
@retval 0 Callback was removed successfully
@retval ISMD_ERROR_OBJECT_DELETED Callback not found: already called and removed.
*/
int ismd_connection_unregister( void (*callback)(void *), void *data);

/* @} @} */

#endif
