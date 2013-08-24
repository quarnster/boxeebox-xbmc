/*
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
 */

 /* This file contains XFree86 implementations for the OSAL abstractions
  * in list.h
  */

#ifndef _OSAL_XFREE86_LIST_H
#define _OSAL_XFREE86_LIST_H

typedef struct _list_head {
    struct _list_head *next;
    struct _list_head *last;
}_os_list_head_t;

#define _OS_LIST_HEAD_INIT(name) { &(name), &(name) }
#define _OS_INIT_LIST_HEAD(head) head->next = head; head->last = head;
#define _OS_LIST_HEAD(a) _os_list_head_t a = {&a, &a}

#define _OS_LIST_ADD_TAIL(node, head) \
   ((_os_list_head_t *)node)->last = ((_os_list_head_t *)head)->last; \
   ((_os_list_head_t *)node)->next = ((_os_list_head_t *)head);       \
   ((_os_list_head_t *)head)->last = ((_os_list_head_t *)node);       \
   ((_os_list_head_t *)node)->last->next = ((_os_list_head_t *)node);

#define _OS_LIST_DEL(a)     \
   ((_os_list_head_t *)a)->last->next = ((_os_list_head_t *)a)->next; \
   ((_os_list_head_t *)a)->next->last = ((_os_list_head_t *)a)->last;

#define _OS_LIST_FOR_EACH(entry, head) \
   for(entry = (os_list_head_t *)((os_list_head_t *)head)->next; \
    entry != head; \
    entry = (os_list_head_t *)((os_list_head_t *)entry)->next)

#define _OS_LIST_FOR_EACH_SAFE(entry,n,head) \
   for(entry = (os_list_head_t *)((os_list_head_t *)head)->next,n = (os_list_head_t *)((os_list_head_t *)entry)->next; \
    entry != head; \
    entry = n, n = (os_list_head_t *)((os_list_head_t *)entry)->next)

#define _OS_LIST_EMPTY(head) (((os_list_head_t *)head)->next == head)

#define _OS_LIST_NEXT(a) ((_os_list_head_t *)a)->next
#define _OS_LIST_PREV(a) ((_os_list_head_t *)a)->last

#define _OS_LIST_ENTRY(ptr, type, member) \
   ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#endif
