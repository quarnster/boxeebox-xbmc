/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeBoxee.h"
#include "guilib/gui3d.h"
#include "utils/log.h"

CEGLNativeTypeBoxee::CEGLNativeTypeBoxee()
{
}

CEGLNativeTypeBoxee::~CEGLNativeTypeBoxee()
{
} 

bool CEGLNativeTypeBoxee::CheckCompatibility()
{
#if defined(TARGET_BOXEE)
  return true;
#endif
  return false;
}

void CEGLNativeTypeBoxee::Initialize()
{
  return;
}
void CEGLNativeTypeBoxee::Destroy()
{
  return;
}

bool CEGLNativeTypeBoxee::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeBoxee::CreateNativeWindow()
{
#if defined(TARGET_BOXEE)
  m_nativeWindow = (void*)0x6;
  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeBoxee::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeBoxee::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow || !m_nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}

bool CEGLNativeTypeBoxee::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeBoxee::DestroyNativeWindow()
{
  m_nativeWindow = NULL;
  return true;
}

bool CEGLNativeTypeBoxee::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(TARGET_BOXEE)
  res->iWidth = 1280;
  res->iHeight= 720;

  res->fRefreshRate = 60;
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeBoxee::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeBoxee::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  RESOLUTION_INFO res;
  bool ret = false;
  ret = GetNativeResolution(&res);
  if (ret && res.iWidth > 1 && res.iHeight > 1)
  {
    resolutions.push_back(res);
    return true;
  }
  return false;
}

bool CEGLNativeTypeBoxee::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeBoxee::ShowWindow(bool show)
{
  return false;
}
