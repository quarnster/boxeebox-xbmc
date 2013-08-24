/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2010 Intel Corporation. All rights reserved.

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

  Copyright(c) 2007-2010  Intel Corporation. All rights reserved.
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

#ifndef __CLOCK_SYNC_H__
#define __CLOCK_SYNC_H__

#include "ismd_msg.h"
#include "ismd_global_defs.h"
#include "ismd_core.h"
#include "ismd_core_protected.h"
#include "gen3_clock.h"
#include "sven_devh.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    THRESHOLD_MONITOR = 1,
    PID_FILTERING    = 2,
    PID_FILTERING_V1    = 3
} ismd_clock_sync_algo_t;

typedef ismd_clock_sync_algo_t clock_sync_algo_t;     /**< Deprecated.  Do not use. */


typedef int ismd_clock_sync_t;
typedef int      clock_sync_t;                        /**< Deprecated.  Do not use. */

typedef struct
{
   bool                 clock_locked;
   bool                 error_untrackable;
} ismd_clock_sync_statistics_t;

typedef ismd_clock_sync_statistics_t clock_sync_statistics_t;    /**< Deprecated.  Do not use. */

/**
Allocates a new clock recoverer instance.

@param[out] handle : a handle to the newly-allocated clock recoverer.  Not modified if
the operation is not successful.

@retval ISMD_SUCCESS : the clock recoverer was successfully opened / allocated.
@retval ISMD_ERROR_NO_RESOURCES :  not enough resources existed to open the
clock recoverer.
*/

ismd_result_t SMDCONNECTION ismd_clock_sync_open(ismd_clock_sync_t *handle);
/* Deprecated.  Do not use. */
#define clock_sync_open ismd_clock_sync_open

/**
Closes / Frees a currently allocated clock recoverer instance.
@param[in] handle : handle to the clock recoverer to free.
@retval ISMD_SUCCESS : the clock was successfully closed/freed.
@retval ISMD_ERROR_INVALID_HANDLE : the specified clock handle was stale
or invalid.
*/

ismd_result_t SMDCONNECTION ismd_clock_sync_close(ismd_clock_sync_t handle);

/* Deprecated.  Do not use. */
#define clock_sync_close ismd_clock_sync_close

/**
Associates a platform specific clock to the clock recoverer.  This is the clock
which will be adjusted/recovered by the specified clock recoverer.

@param[in] handle : a handle to a clock recoverer instance.   This handle was
originally returned from ismd_clock_sync_open.

@param[in] handle : a handle to a clock instance.   This handle was
originally returned from ismd_clock_alloc.

@retval ISMD_SUCCESS : the clock value was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock handle was stale or invalid.
*/

ismd_result_t SMDEXPORT ismd_clock_sync_set_clock(ismd_clock_sync_t handle, ismd_clock_t clock);

/* Deprecated.  Do not use. */
#define clock_sync_set_clock ismd_clock_sync_set_clock


/**
Associates a clock recoverer algorithm (listed under the enum
"clock_sync_algo_t" in this header file)  to the clock recoverer.  This is the
algorithm which will be used to adjust/recover clock sby the specified clock recoverer.
Client should call this API atleast once to set the algorithm before starting clock recovery.
Most of the algorithm specific initialization is done in this API.  Hence the client
should call this API once, before using the clock recovery.

@param[in] handle : a handle to a clock recoverer instance.   This handle was
originally returned from ismd_clock_alloc.
@param[in] algorithm_type : One of the algorithms specified in
clock_sync_algo_t.  Default is "THRESHOLD_MONITOR".

@retval ISMD_SUCCESS : the algorithm was successfully set.
@retval ISMD_ERROR_INVALID_HANDLE :  the clock recoverer handle was stale or invalid.
@retval ISMD_ERROR_INVALID_PARAMETER : the algorithm_type is invalid.
*/

ismd_result_t SMDEXPORT ismd_clock_sync_set_algorithm(ismd_clock_sync_t handle, ismd_clock_sync_algo_t algorithm_type);

/* Deprecated.  Do not use. */
#define clock_sync_set_algorithm ismd_clock_sync_set_algorithm


/** In case of clock recovery PCR = Expected value
                              STC = Actual value
*/

ismd_result_t SMDEXPORT ismd_clock_sync_add_pair(ismd_clock_sync_t handle, int expected_value, int actual_value);

/* Deprecated.  Do not use. */
#define clock_sync_add_pair ismd_clock_sync_add_pair


ismd_result_t SMDEXPORT ismd_clock_sync_add_pair_th_safe(ismd_clock_sync_t handle, int expected_value, int actual_value);

/* Deprecated.  Do not use. */
#define clock_sync_add_pair_th_safe ismd_clock_sync_add_pair_th_safe

ismd_result_t SMDEXPORT ismd_clock_sync_statistics(ismd_clock_sync_t handle, ismd_clock_sync_statistics_t  * stats);

/* Deprecated.  Do not use. */
#define clock_sync_statistics ismd_clock_sync_statistics

ismd_result_t SMDEXPORT ismd_clock_sync_reset(ismd_clock_sync_t handle);

/* Deprecated.  Do not use. */
#define clock_sync_reset ismd_clock_sync_reset

#ifdef __cplusplus
}
#endif

#endif  /* __CLOCK_SYNC_H__ */
