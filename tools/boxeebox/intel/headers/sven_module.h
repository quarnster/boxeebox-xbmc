#ifndef SVEN_MODULE_H
#define SVEN_MODULE_H 1
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
/** SVEN Module enumeration
 * WARNING:  If you make changes here, you must make exactly
 * equivalent changes inside sven_modules.c otherwise units will
 * not get properly "reversed"
 */
enum SVEN_Module
{
/*00*/    SVEN_module_invalid,              /* NO ZEROES ALLOWED */
          SVEN_module_CPU,
          SVEN_module_GEN2_MCH,
          SVEN_module_GEN2_GPU,
          SVEN_module_GEN2_MPG2VD,
          SVEN_module_GEN2_PREFILT,
          SVEN_module_GEN2_TSDEMUX,
          SVEN_module_GEN2_VCAP,
/*08*/    SVEN_module_GEN2_HDMI,
          SVEN_module_GEN2_AUDIO,
          SVEN_module_GEN2_CHAP,
          SVEN_module_GEN2_VDC,
          SVEN_module_GEN2_DPE,
          SVEN_module_GEN2_UEE,
          SVEN_module_GEN2_EXBUS,
          SVEN_module_GEN2_UART0,
/*10*/    SVEN_module_GEN2_UART1,
          SVEN_module_GEN2_GPIO,
          SVEN_module_GEN2_I2C0,
          SVEN_module_GEN2_I2C1,
          SVEN_module_GEN2_I2C2,
          SVEN_module_GEN2_SC0,
          SVEN_module_GEN2_SC1,
          SVEN_module_GEN2_SPI,
/*18*/    SVEN_module_GEN2_MSPOD,
          SVEN_module_GEN2_FW_TSDEMUX,
          SVEN_module_GEN1_AUDIO,
          SVEN_module_GEN1_PREFILT,
          SVEN_module_GEN1_MPEG2E,
          SVEN_module_GEN1_VCAP,
          SVEN_module_GEN1_PMU,
          SVEN_module_TBE_AVI,
/*20*/    SVEN_module_TBE_AVO,
          SVEN_module_TEST_APP,
          SVEN_module_GEN1_H264VD,
          SVEN_module_GEN1PLUS_OMAR,
          SVEN_module_GEN1_ADI,
          SVEN_module_GEN1_TSDEMUX,
          SVEN_module_GEN1_VDC,
          SVEN_module_GEN1_MPG2VD,
/*28*/    SVEN_module_GEN1_EXPBUS,
          SVEN_module_GEN1_UEE,
          SVEN_module_GEN1_I2C,
          SVEN_module_GEN1_UART,
          SVEN_module_GEN1_GFX,
          SVEN_module_VIDREND,
          SVEN_module_GEN1_MCU,
          SVEN_module_GEN1_XAB_VID,
/*30*/    SVEN_module_GEN1_XAB_APER,
          SVEN_module_GEN1_XAB_XPORT,
          SVEN_module_GEN1_VCMH,
          SVEN_module_GEN1_XAB_MEM,
          SVEN_module_GEN1PLUS_OMAR_CW,
          SVEN_module_SMD_CORE,
          SVEN_module_GST_PLUGIN,
          SVEN_module_GEN3_AUD_DSP0,     /* Audio DSP0 unit*/
/*38*/    SVEN_module_GEN3_DPE,
          SVEN_module_GEN3_DEMUX,
          SVEN_module_GEN3_HDMI_TX,
          SVEN_module_GEN3_VDC,
          SVEN_module_GEN3_GFX,
          SVEN_module_GEN3_COMPOSITOR,
          SVEN_module_GEN3_MFD,
          SVEN_module_GEN3_SEC,
/*40*/    SVEN_module_GEN3_OMAR,
          SVEN_module_GEN3_AUD_IO,      /* Audio IO Subsystem */
          SVEN_module_GEN3_PMU,
          SVEN_module_GEN3_AUD_DSP1,    /* Audio DSP1 unit*/
          SVEN_module_GEN3_UART,
          SVEN_module_GEN3_GPIO,
          SVEN_module_GEN3_I2C,
          SVEN_module_GEN3_GPU_GMH,
/*48*/    SVEN_module_GEN3_PREFILT,
          SVEN_module_GEN3_SRAM,        /* SEC SRAM */
          SVEN_module_GEN3_EXP,
          SVEN_module_GEN3_EXP_WIN,
          SVEN_module_GEN3_SCARD,
          SVEN_module_GEN3_SPI,
          SVEN_module_GEN3_MSPOD,
          SVEN_module_GEN3_IR,
/*50*/    SVEN_module_GEN3_DFX,
          SVEN_module_GEN3_GBE,
          SVEN_module_GEN3_GBE_MDIO,
          SVEN_module_GEN3_CRU,
          SVEN_module_GEN3_USB,
          SVEN_module_GEN3_SATA,
          SVEN_module_GEN3_TVE,
          SVEN_module_IPCLIB,
          SVEN_module_GEN3_TSOUT,
/*58*/    SVEN_module_BUFMON,
          SVEN_module_GEN4_MFD,
          SVEN_module_GEN4_GV,
          SVEN_module_GEN4_NAND_DEV,
          SVEN_module_GEN4_NAND_CSR,
          SVEN_module_GEN4_MEU,
          SVEN_module_GEN4_HDVCAP,
          SVEN_module_APP_EVENTS,
/*60*/    SVEN_module_GEN4_AUD_IO,      /* Audio IO Subsystem */
          SVEN_module_GEN4_DPE,

/* WARNING: ONLY APPEND OR YOU WILL BREAK RECORDED SVENLOGS FOR EVERYONE */
                    
/** WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
 * If you make changes here, you must make exactly
 * equivalent changes inside:
 *
 *          sven_modules.c
 *
 * otherwise units will not get properly "reversed"
 */

