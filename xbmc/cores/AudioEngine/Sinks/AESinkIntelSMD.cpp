/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#ifdef HAS_INTEL_SMD

#include "AESinkIntelSMD.h"
#include "utils/log.h"
#include "../../IntelSMDGlobals.h"
#include "Utils/AEDeviceInfo.h"
extern "C"
{
#include <ismd_core.h>
#include <ismd_audio.h>
#include <ismd_audio_ac3.h>
#include <ismd_audio_ddplus.h>
#include <ismd_audio_truehd.h>
#include <ismd_audio_dts.h>
#include <pal_soc_info.h>
}

#define __MODULE_NAME__ "AESinkIntelSMD"
#define VERBOSE2() CLog::Log(LOGDEBUG, "%s", __DEBUG_ID__)

#if 0
#define VERBOSE() VERBOSE2()
#else
#define VERBOSE()
#endif


CCriticalSection CAESinkIntelSMD::m_SMDAudioLock;

ismd_dev_t tmpDevice = -1;

CAESinkIntelSMD::CAESinkIntelSMD()
{
  VERBOSE2();
  m_bIsAllocated = false;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;

  m_audioDevice = -1;
  m_audioDeviceInput = -1;

//  m_fCurrentVolume = 0;
}

CAESinkIntelSMD::~CAESinkIntelSMD()
{
  VERBOSE2();
  Deinitialize();
}

bool CAESinkIntelSMD::Initialize(AEAudioFormat &format, std::string &device)
{
  VERBOSE2();
  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t result;

  // TODO(q)
  m_dwChunkSize = 4*1024;
  m_dwBufferLen = m_dwChunkSize;
  static enum AEChannel map[3] = {AE_CH_FL, AE_CH_FR , AE_CH_NULL};
  format.m_dataFormat = AE_FMT_S16LE;
  format.m_sampleRate = 48000;
  format.m_channelLayout = CAEChannelInfo(map);
  format.m_frameSize = 4;
  format.m_frames = m_dwChunkSize/format.m_frameSize;
  format.m_frameSamples = format.m_frames*2;


  ismd_audio_processor_t  audioProcessor = -1;
  ismd_audio_format_t     ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;

  int inputChannelConfig = AUDIO_CHAN_CONFIG_2_CH;

  audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if(audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "%s audioProcessor is not valid", __DEBUG_ID__);
    return false;
  }

  // disable all outputs
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  // g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  // g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  m_audioDevice = g_IntelSMDGlobals.CreateAudioInput(false);
  if(m_audioDevice == -1)
  {
    CLog::Log(LOGERROR, "%s failed to create audio input", __DEBUG_ID__);
    return false;
  }

  g_IntelSMDGlobals.SetPrimaryAudioDevice(m_audioDevice);
  m_audioDeviceInput = g_IntelSMDGlobals.GetAudioDevicePort(m_audioDevice);
  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "%s failed to create audio input port", __DEBUG_ID__);
    return false;
  }

  ismdAudioInputFormat = GetISMDFormat(format.m_dataFormat);

  // Can we do hardware decode?
  bool bHardwareDecoder = (ismd_audio_codec_available(ismdAudioInputFormat) == ISMD_SUCCESS);
  if(ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_PCM)
    bHardwareDecoder = false;

  int uiBitsPerSample = 16;
  CLog::Log(LOGINFO, "%s ismdAudioInputFormat %d Hardware decoding (non PCM) %d\n", __DEBUG_ID__,
      ismdAudioInputFormat, bHardwareDecoder);

  int counter = 0;
  while(counter < 5)
  {
    result = ismd_audio_input_set_data_format(audioProcessor, m_audioDevice, ismdAudioInputFormat);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s ismd_audio_input_set_data_format failed. retrying %d %d", __DEBUG_ID__, counter, result);
      counter++;
      usleep(1000);
    }
    else
      break;
  }

  // TODO(q) never actually take anything other than pcm
  result = ismd_audio_input_set_pcm_format(audioProcessor, m_audioDevice, uiBitsPerSample, format.m_sampleRate, inputChannelConfig);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - ismd_audio_input_set_pcm_format: %d", __DEBUG_ID__, result);
    return false;
  }

  // I2S. Nothing to touch here. we always use defaults

  // SPDIF
