#ifndef SVEN_EVENT_TYPE_H
#define SVEN_EVENT_TYPE_H 1
/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2008 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007-2008 Intel Corporation. All rights reserved.
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

/* SVEN Major Event Types */
enum SVEN_MajorEventType_t
{
   SVEN_event_type_invalid,         /* NO ZEROES ALLOWED */
   SVEN_event_type_trigger,         /* 1 */
   SVEN_event_type_debug_string,    /* 2 */
   SVEN_event_type_register_io,     /* 3 */
   SVEN_event_type_port_io,         /* 4 */
   SVEN_event_type_module_isr,      /* 5 */
   SVEN_event_type_os_isr,          /* 6 */
   SVEN_event_type_os_thread,       /* 7 */
   SVEN_event_type_smd,             /* 8 - Streaming Media Drivers */
   SVEN_event_type_module_specific, /* 9 */
   SVEN_event_type_PMU,             /* 10 HW Performance */
   SVEN_event_type_performance,     /* 11 SW Performance */
   SVEN_event_type_API,             /* 12 api functions */


  /** WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
   * If you make changes here, you must make exactly
   * equivalent changes inside:
   * 
   *          svenreverse.c
   * 
   * otherwise units will not get properly "reversed"
   */
   SVEN_event_type_MAX
};


enum SVEN_EV_API_t
{
   SVEN_EV_API_INVALID, /*No Zeros Allowed*/
   SVEN_EV_API_FunctionCalled,
   SVEN_EV_API_FunctionReturned,

   SVEN_EV_API_MAX

};
enum SVEN_EV_PMU_t
{
   SVEN_EV_PMU_INVALID, /*No Zeros Allowed*/
   SVEN_EV_PMU_MCU,
   SVEN_EV_PMU_VCMH1,
   SVEN_EV_PMU_VCMH2,
   SVEN_EV_PMU_XAB,
   SVEN_EV_PMU_AUDIO,

   SVEN_EV_PMU_MAX

};
enum SVEN_EV_performance_t
{
   SVEN_EV_performance_invalid, /*No Zeros Allowed*/
   SVEN_EV_performance_start,
   SVEN_EV_performance_stop,
   SVEN_EV_performance_measurement,


   SVEN_EV_performance_MAX

};
/* SVEN_event_type_trigger SUBTypes */
enum SVEN_EV_trigger_t
{
   SVEN_EV_trigger_invalid,            /* no zeroes allowed */

   SVEN_EV_trigger_alarm,              /* Launch Trigger Mechanism (logic analyzer, etc) */

   SVEN_EV_trigger_MAX
};

/* SVEN_event_type_debug_string SUBTypes */
enum SVEN_DEBUGSTR_t
{
   SVEN_DEBUGSTR_invalid,            /* no zeroes allowed */

   SVEN_DEBUGSTR_Generic,            /* string generic debug */

   SVEN_DEBUGSTR_FunctionEntered,    /* string is function name */

   SVEN_DEBUGSTR_FunctionExited,     /* string is function name */

   SVEN_DEBUGSTR_AutoTrace,          /* string is __FILE__:__LINE__ */

   SVEN_DEBUGSTR_InvalidParam,       /* Invalid parameter passed to a function */

   SVEN_DEBUGSTR_Checkpoint,         /* Execution Checkpoint string */

   SVEN_DEBUGSTR_Assert,             /* Software Assert: failure  */

   SVEN_DEBUGSTR_Warning,            /* Software Warning: text description  */

   SVEN_DEBUGSTR_FatalError,         /* Fatal Software Error: text description */

   SVEN_DEBUGSTR_PresTiming,         /* Presentation-timing-related debug */

   SVEN_DEBUGSTR_MAX
};

/* SVEN_event_type_register_io SUBTypes */
enum SVEN_EV_RegIo_t
{
   SVEN_EV_RegIo_invalid,              /* no zeroes allowed */

