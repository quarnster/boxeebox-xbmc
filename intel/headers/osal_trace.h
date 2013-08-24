/*==========================================================================
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
 =========================================================================*/

 /*
  *  This file contains OS abstracted interfaces for the TRACE and VERIFY
  *  interfaces
  */

/** \file 
 */

#ifndef __TRACE_H__
#define __TRACE_H__
#include "osal_char.h"
#include "osal_config.h"

/** \defgroup OSAL_TRACE Depreciated OSAL trace functions
 *
 * <I>Status: This is the API definition for the TRACE and VERIFY macros
 * </I>
 *\{
 */

typedef enum
{
    T_NONE      = 0,
    T_ERROR     = 1,
    T_WARNING   = 2,
    T_INFO      = 3
} TRACE_LEVEL;

typedef struct
{
    TCHAR *     label;
    TCHAR *     subsys;
    TRACE_LEVEL level;
} TRACE_PARMS;

#ifdef __cplusplus
extern "C" {
#endif

TRACE_PARMS * trace_init(  TCHAR *subsys,  TCHAR * name,  TRACE_LEVEL level );
void trace_deinit(TRACE_PARMS *tp);
void trace( TRACE_PARMS *subsys, TRACE_LEVEL level, TCHAR *szFormat, ... );
void _os_print( TCHAR *szFormat, ... );
void _os_debug( TCHAR *szFormat, ... );
void _os_error( TCHAR *szFormat, ... );
void _os_info( TCHAR *szFormat, ... );

// Function for printing data values in the form:
// 0xYYYYYYYY: val val val ...
// 0xZZZZZZZZ: val val val ...

void trace_data(
    TRACE_PARMS *   subsys,
    TRACE_LEVEL     level,
    unsigned char * pData,      // pointer to data to print
    int             nElemSize,  // size of each data element (1, 2 or 4)
    int             nCount,     // how many data elements
    char *          pFmtStr,    // format string for data elements, eg " %02x"
    int             nElemPerRow);

#ifdef __cplusplus
}
#endif

#ifdef _TRACE

#define TRACE trace
#define TRACE_DATA trace_data
/**
 *  Declaration of Trace ID.
 *
 *  Before using the TRACE macro, you need to declare and define a Trace ID
 *  using TRACE_DECLARE() and TRACE_DEFINE() macros. E.g:
 *          TRACE_DECLARE(TSD)
 *          TRACE_DEFINE (TSD)
 *
 *  Note that Trace messages are shown only in Debug build and when the
 *  preprocessor definition _TRACE is defined.
 */

/**
 * @def    TRACE_DECLARE(id)
 * @param  id  A string identifying a component.
 */
#define TRACE_DECLARE( name )  extern TRACE_PARMS * name;

/**
 * @def    TRACE_DEFINE(id)
 * @param  id  An id previously declared using the macro TRACE_DECLARE().
 */
#define TRACE_DEFINE( name )  TRACE_PARMS * name;

/**
 * Initialization of Trace within a component.
 *
 * @def TRACE_INIT( subsys, id, level )
 *
 * @param  subsys   A string identifying a subsystem.
 * @param  id       Trace ID, previously initialized with TRACE_INIT().
 * @param  level    Trace level for this component.  One of:
 *                      - T_NONE
 *                      - T_ERROR
 *                      - T_WARNING
 *                      - T_INFO
 */
#define TRACE_INIT( subsys, name, level ) name = trace_init( _T(#subsys), _T(#name), level );
#define TRACE_DEINIT trace_deinit

#else

#define TRACE_DATA(a,b,c,d,e,f,g) {}
#define TRACE_INIT( subsys, name, level ) {}
#define TRACE_DEINIT trace_deinit(name) {}

// Only one of the OSAL_XXXX should be defined at a single time.

#if defined (OSAL_WINHANDHELD) || defined (OSAL_WIN32)

#define TRACE_DECLARE(name) extern char name;
#define TRACE_DEFINE(name)  char name;
#define TRACE {}
#endif

#if defined (OSAL_LINUX) || defined (OSAL_LINUXUSER) 
#define TRACE_DECLARE(name) 
#define TRACE_DEFINE(name) 
#define TRACE(...)
#endif

#endif


#ifdef __cplusplus
extern "C" {
#endif
void verify_trace(TCHAR *szFormat, ...); 

#ifdef __cplusplus
}
#endif

#ifdef _VERIFY_LOG
#define VERIFY_TRACE verify_trace
#else
#if defined (OSAL_WINHANDHELD) || defined (OSAL_WIN32)
#define VERIFY_TRACE
#endif
#if defined (OSAL_LINUX) || defined (OSAL_LINUXUSER) 
#define VERIFY_TRACE(...)
#endif
#endif

#ifdef _VERIFY_THROW
#if defined OSAL_WIN32
#define action(x) throw(x)
#elif defined OSAL_WINHANDHELD  || defined OSAL_LINUXUSER
#define action(x) exit(0)
#else
#error "action(x) not defined for this OS"
#endif
#else
#define action(x) do { goto end; } while(0)
#endif

#define OS_VERIFY(x) \
    if ( !(x) ) {    \
        VERIFY_TRACE(_T(#x) _T( " failed at %d, %s\n"), __LINE__, __FILE__ ); \
        action(#x);  \
    }

/**
 * Try to print a backtrace of the current stack.  For linux, the backtrace
 * is sent to stdout in userspace and the kernel log in kernelspace.
 */
void os_backtrace(void);

#endif // __TRACE_H__