//   if(m_bIsSPDIF)
//   {
//     ismd_audio_output_t OutputSPDIF = g_IntelSMDGlobals.GetSPDIFOutput();
//     ismd_audio_output_config_t spdif_output_config;
//     unsigned int samplesPerSec = format.m_sampleRate;
//     unsigned int bitsPerSec = uiBitsPerSample;

//     ConfigureAudioOutputParams(spdif_output_config, AE_DEVTYPE_IEC958, bitsPerSec,
//         samplesPerSec, format.m_channelLayout.Count(), ismdAudioInputFormat, bSPDIFPassthrough);
//     if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputSPDIF, spdif_output_config))
//     {
//       CLog::Log(LOGERROR, "%s ConfigureAudioOutput SPDIF failed %d", __DEBUG_ID__, result);
// //      return false;
//     }
//     format.m_sampleRate = spdif_output_config.sample_rate;
//   }

  // HDMI
//  if(m_bIsHDMI && !m_bIsAllOutputs)
  {
    ismd_audio_output_t OutputHDMI = g_IntelSMDGlobals.GetHDMIOutput();
    ismd_audio_output_config_t hdmi_output_config;
    ConfigureAudioOutputParams(hdmi_output_config, AE_DEVTYPE_HDMI, uiBitsPerSample,
      format.m_sampleRate, format.m_channelLayout.Count(), ismdAudioInputFormat, false);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputHDMI, hdmi_output_config))
    {
      CLog::Log(LOGERROR, "%s ConfigureAudioOutput HDMI failed %d", __DEBUG_ID__, result);
//      return false;
    }
   format.m_sampleRate = hdmi_output_config.sample_rate;
  }

  // Configure the master clock frequency

  CLog::Log(LOGINFO, "%s ConfigureMasterClock %d", __DEBUG_ID__, format.m_sampleRate);
  g_IntelSMDGlobals.ConfigureMasterClock(format.m_sampleRate);

  ismd_audio_input_pass_through_config_t passthrough_config;
  memset(&passthrough_config, 0, sizeof(&passthrough_config));
  result = ismd_audio_input_set_as_primary(audioProcessor, m_audioDevice, passthrough_config);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s  ismd_audio_input_set_as_primary failed %d", __DEBUG_ID__, result);
//      return false;
  }

  if(!g_IntelSMDGlobals.EnableAudioInput(m_audioDevice))
  {
    CLog::Log(LOGERROR, "%s  EnableAudioInput", __DEBUG_ID__);
//    return false;
  }

  // enable outputs
  if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput()))
  {
    CLog::Log(LOGERROR, "%s  EnableAudioOutput HDMI failed", __DEBUG_ID__);
//      return false;
  }

//   if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput()))
//   {
//     CLog::Log(LOGERROR, "%s  EnableAudioOutput SPDIF failed", __DEBUG_ID__);
// //      return false;
//   }

//   if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput()))
//   {
//     CLog::Log(LOGERROR, "%s  EnableAudioOutput I2S failed", __DEBUG_ID__);
// //      return false;
//   }

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);

//  m_fCurrentVolume = g_settings.m_fVolumeLevel;
//  g_IntelSMDGlobals.SetMasterVolume(m_fCurrentVolume);

  m_bPause = false;
  m_bIsAllocated = true;

  CLog::Log(LOGINFO, "%s done", __DEBUG_ID__);

  return true;
}

void CAESinkIntelSMD::Deinitialize()
{
  VERBOSE2();
  CSingleLock lock(m_SMDAudioLock);
  CLog::Log(LOGINFO, "%s", __DEBUG_ID__);

  if(!m_bIsAllocated)
    return;

  g_IntelSMDGlobals.BuildAudioOutputs();
  g_IntelSMDGlobals.ConfigureMasterClock(48000);

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_STOP, m_audioDevice);
  g_IntelSMDGlobals.DisableAudioInput(m_audioDevice);
  g_IntelSMDGlobals.RemoveAudioInput(m_audioDevice);

  g_IntelSMDGlobals.SetPrimaryAudioDevice(-1);

  m_audioDevice = -1;
  m_audioDeviceInput = -1;

  // enable outputs
  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  // g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  // g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  m_bIsAllocated = false;

  return;
}