          SVEN_module_MAX               /* MAX Placeholder */
};

#ifdef SVEN_DEPRECATED
    #define SVEN_module_VR_MCH          SVEN_module_GEN2_MCH
    #define SVEN_module_VR_GPU          SVEN_module_GEN2_GPU
    #define SVEN_module_VR_MPG2VD       SVEN_module_GEN2_MPG2VD
    #define SVEN_module_VR_PREFILT      SVEN_module_GEN2_PREFILT
    #define SVEN_module_VR_TSDEMUX      SVEN_module_GEN2_TSDEMUX
    #define SVEN_module_VR_VCAP         SVEN_module_GEN2_VCAP
    #define SVEN_module_VR_HDMI         SVEN_module_GEN2_HDMI
    #define SVEN_module_VR_AUDIO        SVEN_module_GEN2_AUDIO
    #define SVEN_module_VR_CHAP         SVEN_module_GEN2_CHAP
    #define SVEN_module_VR_VDC          SVEN_module_GEN2_VDC
    #define SVEN_module_VR_DPE          SVEN_module_GEN2_DPE
    #define SVEN_module_VR_UEE          SVEN_module_GEN2_UEE
    #define SVEN_module_VR_EXBUS        SVEN_module_GEN2_EXBUS
    #define SVEN_module_VR_UART0        SVEN_module_GEN2_UART0
    #define SVEN_module_VR_UART1        SVEN_module_GEN2_UART1
    #define SVEN_module_VR_GPIO         SVEN_module_GEN2_GPIO
    #define SVEN_module_VR_I2C0         SVEN_module_GEN2_I2C0
    #define SVEN_module_VR_I2C1         SVEN_module_GEN2_I2C1
    #define SVEN_module_VR_I2C2         SVEN_module_GEN2_I2C2
    #define SVEN_module_VR_SC0          SVEN_module_GEN2_SC0
    #define SVEN_module_VR_SC1          SVEN_module_GEN2_SC1
    #define SVEN_module_VR_SPI          SVEN_module_GEN2_SPI
    #define SVEN_module_VR_MSPOD        SVEN_module_GEN2_MSPOD
    #define SVEN_module_VR_FW_TSDEMUX   SVEN_module_GEN2_FW_TSDEMUX

    #define SVEN_module_GEN2_AUDIO       SVEN_module_GEN3_AUDIO
    #define SVEN_module_GEN2_PREFILT     SVEN_module_GEN3_PREFILT
    #define SVEN_module_GEN2_MPEG2E      SVEN_module_GEN3_MPEG2E
    #define SVEN_module_GEN2_VCAP        SVEN_module_GEN3_VCAP
    #define SVEN_module_GEN2_PMU         SVEN_module_GEN3_PMU
    #define SVEN_module_GEN2_H264VD      SVEN_module_GEN3_H264VD
    #define SVEN_module_GEN2_ADI         SVEN_module_GEN3_ADI
    #define SVEN_module_GEN2_TSDEMUX     SVEN_module_GEN3_TSDEMUX
    #define SVEN_module_GEN2_VDC         SVEN_module_GEN3_VDC
    #define SVEN_module_GEN2_MPG2VD      SVEN_module_GEN3_MPG2VD
    #define SVEN_module_GEN2_EXPBUS      SVEN_module_GEN3_EXPBUS
    #define SVEN_module_GEN2_UEE         SVEN_module_GEN3_UEE
    #define SVEN_module_GEN2_I2C         SVEN_module_GEN3_I2C
    #define SVEN_module_GEN2_UART        SVEN_module_GEN3_UART
    #define SVEN_module_GEN2_GFX         SVEN_module_GEN3_GFX
    #define SVEN_module_GEN2_MCU         SVEN_module_GEN3_MCU
    #define SVEN_module_GEN2_XAB_VID     SVEN_module_GEN3_XAB_VID
    #define SVEN_module_GEN2_XAB_APER    SVEN_module_GEN3_XAB_APER
    #define SVEN_module_GEN2_XAB_XPORT   SVEN_module_GEN3_XAB_XPORT
    #define SVEN_module_GEN2_VCMH        SVEN_module_GEN3_VCMH
    #define SVEN_module_GEN2_XAB_MEM     SVEN_module_GEN3_XAB_MEM

    #define SVEN_module_CE31XX_AUD_DSP0     SVEN_module_GEN3_AUD_DSP0
    #define SVEN_module_CE31XX_DPE          SVEN_module_GEN3_DPE
    #define SVEN_module_CE31XX_DEMUX        SVEN_module_GEN3_DEMUX
    #define SVEN_module_CE31XX_HDMI_TX      SVEN_module_GEN3_HDMI_TX
    #define SVEN_module_CE31XX_VDC          SVEN_module_GEN3_VDC
    #define SVEN_module_CE31XX_GFX          SVEN_module_GEN3_GFX
    #define SVEN_module_CE31XX_COMPOSITOR   SVEN_module_GEN3_COMPOSITOR
    #define SVEN_module_CE31XX_MFD          SVEN_module_GEN3_MFD
    #define SVEN_module_CE31XX_SEC          SVEN_module_GEN3_SEC
    #define SVEN_module_CE31XX_OMAR         SVEN_module_GEN3_OMAR
    #define SVEN_module_CE31XX_AUD_IO       SVEN_module_GEN3_AUD_IO
    #define SVEN_module_CE31XX_PMU          SVEN_module_GEN3_PMU
    #define SVEN_module_CE31XX_AUD_DSP1     SVEN_module_GEN3_AUD_DSP1
    #define SVEN_module_CE31XX_UART         SVEN_module_GEN3_UART
    #define SVEN_module_CE31XX_GPIO         SVEN_module_GEN3_GPIO
    #define SVEN_module_CE31XX_I2C          SVEN_module_GEN3_I2C
    #define SVEN_module_CE31XX_GPU_GMH      SVEN_module_GEN3_GPU_GMH
    #define SVEN_module_CE31XX_PREFILT      SVEN_module_GEN3_PREFILT
    #define SVEN_module_CE31XX_SRAM         SVEN_module_GEN3_SRAM
    #define SVEN_module_CE31XX_EXP          SVEN_module_GEN3_EXP
    #define SVEN_module_CE31XX_EXP_WIN      SVEN_module_GEN3_EXP_WIN
    #define SVEN_module_CE31XX_SCARD        SVEN_module_GEN3_SCARD
    #define SVEN_module_CE31XX_SPI          SVEN_module_GEN3_SPI
    #define SVEN_module_CE31XX_MSPOD        SVEN_module_GEN3_MSPOD
    #define SVEN_module_CE31XX_IR           SVEN_module_GEN3_IR
    #define SVEN_module_CE31XX_DFX          SVEN_module_GEN3_DFX
    #define SVEN_module_CE31XX_GBE          SVEN_module_GEN3_GBE
    #define SVEN_module_CE31XX_GBE_MDIO     SVEN_module_GEN3_GBE_MDIO
    #define SVEN_module_CE31XX_CRU          SVEN_module_GEN3_CRU
    #define SVEN_module_CE31XX_USB          SVEN_module_GEN3_USB
    #define SVEN_module_CE31XX_SATA         SVEN_module_GEN3_SATA
    #define SVEN_module_CE31XX_TVE          SVEN_module_GEN3_TVE

#endif

#endif /* SVEN_MODULE_H */
