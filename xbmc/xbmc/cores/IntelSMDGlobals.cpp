#include "system.h"

#ifdef HAS_INTEL_SMD

/*
 * Boxee
 * Copyright (c) 2009, Boxee Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "xbmc/settings/Settings.h"
#include "xbmc/settings/GUISettings.h" // for AUDIO_*
#include "IntelSMDGlobals.h"
#include <ismd_vidpproc.h>
#include "utils/log.h"
#include "../threads/SingleLock.h"

#ifndef UINT64_C
#define UINT64_C(x) (const unsigned long long)(x)
#endif

extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/avcodec.h>
  #elif (defined HAVE_FFMPEG_AVCODEC_H)
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "libavcodec/avcodec.h"
#endif
}

#define __MODULE_NAME__ "IntelSMDGlobals"

#if 0
#define VERBOSE() CLog::Log(LOGDEBUG, "%s::%s", __MODULE_NAME__, __FUNCTION__)
#else
#define VERBOSE()
#endif

#define AUDIO_OUTPUT_DELAY 39 // requested by dolby cert

CIntelSMDGlobals g_IntelSMDGlobals;

CIntelSMDGlobals::CIntelSMDGlobals()
{
  VERBOSE();

  m_main_clock = -1;
  m_audioProcessor = -1;
  m_viddec = -1;
  m_viddec_input_port = -1;
  m_viddec_output_port = -1;
  m_video_proc = -1;
  m_video_render = -1;
  m_video_input_port_proc = -1;
  m_video_input_port_renderer = -1;
  m_video_output_port_renderer = -1;
  m_video_output_port_proc = -1;
  m_video_codec = ISMD_CODEC_TYPE_INVALID;

  m_base_time = 0;
  m_pause_base_time = 0;
  m_RenderState = ISMD_DEV_STATE_INVALID;
  m_audio_start_pts = ISMD_NO_PTS;
  m_video_start_pts = ISMD_NO_PTS;

  m_bFlushFlag = false;

  m_audioOutputHDMI = -1;
  m_audioOutputSPDIF = -1;
  m_audioOutputI2S0 = -1;

  m_primaryAudioInput = -1;

  ResetClock();
  CreateMainClock();
}

CIntelSMDGlobals::~CIntelSMDGlobals()
{
  VERBOSE();

  DestroyMainClock();
}

bool CIntelSMDGlobals::CreateMainClock()
{
  VERBOSE();

  ismd_result_t ret;

  CSingleLock lock(m_Lock);

  if (m_main_clock != -1)
    return true;

  ret = ismd_clock_alloc(ISMD_CLOCK_TYPE_FIXED, &m_main_clock);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "ERROR: CIntelSMDGlobals::CreateMainClock CLOCK ALLOC FAIL: %d", ret);
    return false;
  }

  if (!SetClockPrimary())
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::CreateMainClock SetClockPrimary failed");
    return false;
  }

  return true;
}

void CIntelSMDGlobals::DestroyMainClock()
{
  VERBOSE();

  CSingleLock lock(m_Lock);

  if (m_main_clock != -1)
    ismd_clock_free(m_main_clock);

  m_main_clock = -1;
}

bool CIntelSMDGlobals::SetClockPrimary()
{
  VERBOSE();

  ismd_result_t ret;

  if (m_main_clock == -1)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SetClockPrimary  m_main_clock == -1");
    return false;
  }

  ret = ismd_clock_make_primary(m_main_clock);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(
        LOGERROR,
        "ERROR: CIntelSMDGlobals::SetClockPrimary ismd_clock_make_primary failed %d",
        ret);
    return false;
  }

  return true;
}

ismd_time_t CIntelSMDGlobals::GetCurrentTime()
{
  ismd_result_t ret;
  ismd_time_t current;

  CSingleLock lock(m_Lock);

  if (m_main_clock == -1)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::GetCurrentTime main clock = -1");
    return 0;
  }

  ret = ismd_clock_get_time(m_main_clock, &current);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::GetCurrentTime ismd_clock_get_time failed %d",
        ret);
    return 0;
  }

  return current;
}

void CIntelSMDGlobals::SetBaseTime(ismd_time_t time)
{
  CLog::Log(LOGINFO, "Setting base time %d ms", DVD_TIME_TO_MSEC(IsmdToDvdPts(time)));
  // TODO(q): why + 45000??
  m_base_time = time + 45000; 
}

bool CIntelSMDGlobals::SetCurrentTime(ismd_time_t time)
{
  VERBOSE();
  ismd_result_t ret;

  CSingleLock lock(m_Lock);


  if (m_main_clock == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::SetCurrentTime main clock = -1");
    return false;
  }

  ret = ismd_clock_set_time(m_main_clock, time);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SetCurrentTime ismd_clock_set_time failed %d", ret);
    return false;
  }

  return true;
}

void CIntelSMDGlobals::PauseClock()
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  m_pause_base_time = GetBaseTime();
  m_pause_cur_time = GetCurrentTime();
}

void CIntelSMDGlobals::ResumeClock()
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  if (m_pause_base_time == 0)
  {
    if (m_base_time != 0)
      m_pause_base_time = GetBaseTime();
    else
      m_pause_base_time = GetCurrentTime();
  }
  if (m_pause_cur_time == 0)
    m_pause_cur_time = GetCurrentTime();

  ismd_time_t offset = GetCurrentTime() - m_pause_cur_time;
  m_base_time = m_pause_base_time + offset;

  m_pause_base_time = 0;
  m_pause_cur_time = 0;
}

void CIntelSMDGlobals::ResetClock()
{
  VERBOSE();

  m_base_time = 0;
  m_pause_base_time = 0;
  m_pause_cur_time = 0;
  m_audio_start_pts = ISMD_NO_PTS;
  m_video_start_pts = ISMD_NO_PTS;
  m_bFlushFlag = false;
}

ismd_pts_t CIntelSMDGlobals::DvdToIsmdPts(double pts)
{
  ismd_pts_t ismd_pts = ISMD_NO_PTS;

  if (pts != DVD_NOPTS_VALUE
    )
    ismd_pts = (ismd_pts_t) (pts / DVD_TIME_BASE * SMD_CLOCK_FREQ);

  return ismd_pts;
}

double CIntelSMDGlobals::IsmdToDvdPts(ismd_pts_t pts)
{
  double dvd_pts = DVD_NOPTS_VALUE;

  if (pts != ISMD_NO_PTS)
    dvd_pts = (double) ((double) pts / SMD_CLOCK_FREQ * DVD_TIME_BASE);

  return dvd_pts;
}

bool CIntelSMDGlobals::InitAudio()
{
  VERBOSE();
  ismd_result_t result;


  if(!CreateAudioProcessor())
   return false;

  // Configure audio outputs according to settings
  BuildAudioOutputs();

  return true;
}

bool  CIntelSMDGlobals::BuildAudioOutputs()
{
  ismd_result_t result;

  int audioOutputMode = g_guiSettings.GetInt("audiooutput.mode");

  bool bIsHDMI = true; //TODO(q) (AUDIO_HDMI == audioOutputMode);
  bool bIsSPDIF = true; //TODO(q) (AUDIO_IEC958 == audioOutputMode);
  bool bIsAnalog = true; //TODO(q) (AUDIO_ANALOG == audioOutputMode);
  // bool bIsAllOutputs = (audioOutputMode == AUDIO_ALL_OUTPUTS);
  // if (bIsAllOutputs)
  //   bIsHDMI = bIsSPDIF = bIsAnalog = true;

  CLog::Log(LOGINFO, "CIntelSMDGlobals::BuildAudioOutputs: HDMI %d SPDIF %d Analog %d", bIsHDMI, bIsSPDIF, bIsAnalog);

  RemoveAllAudioOutput();

  if (bIsHDMI)
  {
    m_audioOutputHDMI = AddAudioOutput(AUDIO_HDMI);
    EnableAudioOutput(m_audioOutputHDMI);
  }
  if (bIsSPDIF)
  {
    m_audioOutputSPDIF = AddAudioOutput(AUDIO_IEC958);
    EnableAudioOutput(m_audioOutputSPDIF);
  }
  if (bIsAnalog)
  {
    m_audioOutputI2S0 = AddAudioOutput(AUDIO_ANALOG);
    EnableAudioOutput(m_audioOutputI2S0);
  }
}

ismd_audio_output_t CIntelSMDGlobals::AddAudioOutput(int output)
{
  VERBOSE();
  ismd_result_t result;
  int hwId = 0;
  CStdString name;

  ismd_audio_output_t audio_output = -1;

  // Use defaults
  ismd_audio_output_config_t output_config;
  output_config.stream_delay = 0;
  output_config.sample_size = 16;
  output_config.ch_config = ISMD_AUDIO_STEREO;
  output_config.out_mode = ISMD_AUDIO_OUTPUT_PCM;
  output_config.ch_map = 0;
  output_config.sample_rate = 48000;

  ismd_dev_t dev;

  switch(output)
  {
  case AUDIO_HDMI:
      dev = m_audioOutputHDMI;
      hwId = GEN3_HW_OUTPUT_HDMI;
      name = "HDMI";
      break;
  case AUDIO_IEC958:
    dev = m_audioOutputSPDIF;
    hwId = GEN3_HW_OUTPUT_SPDIF;
    name = "SPDIF";
    break;
  case AUDIO_ANALOG:
    dev = m_audioOutputI2S0;
    hwId = GEN3_HW_OUTPUT_I2S0;
    name = "I2S0";
    break;
  default:
    CLog::Log(LOGERROR, "CIntelSMDGlobals::AddAudioOutput - Unkown output");
    return -1;
  }

  result = ismd_audio_add_phys_output(m_audioProcessor, hwId, output_config, &audio_output);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::AddAudioOutput - error add output %s %d, returning existing: %d", name.c_str(), result, dev);
    return dev;
  }

  if(g_guiSettings.GetBool("audiooutput.enable_audio_output_delay"))
  {
    result = ismd_audio_output_set_delay(m_audioProcessor, hwId, AUDIO_OUTPUT_DELAY);
    if (result != ISMD_SUCCESS)
      CLog::Log(LOGWARNING, "CIntelSMDGlobals::AddAudioOutput - ismd_audio_output_set_delay %s %d failed %d", name.c_str(), AUDIO_OUTPUT_DELAY, result);

    CLog::Log(LOGINFO, "CIntelSMDGlobals::AddAudioOutput - Output Added %s", name.c_str());
  }

  CLog::Log(LOGINFO, "CIntelSMDGlobals::AddAudioOutput %s", name.c_str());

  return audio_output;
}


bool CIntelSMDGlobals::RemoveAllAudioOutput()
{
  VERBOSE();

  if (m_audioOutputHDMI != -1)
  {
    DisableAudioOutput(m_audioOutputHDMI);
    RemoveAudioOutput(m_audioOutputHDMI);
    m_audioOutputHDMI = -1;
  }

  if (m_audioOutputSPDIF != -1)
  {
    DisableAudioOutput(m_audioOutputSPDIF);
    RemoveAudioOutput(m_audioOutputSPDIF);
    m_audioOutputSPDIF = -1;
  }

  if (m_audioOutputI2S0 != -1)
  {
    DisableAudioOutput(m_audioOutputI2S0);
    RemoveAudioOutput(m_audioOutputI2S0);
    m_audioOutputI2S0 = -1;
  }

  return true;
}

bool CIntelSMDGlobals::RemoveAudioOutput(ismd_audio_output_t output)
{
  VERBOSE();
  ismd_result_t result;
  CStdString name;

  if(output == m_audioOutputHDMI)
    name = "HDMI";
  else if(output == m_audioOutputSPDIF)
    name = "SPDIF";
  else if(output == m_audioOutputI2S0)
    name = "I2S0";
  else
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::RemoveAudioOutput - Unknown output");
    return false;
  }

  result = ismd_audio_remove_output(m_audioProcessor, output);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::RemoveAudioOutput - error remove output %s %d",name.c_str(), result);
    return false;
  }

  CLog::Log(LOGINFO, "CIntelSMDGlobals::RemoveAudioOutput %s", name.c_str());

  return true;
}

bool CIntelSMDGlobals::DeInitAudio()
{
  VERBOSE();

  RemoveAllAudioOutput();

  DeleteAudioProcessor();

  return true;
}

ismd_port_handle_t CIntelSMDGlobals::GetAudioDevicePort(ismd_dev_t device)
{
  VERBOSE();
  ismd_result_t result;
  ismd_port_handle_t  input_port;

  result = ismd_audio_input_get_port(m_audioProcessor, device, &input_port);
  if(result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetAudioDevicePort failed to get audio input port %d", result);
    return -1;
  }

  return input_port;
}

ismd_dev_t CIntelSMDGlobals::CreateAudioInput(bool timed)
{
  VERBOSE();
  ismd_dev_t          device;
  ismd_port_handle_t  input_port;
  ismd_result_t result;

  // Create audio inputs

  result = ismd_audio_add_input_port(m_audioProcessor, timed, &device, &input_port);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::CreateAudioInput failed (timed %d) %d", timed, result);
    return -1;
  }

  result = ismd_dev_set_clock(device, m_main_clock);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::CreateAudioInput ismd_dev_set_clock failed (timed %d) %d", timed, result);
    return -1;
  }

  return device;
}


bool CIntelSMDGlobals::RemoveAudioInput(ismd_dev_t device)
{
  VERBOSE();
  ismd_result_t result;

  SetAudioDeviceState(ISMD_DEV_STATE_STOP, device);
  FlushAudioDevice(device);

  result = ismd_audio_remove_input(m_audioProcessor, device);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::RemoveAudioInput - error closing device: %d", result);
      return false;
  }

  return true;
}

bool CIntelSMDGlobals::SetAudioStartPts(ismd_pts_t pts)
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  if (pts == ISMD_NO_PTS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::SetAudioStartPts got ISMD_NO_PTS");
    return false;
  }

  if (pts < 0 && pts != ISMD_NO_PTS)
    pts = 0;

  CLog::Log(LOGINFO, "CIntelSMDGlobals::SetAudioStartPts %d ms ", DVD_TIME_TO_MSEC(IsmdToDvdPts(pts)));

  m_audio_start_pts = pts;

  return true;
}

bool CIntelSMDGlobals::SetVideoStartPts(ismd_pts_t pts)
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  if (pts == ISMD_NO_PTS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::SetVideoStartPts got ISMD_NO_PTS");
    return false;
  }

  if (pts < 0 && pts != ISMD_NO_PTS)
    pts = 0;

  CLog::Log(LOGINFO, "CIntelSMDGlobals::SetVideoStartPts %d ms", DVD_TIME_TO_MSEC(IsmdToDvdPts(pts)));

  m_video_start_pts = pts;

  return true;
}

ismd_pts_t CIntelSMDGlobals::GetAudioCurrentTime()
{
  VERBOSE();
  ismd_pts_t start = GetAudioStartPts();

  if (start == ISMD_NO_PTS || (GetCurrentTime() < GetBaseTime()))
    return ISMD_NO_PTS;

  return start + GetCurrentTime() - GetBaseTime();
}

ismd_pts_t CIntelSMDGlobals::GetVideoCurrentTime()
{
  VERBOSE();
  ismd_pts_t start = GetVideoStartPts();

  if (start == ISMD_NO_PTS || (GetCurrentTime() < GetBaseTime()))
    return ISMD_NO_PTS;

  return start + GetCurrentTime() - GetBaseTime();
}

ismd_pts_t CIntelSMDGlobals::GetAudioPauseCurrentTime()
{
  VERBOSE();
  ismd_pts_t start = GetAudioStartPts();

  if (start == ISMD_NO_PTS || (GetCurrentTime() < GetBaseTime()))
    return ISMD_NO_PTS;

  return start + GetPauseCurrentTime() - GetBaseTime();
}

ismd_pts_t CIntelSMDGlobals::GetVideoPauseCurrentTime()
{
  VERBOSE();
  ismd_pts_t start = GetVideoStartPts();

  if (start == ISMD_NO_PTS || (GetCurrentTime() < GetBaseTime()))
    return ISMD_NO_PTS;

  return start + GetPauseCurrentTime() - GetBaseTime();
}

ismd_pts_t CIntelSMDGlobals::Resync(bool bDisablePtsCorrection)
{
  VERBOSE();
  ismd_pts_t audioStart;
  ismd_pts_t videoStart;
  ismd_pts_t deltaPTS;
  ismd_pts_t eps = 10000;
  ismd_pts_t newPTS;

  audioStart = GetAudioStartPts();
  videoStart = GetVideoStartPts();

  if (audioStart == ISMD_NO_PTS)
    return ISMD_NO_PTS;

  if (videoStart == ISMD_NO_PTS)
    return audioStart;

  deltaPTS = abs(videoStart - audioStart);

  if (deltaPTS == 0)
    return videoStart;

  if (bDisablePtsCorrection)
    CLog::Log(LOGINFO, "CIntelSMDGlobals::Resync no pts correction");

  if (deltaPTS > eps && deltaPTS < 90000 * 15 && !bDisablePtsCorrection)
  {
    if (videoStart < audioStart)
      newPTS = videoStart + deltaPTS / 4;
    else
      newPTS = videoStart - deltaPTS / 4;
  }
  else
  {
    newPTS = audioStart;
  }

  return newPTS;
}

void CIntelSMDGlobals::CreateStartPacket(ismd_pts_t start_pts,
    ismd_buffer_handle_t buffer_handle)
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  ismd_newsegment_tag_t newsegment_data;
  ismd_result_t ismd_ret;

  if (buffer_handle == 0)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::CreateStartPacket buffer_handle = NULL");
    return;
  }

  newsegment_data.linear_start = 0;
  newsegment_data.start = start_pts;
  newsegment_data.stop = ISMD_NO_PTS;
  newsegment_data.requested_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.applied_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.rate_valid = true;
  newsegment_data.segment_position = ISMD_NO_PTS;

  ismd_ret = ismd_tag_set_newsegment_position(buffer_handle, newsegment_data);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SendStartPacket ismd_tag_set_newsegment failed");
    return;
  }
}

void CIntelSMDGlobals::SendStartPacket(ismd_pts_t start_pts,
    ismd_port_handle_t port, ismd_buffer_handle_t buffer)
{
  VERBOSE();
  CSingleLock lock(m_Lock);

  ismd_newsegment_tag_t newsegment_data;
  ismd_buffer_handle_t buffer_handle;
  ismd_result_t ismd_ret;
  //create a carrier buffer for the new segment
  if (buffer != 0)
    buffer_handle = buffer;
  else
  {
    ismd_ret = ismd_buffer_alloc(0, &buffer_handle);
    if (ismd_ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDGlobals::SendStartPacket ismd_buffer_alloc failed");
      return;
    }
  }

  newsegment_data.linear_start = 0;
  newsegment_data.start = start_pts;
  newsegment_data.stop = ISMD_NO_PTS;
  newsegment_data.requested_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.applied_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.rate_valid = true;
  newsegment_data.segment_position = ISMD_NO_PTS;

  ismd_ret = ismd_tag_set_newsegment_position(buffer_handle, newsegment_data);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SendStartPacket ismd_tag_set_newsegment failed");
    return;
  }

  ismd_ret = ismd_port_write(port, buffer_handle);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SendStartPacket ismd_port_write failed");
    return;
  }
}

bool CIntelSMDGlobals::CreateVideoDecoder(ismd_codec_type_t codec_type)
{
  VERBOSE();

  ismd_result_t res;

  CSingleLock lock(m_Lock);

  if (m_viddec != -1)
    return true;

  // open decoder
  res = ismd_viddec_open(codec_type, &m_viddec);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "viddec open failed <%d>", res);
    return false;
  }

  res = ismd_viddec_get_input_port(m_viddec, &m_viddec_input_port);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "viddec get input port failed <%d>", res);
    return false;
  }

  res = ismd_viddec_get_output_port(m_viddec, &m_viddec_output_port);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "viddec get output port failed <%d>", res);
    return false;
  }

  res = ismd_viddec_set_pts_interpolation_policy(m_viddec,
      ISMD_VIDDEC_INTERPOLATE_MISSING_PTS, 0);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismd_viddec_set_pts_interpolation_policy failed <%d>",
        res);
    return false;
  }

  res = ismd_viddec_set_max_frames_to_decode(m_viddec, ISMD_VIDDEC_ALL_FRAMES);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismd_viddec_set_max_frames_to_decode failed <%d>",
        res);
    return false;
  }

  SetVideoDecoderState(ISMD_DEV_STATE_PAUSE);

  m_video_codec = codec_type;

  return true;
}

bool CIntelSMDGlobals::DeleteVideoDecoder()
{
  VERBOSE();

  ismd_result_t res;

  if (m_viddec == -1)
    return true;

  SetVideoDecoderState(ISMD_DEV_STATE_STOP);
  FlushVideoDecoder();

  ismd_dev_close(m_viddec);

  m_viddec = -1;
  m_viddec_input_port = -1;
  m_viddec_output_port = -1;

  return true;
}

bool CIntelSMDGlobals::PrintVideoStreamStats()
{
  VERBOSE();
  ismd_result_t result;
  ismd_viddec_stream_statistics_t stat;

  result = ismd_viddec_get_stream_statistics(m_viddec, &stat);

  if (result == ISMD_SUCCESS)
    CLog::Log(LOGDEBUG,
        "current_bit_rate %lu \
        total_bytes %lu \
        frames_decoded %lu \
        frames_dropped %lu \
        error_frames %lu \
        invalid_bitstream_syntax_errors %lu \
        unsupported_profile_errors %lu \
        unsupported_level_errors %lu \
        unsupported_feature_errors %lu \
        unsupported_resolution_errors %lu ",

        stat.current_bit_rate, stat.total_bytes, stat.frames_decoded,
        stat.frames_dropped, stat.error_frames,
        stat.invalid_bitstream_syntax_errors, stat.unsupported_profile_errors,
        stat.unsupported_level_errors, stat.unsupported_feature_errors,
        stat.unsupported_resolution_errors);
  else
    CLog::Log(LOGERROR, "CIntelSMDGlobals::PrintVideoStreamStats failed");

  return (result == ISMD_SUCCESS);
}

bool CIntelSMDGlobals::PrintVideoStreaProp()
{
  VERBOSE();
  ismd_result_t result;

  ismd_viddec_stream_properties_t prop;

  result = ismd_viddec_get_stream_properties(m_viddec, &prop);

  if (result == ISMD_SUCCESS)
    CLog::Log(LOGDEBUG,
        "codec_type %lu \
        nom_bit_rate %lu \
        frame_rate_num %lu \
        frame_rate_den %lu \
        coded_height %lu \
        coded_width %lu \
        display_height %lu \
        display_width %lu \
        profile %lu \
        level %lu \
        is_stream_interlaced %lu \
        sample_aspect_ratio %lu ",
        prop.codec_type, prop.nom_bit_rate, prop.frame_rate_num,
        prop.frame_rate_den, prop.coded_height, prop.coded_width,
        prop.display_height, prop.display_width, prop.profile, prop.level,
        prop.is_stream_interlaced, prop.sample_aspect_ratio);
  else
    CLog::Log(LOGERROR, "CIntelSMDGlobals::PrintVideoStreaProp failed");

  return (result == ISMD_SUCCESS);
}

bool CIntelSMDGlobals::CreateVideoRender(gdl_plane_id_t plane)
{
  VERBOSE();

  ismd_result_t res;

  CSingleLock lock(m_Lock);

  if (m_video_proc == -1)
  {
    //Open and initialize DPE
    res = ismd_vidpproc_open(&m_video_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_open failed. %d", res);
      return false;
    }

    // We use SCALE_TO_FIT here for the scaling policy because it is easier for
    // us to create the dest rect based on baserenderer and tell SMD to scale that way,
    // than to reconcile the aspect ratio calculations and convoluded scaling policies
    // in SMD which appear to have many edge cases where it 'gives up'
    res = ismd_vidpproc_set_scaling_policy(m_video_proc, SCALE_TO_FIT);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_set_scaling_policy failed. %d", res);
      return false;
    }

    res = ismd_vidpproc_get_input_port(m_video_proc, &m_video_input_port_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_get_input_port failed. %d", res);
      return false;
    }

    res = ismd_vidpproc_get_output_port(m_video_proc,
        &m_video_output_port_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_get_output_port failed. %d", res);
      return false;
    }

    res = ismd_vidpproc_pan_scan_disable(m_video_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_pan_scan_disable failed. %d", res);
      return false;
    }

    res = ismd_vidpproc_gaussian_enable(m_video_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_gaussian_enable failed. %d", res);
      return false;
    }

    res = ismd_vidpproc_deringing_enable(m_video_proc);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_vidpproc_deringing_enable failed. %d", res);
      return false;
    }
  }

  if (m_video_render == -1)
  {
    //Open and initialize Render
    res = ismd_vidrend_open(&m_video_render);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ERROR: FAIL TO OPEN VIDEO RENDERER .....");
      return false;
    }

    res = ismd_vidrend_get_input_port(m_video_render,
        &m_video_input_port_renderer);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ERROR: FAIL TO GET INPUT PORT FOR RENDERER .....");
      return false;
    }

    res = ismd_dev_set_clock(m_video_render, m_main_clock);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ERROR: FAIL TO SET CLOCK FOR RENDERER .....");
      return false;
    }

    res = ismd_clock_set_vsync_pipe(m_main_clock, 0);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ERROR: ismd_clock_set_vsync_pipe .....");
      return false;
    }

    res = ismd_vidrend_set_video_plane(m_video_render, plane);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, " ERROR: FAIL TO SET GDL UPP FOR THIS RENDERER .....");
      return false;
    }

    res = ismd_vidrend_set_flush_policy(m_video_render,
        ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::CreateVideoRender ismd_vidrend_set_flush_policy failed");
      return false;
    }

    res = ismd_vidrend_set_stop_policy(m_video_render,
        ISMD_VIDREND_STOP_POLICY_DISPLAY_BLACK);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::CreateVideoRender ismd_vidrend_set_stop_policy failed");
      return false;
    }

    res = ismd_vidrend_enable_max_hold_time(m_video_render, 30, 1);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::CreateVideoRender ismd_vidrend_enable_max_hold_time failed");
      return false;
    }
  }

  //Connect DPE and Render
  if (m_video_output_port_proc != -1 && m_video_input_port_renderer != -1)
  {
    res = ismd_port_connect(m_video_output_port_proc,
        m_video_input_port_renderer);
    if (res != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "IntelSMDGlobals::CreateVideoRender ismd_port_connect failed: %d", res);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "IntelSMDGlobals::CreateVideoRender failed");
    return false;
  }

  SetVideoRenderState(ISMD_DEV_STATE_PAUSE);
  m_RenderState = ISMD_DEV_STATE_PAUSE;

  return true;
}

bool CIntelSMDGlobals::MuteVideoRender(bool mute)
{
  VERBOSE();
  ismd_result_t res;

  if (m_video_render == -1)
      return false;

  res = ismd_vidrend_mute(m_video_render, mute ? ISMD_VIDREND_MUTE_DISPLAY_BLACK_FRAME : ISMD_VIDREND_MUTE_NONE);
  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::MuteVideoRender with %d failed. %d", mute, res);
    return false;
  }

  return true;
}

float CIntelSMDGlobals::GetRenderFPS()
{
  VERBOSE();
  ismd_result_t res;
  float fps = 0;
  ismd_vidrend_status_t status;
  ismd_vidrend_stats_t stat;

  if (m_video_render == -1)
    return 0;

  res = ismd_vidrend_get_status(m_video_render, &status);
  if (res == ISMD_SUCCESS)
    res = ismd_vidrend_get_stats(m_video_render, &stat);

  if (res == ISMD_SUCCESS)
    // we want to make sure some frames were actually rendered
    if (stat.frames_displayed > 30)
      fps = 90000.0f / status.content_pts_interval;

  return fps;
}

float CIntelSMDGlobals::GetDecoderFPS()
{
  float fps = 0;

  // check some frames were actually rendered
  if (GetRenderFPS() == 0)
    return 0;

  ismd_viddec_stream_properties_t prop;
  ismd_result_t res = ismd_viddec_get_stream_properties(GetVidDec(), &prop);
  if (res == ISMD_SUCCESS)
  {
    int num = prop.frame_rate_num;
    int den = prop.frame_rate_den;
    if (num != 0 && den != 0)
    {
      fps = (float) num / (float) den;
      //printf("SMD fps %f", fps);
    }
  }

  return fps;
}

ismd_result_t CIntelSMDGlobals::GetPortStatus(ismd_port_handle_t port,
    unsigned int& curDepth, unsigned int& maxDepth)
{
  VERBOSE();
  ismd_result_t ret;
  ismd_port_status_t portStatus;

  ret = ismd_port_get_status(port, &portStatus);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::GetPortStatus ismd_port_get_status failed %d", ret);
    curDepth = -1;
    maxDepth = -1;
    return ret;
  }

  curDepth = portStatus.cur_depth;
  maxDepth = portStatus.max_depth;

  return ret;
}

double CIntelSMDGlobals::GetFrameDuration()
{
  ismd_result_t res;
  float duration = 0;
  ismd_vidrend_status_t status;
  ismd_vidrend_stats_t stat;

  if (m_video_render == -1)
    return 0;

  res = ismd_vidrend_get_status(m_video_render, &status);
  if (res == ISMD_SUCCESS)
    res = ismd_vidrend_get_stats(m_video_render, &stat);

  if (res == ISMD_SUCCESS)
    // we want to make sure some frames were actually rendered
    if (stat.frames_displayed > 30)
      duration = IsmdToDvdPts(status.content_pts_interval);

  return duration;
}

bool CIntelSMDGlobals::DeleteVideoRender()
{
  VERBOSE();

  SetVideoRenderState(ISMD_DEV_STATE_STOP);
  FlushVideoRender();

  if (m_video_proc != 1)
    ismd_dev_close(m_video_proc);
  if (m_video_render != -1)
    ismd_dev_close(m_video_render);

  m_video_proc = -1;
  m_video_input_port_proc = 0;
  m_video_output_port_proc = 0;

  m_video_render = -1;
  m_video_input_port_renderer = -1;
  m_video_output_port_renderer = -1;

  return true;
}

bool CIntelSMDGlobals::ConnectDecoderToRenderer()
{
  VERBOSE();

  ismd_result_t ret;

  if (m_viddec_output_port != -1 && m_video_input_port_proc != -1)
  {
    ret = ismd_port_connect(m_viddec_output_port, m_video_input_port_proc);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "IntelSMDGlobals::ConnectDecoderToRenderer ismd_port_connect failed");
      return false;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "IntelSMDGlobals::ConnectDecoderToRenderer failed");
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::PrintRenderStats()
{
  VERBOSE();
  ismd_result_t result;
  ismd_vidrend_stats_t stat;

  result = ismd_vidrend_get_stats(m_video_render, &stat);

  if (result == ISMD_SUCCESS)
    CLog::Log(LOGDEBUG,
        "vsyncs_frame_received %d\
        vsyncs_frame_skipped %d\
        vsyncs_top_received %d\
        vsyncs_top_skipped %d\
        vsyncs_bottom_received %d\
        vsyncs_bottom_skipped %d\
        frames_input %d\
        frames_displayed %d\
        frames_released %d\
        frames_dropped %d\
        frames_repeated %d\
        frames_late %d\
        frames_out_of_order %d\
        frames_out_of_segment %d\
        late_flips %d",

        stat.vsyncs_frame_received, stat.vsyncs_frame_skipped,
        stat.vsyncs_top_received, stat.vsyncs_top_skipped,
        stat.vsyncs_bottom_received, stat.vsyncs_bottom_skipped,
        stat.frames_input, stat.frames_displayed, stat.frames_released,
        stat.frames_dropped, stat.frames_repeated, stat.frames_late,
        stat.frames_out_of_order, stat.frames_out_of_segment, stat.late_flips);
  else
    CLog::Log(LOGERROR, "CIntelSMDGlobals::PrintRenderStats failed");

  return result == ISMD_SUCCESS;
}

bool CIntelSMDGlobals::CreateAudioProcessor()
{
  VERBOSE();

  ismd_result_t result;

  if (m_audioProcessor != -1)
    return true;

  result = ismd_audio_open_global_processor(&m_audioProcessor);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize - error open global processor: %d", result);
    return false;
  }

  // Configure the master clock frequency
  ConfigureMasterClock(48000);

  return true;
}

bool CIntelSMDGlobals::DeleteAudioProcessor()
{
  VERBOSE();

  ismd_result_t result;

  if (m_audioProcessor == -1)
    return true;

  result = ismd_audio_close_processor(m_audioProcessor);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::DestroyAudioProcessor - error closing device: %d", result);
    return false;
  }

  m_audioProcessor = -1;
  return true;
}

bool CIntelSMDGlobals::ConfigureMasterClock(unsigned int frequency)
{
  VERBOSE();
  ismd_result_t result;

  // Set clock source (EXTERNAL for CE4100)
  ismd_audio_clk_src_t clockSource = ISMD_AUDIO_CLK_SRC_EXTERNAL;

  unsigned int clockrate = 0;
  switch (frequency)
  {
    case 12000:
    case 32000:
    case 48000:
    case 96000:
    case 192000:
      clockrate = 36864000;
      break;
    case 44100:
    case 88200:
    case 176400:
      clockrate = 33868800;
      break;
    default:
      break;
  }

  if(clockrate == 0)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureMasterClock - unknown frequency %d", frequency);
    return false;
  }

  result = ismd_audio_configure_master_clock(m_audioProcessor, clockrate, clockSource);

  if(result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureMasterClock - configure_audio_master_clock %d failed (%d)!", clockrate, result);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::EnableAudioInput(ismd_dev_t audioDev)
{
  VERBOSE();
  ismd_result_t result;

  if (audioDev == -1 || m_audioProcessor == -1)
    return true;

  result = ismd_audio_input_enable(m_audioProcessor, audioDev);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::EnableAudioInput failed %d", result);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::DisableAudioInput(ismd_dev_t audioDev)
{
  VERBOSE();
  ismd_result_t result;

  if (audioDev == -1 || m_audioProcessor == -1)
    return true;

  result = ismd_audio_input_disable(m_audioProcessor, audioDev);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::DisableAudioInput failed %d", result);
    return false;
  }

  return true;
}


bool CIntelSMDGlobals::EnableAudioOutput(ismd_audio_output_t audio_output)
{
  VERBOSE();
  ismd_result_t result;

  if (m_audioProcessor == -1)
    return true;

  result = ismd_audio_output_enable(m_audioProcessor, audio_output);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::EnableAudioOutput failed - %d", result);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::DisableAudioOutput(ismd_audio_output_t audio_output)
{
  VERBOSE();
  ismd_result_t result;

  if (m_audioProcessor == -1)
    return true;

  result = ismd_audio_output_disable(m_audioProcessor, audio_output);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::DisableAudioOutput failed - %d", result);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::ConfigureAudioOutput(ismd_audio_output_t output, ismd_audio_output_config_t output_config)
{
  VERBOSE();
  ismd_result_t result;

  result = ismd_audio_output_set_delay(m_audioProcessor, output, output_config.stream_delay);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::ConfigureAudioOutput ismd_audio_output_set_delay failed %d", result);
//    return false;
  }

  result = ismd_audio_output_set_sample_size(m_audioProcessor, output, output_config.sample_size);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::ConfigureAudioOutput ismd_audio_output_set_sample_size failed %d", result);
//    return false;
  }

  result = ismd_audio_output_set_channel_config(m_audioProcessor, output, output_config.ch_config);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::ConfigureAudioOutput ismd_audio_output_set_sample_size failed %d", result);
//    return false;
  }

  result = ismd_audio_output_set_mode(m_audioProcessor, output, output_config.out_mode);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::ConfigureAudioOutput ismd_audio_output_set_sample_size failed %d", result);
//    return false;
  }

  result = ismd_audio_output_set_sample_rate(m_audioProcessor, output, output_config.sample_rate);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::ConfigureAudioOutput ismd_audio_output_set_sample_size failed %d", result);
//    return false;
  }

  return true;
}

bool CIntelSMDGlobals::SetAudioDeviceState(ismd_dev_state_t state, ismd_dev_t device)
{
  VERBOSE();
  ismd_result_t ret;

  if (state == ISMD_DEV_STATE_INVALID)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SetAudioDeviceState setting device to invalid");
    return true;
  }

  if (device == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::SetAudioDeviceState device = -1");
    return false;
  }

  CSingleLock lock(m_Lock);

  ret = ismd_dev_set_state(device, state);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGWARNING, "CIntelSMDGlobals::SetAudioDeviceState ismd_dev_set_state %d on audio device %d failed %d",
        state, device, ret);
  }

  return true;
}

ismd_dev_state_t CIntelSMDGlobals::GetAudioDeviceState(ismd_dev_t audioDev)
{
  VERBOSE();
  //printf("CIntelSMDGlobals::GetAudioDeviceState");

  CSingleLock lock(m_Lock);

  ismd_result_t ret;
  ismd_dev_state_t state;

  if (audioDev == -1)
  {
    //printf("CIntelSMDGlobals::GetAudioDeviceState audioDev = -1");
    return ISMD_DEV_STATE_INVALID;
  }

  ret = ismd_dev_get_state(audioDev, &state);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::GetAudioDeviceState ismd_dev_get_state failed %d",
        ret);
    return ISMD_DEV_STATE_INVALID;
  }

  return state;
}

bool CIntelSMDGlobals::SetVideoDecoderState(ismd_dev_state_t state)
{
  CLog::Log(LOGDEBUG, "CIntelSMDGlobals::%s %d", __FUNCTION__, state);
  //printf("%s Video Decoder State: %d", __FUNCTION__, state);
  CSingleLock lock(m_Lock);

  ismd_result_t ret;

  if (state == ISMD_DEV_STATE_INVALID)
  {
    CLog::Log(LOGWARNING,
        "CIntelSMDGlobals::SetVideoDecoderState setting device to invalid");
    return true;
  }

  if (m_viddec != -1)
  {
    ret = ismd_dev_set_state(m_viddec, state);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(
          LOGWARNING,
          "CIntelSMDGlobals::SetVideoDecoderState ismd_dev_set_state on video decoder failed %d",
          ret);
      return true;
    }
  }

  CLog::Log(LOGDEBUG, "SetVideoDecoderState state %d ret %d", state, ret);

  return true;
}

bool CIntelSMDGlobals::SetVideoRenderState(ismd_dev_state_t state)
{
  CLog::Log(LOGDEBUG, "CIntelSMDGlobals::%s %d", __FUNCTION__, state);
  //printf("%s Video Render State: %d", __FUNCTION__, state);

  CSingleLock lock(m_Lock);

  ismd_result_t ret;

  if (state == ISMD_DEV_STATE_INVALID)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::SetVideoRenderState setting device to invalid");
    return true;
  }

  if (m_video_proc != -1)
  {
    ret = ismd_dev_set_state(m_video_proc, state);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::SetVideoRenderState ismd_dev_set_state on video proc failed %d",
          ret);
      return false;
    }
  }

  if (m_video_render != -1)
  {
    ret = ismd_dev_set_state(m_video_render, state);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::SetVideoRenderState ismd_dev_set_state on video render failed %d",
          ret);
      return false;
    }
  }

  m_RenderState = state;

  CLog::Log(LOGDEBUG, "SetRenderrState state %d ret %d", state, ret);

  return true;
}

bool CIntelSMDGlobals::FlushAudioDevice(ismd_dev_t audioDev)
{
  VERBOSE();
  //printf("CIntelSMDGlobals::FlushAudioDevice", __FUNCTION__);

  CSingleLock lock(m_Lock);
  ismd_result_t ret = ISMD_ERROR_UNSPECIFIED;

  SetAudioDeviceState(ISMD_DEV_STATE_PAUSE, audioDev);

  if (audioDev != -1)
  {
    ret = ismd_dev_flush(audioDev);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGDEBUG, "CIntelSMDGlobals::FlushAudioDevice ismd_dev_flush timed failed %d", ret);
      return false;
    }
  }

  return true;
}

bool CIntelSMDGlobals::FlushVideoDecoder()
{
  VERBOSE();
  CSingleLock lock(m_Lock);
  ismd_result_t ret = ISMD_ERROR_UNSPECIFIED;

  if(m_viddec == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::FlushVideoDecoder m_viddec == -1");
    return false;
  }

  SetVideoDecoderState(ISMD_DEV_STATE_PAUSE);

  if (m_viddec != -1)
    ret = ismd_dev_flush(m_viddec);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::FlushVideoDecoder ismd_dev_flush failed %d", ret);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::FlushVideoRender()
{
  VERBOSE();
  //printf("CIntelSMDGlobals::FlushVideoRender");

  CSingleLock lock(m_Lock);
  ismd_result_t ret;

  if(m_video_render == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::FlushVideoDecoder m_video_render == -1");
    return false;
  }

  if (m_video_render != -1)
    ret = ismd_vidrend_set_flush_policy(m_video_render,
        ISMD_VIDREND_FLUSH_POLICY_REPEAT_FRAME);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::CreateVideoRender ismd_vidrend_set_flush_policy failed after pause");
    return false;
  }

  SetVideoRenderState(ISMD_DEV_STATE_PAUSE);

  if (m_video_proc != -1)
    ret = ismd_dev_flush(m_video_proc);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::FlushVideoRender ismd_dev_flush video proc failed %d",
        ret);
    return false;
  }

  if (m_video_render != -1)
    ret = ismd_dev_flush(m_video_render);
  if (ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::FlushVideoRender ismd_dev_flush failed %d",
        ret);
    return false;
  }

  return true;
}

bool CIntelSMDGlobals::SetVideoRenderBaseTime(ismd_time_t time)
{
  //printf("%s", __FUNCTION__);
  CLog::Log(LOGINFO, "SetVideoRenderBaseTime base time %d ms", DVD_TIME_TO_MSEC(IsmdToDvdPts(time)));

  ismd_result_t ret;
  ismd_dev_state_t state = ISMD_DEV_STATE_INVALID;

  CSingleLock lock(m_Lock);

  if (m_RenderState == ISMD_DEV_STATE_PLAY)
  {
    //printf("Warning - Trying to set base time on renderer while in play mode. Ignoring");
    return true;
  }

  if (m_video_render != -1)
  {
//    printf("CIntelSMDGlobals::SetVideoRenderBaseTime current state %d setting time to %.2f", state, IsmdToDvdPts(time) / 1000000.0f);
    ret = ismd_dev_get_state(m_video_render, &state);
    if (ret != ISMD_SUCCESS)
    {
      // Currently SDK always return error when trying to get renderer state
      //printf("CIntelSMDGlobals::SetVideoRenderBaseTime ismd_dev_get_state failed %d", ret);
    }
    else if (state == ISMD_DEV_STATE_PLAY)
    {
      CLog::Log(LOGDEBUG, "CIntelSMDGlobals::SetVideoRenderBaseTime device is running/n");
      return true;
    }

    //printf("CIntelSMDGlobals::SetVideoRenderBaseTime current state %d setting time to %.2f", state, IsmdToDvdPts(time) / 1000000.0f);

    ret = ismd_dev_set_stream_base_time(m_video_render, time);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDGlobals::SetVideoRenderBaseTime ismd_dev_set_stream_base_time for video render failed %d",
          ret);
      return false;
    }
  }

  return true;
}

bool CIntelSMDGlobals::SetAudioDeviceBaseTime(ismd_time_t time, ismd_dev_t device)
{
  //printf("%s", __FUNCTION__);
  CLog::Log(LOGINFO, "SetAudioDeviceBaseTime base time %d ms", DVD_TIME_TO_MSEC(IsmdToDvdPts(time)));

  ismd_result_t ret;
  ismd_dev_state_t state = ISMD_DEV_STATE_INVALID;

  CSingleLock lock(m_Lock);

  if (device != -1)
  {
    ret = ismd_dev_get_state(device, &state);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDGlobals::SetAudioDeviceState ismd_dev_get_state for audio device %d failed %d", device, ret);
    }
    if (state == ISMD_DEV_STATE_PLAY)
    {
      CLog::Log(LOGDEBUG, "CIntelSMDGlobals::SetAudioDeviceBaseTime device is running. Ignoring request");
      return true;
    }

    ret = ismd_dev_set_stream_base_time(device, time);
    if (ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDGlobals::SetAudioDeviceState ismd_dev_set_stream_base_time for audio device %d failed %d", device, ret);
    }
  }

  return true;
}

void CIntelSMDGlobals::Mute(bool bMute)
{
  VERBOSE();
  ismd_audio_processor_t audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if (audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::%s - audioProcessor == -1",
        __FUNCTION__);
    return;
  }
  ismd_audio_mute(audioProcessor, bMute);
}

bool CIntelSMDGlobals::SetMasterVolume(float nVolume)
{
  VERBOSE();
  ismd_audio_processor_t audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if (audioProcessor == -1)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::SetMasterVolume - audioProcessor == -1");
    return false;
  }

  bool muted = false;
  ismd_result_t result = ismd_audio_is_muted(audioProcessor, &muted);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR,
        "CIntelSMDGlobals::CIntelSMDGlobals - ismd_audio_is_muted failed.  %d",
        result);
    return false;
  }

  if (nVolume == VOLUME_MINIMUM)
  {
    if (!muted)
      result = ismd_audio_mute(audioProcessor, true);
  }
  else
  {
    if (muted)
      result = ismd_audio_mute(audioProcessor, false);

    if (result == ISMD_SUCCESS)
    {
      // SMD volume scales from +18.0dB to -145.0dB with values of +180 to -1450
      // anything over 0dB will cause audio degradation
      // in practice -50dB is extremely quiet, so we scale our standard volume from MAX_VOLUME to -30dB
      long smdVolume = (long) (((float) nVolume) * 300.0f / 6000.0f);
      result = ismd_audio_set_master_volume(audioProcessor, smdVolume);
    }

    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,
          "CIntelSMDAudioRenderer::Resume - ismd_audio_set_master_volume: %d",
          result);
    }
  }

  return (result == ISMD_SUCCESS);
}

bool CIntelSMDGlobals::CheckCodecHWDecode(int Codec)
{

  VERBOSE();
  switch(Codec)
  {
  case CODEC_ID_MP3:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_MPEG));
  case CODEC_ID_AAC:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_AAC));
  case CODEC_ID_AC3:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_DD));
  case CODEC_ID_EAC3:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_DD_PLUS));
  case CODEC_ID_TRUEHD:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_TRUE_HD));
  case CODEC_ID_DTS:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_DTS));
  case SMD_CODEC_DTSHD:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_DTS_HD));
  case CODEC_ID_AAC_LATM:
    return (ISMD_SUCCESS == ismd_audio_codec_available(ISMD_AUDIO_MEDIA_FMT_AAC_LOAS));
  default:
    return false;
  }
}

#endif
