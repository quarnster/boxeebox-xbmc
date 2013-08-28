/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2006-2008 Intel Corporation. All rights reserved.

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

  Copyright(c) 2006 Intel Corporation. All rights reserved.
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
#ifndef _HTUPLE_H
#define _HTUPLE_H 1

#define HTUPLE_DELIMITER_CHAR       '.'

#define HTUPLE_ERR_NONE             0
#define HTUPLE_ERR_BAD_HANDLE       1
#define HTUPLE_ERR_BAD_TYPE         2
#define HTUPLE_ERR_LOW_RESOURCES    3
#define HTUPLE_ERR_NOT_FOUND        4
#define HTUPLE_ERR_INVALID_PARAM    5

typedef unsigned int    htuple_t;

enum htuple_type
{
    HTUPLE_TYPE_INVALID,
    HTUPLE_TYPE_INT,
    HTUPLE_TYPE_STRING,
    HTUPLE_TYPE_MAX
};

struct htuple_Value
{
    enum htuple_type         type;
    union
    {
        char                *str;
        int                  intval;
    };
};

int htuple_initialize( void );

int htuple_deinitialize( void );

htuple_t htuple_find_child(
    htuple_t      id,
    const char   *name,
    unsigned int  namelen );

htuple_t htuple_first_child(
    htuple_t      id );

htuple_t htuple_next_sibling(
    htuple_t      id );

int htuple_node_name(
    htuple_t      id,
    const char   **pname );

int htuple_node_get_value(
    htuple_t      id,
    struct htuple_Value *pval );

int htuple_node_int_value(
    htuple_t      id,
    unsigned int *pval );

int htuple_node_str_value(
    htuple_t      id,
    const char   **pval );

int htuple_get_int_value(
    htuple_t      id,
    const char   *valname,
    unsigned int  valnamelen,
    unsigned int *pval );

int htuple_get_str_value(
    htuple_t      id,
    const char   *valname,
    unsigned int  valnamelen,
    const char   **pval );

int htuple_set_int_value(
    htuple_t      id,
    const char   *valname,
    unsigned int  valnamelen,
    unsigned int  value );

int htuple_set_str_value(
    htuple_t      id,
    const char   *valname,
    unsigned int  valnamelen,
    const char   *value,
    unsigned int  valuestrlen );

int htuple_parse_config_string(
    htuple_t      id,
    const char   *cfg,
    unsigned int  cfglen );

int htuple_create_private_tree(
    htuple_t     *pprivate_id );

int htuple_delete_private_tree(
    htuple_t      private_id );

#endif /* _HTUPLE_H */
