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
#define VERBOSE() CLog::Log(LOGDEBUG, "%s::%s", __MODULE_NAME__, __FUNCTION__)
#else
#define VERBOSE()
#endif


CDVDVideoCodecSMD::CDVDVideoCodecSMD() :
m_Device(NULL),
m_DecodeStarted(false),
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
  CLog::Log(LOGDEBUG, "CDVDVideoCodecSMD::Open force hardware %d\n", !hints.software);
  CLog::Log(LOGDEBUG, "CDVDVideoCodecSMD::Open width %d  height %d type %d\n", hints.width, hints.height, hints.codec);

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
    CLog::Log(LOGERROR, "%s: Failed to open Intel SMD device", __MODULE_NAME__);
    return false;
  }

  m_Device->SetWidth(hints.width);
  m_Device->SetHeight(hints.height);

  if (m_Device && !m_Device->OpenDecoder(hints.codec, codec_type, hints.extrasize, hints.extradata))
  {
    CLog::Log(LOGERROR, "%s: Failed to open Intel SMD decoder", __MODULE_NAME__);
    return false;
  }

  m_DecodeStarted = false;

  CLog::Log(LOGINFO, "Opened Intel SMD Codec, %s", m_pFormatName);

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

  double new_pts = pts;
  if(new_pts == DVD_NOPTS_VALUE && dts != DVD_NOPTS_VALUE)
    new_pts = dts;

  if (pData)
  {
    ret = m_Device->AddInput(pData, iSize, 0, new_pts);
  }

  if(!ret)
    return VC_ERROR;

  // We don't want to drain the demuxer too fast.
  // quarnster: This is likely to force a more realtime
  //            feel for pause/resume as the queue will
  //            never be too long.
  //            It could potentially cause unwanted
  //            buffer underruns, no? TODO(q)
  unsigned int curDepth = 0, maxDepth = 0;
  m_Device->GetInputPortStatus(curDepth, maxDepth);

  int sleepLen = (int)(((float)curDepth / maxDepth) * 10000);

  if(ret)
  {
    usleep(sleepLen);
    ret = VC_PICTURE | VC_BUFFER;
  }
  else
    ret = VC_BUFFER;

  return ret;
}

void CDVDVideoCodecSMD::Reset(void)
{
  VERBOSE();
  // Decoder flush, reset started flag and dump all input and output.
  if(m_Device)
  {
    m_DecodeStarted = false;
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

#endif