   SVEN_EV_RegIo32_Read,
   SVEN_EV_RegIo32_Write,
   SVEN_EV_RegIo32_OrBits,
   SVEN_EV_RegIo32_AndBits,
   SVEN_EV_RegIo32_SetMasked,          /* Clear, then set register bits */

   SVEN_EV_RegIo16_Read,
   SVEN_EV_RegIo16_Write,
   SVEN_EV_RegIo16_OrBits,
   SVEN_EV_RegIo16_AndBits,
   SVEN_EV_RegIo16_SetMasked,          /* Clear, then set register bits */

   SVEN_EV_RegIo8_Read,
   SVEN_EV_RegIo8_Write,
   SVEN_EV_RegIo8_OrBits,
   SVEN_EV_RegIo8_AndBits,
   SVEN_EV_RegIo8_SetMasked,          /* Clear, then set register bits */

   SVEN_EV_RegIo64_Read,
   SVEN_EV_RegIo64_Write,
   SVEN_EV_RegIo64_OrBits,
   SVEN_EV_RegIo64_AndBits,
   SVEN_EV_RegIo64_SetMasked,          /* Clear, then set register bits */

   SVEN_EV_RegIo_MAX
};

/* SVEN_event_type_port_io SUBTypes */
enum SVEN_EV_PortIo_t
{
   SVEN_EV_PortIo_invalid,			    /* no zeroes allowed */

   SVEN_EV_PortIo32_Read,
   SVEN_EV_PortIo32_Write,
   SVEN_EV_PortIo32_OrBits,
   SVEN_EV_PortIo32_AndBits,
   SVEN_EV_PortIo32_SetMasked,

   SVEN_EV_PortIo16_Read,
   SVEN_EV_PortIo16_Write,
   SVEN_EV_PortIo16_OrBits,
   SVEN_EV_PortIo16_AndBits,
   SVEN_EV_PortIo16_SetMasked,

   SVEN_EV_PortIo8_Read,
   SVEN_EV_PortIo8_Write,
   SVEN_EV_PortIo8_OrBits,
   SVEN_EV_PortIo8_AndBits,
   SVEN_EV_PortIo8_SetMasked,

   SVEN_EV_PortIo_MAX
};

/* SVEN_event_type_unit_isr SUBTypes */
enum SVEN_EV_UnitIsr_t
{
   SVEN_EV_UnitIsr_invalid,            /* no zeroes allowed */

   SVEN_EV_UnitIsr_Enter,              /* Unit ISR Function Call Entered */
   SVEN_EV_UnitIsr_Exit,               /* Unit ISR Function Call Exited */

   SVEN_EV_UnitIsr_MAX
};

/* SVEN_event_type_os_isr SUBTypes */
enum SVEN_EV_OSIsr_t
{
   SVEN_EV_OSIsr_invalid,              /* no zeroes allowed */

   SVEN_EV_OSIsr_Enter,                /* OS ISR Function Call Entered */
   SVEN_EV_OSIsr_Exit,                 /* OS ISR Function Call Exited */

   SVEN_EV_OSIsr_MAX
};

/* SVEN_event_type_os_thread SUBTypes */
enum SVEN_EV_OS_Thread_t
{
   SVEN_EV_OS_Thread_invalid,          /* no zeroes allowed */

   SVEN_EV_OS_Thread_Run,              /* Thread (pid) Scheduled to run on CPU */
   SVEN_EV_OS_Thread_Wait,             /* Thread (pid) put in Wait Queue */
   SVEN_EV_OS_Thread_Create,           /* New Thread (pid) created */
   SVEN_EV_OS_Thread_Delete,           /* Thread (pid) deleted */
   SVEN_EV_OS_Thread_Suspend,          /* Thread (pid) suspended (permanently) */

   SVEN_EV_OS_Thread_MAX
};

/* SVEN_event_type_SMDCore Subtypes */
enum SVEN_EV_SMDCore_t
{
    SVEN_EV_SMDCore_invalid,        /* no zeroes allowed */