double CAESinkIntelSMD::GetCacheTotal()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0.0f;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "%s - inputPort == -1", __DEBUG_ID__);
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error getting port status: %d", __DEBUG_ID__, smd_ret);
    return 0;
  }

  return ((double) (maxDepth * m_dwBufferLen)/(2*2*48000.0));
}
double CAESinkIntelSMD::GetDelay()
{
  return GetCacheTime()+m_dwBufferLen/(2*2*48000.0);
}

double CAESinkIntelSMD::GetCacheTime()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0.0f;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "%s - inputPort == -1", __DEBUG_ID__);
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error getting port status: %d", __DEBUG_ID__, smd_ret);
    return 0;
  }

  return ((double) (curDepth * m_dwBufferLen)/(2*2*48000.0));

}

unsigned int CAESinkIntelSMD::SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size, ismd_pts_t local_pts)
{
  VERBOSE();
  ismd_result_t smd_ret;
  ismd_buffer_handle_t ismdBuffer;
  ismd_buffer_descriptor_t ismdBufferDesc;

  //printf("audio packet size %d\n", buffer_size);

  if(m_dwBufferLen < buffer_size)
  {
    CLog::Log(LOGERROR, "%s data size %d is bigger that smd buffer size %d\n", __DEBUG_ID__,
        buffer_size, m_dwBufferLen);
    return 0;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "%s - inputPort == -1", __DEBUG_ID__);
    return 0;
  }


  int counter = 0;
  while (counter < 1000)
  {
    smd_ret = ismd_buffer_alloc(m_dwBufferLen, &ismdBuffer);
    if (smd_ret != ISMD_SUCCESS)
    {
      if(g_IntelSMDGlobals.GetAudioDeviceState(m_audioDevice) != ISMD_DEV_STATE_STOP)
      {
        counter++;
        usleep(5000);
      }
      else
      {
        break;
      }
    }
    else
      break;
  }

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error allocating buffer: %d", __DEBUG_ID__, smd_ret);
    return 0;
  }

  smd_ret = ismd_buffer_read_desc(ismdBuffer, &ismdBufferDesc);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error reading descriptor: %d", __DEBUG_ID__, smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }

  short* buf_ptr = (short *) OS_MAP_IO_TO_MEM_NOCACHE(ismdBufferDesc.phys.base, buffer_size);
  if(buf_ptr == NULL)
  {
    CLog::Log(LOGERROR, "%s - unable to mmap buffer %d", __DEBUG_ID__, ismdBufferDesc.phys.base);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }

  memcpy(buf_ptr, buffer_data, buffer_size);
  OS_UNMAP_IO_FROM_MEM(buf_ptr, buffer_size);

  ismdBufferDesc.phys.level = buffer_size;

  smd_ret = ismd_buffer_update_desc(ismdBuffer, &ismdBufferDesc);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error updating descriptor: %d", __DEBUG_ID__, smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }
  counter = 0;
  while (counter < 1000)
  {
    smd_ret = ismd_port_write(m_audioDeviceInput, ismdBuffer);
    if (smd_ret != ISMD_SUCCESS)
    {
      if(g_IntelSMDGlobals.GetAudioDeviceState(m_audioDevice) != ISMD_DEV_STATE_STOP)
      {
        counter++;
        usleep(5000);
      }
      else
      {
        break;
      }
    }
    else
      break;
  }
  if(smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s failed to write buffer %d\n", __DEBUG_ID__, smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    buffer_size = 0;
  }

  return buffer_size;
}


