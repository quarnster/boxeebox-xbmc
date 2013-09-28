#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef USE_VCHIQ_ARM
#define USE_VCHIQ_ARM
#endif
#ifndef __VIDEOCORE4__
#define __VIDEOCORE4__
#endif
#ifndef HAVE_VMCS_CONFIG
#define HAVE_VMCS_CONFIG
#endif

#if defined(HAVE_CONFIG_H) && !defined(TARGET_WINDOWS)
#include "config.h"
#define DECLARE_UNUSED(a,b) a __attribute__((unused)) b;
#endif

#if defined(TARGET_RASPBERRY_PI)
#include "DllBCM.h"
#include "OMXCore.h"

class CRBP
{
public:
  CRBP();
  ~CRBP();

  bool Initialize();
  void LogFirmwareVerison();
  void Deinitialize();
  int GetArmMem() { return m_arm_mem; }
  int GetGpuMem() { return m_gpu_mem; }
  void GetDisplaySize(int &width, int &height);
  // stride can be null for packed output
  unsigned char *CaptureDisplay(int width, int height, int *stride, bool swap_red_blue, bool video_only = true);

private:
  DllBcmHost *m_DllBcmHost;
  bool       m_initialized;
  bool       m_omx_initialized;
  int        m_arm_mem;
  int        m_gpu_mem;
  COMXCore   *m_OMX;
};

extern CRBP g_RBP;
#endif
