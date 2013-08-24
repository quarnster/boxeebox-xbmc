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

#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecSMD.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "IntelSMDGlobals.h"

#define __MODULE_NAME__ "DVDVideoCodecSMD"

CDVDVideoCodecSMD::CDVDVideoCodecSMD() :
m_Device(NULL),
m_DecodeStarted(false),
m_DropPictures(false),
m_Duration(0.0),
m_pFormatName("")
{
}

CDVDVideoCodecSMD::~CDVDVideoCodecSMD()
{
  Dispose();
}

bool CDVDVideoCodecSMD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  ismd_codec_type_t codec_type;

  // found out if video hardware decoding is enforced
  printf("CDVDVideoCodecSMD::Open force hardware %d\n", g_advancedSettings.m_bForceVideoHardwareDecoding);
  printf("CDVDVideoCodecSMD::Open width %d  height %d type %d\n", hints.width, hints.height, hints.codec);

  // Run some SD content in software mode
  // since hardware is not always compatible
  if(hints.width * hints.height <= 720 * 576 && hints.width * hints.height > 0 &&
      !g_advancedSettings.m_bForceVideoHardwareDecoding &&
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


  // default duration to 23.976 fps, have to guess something.
  m_Duration = (DVD_TIME_BASE / (24.0 * 1000.0/1001.0));
  m_DropPictures = false;
  m_DecodeStarted = false;
  m_bIsDirectRendering = true;

  CLog::Log(LOGINFO, "Opened Intel SMD Codec, %s", m_pFormatName);

  return true;
}

void CDVDVideoCodecSMD::Dispose(void)
{
  if (m_Device)
  {
    m_Device->CloseDecoder();
    m_Device = NULL;
  }
}

void CDVDVideoCodecSMD::SetSpeed(int speed)
{
  if(speed == DVD_PLAYSPEED_PAUSE)
    m_Device->Pause();
  else
    m_Device->Resume();
}

int CDVDVideoCodecSMD::Decode(BYTE *pData, int iSize, double pts, double dts)
{
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

  // We don't want to drain the demuxer too fast
  unsigned int curDepth = 0, maxDepth = 0;
  m_Device->GetInputPortStatus(curDepth, maxDepth);

  int sleepLen = (int)(((float)curDepth / maxDepth) * 100);

  if(ret)
  {
    Sleep(sleepLen);
    ret = VC_PICTURE | VC_BUFFER;
  }
  else
    ret = VC_BUFFER;

  return ret;
}

void CDVDVideoCodecSMD::Reset(void)
{
  // Decoder flush, reset started flag and dump all input and output.
  if(m_Device)
  {
    m_DecodeStarted = false;
    m_Device->Reset();
  }
}

void CDVDVideoCodecSMD::Resync(double pts)
{
  if(m_Device)
  {
    m_Device->Resync(pts);
  }
}

bool CDVDVideoCodecSMD::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  bool  ret;

  ret = m_Device->GetPicture(pDvdVideoPicture);
  m_Duration = 0.0;//pDvdVideoPicture->iDuration;
  pDvdVideoPicture->iDuration = 0;

  return ret;
}

bool CDVDVideoCodecSMD::GetUserData(DVDVideoUserData* pDvdVideoUserData)
{
  ismd_result_t ismd_ret;
  ismd_port_handle_t user_data_port;
  ismd_event_t user_data_event;
  ismd_buffer_handle_t buffer;
  ismd_buffer_descriptor_t ismd_buf_desc;
  uint8_t *buf_ptr;

  pDvdVideoUserData->data = NULL;
  pDvdVideoUserData->size = 0;

  user_data_port = g_IntelSMDGlobals.GetVideDecUserDataPort();
  if(user_data_port == -1)
    return false;

  user_data_event = g_IntelSMDGlobals.GetVideoDecUserEvent();
  if(user_data_event == -1)
      return false;

  ismd_ret = ismd_event_wait(user_data_event, 5000); //only wait 5 seconds so there is a way out of the loop
        if (ismd_ret == ISMD_ERROR_TIMEOUT) {
            printf("Time out for user data\n");
            return false;
        } else if (ismd_ret != ISMD_SUCCESS) {
           printf("got unexpected error waiting for user data\n");
           return false;
        }

  ismd_ret = ismd_port_read(user_data_port, &buffer);
  if (ismd_ret != ISMD_SUCCESS)
  {
    //CLog::Log(LOGERROR, "CDVDVideoCodecSMD::GetUserData ismd_port_read failed. %d", ismd_ret);
    return false;
  }

  printf("#@#@# Data found on user data\n");

  ismd_ret = ismd_buffer_read_desc(buffer, &ismd_buf_desc);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecSMD::GetUserData ismd_buffer_read_desc failed. %d", ismd_ret);
    return false;
  }

  printf("successfully read buffer, data level: %d\n", ismd_buf_desc.phys.level);

  buf_ptr = (uint8_t *)OS_MAP_IO_TO_MEM_NOCACHE(ismd_buf_desc.phys.base,ismd_buf_desc.phys.size);

  //here is the spot to do stuff with the data

  OS_UNMAP_IO_FROM_MEM(buf_ptr, ismd_buf_desc.phys.size);

  //need to dereference this buffer as SMD driver created reference when it made the buffer so it can be freed
  ismd_ret = ismd_buffer_dereference(buffer);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecSMD::GetUserData ismd_buffer_dereference failed. %d", ismd_ret);
    return false;
  }

  Sleep(1);

  return false;
}

void CDVDVideoCodecSMD::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
  m_Device->SetDropState(m_DropPictures);
}

void CDVDVideoCodecSMD::DisablePtsCorrection(bool bDisable)
{
  if(m_Device)
    m_Device->DisablePtsCorrection(bDisable);
}

#endif