unsigned int CAESinkIntelSMD::AddPackets(uint8_t* data, unsigned int len, bool hasAudio, bool blocking)
{
  VERBOSE();
  // Len is in frames, we want it in bytes
  len *= 4;


  // // Let it drain for a bit before we lock the pipe down
  // unsigned int curDepth = 0;
  // unsigned int maxDepth = 0;
  // g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  // if (maxDepth != 0)
  // {
  //   Sleep(10*curDepth/maxDepth);
  // }


  // What's the maximal write size - either a chunk if provided, or the full buffer
  unsigned int block_size = (m_dwChunkSize ? m_dwChunkSize : m_dwBufferLen);

//  CLog::Log(LOGDEBUG, "%s: %d %d %d %d", __DEBUG_ID__, len, block_size, m_dwChunkSize, m_dwBufferLen);

  CSingleLock lock(m_SMDAudioLock);

  ismd_pts_t local_pts = ISMD_NO_PTS;
  unsigned char* pBuffer = NULL;
  unsigned int total = 0;
  if (!m_bIsAllocated)
  {
    CLog::Log(LOGERROR,"%s - sanity failed. no valid play handle!", __DEBUG_ID__);
    return len;
  }

  pBuffer = (unsigned char*)data;
  total = len;

  //printf("len %d chunksize %d m_bTimed %d\n", len, m_dwChunkSize, m_bTimed);

  while (len)
  {
    unsigned int bytes_to_copy = len > block_size ? block_size : len;
    unsigned int dataSent = SendDataToInput(pBuffer, bytes_to_copy, local_pts);

    if(dataSent != bytes_to_copy)
    {
      CLog::Log(LOGERROR, "%s SendDataToInput failed. req %d actual %d", __DEBUG_ID__, len, dataSent);
      return 0;
    }

    pBuffer += dataSent; // Update buffer pointer
    len -= dataSent; // Update remaining data len
  }

  return (total - len)/4; // frames used
}

void CAESinkIntelSMD::Drain()
{
  VERBOSE2();
  CSingleLock lock(m_SMDAudioLock);
  g_IntelSMDGlobals.FlushAudioDevice(m_audioDevice);

  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated || m_bPause)
    return;

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "%s - inputPort == -1", __DEBUG_ID__);
    return;
  }

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s - error getting port status: %d", __DEBUG_ID__, smd_ret);
    return;
  }

  while (curDepth != 0)
  {
    usleep(5000);

    ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
    if (smd_ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "%s - error getting port status: %d", __DEBUG_ID__, smd_ret);
      return;
    }
  }
}

//void CAESinkIntelSMD::SetVolume(float nVolume)
//{
  // If we fail or if we are doing passthrough, don't set
 // if (!m_bIsAllocated )
 //   return;

//  m_fCurrentVolume = nVolume;
//  g_IntelSMDGlobals.SetMasterVolume(nVolume);
//}

bool CAESinkIntelSMD::SoftSuspend()
{
  VERBOSE2();

  // We need to wait until we're done with our packets
  CSingleLock lock(m_SMDAudioLock);

  if (!m_bIsAllocated)
    return false;

  if (m_bPause)
    return true;

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PAUSE, m_audioDevice);

  m_bPause = true;

  return true;

}

bool CAESinkIntelSMD::SoftResume()
{
  VERBOSE2();
  CSingleLock lock(m_SMDAudioLock);
  if (!m_bIsAllocated)
    return false;

  if (!m_bPause)
    return true;

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);
  m_bPause = false;

  return true;
}

ismd_audio_format_t CAESinkIntelSMD::GetISMDFormat(AEDataFormat audioMediaFormat)
{
  VERBOSE();
  ismd_audio_format_t ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;

  switch (audioMediaFormat)
    {
     case AE_FMT_U8:
     case AE_FMT_S8:
     case AE_FMT_S16BE:
     case AE_FMT_S16LE:
     case AE_FMT_S16NE:
     case AE_FMT_S32BE:
     case AE_FMT_S32LE:
     case AE_FMT_S32NE:
     case AE_FMT_S24BE4:
     case AE_FMT_S24LE4:
     case AE_FMT_S24NE4:
     case AE_FMT_S24BE3:
     case AE_FMT_S24LE3:
     case AE_FMT_S24NE3:
     case AE_FMT_LPCM:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_PCM;
       break;
     case AE_FMT_AAC:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_AAC;
       break;
     case AE_FMT_TRUEHD:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_TRUE_HD;
       break;
     case AE_FMT_DTSHD:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_HD;
       break;
     case AE_FMT_DTS:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS;
       break;
     default:
       CLog::Log(LOGERROR, "%s - unknown audio media format requested: %d", __DEBUG_ID__, audioMediaFormat);
       return ISMD_AUDIO_MEDIA_FMT_INVALID;
    }

  return ismdAudioInputFormat;
}

