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
#include "cores/IntelSMDGlobals.h"
#include "utils/log.h"
#include <math.h>
#include "utils/StringUtils.h"


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
  gdl_init();
  int enable = 1;

  gdl_rectangle_t srcRect, dstRect;
  gdl_display_info_t   display_info;
  gdl_get_display_info(GDL_DISPLAY_ID_0, &display_info);

  dstRect.origin.x = 0;
  dstRect.origin.y = 0;
  dstRect.width = display_info.tvmode.width;
  dstRect.height = display_info.tvmode.height;

  srcRect.origin.x = 0;
  srcRect.origin.y = 0;
  srcRect.width = display_info.tvmode.width;
  srcRect.height = display_info.tvmode.height;


  gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_POWER, &enable);
  gdl_plane_reset(GDL_GRAPHICS_PLANE);
  gdl_plane_config_begin(GDL_GRAPHICS_PLANE);
  gdl_plane_set_uint(GDL_PLANE_SRC_COLOR_SPACE, GDL_COLOR_SPACE_RGB);
  gdl_plane_set_uint(GDL_PLANE_PIXEL_FORMAT, GDL_PF_ARGB_32);
  gdl_plane_set_rect(GDL_PLANE_DST_RECT, &dstRect);
  gdl_plane_set_rect(GDL_PLANE_SRC_RECT, &srcRect);
  gdl_plane_config_end(GDL_FALSE);
  return;
}
void CEGLNativeTypeBoxee::Destroy()
{
  gdl_close();
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
  m_nativeWindow = (void*)GDL_GRAPHICS_PLANE;
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

static void tvmode_to_RESOLUTION_INFO(gdl_tvmode_t tvmode, RESOLUTION_INFO *res)
{
  res->iWidth = tvmode.width;
  res->iHeight= tvmode.height;

  switch (tvmode.refresh)
  {
    case GDL_REFRESH_23_98: res->fRefreshRate = 23.98; break;
    case GDL_REFRESH_24:    res->fRefreshRate = 24;    break;
    case GDL_REFRESH_25:    res->fRefreshRate = 25;    break;
    case GDL_REFRESH_29_97: res->fRefreshRate = 29.97; break;
    case GDL_REFRESH_30:    res->fRefreshRate = 30;    break;
    case GDL_REFRESH_50:    res->fRefreshRate = 50;    break;
    case GDL_REFRESH_59_94: res->fRefreshRate = 59.94; break;
    case GDL_REFRESH_60:    res->fRefreshRate = 60;    break;
    case GDL_REFRESH_48:    res->fRefreshRate = 48;    break;
    case GDL_REFRESH_47_96: res->fRefreshRate = 47.96; break;
    default:                res->fRefreshRate = 0;     break;
  }

  if (tvmode.interlaced)
  {
    res->dwFlags = D3DPRESENTFLAG_INTERLACED;
  }
  else
  {
    res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  }
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode       = StringUtils::Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
  res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
}

static void RESOLUTION_INFO_to_tvmode(const RESOLUTION_INFO &res, gdl_tvmode_t *tvmode)
{
  tvmode->width  = res.iWidth;
  tvmode->height = res.iHeight;

  const gdl_refresh_t lut[] =  {
      GDL_REFRESH_23_98,
      GDL_REFRESH_24,
      GDL_REFRESH_25,
      GDL_REFRESH_29_97,
      GDL_REFRESH_30,
      GDL_REFRESH_50,
      GDL_REFRESH_59_94,
      GDL_REFRESH_60,
      GDL_REFRESH_48,
      GDL_REFRESH_47_96
  };
  const float vals[] = {23.98, 24, 25, 29.97, 30, 50, 59.94, 60, 48, 47.96};
  float best = 1000;
  int bi = 0;
  for (int i = 0; i < 10; i++)
  {
    float curr = fabs(res.fRefreshRate-vals[i]);
    if (curr < best)
    {
      best = curr;
      bi = i;
    }
  }
  tvmode->refresh = lut[bi];
  tvmode->interlaced = (gdl_boolean_t) (res.dwFlags == D3DPRESENTFLAG_INTERLACED);
}

bool CEGLNativeTypeBoxee::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(TARGET_BOXEE)
  gdl_display_info_t   display_info;
  gdl_get_display_info(GDL_DISPLAY_ID_0, &display_info);
  tvmode_to_RESOLUTION_INFO(display_info.tvmode, res);
  CLog::Log(LOGNOTICE,"Current resolution: %s\n",res->strMode.c_str());
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeBoxee::SetNativeResolution(const RESOLUTION_INFO &res)
{
  CLog::Log(LOGNOTICE,"Setting resolution: %s\n",res.strMode.c_str());


  gdl_pixel_format_t pixelFormat = GDL_PF_ARGB_32;
  gdl_color_space_t colorSpace = GDL_COLOR_SPACE_RGB;
  gdl_rectangle_t srcRect;
  gdl_rectangle_t dstRect;
  gdl_ret_t rc = GDL_SUCCESS;
  gdl_boolean_t hdmiEnabled = GDL_FALSE;

  gdl_plane_id_t m_gdlPlane = GDL_GRAPHICS_PLANE;

  gdl_display_info_t   display_info;
  memset(&display_info, 0, sizeof(display_info));
  RESOLUTION_INFO_to_tvmode(res, &display_info.tvmode);
  display_info.id          = GDL_DISPLAY_ID_0;
  display_info.flags       = 0;
  display_info.bg_color    = 0;
  display_info.color_space = GDL_COLOR_SPACE_RGB;
  display_info.gamma       = GDL_GAMMA_LINEAR;

  rc = gdl_set_display_info(&display_info);

  if ( rc != GDL_SUCCESS)
    {
      CLog::Log(LOGERROR, "Could not set display mode for display 0");
      return false;
    }

  // Setup composite output to NTSC. In order to support PAL we need to use 720x576i50.
  display_info.id           = GDL_DISPLAY_ID_1;
  display_info.flags        = 0;
  display_info.bg_color     = 0;
  display_info.color_space  = GDL_COLOR_SPACE_RGB;
  display_info.gamma        = GDL_GAMMA_LINEAR;
  display_info.tvmode.width      = 720;
  display_info.tvmode.height     = 480;
  display_info.tvmode.refresh    = GDL_REFRESH_59_94;
  display_info.tvmode.interlaced = GDL_TRUE;

  rc = gdl_set_display_info(&display_info);

  dstRect.origin.x = 0;
  dstRect.origin.y = 0;
  dstRect.width = res.iWidth;
  dstRect.height = res.iHeight;

  srcRect.origin.x = 0;
  srcRect.origin.y = 0;
  srcRect.width = res.iWidth;
  srcRect.height = res.iHeight;

  if (gdl_port_set_attr(GDL_PD_ID_HDMI, GDL_PD_ATTR_ID_HDCP, &hdmiEnabled) != GDL_SUCCESS)
    {
      CLog::Log(LOGWARNING, "Could not disable HDCP");
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_config_begin(m_gdlPlane);
    }

  rc = gdl_plane_reset(m_gdlPlane);
  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_config_begin(m_gdlPlane);
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_set_attr(GDL_PLANE_SRC_COLOR_SPACE, &colorSpace);
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_set_attr(GDL_PLANE_PIXEL_FORMAT, &pixelFormat);
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_set_attr(GDL_PLANE_DST_RECT, &dstRect);
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_set_attr(GDL_PLANE_SRC_RECT, &srcRect);
    }

  if(GDL_SUCCESS == rc)
    {
      gdl_boolean_t scalineEnabled = GDL_FALSE;
      rc = gdl_plane_set_attr(GDL_PLANE_UPSCALE, &scalineEnabled);
    }

  if (GDL_SUCCESS == rc)
    {
      rc = gdl_plane_config_end(GDL_FALSE);
    }
  else
    {
      gdl_plane_config_end(GDL_TRUE);
    }

  if (GDL_SUCCESS != rc)
    {
      CLog::Log(LOGERROR, "GDL configuration failed! GDL error code is 0x%x\n", rc);
      return false;
    }

  CLog::Log(LOGINFO, "GDL plane setup complete");


  return true;
}

bool CEGLNativeTypeBoxee::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  for (int i = 0; true; i++)
  {
    RESOLUTION_INFO res;
    gdl_tvmode_t tvmode;
    if (gdl_get_port_tvmode_by_index(GDL_PD_ID_HDMI, i, &tvmode) != GDL_SUCCESS)
    {
      break;
    }
    tvmode_to_RESOLUTION_INFO(tvmode, &res);
    printf("Available: %s\n", res.strMode.c_str());
    resolutions.push_back(res);
  }
  return !resolutions.empty();
}

bool CEGLNativeTypeBoxee::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeBoxee::ShowWindow(bool show)
{
  return false;
}
