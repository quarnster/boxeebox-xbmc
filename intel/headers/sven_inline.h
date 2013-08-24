#ifndef SVEN_INLINE_H
#define SVEN_INLINE_H 1
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

#ifndef SVEN_H
#include "sven.h"
#endif

#include "sven_protos.h"
#ifndef __KERNEL__
#include <stdio.h>
#endif

#ifndef CONFIG_ARM
//#define flush_dcache_page(x)
#else
#include <asm/cacheflush.h>
#endif

#if defined(__i386__) || defined(__x86_64__)

extern volatile unsigned int *g_sven_dfx_time;

#define sventimestamp() ( (NULL != g_sven_dfx_time) ? \
   *g_sven_dfx_time : \
   ({ unsigned int a,d; asm volatile("rdtsc":"=a" (a), "=d" (d)); (a>>10)|(d<<22); }) )

static __inline unsigned int _sven_atomic_increment(
	volatile unsigned int			*ppos )
{
	unsigned slot = 1;

	asm( "xadd %1, %0" :
	     "=m" (*ppos), "=r"(slot) :
	     "1" (slot));

	return slot;
}
#elif defined(__XSCALE__) || defined(__LINUX_ARM_ARCH__)

#if defined(__KERNEL__)
#include <asm/system.h>
#include <asm/hardware.h>

extern volatile unsigned int *g_sven_dfx_time;

#define sventimestamp() (*g_sven_dfx_time)

static __inline unsigned int _sven_atomic_increment(
    volatile unsigned int           *ppos )
{
    unsigned slot;
    /* NOTE: Without arbitration (atomic increment), it is
     * theoretically possible for two writers to write to the
     * same event, so an event would get lost. 
     * This code will proceed without arbitration in
     * order to guarantee predictable timing.
     */
    slot = (*ppos)++;
    //flush_dcache_page(virt_to_page(ppos));
    return(slot);
}

#else

#include <sched.h>

extern unsigned int sventimestamp();

static __inline unsigned int _sven_atomic_increment(
    volatile unsigned int           *ppos )
{
    unsigned slot;
    slot = *ppos;
    *ppos = (slot + 1) & 0xffffff;

    return slot;
}


#endif /*__KERNEL__*/

#elif defined(sparc)

#define sventimestamp() (0)

	static __inline unsigned int _sven_atomic_increment(
		volatile unsigned int           *ppos )
	{
		return (*ppos)++;
	}

#else /* unknown arch */
//#warning using generic sveninlines
#define sventimestamp() (0)

static __inline unsigned int _sven_atomic_increment(
    volatile unsigned int           *ppos )
{
    return (*ppos)++;
}


#endif

/**
 * @brief         Atomically Write an unsigned int to a memory location
 *
 * @param         pvalue      : address of unsigned int to write
 * @param         value       : value to write
 */
static __inline void _sven_atomic_write(
	volatile unsigned int      *pvalue,
	unsigned int					 value )
{
	*pvalue = value;	/* UNFINISHED: do not necessarily need to flush caches here */
}

/**
 * @brief         Read a 32 bit register at an offset from the device handle
 *
 * @param         svenh       : Sven Queue handle
 *
 * @returns       reserved event position within queue to write the event into
 *
 */
static __inline int _sven_reserve_event_slot(
	struct SVENHandle		*svenh )
{
	return( (svenh->pcur != svenh->pinc) ?
      *svenh->pinc : _sven_atomic_increment( svenh->pinc ) );
}

/**
 * @brief         Get position of event at position N
 *
 * @param         svenh       : Sven Queue handle
 * @param         position    : position within queue of event
 *
 * @returns       pointer to event within queue
 */
static __inline struct SVENEvent *_sven_get_event_ptr(
	struct SVENHandle		*svenh,
	int						 position )
{
	/* mask out the position */
	return( &svenh->en[ position & svenh->event_pos_mask ] );
}

/**
 * @brief         Write and Validate event to where it can be read by observers
 *
 * @param         svenh       : Sven Queue handle
 * @param         ev          : SVENEvent to complete writing
 * @param         epos        : position returned by _sven_reserve_event_slot()
 * @param         tag         : Event Tag to write with correct position
 *
 */
static __inline void _sven_write_event_time_tag(
	struct SVENHandle		*svenh,
	struct SVENEvent        *ev,
    int                      epos,
	struct SVENEventTag      et )
{
    union
    {
        struct SVENEventTag      et;
        unsigned int             ul;
    }tag;

    ev->se_timestamp = sventimestamp();

	/* WARNING: MUST Write the Event Tag _LAST_, this is how the
	 * event logging software proceeds through the event list safely.
	 */
    tag.et = et;
    tag.et.et_gencount = epos >> svenh->event_pos_lsb;
    _sven_atomic_write( ((volatile unsigned int *)((char *)&ev->se_et)), tag.ul );
    //flush_dcache_page(virt_to_page(ev));
} // _sven_write_event_time_tag()


/**
 * @brief         Write a prepared event into the event queue so it may
 *                be read by all event observers
 *
 * @param         svenh       : Sven Queue handle
 * @param         ev          : SVENEvent to copy into queue
 *
 */
