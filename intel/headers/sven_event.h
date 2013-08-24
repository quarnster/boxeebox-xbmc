#ifndef SVEN_EVENT_H
#define SVEN_EVENT_H 1
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

/** Event Tag for SVEN Events, allows decoding of event owner
 * and event type/subtype
 */
struct SVENEventTag
{
#ifndef _SVEN_FIRMWARE_
   unsigned int       et_gencount   : 2;
   unsigned int       et_module     : 9;
   unsigned int       et_unit       : 5;
   unsigned int       et_type       : 6;
   unsigned int       et_subtype    : 10;
#else
   unsigned int       et_subtype    : 10;
   unsigned int       et_type       : 6;
   unsigned int       et_unit       : 5;
   unsigned int       et_module     : 9;
   unsigned int       et_gencount   : 2;
#endif
};

/** DEPRICATED: Only here to be used by csven so it can parse old saved log
 *              files.  Only csven should use this.
 *
 * Old version 0 Event Tag for SVEN Events, allows decoding of event owner
 * and event type/subtype.  This old version of the struct is kept so that it
 * can be used by csven to parse old SVEN log files.
 */
struct SVENEventTag_v0
{
   // This struct is only used by csven so we only need the IA version
   // of the old version 0 bit field layout.
   unsigned int       et_gencount   : 2;
   unsigned int       et_module     : 10;
   unsigned int       et_unit       : 4;
   unsigned int       et_type       : 8;
   unsigned int       et_subtype    : 8;
};

/** NOTE: Event Size MUST Be a Power of 2 */
#define SVEN_EVENT_SIZE_BITS	5
/** NOTE: Event Size MUST Be a Power of 2 */
#define SVEN_EVENT_SIZE			(1<<SVEN_EVENT_SIZE_BITS)
/** NOTE: Event Size MUST Be a Power of 2 */
#define	SVEN_EVENT_SIZE_MASK	(SVEN_EVENT_SIZE-1)

#if defined(__x86_64__)
	#define SVEN_EVENT_PAYLOAD_NUM_ULONGS   3
#else
	#define SVEN_EVENT_PAYLOAD_NUM_ULONGS   6
#endif
#define SVEN_EVENT_PAYLOAD_NUM_UINTS    6
#define SVEN_EVENT_PAYLOAD_NUM_USHORTS  (SVEN_EVENT_PAYLOAD_NUM_UINTS<<1)
#define SVEN_EVENT_PAYLOAD_NUM_UCHARS   (SVEN_EVENT_PAYLOAD_NUM_UINTS<<2)
#define SVEN_EVENT_PAYLOAD_NUM_CHARS    (SVEN_EVENT_PAYLOAD_NUM_UINTS<<2)
#define SVEN_EVENT_PAYLOAD_NUM_UCHARS_SMD (SVEN_EVENT_PAYLOAD_NUM_UCHARS-4)

/** The core real-time published/gathered events by SVEN API
 * This struct should always be 32 bytes (or at least a power of two),
 * as the circular buffer it's written into depends on a size that's
 * a power of two to allow stateless event writer synchronization.
 * The #define, SVEN_EVENT_SIZE_BITS, must indicate the power of
 * two that matches this structure size.
 */

struct SVENEvent
{
	unsigned int            se_timestamp;

    /** Embedded EventTag within all SVENEvents */
	struct SVENEventTag     se_et;

	/* struct is 8 bytes to this point */
	/* there is room for 24 bytes, (six dwords) */

    /** Embedded UNION of SVENEvent struct types, this is the
     *  "Payload" area of a struct SVENEvent.  Its content is
     *  event-type, and event-subtype dependant, which is why
     *  its declared as a union.
     */
	union
	{
        /* Generic unsigned types are part of the union */
        /* ------------------------------------------ */
        unsigned long       se_ulong[ SVEN_EVENT_PAYLOAD_NUM_ULONGS ];
        unsigned int        se_uint[ SVEN_EVENT_PAYLOAD_NUM_UINTS ];
        unsigned short      se_ushort[ SVEN_EVENT_PAYLOAD_NUM_USHORTS ];
        unsigned char       se_uchar[ SVEN_EVENT_PAYLOAD_NUM_UCHARS ];


		/* 24 bytes for a debug string, not a lot, but if you need
		 * more you should break it up into multiple events
		 */
		/** buffer for SVEN_event_type_debug_string */
		char                se_dbg_str[ SVEN_EVENT_PAYLOAD_NUM_CHARS ];

        /** embedded SVENEvent struct for SVEN_event_type_trigger */
        struct
        {
            unsigned int    flags;
        }se_trigger;

		/** embedded SVENEvent struct for subtypes of:
		 *  SVEN_event_type_register_io
		 *  SVEN_event_type_port_io
		 */
        struct
        {
            unsigned long   phys_addr;  /* physical address (including offset) */
            unsigned int    value;      /* Value read or written */
            unsigned int    mask;       /* only on SVEN_EV_RegIo_SetMasked */
            unsigned int    log_prev;   /* previous value (filled in by SVENLog) */
            unsigned int    log_flags;  /* flags (filled in by SVENLog) */
                                #define SVEN_REGIO_LOG_FLAG_PREV_UNKNOWN (1<<0)
        }se_reg;

