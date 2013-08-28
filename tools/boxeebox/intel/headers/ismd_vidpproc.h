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


#ifndef __ISMD_VIDPPROC_H__
#define __ISMD_VIDPPROC_H__

#include "ismd_global_defs.h"
#include "ismd_msg.h"

#ifdef __cplusplus
extern "C"
{
#endif



   /** @weakgroup ismd_vidpproc Video Post-Processing
   Video Post-Processing (vidpproc) Module

   The video post-processing module provides scaling, de-interlacing,
   filters de-ringing noise, provides Gaussian filtering, and 
   source-cropping capabilities.

   Theory of Operation:

   I. Normal playback:
   -# Open the vidpproc device.
   -# If scaling is desired, set the scaling mode.
   -# If de-interlacing is desired, set the scaling mode.
   -# If source content cropping is desired, set the cropping rectangle
   -# If de-ringing noise reduction is not desired, disable the de-ringing filter
   -# If Gaussian noise reduction is not desired, disable the Gaussian filter
   -# Set the output parameters (width,height, pixel aspect ratio)
   -# Get the input and output ports.
   -# Write to the input port

   Steps 2 through 8 can be done in any order.

   On close, all resources are deallocated and everything is reset to the initial state.

   II. Trick modes:

   - There are no external API calls necessary for trick mode.

   III. Default states for vidpproc:

   - De-ringing Noise Filter:  Enabled (using HW defaults)
   - Gaussian Noise Filter:  Enabled (using HW defaults)
   - Scaling Policy:  Scale-to-fit
   - De-interlacing Policy:        Auto
   - Cropping:   Disabled
   - Input Format:   4:2:0, 8-bit pseudo-planar(NV12)
   - Output Format:  4:2:2, 8-bit pseudo-planar (NV16)

   IV. Important Numbers
   
   - Widths may range from 48 pixels to 1920 pixels
   - Heights may range from 48 pixels to 1088 pixels
   - Horizontal Scaling Ratios may range from 20x to 1/20x
   - Vertcal Scaling Ratios may range from 32x to 1/32x 

   */

   /** @ingroup ismd_vidpproc */
   /** @{ */

   /**
   Initialize a vidpproc device - this should only be called once. For
   Linux kernel builds, this will automatically be called in module
   init

   @retval ISMD_SUCCESS : The device was successfully opened

   @see ismd_module_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_init(void);


   /**
   De-initialize a vidpproc device - this should only be called once.  For
   Linux kernel builds, this will automatically be called in module
   de-init

   @retval ISMD_SUCCESS : The device was successfully de-initialized

   @see ismd_module_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_deinit(void);

   /**
   Close a vidpproc device that has been previously opened.

   @param[in] dev: vidpproc device handle

   @retval ISMD_SUCCESS : The vidpproc device was successfully closed.
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_close(ismd_dev_t dev);

   /**
   Open a vidpproc device.  This should be called before
   any other vidpproc functions are called.

   If a vidpproc is successfully opened it should be closed when the 
   application is done using it.

   If a vidpproc is not successfully opened the handle is in an undefined 
   state and should not be passed to other API functions, including close.

   @param[in] dev: vidpproc device handle

   @retval ISMD_SUCCESS : The vidpproc device was successfully opened.
   @retval all other values : An error occurred and the vidpproc device was not opened.

   @see ismd_dev_t
   */
   ismd_result_t SMDCONNECTION ismd_vidpproc_open( ismd_dev_t *dev);

   /**
   Set the vidpproc out pixel format.
   The supported output formats are:
   4:2:2 8-bit pseudo-planar NV16 (ISMD_PF_NV16),
   4:2:2 8-bit YUY2 (aka YUYV) (ISMD_PF_YUY2)

   @param[in] dev:  vidpproc device handle
   @param[in] output_fmt:  pixel format

   @retval ISMD_SUCCESS : The output format was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   @retval ISMD_ERROR_INVALID_PARAMETER: The output format was not supported.

   @see  ismd_pixel_format_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_set_output_format(
      ismd_dev_t dev,
      ismd_pixel_format_t output_fmt
   );

   /**
   Set the destination pixels for the horizontal scaler.

   @param[in] dev:  vidpproc device handle
   @param[in] width: destination width
   @param[in] height: destination height
   @param[in] aspect_ratio_num: output pixel aspect ratio numerator
   @param[in] aspect_ratio_denom: output pixel aspect ratio denominator

   @retval ISMD_SUCCESS : The destination width and height were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed so it is stale.
   @retval ISMD_INVALID_PARAMETER: The destination rectangle was larger than
   supported values. This may also occur if the width or height is not a
   multiple of four

   @see  ismd_dev_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_dest_params(
      ismd_dev_t dev,
      unsigned int width,
      unsigned int height,
      int aspect_ratio_num,
      int aspect_ratio_denom
   );

#ifdef ECR200090_DPE_PART_ENABLE
   /**
   Set the destination pixels for the horizontal scaler.

   @param[in] dev:  vidpproc device handle
   @param[in] width: destination width
   @param[in] height: destination height
   @param[in] aspect_ratio_num: output pixel aspect ratio numerator
   @param[in] aspect_ratio_denom: output pixel aspect ratio denominator
   @param[in] dest_x: destination rectangle origin x
   @param[in] dest_y: destination rectangle origin y

   @retval ISMD_SUCCESS : The destination width and height were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed so it is stale.
   @retval ISMD_INVALID_PARAMETER: The destination rectangle was larger than
   supported values. This may also occur if the width or height is not a
   multiple of four

   @see  ismd_dev_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_dest_params2(
      ismd_dev_t dev,
      unsigned int width,
      unsigned int height,
      int aspect_ratio_num,
      int aspect_ratio_denom,
      unsigned int dest_x,
      unsigned int dest_y
   );
#endif

   /**
   Enable the de-ringing filter for a particular context, using hardware defaults.
   There will eventually be a separate function to tweak hardware de-ringing
   threshold values.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The de-ringing filter was successfully enabled
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_deringing_enable( ismd_dev_t dev);

   /**
   Disable the de-ringing filter for a particular context

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The de-ringing filter was successfully disabled
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_deringing_disable( ismd_dev_t dev);


   /**
   @brief Enable the vidpproc Gaussian noise reduction filter in the de-interlacer

   Enable the vidpproc Gaussian noise reduction filter in the de-interlacer.
   his will use hardware defaults.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The Gaussian filter was enabled
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_gaussian_enable( ismd_dev_t dev);

   /**
   @brief Disable the vidpproc Gaussian noise reduction filter in the de-interlacer

   Disable the Gaussian noise reduction filter in the de-interlacer.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The Gaussian filter was disabled
   @retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_gaussian_disable( ismd_dev_t dev);



   /**
   @brief Set vidpproc playback rate

   Set the vidpproc  playback rate for the scaler and deinterlacer.  This will
   only be used in the deinterlacing case.
   Spatial de-interlacing only in trick mode.

   @param[in] dev:  vidpproc device handle
   @param[in] playback_rate_fixed : rate to set the VIDPPROC playback rate.

   @retval ISMD_SUCCESS : The playback rate was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_set_playback_rate( ismd_dev_t dev, int playback_rate_fixed);

   /**
   @brief Set cropping of the source pixels.

   Set cropping of the source pixels.  Must also provide source x and y pixels.
   The cropping rectangle must follow the restrictions in
   ismd_vidpproc_set_dest_params.  Additionally, the source rectangle and x
   and y are also subject to the restrictions in ismd_vidpproc_set_dest_parms.

   @param[in] dev:  vidpproc device handle
   @param[in] x: source x pixel
   @param[in] y: source y pixel
   @param[in] width: crop rectangle width
   @param[in] height: crop rectangle height

   @retval ISMD_SUCCESS : The source cropping rectangle pixel parameters were
   successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.  Note
   that this may indicate that handle was recently closed.
   @retval ISMD_INVALID_PARAMETER: The cropping rectangle or the cropping
   rectangle plus x and y was larger than supported values.

   @see  ismd_dev_t
   @see ismd_vidpproc_set_dest_params
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_crop_input( ismd_dev_t dev,
         unsigned int x, unsigned int y,
         unsigned int width, unsigned int height);


   /**
   Disable cropping of the source pixels.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The source cropping was disabled
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_disable_crop_input( ismd_dev_t dev);

   /**
   Enable vidpproc pan-scan mode.  This will cause the vidpproc to check the
   stream data for pan-scan information.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : Pan and scan was successfully enabled
   @retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_pan_scan_enable( ismd_dev_t dev);

   /**
   Disable vidpproc pan-scan mode

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : Pan and scan was disabled
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_pan_scan_disable( ismd_dev_t dev);




#define MAX_Y_FILTER_SIZE 128*9*4 /* This is non-HW-packed size */
#define MAX_UV_FILTER_SIZE 128*5*4 /* This is non-HW-packed size */

   /** This data definition is necessary to pass data across the SMD Auto-API marshalling infrastructure */
   typedef unsigned char y_filter_data[MAX_Y_FILTER_SIZE];
   typedef unsigned char uv_filter_data[MAX_UV_FILTER_SIZE];

   /**

   Load new horizontal filter coefficients for the luma and chroma components.
   Luma and chroma must be configured separately.

   @note rough draft

   @param[in] dev:  vidpproc device handle
   @param[in] y_filter_data: the new luma coefficients to be loaded
   @param[in] uv_filter_data: the new chroma coefficients to be loaded
   @param[in] y_size: the size of the luma coefficients, in bytes
   @param[in] uv_size: the size of the chroma coefficients, in bytes

   @retval ISMD_SUCCESS : The horizontal filter coefficients were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_override_horizontal_filter_coeffcients(
      ismd_dev_t dev,
      unsigned char y_filter_data[MAX_Y_FILTER_SIZE],
      unsigned char uv_filter_data[MAX_UV_FILTER_SIZE],
      unsigned int y_size,
      unsigned int uv_size
   );


   /**

   Set default horizontal filter coefficients for the luma component if a custom
   set has been overridden, otherwise this function call is ignored.

   @param[in] dev:  vidpproc device handle


   @retval ISMD_SUCCESS : The horizontal filter coefficients were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The port handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */


   ismd_result_t SMDEXPORT ismd_vidpproc_set_default_horizontal_filter_coeffcients( ismd_dev_t dev);


   /**
   Load new vertical filter coefficients for the luma and chroma components.
   Luma and chroma must be configured separately.

   @param[in] dev:  vidpproc device handle
   @param[in] y_filter_data: the new luma coefficients to be loaded
   @param[in] uv_filter_data: the new chroma coefficients to be loaded
   @param[in] y_size: the size of the luma coefficients, in bytes
   @param[in] uv_size: the size of the chroma coefficients, in bytes

   @retval ISMD_SUCCESS : The vertical filter coefficients were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_override_vertical_filter_coeffcients(
      ismd_dev_t dev,
      unsigned char y_filter_data[MAX_Y_FILTER_SIZE],
      unsigned char uv_filter_data[MAX_UV_FILTER_SIZE],
      unsigned int y_size,
      unsigned int uv_size
   );



   /**
   Set default vertical filter coefficients for the luma component if a custom
   set has been overridden, otherwise this function call is ignored.

   @param[in] dev:  vidpproc device handle

   @retval ISMD_SUCCESS : The horizontal filter coefficients were successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   */


   ismd_result_t SMDEXPORT ismd_vidpproc_set_default_vertical_filter_coeffcients( ismd_dev_t dev);


   /**
   Set a blur factor for an existing scaler coefficient set.  This operation
   fails if filter coefficients have been overridden previously.

   @param[in] dev: vidpproc device handle
   @param[in] blur_factor: range from 0-99 (0 is no blurring, 99 is max)

   @retval ISMD_SUCCESS : The horizontal degradation mode was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   @see coefficient_set_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_set_degrade_factor( ismd_dev_t dev, unsigned int blur_factor);

   /**
   @brief Disable the blur factor for an existing scaler coefficient set

   Disable the blur factor for an existing scaler coefficient set

   @param[in] dev: vidpproc device handle


   @retval ISMD_SUCCESS : The horizontal degradation mode was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   @see coefficient_set_t
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_disable_degrade_factor( ismd_dev_t dev);

   /** This enumeration contains scaling policies */
   typedef enum {
      /**
       Basic independent x and y scaling, ignores pixel aspect ratio
      */
      SCALE_TO_FIT,

      /**
       Force the output to be same size as the input ignores pixel aspect ratio
      */
      VPP_NO_SCALING,

      /**
       Fill entire screen . use both the source and destination pixel aspect ratios
      */
      ZOOM_TO_FILL,

      /**
       Fit at least one side of the destination rectangle,letterbox/pillarbox
       the remainder. This mode ensures that the entire source buffer is
       displayed. Accounts for both source and destination pixel aspect ratios
      */
      ZOOM_TO_FIT,

      /**
       Anamorphic scaling
      */
      VPP_SCP_ANAMORPHIC,

      /**
       Maximum number of scaling policies
      */
      VPP_MAX_SCALING_POLICIES
   } ismd_vidpproc_scaling_policy_t;

   /**
   Set the scaling policy for the pipeline.

   @param[in] dev: vidpproc device handle
   @param[in] policy: which scaling policy to use

   @retval ISMD_SUCCESS : The scaling policy was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see ismd_dev_t
   @see ismd_vidpproc_scaling_policy_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_scaling_policy(
      ismd_dev_t dev,
      ismd_vidpproc_scaling_policy_t policy
   );


   /** This enumeration contains supporting deinterlacing policies */
   typedef enum {
      /**
       do not deinterlace unless there is vertical scaling.
      */
      NONE = 0,

      /**
       no motion adaptive filtering
      */
      SPATIAL_ONLY,

      /**
       use a lower film mode detection threshold
      */
      FILM,

      /**
       disable film mode detection
      */
      VIDEO,

      /**
       use motion-adaptive deinterlacing if video, also detect film mode with
       high film mode threshold
      */
      AUTO,

      /**
       deinterlace top field of the frame and drop bottom field
      */
      TOP_FIELD_ONLY,

      /**
       do not deinterlace.
      */
      NEVER,

      /**
       Maximum number of deinterlacing policies
      */
      VPP_MAX_DEINTERLACE_POLICIES,
   } ismd_vidpproc_deinterlace_policy_t;

   /**
   Set the deinterlace policy for the pipeline.

   @param[in] dev: vidpproc device handle
   @param[in] policy: which deinterlace policy to use

   @retval ISMD_SUCCESS : The de-interlacing policy was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see  ismd_dev_t
   @see ismd_vidpproc_deinterlace_policy_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_deinterlace_policy(
      ismd_dev_t dev,
      ismd_vidpproc_deinterlace_policy_t policy
   );

   /**
   This function returns the event that is set when the input source resolution
   changes.  This event will occur when the first frame is received after a
   device close or flush.

   @param[in] dev: vidpproc device handle
   @param[out] *input_src_res_event: event that will be triggered when input source 
   resolution changes

   @retval ISMD_SUCCESS: The input port was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_get_input_src_resolution_event(
      ismd_dev_t dev,
      ismd_event_t *input_src_res_event
   );
   /**
   This function returns the event that is set when the input display resolution
   changes.  This event will occur when the first frame is received after a
   device close or flush.

   @param[in] dev: vidpproc device handle
   @param[out] *input_disp_res_event: event that will be triggered when input display 
   resolution changes

   @retval ISMD_SUCCESS: The input port was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_get_input_disp_resolution_event(
      ismd_dev_t dev,
      ismd_event_t *input_disp_res_event
   );

   /**
   This function returns the input buffer that is currently in the DPE.  This
   will add a buffer reference count to the input buffer that is returned.  The
   calling application must dereference the input buffer, else a buffer memory
   leak will occur.

   @param[in] dev: vidpproc device handle
   @param[out] *input_buf: buffer that is currently has been read from the DPE
   input port

   @retval ISMD_SUCCESS: The input port was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */

   ismd_result_t SMDEXPORT ismd_vidpproc_get_input_buffer( ismd_dev_t dev, ismd_buffer_handle_t *input_buf);

   /**
   Gets the input port handle for the given vidpproc instance.
   Decoded video frames are fed to the viddproc through this handle

   @param[in] dev: vidpproc device handle
   @param[out] smd_port_handle: input port

   @retval ISMD_SUCCESS: The input port was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_get_input_port( ismd_dev_t dev, ismd_port_handle_t *smd_port_handle);

   /**
   Gets the output port handle for the given vidpproc instance.
   Processed video frames are fed to this output port

   @param[in] dev: vidpproc device handle
   @param[out] smd_port_handle: output port

   @retval ISMD_SUCCESS : The output port was successfully retrieved
   @retval
   @note rough draft ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.
   Note that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_get_output_port( ismd_dev_t dev, ismd_port_handle_t *smd_port_handle);
   /**
   Set the Ringing Noise Reduction (RNR) value per-stream.  Values range from 1-100
   with 100 being the highest.  Default value is four.

   @param[in] dev: vidpproc device handle
   @param[in] level: RNR level (1-100)

   @retval ISMD_SUCCESS : The RNR level was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_deringing_level( ismd_dev_t dev, int level);

   /**
   Set the Gaussian (temporal) noise removal value per-stream.  Valid values are 0,1 and 2.
   Default value is one.

   @param[in] dev: vidpproc device handle
   @param[in] level: NRF level (0,1 or 2)

   @retval ISMD_SUCCESS : The RNR level was successfully retrieved
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_gaussian_level( ismd_dev_t dev, int level);

   /**
   This enumeration contains supporting parameters
   **/
   typedef enum {
      /**
      Used internally to determine the number of parameter enums
      */
      PAR_FIRST = 0x100,

      /**
      deinterlace policy
      */
      PAR_DI_POLICY = PAR_FIRST,

      /**
      motion threshold
      */
      PAR_MADTH_DELTA,

      /**
      noise filter level
      */
      PAR_NRF_LEVEL,

      /**
      Horizontal inside-scaler deringing
      */
      PAR_ENABLE_HSC_DERING,

      /**
      Vertical inside-scaler deringing
      */
      PAR_ENABLE_VSC_DERING,

      /**
      Horizontal coefficients table index shift Y
      */
      PAR_HSC_COEFF_IDXSHIFT_Y,

      /**
      Horizontal coefficients table index shift UV
      */
      PAR_HSC_COEFF_IDXSHIFT_UV,

      /**
      Vertical coefficients table index shift Y
      */
      PAR_VSC_COEFF_IDXSHIFT_Y,

      /**
      Vertical coefficients table index shift UV
      */
      PAR_VSC_COEFF_IDXSHIFT_UV,

      /**
      Scaler coefficient table number for Y
      */
      PAR_SCALER_TABLES_NUM_Y,

      /**
      Scaler coefficient table number for Y
      */
      PAR_SCALER_TABLES_NUM_UV,

      /**
      Stream Priority
      */
      PAR_STREAM_PRIORITY,

      /**
      Bypass Deinterlacer
      */
      PAR_BYPASS_DEINTERLACE,

	  /**
      Maximum of Absolute Differences Post Motion Detection Weight
      */
      PAR_MAD_WEIGHT_POST,

      /**
      Maximum of Absolute Differences Pre Detection Weight
      */
      PAR_MAD_WEIGHT_PRE,

      /**
      Sum of Absolute Differences Post Motion Detection Weight
      */
      PAR_SAD_WEIGHT_POST,

      /**
      Sum of Absolute Differences Pre Motion Detection Weight (used for low scores)
      */
      PAR_SAD_WEIGHT_PRE1,

      /**
      Sum of Absolute Differences Pre Motion Detection Weight
      */
      PAR_SAD_WEIGHT_PRE2,

      /**
      Enable Gen4 Enhanced DI algorithm
      */
      PAR_ENABLE_GEN4_ENHANCED_DI_ALGORITHM,

      /**
      Bypass Film Mode Cadence Detection in AUTO FMD
      */
	  PAR_BYPASS_AUTO_FMD_32322_23322,
	  PAR_BYPASS_AUTO_FMD_32,
	  PAR_BYPASS_AUTO_FMD_2224_2332,
	  PAR_BYPASS_AUTO_FMD_22,
	  PAR_BYPASS_AUTO_FMD_33,
	  PAR_BYPASS_AUTO_FMD_ETC,
	  
	  /**
      Disable MAD motion detection in Gen4 Deinterlacer
      */
	  PAR_DISABLE_MAD_MOTION_DETECTION,

      /**
      Used internally to determine the number of parameter enums
      */
      PAR_COUNT
   } ismd_vidpproc_param_t;

   /**
   Unified function to set parameter to Driver/FW.

   @param[in] dev: vidpproc device handle
   @param[in] par_name: which parameter to set
   @param[in] par_value: what value to set

   @retval ISMD_SUCCESS : The parameter was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see ismd_dev_t
   @see ismd_vidpproc_param_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_parameter(
      ismd_dev_t dev,
      ismd_vidpproc_param_t par_name,
      int par_value
   );

   /**
   Unified function to get parameter from Driver/FW.

   @param[in] dev: vidpproc device handle
   @param[in] par_name: which parameter to get
   @param[out] par_value: the variable address for returned parameter value

   @retval ISMD_SUCCESS : The parameter was successfully got
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see ismd_dev_t
   @see ismd_vidpproc_param_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_get_parameter(
      ismd_dev_t dev,
      ismd_vidpproc_param_t par_name,
      int* par_value
   );

   /**
   Overrides the existing luma coefficients for a given table.
   @param[in] dev:  vidpproc device handle
   @param[in] y_filter_table_index index of the table to be updated
   @param y_filter_data a pointer to a buffer holding the new data.
   @param y_size the size of the filter data buffer. checked but not used.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_override_horizontal_filter_luma_coefficients(
      ismd_dev_t dev,
      unsigned int y_filter_table_index,
      unsigned int y_filter_data,
      unsigned int y_size
   );

   /**
   Overrides the existing horizontal chroma coefficients for a given table.
   @param[in] dev:  vidpproc device handle
   @param[in] uv_filter_table_index: index of the table to be updated
   @param[in] uv_filter_data: a pointer to a buffer holding the new data.
   @param[in] uv_size: the size of the filter data buffer. checked but not used.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_override_horizontal_filter_chroma_coefficients(
      ismd_dev_t dev,
      unsigned int uv_filter_table_index,
      unsigned int uv_filter_data,
      unsigned int uv_size
   );

   /**
   Overrides the existing vertical chroma coefficients for a given table.
   @param[in] dev:  vidpproc device handle
   @param[in] uv_filter_table_index: index of the table to be updated
   @param[in] uv_filter_data: a pointer to a buffer holding the new data.
   @param[in] uv_size: the size of the filter data buffer. checked but not used.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_override_vertical_filter_chroma_coefficients(
      ismd_dev_t dev,
      unsigned int uv_filter_table_index,
      unsigned int uv_filter_data,
      unsigned int uv_size
   );

   /**
   Start a linear animated zoom.
   @param[in] dev: vidpproc device handle

   @param[in] linear_animated_zoom_start_flag: This start flag is used for
   control the start and end of one animated zoom. This flag must be set to
   TRUE if you want to begin linear animated zoom.
   @param[in] transition_frame_cnt: This parameter indicates the frame counters
   for one linear animated zoom procedure e.g. 30 indicates that it will take
   30 frames from begin to end

   @param[in] src_x: Indicate the x coordinate of the left upper corner of the
   input source rectangle image position.
   @param[in] src_y: Indicate the y coordinate of the left upper corner of the
   input source rectangle image position.
   @param[in] src_width: Indicate the width of the input source rectangle image.
   @param[in] src_height:  Indicate the height of the input source rectangle image.

   @param[in] dest_start_x: Indicate the x coordinate of the left upper corner of the
   start output destination rectangle image position.
   @param[in] dest_start_y: Indicate the y coordinate of the left upper corner of the
   start output destination rectangle image position.
   @param[in] dest_start_width: Indicate the width of input start output destination image.
   @param[in] dest_start_height: Indicate the height of input start output destination image.

   @param[in] dest_end_x: Indicate the y coordinate of the left upper corner of the
   end output destination rectangle image position.
   @param[in] dest_end_y: Indicate the y coordinate of the left upper corner of the
   end output destination rectangle image position.
   @param[in] dest_end_width: Indicate the width of input end output destination image.
   @param[in] dest_end_height: Indicate the height of input end output destination image.

   @retval ISMD_SUCCESS : The parameter was successfully got
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_start_linear_animated_zoom(
      ismd_dev_t dev,
      bool linear_animated_zoom_start_flag,
      unsigned int transition_frame_cnt,
      unsigned int src_x,
      unsigned int src_y,
      unsigned int src_width,
      unsigned int src_height,
      unsigned int dest_start_x,
      unsigned int dest_start_y,
      unsigned int dest_start_width,
      unsigned int dest_start_height,
      unsigned int dest_end_x,
      unsigned int dest_end_y,
      unsigned int dest_end_width,
      unsigned int dest_end_height
   );

   /**
   Stop animated zoom.
   Use if the application wants to stop the animated zoom while it is still running
   (both for general and linear animated zooms)

   @param[in] dev: vidpproc device handle
   @retval ISMD_SUCCESS : The parameter was successfully got
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_disable_animated_zoom(ismd_dev_t dev);

   /**
   Set general animated zoom parameters.
   This function needs to be called for every frame if to application wants to
   implement a whole complete animated zoom.
   @param[in] dev: vidpproc device handle
   @param[in] src_x: Indicate the x coordinate of the left upper corner of the
   input source rectangle image position.
   @param[in] src_y: Indicate the y coordinate of the left upper corner of the
   input source rectangle image position.
   @param[in] src_width: Indicate the width of the input source rectangle image.
   @param[in] src_height:  Indicate the height of the input source rectangle image.

   @param[in] dest_x: Indicate the x coordinate of the left upper corner of the
   output destination rectangle image position.
   @param[in] dest_y: Indicate the y coordinate of the left upper corner of the
   output destination rectangle image position.
   @param[in] dest_width: Indicate the width of input output destination image.
   @param[in] dest_height: Indicate the height of input output destination image.

   @retval ISMD_SUCCESS : The parameter was successfully got
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_animated_zoom(
      ismd_dev_t dev,
      unsigned int src_x,
      unsigned int src_y,
      unsigned int src_width,
      unsigned int src_height,

      unsigned int dest_x,
      unsigned int dest_y,
      unsigned int dest_width,
      unsigned int dest_height
   );

#define VIDPPROC_MAX_HANDLES 16

   /**
   Describes the result of a handle query.
   Specifically this a list of handles and flags indicating if those handles
   pointed to valid (open) devices at the time of the query.
   */

   typedef struct ismd_vidpproc_handle_query_result_t {
      ismd_dev_handle_t dev_handle[VIDPPROC_MAX_HANDLES];
      bool dev_handle_valid[VIDPPROC_MAX_HANDLES];
      int  valid_hanlde_cnt;
   }

   ismd_vidpproc_handle_query_result_t ;

   /**
   Queries the driver for a list of open device handles.

   @param[out] handle_info : must point t a valid query result structure. the structure will be populated with the results of the query.
   @param[in] size_of_handle_info : the size of the query info structure, this is used to validate that the correct version of the API is being used.

   @retval ISMD_SUCCESS : The query has successfully run.
   @retval anything else : The query has failed to run.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_query_handle(ismd_vidpproc_handle_query_result_t* handle_info, unsigned int size_of_handle_info);

   /**
   An enumeration of global events available in vidpproc.
   */
   typedef enum ismd_vidpproc_global_event_t {
      /**
      A DPE device has been opened. 
      */
      ISMD_VIDPPROC_DEVICE_OPENED_EVENT,

      /**
      A DPE device has been opened
      */
      ISMD_VIDPPROC_DEVICE_CLOSED_EVENT,

      /**
      Used internally to determine the number of event enums
      */
      ISMD_VIDPPROC_GLOBAL_EVENT_COUNT,
   } ismd_vidpproc_global_event_t;


   /**
   Gets a global event for the vidpproc driver. 
   This may be used to set the Device Closed  or
   Device Opened event.

   @param[in] event_type: operation event type.
   @param[out] vidpproc_event : vidpproc event set by the DPE

   @retval ISMD_SUCCESS : The event is successfully returned.
   @retval ISMD_ERROR_NULL_POINTER : The vidpproc_event pointer is NULL.
   @retval ISMD_ERROR_INVALID_PARAMETER: The event_type parameter is invalid.
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_get_global_event(
      ismd_vidpproc_global_event_t event_type,
      ismd_event_t *vidpproc_event
   );

   /** This enumeration contains supported vertical scaling policies */
   typedef enum ismd_vidpproc_vertical_scaling_policy_t{
      /**
       Normal Scaling
      */
      NORMAL_VERTICAL_SCALING,

      /**
       Halve the vertical resolution before processing the frame
      */
      HALF_VERTICAL_PRESCALING,

      /**
       Maximum number of policies
      */
      VPP_MAX_VERTICAL_SCALING_POLICIES
   } ismd_vidpproc_vertical_scaling_policy_t;

   /**
   Set the vertical scaling policy for the pipeline.

   @param[in] dev: vidpproc device handle
   @param[in] policy: which vertical scaling policy to use

   @retval ISMD_SUCCESS : The de-interlacing policy was successfully set
   @retval ISMD_ERROR_INVALID_HANDLE : The device handle was not valid.  Note
   that this may indicate that handle was recently closed.

   @see ismd_dev_t
   @see ismd_vidpproc_vertical_scaling_policy_t
   */
   ismd_result_t SMDEXPORT ismd_vidpproc_set_vertical_scaling_policy(
      ismd_dev_t dev,
      ismd_vidpproc_vertical_scaling_policy_t policy
   );


   /**\}*/ // end of ingroup ismd_vidpproc

#ifdef __cplusplus
}

#endif


#endif


