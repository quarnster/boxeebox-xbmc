/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"

#if defined(HAS_INTEL_SMD)

#include "settings/AdvancedSettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecSMD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "../../../IntelSMDGlobals.h"

#define __MODULE_NAME__ "DVDVideoCodecSMD"

#if 0
#define VERBOSE() CLog::Log(LOGDEBUG, "%s", __DEBUG_ID__)
#else
#define VERBOSE()
#endif


CDVDVideoCodecSMD::CDVDVideoCodecSMD() :
m_Device(NULL),
m_pFormatName("SMD: codec not configured")
{
}

CDVDVideoCodecSMD::~CDVDVideoCodecSMD()
{
  Dispose();
}

bool CDVDVideoCodecSMD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  VERBOSE();
  ismd_codec_type_t codec_type;

  // found out if video hardware decoding is enforced
  CLog::Log(LOGDEBUG, "%s force hardware %d\n", __DEBUG_ID__, !hints.software);
  CLog::Log(LOGDEBUG, "%s width %d  height %d type %d\n", __DEBUG_ID__, hints.width, hints.height, hints.codec);

  // Run some SD content in software mode
  // since hardware is not always compatible
  if(hints.width * hints.height <= 720 * 576 && hints.width * hints.height > 0 &&
      hints.software &&
      (hints.codec == CODEC_ID_MPEG4))
  {
    return false;
  }

  switch (hints.codec)
  {
  case CODEC_ID_MPEG1VIDEO:
    codec_type = ISMD_CODEC_TYPE_MPEG2;
    m_pFormatName = "SMD-mpeg1";
    break;
  case CODEC_ID_MPEG2VIDEO:
    codec_type = ISMD_CODEC_TYPE_MPEG2;
    m_pFormatName = "SMD-mpeg2";
    break;
  case CODEC_ID_H264:
    codec_type = ISMD_CODEC_TYPE_H264;
    m_pFormatName = "SMD-h264";
    break;
  case CODEC_ID_VC1:
    codec_type = ISMD_CODEC_TYPE_VC1;
    m_pFormatName = "SMD-vc1";
    break;
  case CODEC_ID_WMV3:
    codec_type = ISMD_CODEC_TYPE_VC1;
    m_pFormatName = "SMD-wmv3";
    break;
  case CODEC_ID_MPEG4:
    codec_type = ISMD_CODEC_TYPE_MPEG4;
    m_pFormatName = "SMD-mpeg4";
    break;
  default:
    return false;
    break;
  }

  m_Device = CIntelSMDVideo::GetInstance();

  if (!m_Device)
  {
    CLog::Log(LOGERROR, "%s: Failed to open Intel SMD device", __DEBUG_ID__);
    return false;
  }

  m_Device->SetWidth(hints.width);
  m_Device->SetHeight(hints.height);

  if (m_Device && !m_Device->OpenDecoder(hints.codec, codec_type, hints.extrasize, hints.extradata))
  {
    CLog::Log(LOGERROR, "%s: Failed to open Intel SMD decoder", __DEBUG_ID__);
    return false;
  }

  CLog::Log(LOGINFO, "%s: Opened Intel SMD Codec, %s", __DEBUG_ID__, m_pFormatName);

  return true;
}

void CDVDVideoCodecSMD::Dispose(void)
{
  VERBOSE();
  if (m_Device)
  {
    m_Device->CloseDecoder();
    m_Device = NULL;
  }
}

int CDVDVideoCodecSMD::Decode(BYTE *pData, int iSize, double dts, double pts)
{
  VERBOSE();
  int ret = 0;

  if(!pData)
  {
    return ret;
  }

  if (pData)
  {
    return m_Device->AddInput(pData, iSize, dts, pts);
  }

  return VC_BUFFER;
}

void CDVDVideoCodecSMD::Reset(void)
{
  VERBOSE();
  // Decoder flush, reset started flag and dump all input and output.
  if(m_Device)
  {
    m_Device->Reset();
  }
}

bool CDVDVideoCodecSMD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  VERBOSE();
  bool  ret;

  ret = m_Device->GetPicture(pDvdVideoPicture);
  return ret;
}

bool CDVDVideoCodecSMD::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  VERBOSE();
  if (m_Device->ClearPicture(pDvdVideoPicture))
  {
    return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
  }
  return false;
}

void CDVDVideoCodecSMD::SetSpeed(int iSpeed)
{
  switch (iSpeed)
  {
    case DVD_PLAYSPEED_PAUSE:
      g_IntelSMDGlobals.SetVideoDecoderState(ISMD_DEV_STATE_PAUSE);
      g_IntelSMDGlobals.SetVideoRenderState(ISMD_DEV_STATE_PAUSE);
      break;
  }
}

#endif