static __inline void _sven_write_new_event(
	struct SVENHandle		*svenh,
	const struct SVENEvent	*ev )
{
	struct SVENEvent		*to;
	unsigned int			 epos;

	epos = _sven_reserve_event_slot( svenh );

	/* Event to write TO */
	to = _sven_get_event_ptr( svenh, epos );;

	/* Copy eveything into position except.... */
	to->u.se_ulong[0] = ev->u.se_ulong[0];
	to->u.se_ulong[1] = ev->u.se_ulong[1];
	to->u.se_ulong[2] = ev->u.se_ulong[2];
	to->u.se_ulong[3] = ev->u.se_ulong[3];
	to->u.se_ulong[4] = ev->u.se_ulong[4];
	to->u.se_ulong[5] = ev->u.se_ulong[5];

	/* write out timestamp and EventTag to complete the event */
    _sven_write_event_time_tag( svenh, to, epos, ev->se_et );
}

/**
 * @brief         Get next written event position
 *
 * @param         svenh       : Sven Queue handle
 *
 * @returns       last written event position within queue
 */
static __inline int _sven_get_write_position(
	struct SVENHandle		*svenh )
{
   /* DFX Unit is pre-increment, software is post-increment
    * so when using DFX for acceleration we need to make the write
    * pointer look as if its past the current position
    */
	return( (svenh->pcur != svenh->pinc) ?
      *svenh->pcur + 1 : *svenh->pcur );
}

/**
 * @brief         Read the next event from the queue into a local SVENEvent
 *
 * @param         svenh       : Sven Queue handle
 * @param         ev          : local SVENEvent to copy into
 * @param         preader_pos : last reader postion (will be updated if success)
 *
 * @returns       1 if an event was successfully read, 0 otherwise
 */
static __inline int _sven_read_next_event(
	struct SVENHandle		*svenh,
	struct SVENEvent		*ev,
	int						*preader_pos )
{
    int                  wpos;
    int                  rpos;

    rpos = *preader_pos;
    wpos = _sven_get_write_position(svenh);

	/* have not caught up? */
	if ( rpos != wpos )
	{
		struct SVENEvent    *from;

		from = _sven_get_event_ptr( svenh, rpos );

        /* Has got to be a better way of doing this */

        /** TODO: We may want to do "lap detection" here to see if
         * the reader has been asleep to long.  Probably the better
         * approach is to do that kind of detection at a higher level
         */
        /* Make sure the generation counter in the tag is what we're
         * expecting to see for our reader position.
         */
        if ( from->se_et.et_gencount == ((rpos >> svenh->event_pos_lsb) & 0x3) )
        {
		    /* Copy the structure out */
		    *ev = *from;

		    /* Advance the reader position */
		    *preader_pos = ++rpos;

		    return(1);	/* event grabbed */
        }
	}

	return(0);
}

/**
 * @brief         Initialize a SVENEvent "tag" values, clear "payload" area
 *
 * @param         svenh       : Sven Queue handle
 * @param         ev          : local SVENEvent to initialize
 * @param         module      : event module, see enum SVEN_Module
 * @param         unit        : event unit (e.g. 0, 1, 2...) if there
 *                              is more than one unit of this module type
 * @param         event_type    : one of enum SVEN_MajorEventType_t
 * @param         event_subtype : Subtype of Major event type
 *
 */
static __inline void _sven_initialize_event(
	struct SVENEvent		*ev,
    int                      module,
    int                      unit,
    int                      event_type,
    int                      event_subtype )
{
    ev->se_et.et_gencount = 0;
    ev->se_et.et_module = module;
    ev->se_et.et_unit = unit;
    ev->se_et.et_type = event_type;
    ev->se_et.et_subtype = event_subtype;
    /* Clear remaining event bytes */
    ev->u.se_ulong[0] = 0;
    ev->u.se_ulong[1] = 0;
    ev->u.se_ulong[2] = 0;
    ev->u.se_ulong[3] = 0;
    ev->u.se_ulong[4] = 0;
    ev->u.se_ulong[5] = 0;
}

/**
 * @brief         Initialize a SVENEvent "tag" values, leave "payload" area untouched
 *
 * @param         svenh       : Sven Queue handle
 * @param         ev          : local SVENEvent to initialize
 * @param         module      : event module, see enum SVEN_Module
 * @param         unit        : event unit (e.g. 0, 1, 2...) if there
 *                              is more than one unit of this module type
 * @param         event_type    : one of enum SVEN_MajorEventType_t
 * @param         event_subtype : Subtype of Major event type
 *
 */
static __inline void _sven_initialize_event_top(
	struct SVENEvent		*ev,
    int                      module,
    int                      unit,
    int                      event_type,
    int                      event_subtype )
{
    ev->se_et.et_gencount = 0;
    ev->se_et.et_module = module;
    ev->se_et.et_unit = unit;
    ev->se_et.et_type = event_type;
    ev->se_et.et_subtype = event_subtype;
}


#endif /* SVEN_INLINE_H */