        struct 
        {
            unsigned long   timer;
            unsigned long   counter_0;
            unsigned long   counter_1;
        }se_pmu;

	    /** embedded SVENEvent struct for subtypes of:
	     *  SVEN_event_type_unit_isr
	     *  SVEN_event_type_port_io
	     */
        struct se_int
        {
            int             pid;        /* Process ID */
            int             cpu;        /* CPU number */
            int             line;       /* interrupt line */
            int             pri;        /* interrupt priority */
            int             pad[2];
        }se_int;

		/** embedded SVENEvent struct for subtypes of:
		 *  SVEN_event_type_os_thread
		 */
        struct se_thread
        {
            int             pid;        /* Process ID */
            int             cpu;        /* CPU number */
            int             prevpid;     /* Process ID of previous process*/
            int             pad[3];
        }se_thread;

		/** embedded SVENEvent struct for subtypes of:
		 *  SVEN_event_type_performance
		 */
        struct se_performance
        {
            int             context_id;      /* e.g. stream ID */
            int             workload_id;     /* e.g. buffer ID */
            unsigned int    units_processed; /* May be bytes, packets, audio samples, frames, pixels, gorm */
            unsigned int    time_elapsed;    /* MUST be calculated using sven_get_timestamp() */
            int             pad[2];
        }se_performance;

		/** embedded SVENEvent struct for subtypes of:
		 *  SVEN_event_type_smd
		 */
        union se_smd
        {
            // SVEN_EV_SMDCore_Port_SetName
            // SVEN_EV_SMDCore_Queue_SetName
            // SVEN_EV_SMDCore_Circbuf_SetName
            struct
            {
                int        id;               /* ID used by this port/queue/circbuf */
                char       name[SVEN_EVENT_PAYLOAD_NUM_UCHARS_SMD];
            } setname;

            // SVEN_EV_SMDCore_Viz_Port_Associate
            // SVEN_EV_SMDCore_Viz_Queue_Associate
            // SVEN_EV_SMDCore_Viz_Circbuf_Associate
            struct
            {
                int        id;               /* ID used by this port/queue/circbuf */
                int        viz_id;           /* Visualizer ID to use */
            } viz;

            /* ---------------------------------------------- */

            //SVEN_EV_SMDCore_Port_Alloc,
            struct
            {
                int        port_id;         /* ID used by this port */
                int        queue_id;    
            } port_alloc;

            //SVEN_EV_SMDCore_Port_Free,
            struct
            {
                int        port_id;         /* ID used by this port */
            } port_free;

            //SVEN_EV_SMDCore_Port_Connect,
            //SVEN_EV_SMDCore_Port_Disconnect,
            struct
            {
                int        port_id;         /* ID used by this port */
                int        other_port_id;
            } port_connect;

            /* ---------------------------------------------- */

            //SVEN_EV_SMDCore_Queue_Alloc,
            //SVEN_EV_SMDCore_Queue_Free,
            struct
            {
                int        queue_id;         /* ID used by this queue */
                int        queue_type;
                int        max_depth;
            } queue;

            //SVEN_EV_SMDCore_Queue_SetConfig,
            struct
            {
                int        queue_id;         /* ID used by this queue */
                int        max_buffers;
                int        max_bytes;
                int        high_watermark;
                int        low_watermark;
            } queue_config;

            //SVEN_EV_SMDCore_Queue_Write,
            //SVEN_EV_SMDCore_Queue_Read,
            //SVEN_EV_SMDCore_Queue_Flush,
            //SVEN_EV_SMDCore_Queue_WriteFail,
            //SVEN_EV_SMDCore_Queue_ReadFail,
            //SVEN_EV_SMDCore_Queue_Push,
            //SVEN_EV_SMDCore_Queue_Pull,
            struct
            {
                int         queue_id;         /* ID used by this queue */
                int         tot_buffers_in_q; /* total buffers in queue */
                int         tot_bytes;        /* total bytes in queue */
                int         xact_buffers;     /* this transaction buffers */
                int         xact_bytes;       /* this transaction bytes */
                int         buf_id;           /* Associated Buffer ID (if relevant) */
            } queue_io;
            
            /* ---------------------------------------------- */

            //SVEN_EV_SMDCore_Buf_Alloc,
            struct
            {
                int          buf_id;         /* buffer ID */
                unsigned int buf_size;       /* buffer size */
                unsigned int buf_phys_addr;  /* buffer PhysAddr */
                unsigned int buf_va;         /* buffer Virtual Address */
                unsigned int thread_id;      /* thread id */
            } buf_alloc;

            //SVEN_EV_SMDCore_Buf_Free,
            //SVEN_EV_SMDCore_Buf_AddRef,
            //SVEN_EV_SMDCore_Buf_DeRef,
            //SVEN_EV_SMDCore_Buf_Release,
            struct
            {
                int          buf_id;         /* buffer ID */
                unsigned int buf_size;       /* buffer size */
            } buf;

            //SVEN_EV_SMDCore_fb_count,
            struct
            {
                unsigned int fb_count;       /* FB count */
                unsigned int region;         /* region FB allocated from */
            } fb_count;
        }smd;
	}u;

	/* Struct size must be a power of 2 bytes */
};

#endif /* SVEN_EVENT_H */