    SVEN_EV_SMDCore_Port_Alloc = 0x10,
    SVEN_EV_SMDCore_Port_Free,
    SVEN_EV_SMDCore_Port_Connect,
    SVEN_EV_SMDCore_Port_Disconnect,
    SVEN_EV_SMDCore_Port_SetName,
    
    SVEN_EV_SMDCore_Queue_Alloc = 0x20,
    SVEN_EV_SMDCore_Queue_Free,
    SVEN_EV_SMDCore_Queue_SetConfig,
    SVEN_EV_SMDCore_Queue_Write,
    SVEN_EV_SMDCore_Queue_Read,
    SVEN_EV_SMDCore_Queue_Flush,
    SVEN_EV_SMDCore_Queue_WriteFail,
    SVEN_EV_SMDCore_Queue_ReadFail,
    SVEN_EV_SMDCore_Queue_Push,
    SVEN_EV_SMDCore_Queue_Pull,
    SVEN_EV_SMDCore_Queue_SetName,
    
    SVEN_EV_SMDCore_Buf_Alloc = 0x30,
    SVEN_EV_SMDCore_Buf_Free,
    SVEN_EV_SMDCore_Buf_AddRef,
    SVEN_EV_SMDCore_Buf_DeRef,
    SVEN_EV_SMDCore_Buf_Release,
    SVEN_EV_SMDCore_Buf_fb_count,
    
    SVEN_EV_SMDCore_Clock_Alloc = 0x40,
    SVEN_EV_SMDCore_Clock_Free,
    SVEN_EV_SMDCore_Clock_Status,
    SVEN_EV_SMDCore_Clock_Get,
    SVEN_EV_SMDCore_Clock_Set,
    SVEN_EV_SMDCore_Clock_Adjust,
    SVEN_EV_SMDCore_Clock_LastTrigTime,

    SVEN_EV_SMDCore_SoftPLL_PCR_Arrive = 0x50,
    SVEN_EV_SMDCore_SoftPLL_NudgeClock,
    SVEN_EV_SMDCore_SoftPLL_Discontinuity,
    SVEN_EV_SMDCore_SoftPLL_ScanBuffer,
    SVEN_EV_SMDCore_SoftPLL_DriftStatus,
    SVEN_EV_SMDCore_SoftPLL_UnTrackable,
    
    SVEN_EV_SMDCore_ClockMux_STC = 0x60,
    SVEN_EV_SMDCore_ClockMux_Aud,
    
    SVEN_EV_SMDCore_Circbuf_Alloc = 0x70,
    SVEN_EV_SMDCore_Circbuf_Free,
    SVEN_EV_SMDCore_Circbuf_Write,
    SVEN_EV_SMDCore_Circbuf_Read,
    SVEN_EV_SMDCore_Circbuf_SetName,
    
    SVEN_EV_SMDCore_Viz_Port_Associate = 0x80,
    SVEN_EV_SMDCore_Viz_Queue_Associate,
    SVEN_EV_SMDCore_Viz_Circbuf_Associate,
    
    SVEN_EV_SMDCore_Gst_Transition = 0x90,
    SVEN_EV_SMDCore_Gst_ParentCall,
    SVEN_EV_SMDCore_Gst_ParentRet,
    SVEN_EV_SMDCore_Gst_EventEnter,
    SVEN_EV_SMDCore_Gst_EventExit,
    SVEN_EV_SMDCore_Gst_LoopSleep,
    SVEN_EV_SMDCore_Gst_LoopEnter,
    SVEN_EV_SMDCore_Gst_LoopExit,
    SVEN_EV_SMDCore_Gst_ChainMode,
    SVEN_EV_SMDCore_Gst_ClockId,
    SVEN_EV_SMDCore_Gst_BufDrop,
    SVEN_EV_SMDCore_Gst_BufWrAtmpt,
    SVEN_EV_SMDCore_Gst_BufWrite,
    SVEN_EV_SMDCore_Gst_BufRcv,

    SVEN_EV_SMDCore_MAX
};

#endif /* SVEN_EVENT_TYPE_H */