void CAESinkIntelSMD::ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config, int output,
    int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough)
{
  VERBOSE();
  bool bLPCMMode = false; //TODO(q) g_guiSettings.GetBool("audiooutput.lpcm71passthrough");

  CLog::Log(LOGINFO, "%s %s sample size %d sample rate %d channels %d format %d", __DEBUG_ID__,
      output == AE_DEVTYPE_HDMI ? "HDMI" : "SPDIF", sampleSize, sampleRate, channels, format);

  SetDefaultOutputConfig(output_config);

  if(sampleRate == 24000)
    sampleRate = 48000;

  if (bPassthrough)
  {
    output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
  }

  if(output == AE_DEVTYPE_HDMI)
  {
    // We need to match input to output freq on HDMI
    // We only do this if we have edid info
    // Last item - if we have audio data from EDID and are on HDMI, then validate the output
    sampleRate = 48000;
    sampleSize = 16;
    if(bLPCMMode)
    {
      if(channels == 8)
        output_config.ch_config = ISMD_AUDIO_7_1;
      else if(channels == 6)
        output_config.ch_config = ISMD_AUDIO_5_1;
    }

    if(sampleRate == 48000 || sampleRate == 96000 || sampleRate == 32000 || sampleRate == 44100
        || sampleRate == 88200 || sampleRate == 176400 || sampleRate == 192000)
      output_config.sample_rate = sampleRate;
    if(sampleSize == 16 || sampleSize == 20 || sampleSize == 24 || sampleSize == 32)
      output_config.sample_size = sampleSize;
  } // HDMI


  if(output == AE_DEVTYPE_IEC958)
  {
    if(sampleRate == 48000 || sampleRate == 96000 || sampleRate == 32000 || sampleRate == 44100 || sampleRate == 88200)
      output_config.sample_rate = sampleRate;
    if(sampleSize == 16 || sampleSize == 20 || sampleSize == 24)
      output_config.sample_size = sampleSize;
  } // SPDIF

  CLog::Log(LOGINFO, "%s stream_delay %d sample_size %d \
ch_config %d out_mode %d sample_rate %d ch_map %d",
    __DEBUG_ID__,
    output_config.stream_delay, output_config.sample_size, output_config.ch_config,
    output_config.out_mode, output_config.sample_rate, output_config.ch_map);
}

void CAESinkIntelSMD::SetDefaultOutputConfig(ismd_audio_output_config_t& output_config)
{
  VERBOSE();
  output_config.ch_config = ISMD_AUDIO_STEREO;
  output_config.ch_map = 0;
  output_config.out_mode = ISMD_AUDIO_OUTPUT_PCM;
  output_config.sample_rate = 48000;
  output_config.sample_size = 16;
  output_config.stream_delay = 0;
}

bool CAESinkIntelSMD::IsCompatible(const AEAudioFormat &format, const std::string &device)
{
  VERBOSE();
  // TODO(q)
  switch (format.m_dataFormat) {
  case AE_FMT_S16LE:
    break;
  default:
    return false;
  }
  return true;
}


void CAESinkIntelSMD::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  VERBOSE();
  // most likely TODO(q)- now messed up by quasar?
  CAEDeviceInfo info;
  info.m_channels.Reset();
  info.m_dataFormats.clear();
  info.m_sampleRates.clear();
  
  info.m_deviceType =  AE_DEVTYPE_HDMI; 
  info.m_deviceName = "IntelSMD";
  info.m_displayName = "IntelSMD";
  info.m_displayNameExtra = "HDMI/SPIDF";
  info.m_channels += AE_CH_FL;
  info.m_channels += AE_CH_FR;
  info.m_sampleRates.push_back(48000);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  info.m_dataFormats.push_back(AE_FMT_DTS);
  info.m_dataFormats.push_back(AE_FMT_AC3);
  info.m_dataFormats.push_back(AE_FMT_EAC3);
  info.m_dataFormats.push_back(AE_FMT_DTSHD);
  info.m_dataFormats.push_back(AE_FMT_TRUEHD);
  list.push_back(info);
}


#endif
