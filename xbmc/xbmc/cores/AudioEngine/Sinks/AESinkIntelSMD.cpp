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
#include "settings/GUISettings.h"
#include "settings/Settings.h"
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
#define VERBOSE2() CLog::Log(LOGDEBUG, "%s::%s", __MODULE_NAME__, __FUNCTION__)

#if 0
#define VERBOSE() VERBOSE2()
#else
#define VERBOSE()
#endif


CCriticalSection CAESinkIntelSMD::m_SMDAudioLock;

static const unsigned int s_edidRates[] = {32000,44100,48000,88200,96000,176400,192000};
static const unsigned int s_edidSampleSizes[] = {16,20,24,32};
CAESinkIntelSMD::edidHint* CAESinkIntelSMD::m_edidTable = NULL;

ismd_dev_t tmpDevice = -1;

CAESinkIntelSMD::CAESinkIntelSMD()
{
  VERBOSE2();
  m_bIsAllocated = false;
  m_bFlushFlag = true;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
  m_lastPts = ISMD_NO_PTS;
  m_lastSync = ISMD_NO_PTS;
  
  m_channelMap = NULL;
  m_uiBitsPerSample = 0;
  m_uiChannels = 0;
  m_audioMediaFormat = AE_FMT_LPCM;
  m_uiSamplesPerSec = 0;
  m_bTimed = false;
  m_bDisablePtsCorrection = false;
  m_pChunksCollected = NULL;

  m_audioDevice = -1;
  m_audioDeviceInput = -1;

  m_bIsHDMI = false;
  m_bIsSPDIF = false;
  m_bIsAnalog = false;
  m_bIsAllOutputs = false;
  m_fCurrentVolume = 0;
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

  static enum AEChannel map[3] = {AE_CH_FL, AE_CH_FR , AE_CH_NULL};
  format.m_dataFormat = AE_FMT_S16LE;
  format.m_sampleRate = 48000;
  format.m_channelLayout = CAEChannelInfo(map);
  format.m_frameSize = 2*2;
  format.m_frameSamples = 8192; // TODO(q)
  format.m_frames = 8192;

  m_dwChunkSize = 8192;
  m_dwBufferLen = 8192;

  ismd_audio_processor_t  audioProcessor = -1;
  ismd_audio_format_t     ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;


  bool bAC3pass = g_guiSettings.GetBool("audiooutput.ac3passthrough");
  bool bEAC3pass = g_guiSettings.GetBool("audiooutput.eac3passthrough");
  bool bTruehdPass = g_guiSettings.GetBool("audiooutput.truehdpassthrough");
  bool bDTSPass = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  bool bDTSHDPass = g_guiSettings.GetBool("audiooutput.dtshdpassthrough");
  bool bUseEDID = g_guiSettings.GetBool("videoscreen.forceedid");

  bool bHDMIPassthrough = false;
  bool bSPDIFPassthrough = false;

  int audioOutputMode = g_guiSettings.GetInt("audiooutput.mode");
  bool bCanDDEncode = false;
  int inputChannelConfig = AUDIO_CHAN_CONFIG_2_CH;

  // TODO(q)
  m_bIsHDMI = true;   //(AUDIO_HDMI == audioOutputMode);
  m_bIsSPDIF = true;  //(AUDIO_IEC958 == audioOutputMode);
  m_bIsAnalog = true; //(AUDIO_ANALOG == audioOutputMode);
  m_bIsAllOutputs = false; //(audioOutputMode == AUDIO_ALL_OUTPUTS);
  m_bAC3Encode = false;

  bool bIsUIAudio = false; //strName.Equals("gui", false);
  bool bIsDVB = false; //strName.Equals("dvb", false);
  bool bHardwareDecoder = false;

  audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if(audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize audioProcessor is not valid");
    return false;
  }

  // disable all outputs
  if(m_bIsHDMI)
    g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  if(m_bIsSPDIF)
    g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  if(m_bIsAnalog)
    g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  if(bUseEDID)
  {
    UnloadEDID();
    (void)LoadEDID();
  }

//  m_bTimed = bTimed;

  m_audioDevice = g_IntelSMDGlobals.CreateAudioInput(m_bTimed);
  if(m_audioDevice == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize failed to create audio input");
    return false;
  }

  if(!bIsUIAudio)
    g_IntelSMDGlobals.SetPrimaryAudioDevice(m_audioDevice);

  m_audioDeviceInput = g_IntelSMDGlobals.GetAudioDevicePort(m_audioDevice);
  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize failed to create audio input port");
    return false;
  }

  // This function will prevent bad PTS packets from dropping.
  // The tradeoff is AV sync.
  if (m_bTimed && !bIsDVB)
  {
    int ahead = 40;
    int behind = 80;
    result = ismd_audio_input_set_timing_accuracy(audioProcessor, m_audioDevice,
        ahead, behind);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGWARNING, "CIntelSMDGlobals::InitAudio ismd_audio_input_set_timing_accuracy failed %d", result);
    }
  }

  // Can we do DD+ -> DD?
  if (ISMD_SUCCESS == ismd_audio_codec_available((ismd_audio_format_t) ISMD_AUDIO_ENCODE_FMT_AC3))
    bCanDDEncode = true;

  ismdAudioInputFormat = GetISMDFormat(format.m_dataFormat);

  // Can we do hardware decode?
  bHardwareDecoder = (ismd_audio_codec_available(ismdAudioInputFormat) == ISMD_SUCCESS);
  if(ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_PCM)
    bHardwareDecoder = false;

  int uiBitsPerSample = 16;
  CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize ismdAudioInputFormat %d Hardware decoding (non PCM) %d\n",
      ismdAudioInputFormat, bHardwareDecoder);

  int counter = 0;
  while(counter < 5)
  {
    result = ismd_audio_input_set_data_format(audioProcessor, m_audioDevice, ismdAudioInputFormat);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "ismd_audio_input_set_data_format failed. retrying %d %d", counter, result);
      counter++;
      Sleep(100);
    }
    else
      break;
  }

  // CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize PCM sample size %d sample rate %d channel config 0x%8x",
  //         uiBitsPerSample, uiSamplesPerSec, inputChannelConfig);
  result = ismd_audio_input_set_pcm_format(audioProcessor, m_audioDevice, uiBitsPerSample, format.m_sampleRate, inputChannelConfig);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize - ismd_audio_input_set_pcm_format: %d", result);
    return false;
  }


  if(m_bIsAllOutputs)
  {
    bHDMIPassthrough = bSPDIFPassthrough = false;
  }

  if (bHDMIPassthrough || bSPDIFPassthrough)
  {
    m_dwBufferLen = 8 * 1024;
    m_dwChunkSize = 0;
  }

  CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize HDMI passthrough %d SPDIF passthrough %d buffer size %d chunk size %d",
      bHDMIPassthrough, bSPDIFPassthrough, m_dwBufferLen, m_dwChunkSize);

  // Configure output ports
  if(bIsUIAudio)
  {
    format.m_sampleRate = 48000;
    format.m_dataFormat = AE_FMT_S16LE;
    uiBitsPerSample = 16;
  }

  unsigned int outputFreq = 48000;

  // I2S. Nothing to touch here. we always use defaults

  // SPDIF
  if(m_bIsSPDIF)
  {
    ismd_audio_output_t OutputSPDIF = g_IntelSMDGlobals.GetSPDIFOutput();
    ismd_audio_output_config_t spdif_output_config;
    unsigned int samplesPerSec = format.m_sampleRate;
    unsigned int bitsPerSec = uiBitsPerSample;
    if(m_bIsAllOutputs)
    {
      samplesPerSec = 48000;
      bitsPerSec = 16;
    }

    ConfigureAudioOutputParams(spdif_output_config, AUDIO_IEC958, bitsPerSec,
        samplesPerSec, format.m_channelLayout.Count(), ismdAudioInputFormat, bSPDIFPassthrough);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputSPDIF, spdif_output_config))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize ConfigureAudioOutput SPDIF failed %d", result);
//      return false;
    }
    outputFreq = spdif_output_config.sample_rate;
  }

  // HDMI
  if(m_bIsHDMI && !m_bIsAllOutputs)
  {
    ismd_audio_output_t OutputHDMI = g_IntelSMDGlobals.GetHDMIOutput();
    ismd_audio_output_config_t hdmi_output_config;
    ConfigureAudioOutputParams(hdmi_output_config, AUDIO_HDMI, uiBitsPerSample,
      format.m_sampleRate, format.m_channelLayout.Count(), ismdAudioInputFormat, bHDMIPassthrough);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputHDMI, hdmi_output_config))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize ConfigureAudioOutput HDMI failed %d", result);
//      return false;
    }
    outputFreq = hdmi_output_config.sample_rate;
  }

  // Configure the master clock frequency

  if(m_bIsAllOutputs)
    outputFreq = 48000;

  CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize ConfigureMasterClock %d", outputFreq);
  g_IntelSMDGlobals.ConfigureMasterClock(outputFreq);

  if(bIsUIAudio)
      ismd_audio_input_set_as_secondary(audioProcessor, m_audioDevice);
  else /*TODO(q) if(!bIsMusic || (bIsMusic && (bHDMIPassthrough || bSPDIFPassthrough)))*/
  {
    ismd_audio_input_pass_through_config_t passthrough_config;
    if (bHDMIPassthrough || bSPDIFPassthrough)
      passthrough_config.is_pass_through = true;
    else
      passthrough_config.is_pass_through = false;
    passthrough_config.dts_or_dolby_convert = false;
    passthrough_config.supported_format_count = 1;
    passthrough_config.supported_formats[0] = ismdAudioInputFormat;

    result = ismd_audio_input_set_as_primary(audioProcessor, m_audioDevice, passthrough_config);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  ismd_audio_input_set_as_primary failed %d", result);
//      return false;
    }
  }

  if(!g_IntelSMDGlobals.EnableAudioInput(m_audioDevice))
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioInput");
//    return false;
  }

  // enable outputs
  if (m_bIsHDMI)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput()))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput HDMI failed");
//      return false;
    }
  }

  if (m_bIsSPDIF)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput()))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput SPDIF failed");
//      return false;
    }
  }

  if (m_bIsAnalog)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput()))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput I2S failed");
//      return false;
    }
  }

  if(!m_bTimed)
  {
    g_IntelSMDGlobals.SetBaseTime(0);
    g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);
    m_bFlushFlag = false;
  }
  else
  {
    //g_IntelSMDGlobals.SetAudioDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime(), m_audioDevice);
    g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PAUSE, m_audioDevice);
    m_bFlushFlag = true;
  }

  m_fCurrentVolume = g_settings.m_fVolumeLevel;
  g_IntelSMDGlobals.SetMasterVolume(m_fCurrentVolume);

  if(m_pChunksCollected)
    delete m_pChunksCollected;

  m_bPause = false;
  m_pChunksCollected = new unsigned char[m_dwBufferLen];
  m_first_pts = ISMD_NO_PTS;
  m_lastPts = ISMD_NO_PTS;
  m_ChunksCollectedSize = 0;

  m_bIsAllocated = true;

  CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize done");

  return true;
}

void CAESinkIntelSMD::Deinitialize()
{
  VERBOSE2();
  CSingleLock lock(m_SMDAudioLock);
  CLog::Log(LOGINFO, "CAESinkIntelSMD::Deinitialize");

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
  if (m_bIsHDMI)
    g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  if (m_bIsSPDIF)
    g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  if (m_bIsAnalog)
    g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  m_bIsAllocated = false;

  return;
}

double CAESinkIntelSMD::GetDelay()
{
  return 0.0f;
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
    CLog::Log(LOGERROR, "CAESinkIntelSMD::GetCacheTime - inputPort == -1");
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::GetCacheTime - error getting port status: %d", smd_ret);
    return 0;
  }

  float result = ((float) (curDepth * m_dwBufferLen));

  return result;
}

unsigned int CAESinkIntelSMD::SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size, ismd_pts_t local_pts)
{
  VERBOSE();
  ismd_result_t smd_ret;
  ismd_buffer_handle_t ismdBuffer;
  ismd_es_buf_attr_t *buf_attrs;
  ismd_buffer_descriptor_t ismdBufferDesc;

  //printf("audio packet size %d\n", buffer_size);

  if(m_dwBufferLen < buffer_size)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput data size %d is bigger that smd buffer size %d\n",
        buffer_size, m_dwBufferLen);
    return 0;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - inputPort == -1");
    return 0;
  }

  smd_ret = ismd_buffer_alloc(m_dwBufferLen, &ismdBuffer);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - error allocating buffer: %d", smd_ret);
    return 0;
  }

  smd_ret = ismd_buffer_read_desc(ismdBuffer, &ismdBufferDesc);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - error reading descriptor: %d", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }

  unsigned char* buf_ptr = (uint8_t *) OS_MAP_IO_TO_MEM_NOCACHE(ismdBufferDesc.phys.base, buffer_size);
  if(buf_ptr == NULL)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - unable to mmap buffer %d", ismdBufferDesc.phys.base);
    ismd_buffer_dereference(ismdBuffer);
    return 0;    
  }

  memcpy(buf_ptr, buffer_data, buffer_size);
  OS_UNMAP_IO_FROM_MEM(buf_ptr, buffer_size);

  ismdBufferDesc.phys.level = buffer_size;

  buf_attrs = (ismd_es_buf_attr_t *) ismdBufferDesc.attributes;
  buf_attrs->original_pts = local_pts;
  buf_attrs->local_pts = local_pts;
  buf_attrs->discontinuity = false;

  smd_ret = ismd_buffer_update_desc(ismdBuffer, &ismdBufferDesc);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - error updating descriptor: %d", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }

  int counter = 0;
  while (counter < 20)
  {
    smd_ret = ismd_port_write(m_audioDeviceInput, ismdBuffer);
    if (smd_ret != ISMD_SUCCESS)
    {
      if(g_IntelSMDGlobals.GetAudioDeviceState(m_audioDevice) != ISMD_DEV_STATE_STOP)
      {
        counter++;
        Sleep(100);
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
    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput failed to write buffer %d\n", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    buffer_size = 0;
  }

  return buffer_size;
}


unsigned int CAESinkIntelSMD::AddPackets(uint8_t* data, unsigned int len, bool hasAudio)
{
  VERBOSE();
  // What's the maximal write size - either a chunk if provided, or the full buffer
  unsigned int block_size = (m_dwChunkSize ? m_dwChunkSize : m_dwBufferLen);

//  CLog::Log(LOGDEBUG, "CAESinkIntelSMD::AddPackets: %d %d %d %d %d", len, block_size, m_dwChunkSize, m_dwBufferLen, g_IntelSMDGlobals.IsDemuxToAudio());

  CSingleLock lock(m_SMDAudioLock);

  ismd_pts_t local_pts = ISMD_NO_PTS;
  unsigned char* pBuffer = NULL;
  unsigned int total = 0;
  unsigned int dataSent = 0;
  if (!m_bIsAllocated)
  {
    CLog::Log(LOGERROR,"CAESinkIntelSMD::AddPackets - sanity failed. no valid play handle!");
    return len;
  }

  if (m_bTimed)
  {
// TODO(q)
//    local_pts = g_IntelSMDGlobals.DvdToIsmdPts(pts);
  }

  if(m_bFlushFlag && m_bTimed)
  {
    ismd_pts_t startPTS;

    if(g_IntelSMDGlobals.GetAudioStartPts() != ISMD_NO_PTS)
      startPTS = g_IntelSMDGlobals.GetAudioStartPts();
    else
      startPTS = local_pts;

    if(g_IntelSMDGlobals.GetFlushFlag())
    {
      g_IntelSMDGlobals.SetBaseTime(g_IntelSMDGlobals.GetCurrentTime());
      g_IntelSMDGlobals.SetFlushFlag(false);
    }

    g_IntelSMDGlobals.SetAudioStartPts(startPTS);
    g_IntelSMDGlobals.SetAudioDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime(), m_audioDevice);
    g_IntelSMDGlobals.SendStartPacket(startPTS, m_audioDeviceInput);
    g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);

    // data for small chunks collection
    m_ChunksCollectedSize = 0;
    m_bFlushFlag = false;
  }

  pBuffer = (unsigned char*)data;
  total = len;

  //printf("len %d chunksize %d m_bTimed %d\n", len, m_dwChunkSize, m_bTimed);

  while (len && len >= m_dwChunkSize) // We want to write at least one chunk at a time
  {
    unsigned int bytes_to_copy = len > block_size ? block_size : len;

//    printf("bytes_to_copy %d block size %d m_ChunksCollectedSize %d\n", bytes_to_copy, block_size, m_ChunksCollectedSize);

    // If we've written a frame with this pts, don't write another
    if(local_pts == m_lastPts)
    {
      local_pts = ISMD_NO_PTS;
    }

    if(m_ChunksCollectedSize == 0)
      m_first_pts = local_pts;

    // collect data to avoid small chunks
    if (m_bTimed)
    {
      if (m_ChunksCollectedSize + bytes_to_copy <= m_dwBufferLen)
      {
        memcpy(m_pChunksCollected + m_ChunksCollectedSize, pBuffer,
            bytes_to_copy);
      }
      else
      {
        //printf("Audio Packet: PTS %lld (0x%09lx) size %d\n", m_first_pts, m_first_pts, m_ChunksCollectedSize);
        SendDataToInput(m_pChunksCollected, m_ChunksCollectedSize, m_first_pts);
        memcpy(m_pChunksCollected, pBuffer, bytes_to_copy);
        m_ChunksCollectedSize = 0;
        m_first_pts = local_pts;
      }
      m_ChunksCollectedSize += bytes_to_copy;
      dataSent = bytes_to_copy;
    }
    else
    {
      dataSent = SendDataToInput(pBuffer, bytes_to_copy, local_pts);
    }

    if(dataSent != bytes_to_copy)
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::AddPackets SendDataToInput failed. req %d actual %d", len, dataSent);
      return 0;
    }

    pBuffer += dataSent; // Update buffer pointer
    len -= dataSent; // Update remaining data len

    if( local_pts != ISMD_NO_PTS )
      m_lastPts = local_pts;
  }

  return total - len; // Bytes used
}

void CAESinkIntelSMD::Drain()
{
  VERBOSE();
  CSingleLock lock(m_SMDAudioLock);
  g_IntelSMDGlobals.FlushAudioDevice(m_audioDevice);

  if(m_bTimed)
    m_bFlushFlag = true;

  m_lastPts = ISMD_NO_PTS;
  m_lastSync = ISMD_NO_PTS;

  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated || m_bPause)
    return;

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::GetSpace - inputPort == -1");
    return;
  }


  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::WaitCompletion - error getting port status: %d", smd_ret);
    return;
  }

  while (curDepth != 0)
  {
    usleep(5000);

    ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
    if (smd_ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::WaitCompletion - error getting port status: %d", smd_ret);
      return;
    }
  } 
}

void CAESinkIntelSMD::SetVolume(float nVolume)
{
  // If we fail or if we are doing passthrough, don't set
  if (!m_bIsAllocated )
    return;

  m_fCurrentVolume = nVolume;
  g_IntelSMDGlobals.SetMasterVolume(nVolume);
}

bool CAESinkIntelSMD::SoftSuspend()
{
  VERBOSE();
   CLog::Log(LOGDEBUG, "CAESinkIntelSMD %s\n", __FUNCTION__);

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
  VERBOSE();
  CSingleLock lock(m_SMDAudioLock);
  if (!m_bIsAllocated)
    return false;

  if(!m_bFlushFlag)
  {
    g_IntelSMDGlobals.SetAudioDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime(), m_audioDevice);
    g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);
  }
  else
    CLog::Log(LOGINFO, "CAESinkIntelSMD::Resume flush flag is set, ignoring request\n");

  m_bPause = false;

  return true;
}



// Based on ismd examples
bool CAESinkIntelSMD::LoadEDID()
{
  VERBOSE();
  bool ret = false;
  gdl_hdmi_audio_ctrl_t ctrl;
  edidHint** cur = &m_edidTable;

  // Set up our control
  ctrl.cmd_id = GDL_HDMI_AUDIO_GET_CAPS;
  ctrl.data._get_caps.index = 0;

  while( gdl_port_recv(GDL_PD_ID_HDMI, GDL_PD_RECV_HDMI_AUDIO_CTRL, &ctrl, sizeof(ctrl)) == GDL_SUCCESS )
  {
    edidHint* hint = new edidHint;
    if( !hint ) return false;

    ret = true;

    hint->format = MapGDLAudioFormat( (gdl_hdmi_audio_fmt_t)ctrl.data._get_caps.cap.format );
    if( ISMD_AUDIO_MEDIA_FMT_INVALID == hint->format )
    {
      delete hint;
      ctrl.data._get_caps.index++;
      continue;
    }
    else
    {
      // For any formats we support bitstreaming on, flag them in the smdglobals here
      // TODO(q): Everything was already commented out??
/*  
      switch( hint->format )
      {
        case ISMD_AUDIO_MEDIA_FMT_DD:
          //g_IntelSMDGlobals.SetCodecHDMIBitstream( CODEC_ID_AC3, true);
          break;
        case ISMD_AUDIO_MEDIA_FMT_DD_PLUS:
          //g_IntelSMDGlobals.SetCodecHDMIBitstream( CODEC_ID_EAC3, true );
          break;
        case ISMD_AUDIO_MEDIA_FMT_TRUE_HD:
          //g_IntelSMDGlobals.SetCodecHDMIBitstream( CODEC_ID_TRUEHD, true );
          break;
        case ISMD_AUDIO_MEDIA_FMT_DTS:
          //g_IntelSMDGlobals.SetCodecHDMIBitstream( CODEC_ID_DTS, true );
          break;
        case ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA:
          //g_IntelSMDGlobals.SetCodecHDMIBitstream( SMD_CODEC_DTSHD, true );
          break;
        default:
          break;
      };
*/
    }
    
    hint->channels = (int)ctrl.data._get_caps.cap.max_channels;
    hint->sample_rates = (unsigned char) (ctrl.data._get_caps.cap.fs & 0x7f);
    if( ISMD_AUDIO_MEDIA_FMT_PCM == hint->format )
    {
      hint->sample_sizes = (ctrl.data._get_caps.cap.ss_bitrate & 0x07);
      if( hint->sample_sizes & 0x04 )
        hint->sample_sizes |= 0x08;  // if we support 24, we support 32.
    }
    else
      hint->sample_sizes = 0;

    *cur = hint;
    cur = &( hint->next );

    // get the next block
    ctrl.data._get_caps.index++;
  }

  *cur = NULL;

  DumpEDID();

  return ret;
}
void CAESinkIntelSMD::UnloadEDID()
{
  VERBOSE();
  while( m_edidTable )
  {
    edidHint* nxt = m_edidTable->next;
    delete[] m_edidTable;
    m_edidTable = nxt;
  }
}
void CAESinkIntelSMD::DumpEDID()
{
  VERBOSE();
  const char* formats[] = {"invalid","LPCM","DVDPCM","BRPCM","MPEG","AAC","AACLOAS","DD","DD+","TrueHD","DTS-HD","DTS-HD-HRA","DTS-HD-MA","DTS","DTS-LBR","WM9"};
  CLog::Log(LOGINFO,"HDMI Audio sink supports the following formats:\n");
  for( edidHint* cur = m_edidTable; cur; cur = cur->next )
  {
    CStdString line;
    line.Format("* Format: %s Max channels: %d Samplerates: ", formats[cur->format], cur->channels);

    for( unsigned int z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( cur->sample_rates & (1<<z) )
      {
        line.AppendFormat("%d ",s_edidRates[z]);
      }         
    }
    if( ISMD_AUDIO_MEDIA_FMT_PCM == cur->format )
    {
      line.AppendFormat("Sample Sizes: ");
      for( unsigned int z = 0; z< sizeof(s_edidSampleSizes); z++ )
      {
        if( cur->sample_sizes & (1<<z) )
        {
          line.AppendFormat("%d ",s_edidSampleSizes[z]);
        }
      }
    }
    CLog::Log(LOGINFO, "%s", line.c_str());
  }
}

bool CAESinkIntelSMD::CheckEDIDSupport( ismd_audio_format_t format, int& iChannels, unsigned int& uiSampleRate, unsigned int& uiSampleSize )
{
  VERBOSE();
  //
  // If we have EDID data then see if our settings are viable
  // We have to match the format. After that, we prefer to match channels, then sample rate, then sample size
  // The result is if we have a 7.1 8 channel 192khz stream and have to pick between 2 ch at 192khz or 8 ch at
  // 48khz, we'll do the latter, and keep the surround experience for the user.
  //

  edidHint* cur = NULL;
  unsigned int suggSampleRate = uiSampleRate;
  unsigned int suggSampleSize = uiSampleSize;
  int suggChannels = iChannels;
  bool bFormatSupported = false;
  bool bFullMatch = false;

  if( !m_edidTable )
    return false;

  for( cur = m_edidTable; cur; cur = cur->next )
  {
    bool bSuggested = false;

    // Check by format and channels
    if( cur->format != format )
      continue;

    bFormatSupported = true;

    // Right format. See if the proper channel count is reflected
    // if not, we'll suggest a resample
    // if so, we update our suggestions to reflect this format entry
    if( cur->channels < iChannels )
    {
      suggChannels = cur->channels;
      bSuggested = true;
    }
    else
    {
      suggChannels = iChannels;
      suggSampleRate = uiSampleRate;
      suggSampleSize = uiSampleSize;
    }

    // Next, see if the sample rate is supported
    for( unsigned int z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( s_edidRates[z] == uiSampleRate )
      {
        if( !(cur->sample_rates & (1<<z)) )
        {
          // For now, try force resample to 48khz and worry if that isn't supported
          if( !(cur->sample_rates & 4) )
            CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize - your audio sink indicates it doesn't support 48khz"); 
          suggSampleRate = 48000;
          bSuggested = true;
        }
        break;
      }
    }

    // Last, see if the sample size is supported for PCM
    if( ISMD_AUDIO_MEDIA_FMT_PCM == format )
    {
      for( unsigned int y = 0; y < sizeof(s_edidSampleSizes); y++ )
      {
        if( s_edidSampleSizes[y] == uiSampleSize )
        {
          if( !(cur->sample_sizes & (1<<y)) )
          {
            // For now, try force resample to 16khz and worry if that isn't supported
            if( !(cur->sample_rates & 1) )
              CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize - your audio sink indicates it doesn't support 16bit/sample");
            suggSampleSize = 16;
            bSuggested = true;
          }
          break;
        }
      }
    }

    // If we didn't make any suggestions, then this was a match; exit out
    if( !bSuggested )
    {
      bFullMatch = true;
      break;
    }
  }

  if( bFormatSupported )
  {
    if( bFullMatch )
    {
      return true;
    }
    else
    {
      uiSampleRate = suggSampleRate;
      uiSampleSize = suggSampleSize;
      iChannels = suggChannels;
    }
  }
  return false;
}

//static
ismd_audio_format_t CAESinkIntelSMD::MapGDLAudioFormat( gdl_hdmi_audio_fmt_t f )
{
  VERBOSE();
  ismd_audio_format_t result = ISMD_AUDIO_MEDIA_FMT_INVALID;
  switch(f)
  {
    case GDL_HDMI_AUDIO_FORMAT_PCM:
      result = ISMD_AUDIO_MEDIA_FMT_PCM;
      break;
    case GDL_HDMI_AUDIO_FORMAT_MPEG1:
    case GDL_HDMI_AUDIO_FORMAT_MPEG2:
    case GDL_HDMI_AUDIO_FORMAT_MP3:
      result = ISMD_AUDIO_MEDIA_FMT_MPEG;
      break;
    case GDL_HDMI_AUDIO_FORMAT_AAC:
      result = ISMD_AUDIO_MEDIA_FMT_AAC;
      break;
    case GDL_HDMI_AUDIO_FORMAT_DTS:
      result = ISMD_AUDIO_MEDIA_FMT_DTS;
      break;
    case GDL_HDMI_AUDIO_FORMAT_AC3:
      result = ISMD_AUDIO_MEDIA_FMT_DD;
      break;
    case GDL_HDMI_AUDIO_FORMAT_DDP:
      result = ISMD_AUDIO_MEDIA_FMT_DD_PLUS;
      break;
    case GDL_HDMI_AUDIO_FORMAT_DTSHD:
      result = ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA;
      break;
    case GDL_HDMI_AUDIO_FORMAT_MLP:
      result = ISMD_AUDIO_MEDIA_FMT_TRUE_HD;
      break;
    case GDL_HDMI_AUDIO_FORMAT_WMA_PRO:
      result = ISMD_AUDIO_MEDIA_FMT_WM9;
      break;
    default:
      break;

  };
  return result;
}

void CAESinkIntelSMD::ConfigureDolbyModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
  VERBOSE();
  ismd_result_t result;
  ismd_audio_decoder_param_t param;

  // Dolby DD is downmix to Lt/Rt and not Lo/Ro
  ismd_audio_ac3_stereo_output_mode_t *stereo_mode_dd = (ismd_audio_ac3_stereo_output_mode_t *)&param;
  *stereo_mode_dd = ISMD_AUDIO_AC3_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE;
  CLog::Log(LOGNONE, "ConfigureDolbyModes ISMD_AUDIO_AC3_STEREO_OUTPUT_MODE %d", *stereo_mode_dd);
  result = ismd_audio_input_set_decoder_param( proc_handle, input_handle,
      ISMD_AUDIO_AC3_STEREO_OUTPUT_MODE, &param );

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyModes ISMD_AUDIO_AC3_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE failed %d",
        result);
  }

  // DD works in 2 channels output mode
  ismd_audio_ac3_num_output_channels_t *num_of_ch = (ismd_audio_ac3_num_output_channels_t *) &param;
  *num_of_ch = 2;
  CLog::Log(LOGNONE, "ConfigureDolbyModes ISMD_AUDIO_AC3_NUM_OUTPUT_CHANNELS %d", *num_of_ch);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_NUM_OUTPUT_CHANNELS, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyModes ISMD_AUDIO_AC3_NUM_OUTPUT_CHANNELS failed %d", result);
  }

  // Output configuration for DD
  ismd_audio_ac3_output_configuration_t *output_config = (ismd_audio_ac3_output_configuration_t *) &param;
  *output_config = ISMD_AUDIO_AC3_OUTPUT_CONFIGURATION_2_0_LR;
  CLog::Log(LOGNONE, "ConfigureDolbyModes ISMD_AUDIO_AC3_OUTPUT_CONFIGURATION_2_0_LR %d", *output_config);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_OUTPUT_CONFIGURATION, &param);
  if (result != ISMD_SUCCESS)
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyModes ISMD_AUDIO_AC3_NUM_OUTPUT_CHANNELS failed %d", result);

  // Configure DD channels layout
  ismd_audio_ac3_channel_route_t *channel_route = (ismd_audio_ac3_channel_route_t *) &param;
  result = ISMD_SUCCESS;

  *channel_route = 0;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_CHANNEL_ROUTE_L, &param);
  *channel_route = 1;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_CHANNEL_ROUTE_R, &param);
  *channel_route = -1;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,   ISMD_AUDIO_AC3_CHANNEL_ROUTE_Ls, &param);
  *channel_route = -1;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,   ISMD_AUDIO_AC3_CHANNEL_ROUTE_Rs, &param);
  *channel_route = -1;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_CHANNEL_ROUTE_C, &param);
  *channel_route = -1;
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_AC3_CHANNEL_ROUTE_LFE, &param);

  if(result != ISMD_SUCCESS)
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyModes ISMD_AUDIO_AC3_CHANNEL_ROUTE_* failed %d", result);
}

void CAESinkIntelSMD::ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode)
{
  VERBOSE();
  ismd_result_t result;
  ismd_audio_decoder_param_t param;

  //DD+ we need to operate in 5.1 decoding mode and avoid decoding extension channels.
  ismd_audio_ddplus_num_output_channels_t *num_of_ch = (ismd_audio_ddplus_num_output_channels_t *) &param;
  if(bAC3Encode)
    *num_of_ch = ISMD_AUDIO_DDPLUS_MAX_NUM_OUTPUT_CHANNELS;
  else
    *num_of_ch = 6;
  CLog::Log(LOGNONE, "ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS %d", *num_of_ch);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_NUM_OUTPUT_CHANNELS failed %d", result);
  }

  ismd_audio_ddplus_lfe_channel_output_t *lfe_control = (ismd_audio_ddplus_lfe_channel_output_t *) &param;
  if(bAC3Encode)
    *lfe_control = ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT_ENABLED;
  else
    *lfe_control = (ismd_audio_ddplus_lfe_channel_output_t)g_guiSettings.GetInt("audiooutput.ddplus_lfe_channel_config");
  CLog::Log(LOGNONE, "ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT %d", *lfe_control);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_LFE_CHANNEL_OUTPUT failed %d", result);
  }

  // Dolby DD+ is downmix to Lt/Rt and not Lo/Ro
  ismd_audio_ddplus_stereo_output_mode_t *stereo_mode_ddplus = (ismd_audio_ddplus_stereo_output_mode_t *) &param;
  *stereo_mode_ddplus = ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE;
  CLog::Log(LOGNONE, "ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE %d", *stereo_mode_ddplus);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE, &param);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_STEREO_OUTPUT_MODE_SURROUND_COMPATIBLE failed %d", result);
  }

  ismd_audio_ddplus_output_configuration_t *output_config_ddplus = (ismd_audio_ddplus_output_configuration_t *) &param;
  *output_config_ddplus = (ismd_audio_ddplus_output_configuration_t)g_guiSettings.GetInt("audiooutput.ddplus_output_config");
  if(bAC3Encode) // if the output will be rencode to AC3, output all 5 channels
    *output_config_ddplus = ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_1_LCRlrTs;
  CLog::Log(LOGNONE, "ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION %d", *output_config_ddplus);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION, &param);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION failed %d", result);
  }
}

void CAESinkIntelSMD::ConfigureDolbyTrueHDModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
  VERBOSE();
  ismd_result_t result;
  ismd_audio_decoder_param_t param;

  int drcmode = g_guiSettings.GetInt("audiooutput.dd_truehd_drc");
  int drc_percent = g_guiSettings.GetInt("audiooutput.dd_truehd_drc_percentage");

  //Dolby TrueHD DRC is on by default. We require the ability to set it off and auto mode.
  // This command sets the DRC mode: 0 off, 1 auto (default), 2 forced.
  ismd_audio_truehd_drc_enable_t *drc_control =
      (ismd_audio_truehd_drc_enable_t *) &param;

  // TODO(q)
  // if (drcmode == DD_TRUEHD_DRC_AUTO)
  //   *drc_control = 1; // Set to auto
  // else if (drcmode == DD_TRUEHD_DRC_OFF)
  //   *drc_control = 0;
  // else if (drcmode == DD_TRUEHD_DRC_ON)
  //   *drc_control = 2;
  CLog::Log(LOGNONE, "ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_ENABLE %d", *drc_control);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
      ISMD_AUDIO_TRUEHD_DRC_ENABLE, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_ENABLE failed %d", result);
  }

  // TODO(q)
  // if (drcmode == DD_TRUEHD_DRC_ON)
  // {
  //   ismd_audio_truehd_drc_boost_t *dobly_drc_percent = (ismd_audio_truehd_drc_enable_t *) &param;
  //   *dobly_drc_percent = drc_percent;

  //   CLog::Log(LOGNONE, "ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_BOOST %d", *dobly_drc_percent);
  //   result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
  //       ISMD_AUDIO_TRUEHD_DRC_BOOST, &param);
  //   if (result != ISMD_SUCCESS)
  //   {
  //     CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_BOOST failed %d", result);
  //   }
  // }

  //Dolby TrueHD 2 channels presentation decoding. We need to decode this properly.
  //Instead we decode the multi-channel presentation and downmix to 2 channels.

  /* This command sets the decode_mode and configures the decoder as a two-, six- or eight-channel decoder. Valid values are 2, 6, and 8 (default)*/
  ismd_audio_truehd_decode_mode_t *decode_control =
      (ismd_audio_truehd_decode_mode_t *) &param;
  *decode_control = 2; // Set to 2 channel
  CLog::Log(LOGNONE, "ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DECODE_MODE %d", *decode_control);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
      ISMD_AUDIO_TRUEHD_DECODE_MODE, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DECODE_MODE failed %d", result);
  }

}

void CAESinkIntelSMD::ConfigureDTSModes(
    ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
  VERBOSE();
  ismd_result_t result;
  ismd_audio_decoder_param_t dec_param;


  //please set downmix_mode as ISMD_AUDIO_DTS_DOWNMIX_MODE_INTERNAL
  /* Valid values: "ISMD_AUDIO_DTS_DOWNMIX_MODE_EXTERNAL" | "ISMD_AUDIO_DTS_DOWNMIX_MODE_INTERNAL" */
  {
    /* Set the downmix mode parameter for DTS core decoder. */
    ismd_audio_dts_downmix_mode_t *param = (ismd_audio_dts_downmix_mode_t *) &dec_param;
    *param = ISMD_AUDIO_DTS_DOWNMIX_MODE_INTERNAL;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_DOWNMIXMODE %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_DOWNMIXMODE, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "Setting the downmix mode on DTS CORE decoder failed %d", result);
    }
  }

  //please set speaker_out as 2   /* Please see ismd_audio_dts for acceptable values.(0 = default spkr out)*/
  {
    /* Set the Speaker output configuration of the DTS core decoder. */
    ismd_audio_dts_speaker_output_configuration_t *param = (ismd_audio_dts_speaker_output_configuration_t *) &dec_param;
    *param = 2;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_SPKROUT %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_SPKROUT, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "Setting the speaker output mode on DTS CORE decoder failed %d", result);
    }
  }
  // please set dynamic_range_percent as 0  /* Valid values: 0 - 100*/
  {
    /* Set the DRC percent of the DTS core decoder. */
    ismd_audio_dts_dynamic_range_compression_percent_t *param = (ismd_audio_dts_dynamic_range_compression_percent_t *) &dec_param;
    *param = 0;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_DRCPERCENT %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_DRCPERCENT, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,"Setting the DRC percent on DTS CORE decoder failed %d", result);
    }
  }

  // please set number_output_channels as 2  /* Valid values:  Specify number of output channels 1 - 6.*/
  {
    /* Set the number of output channels of the DTS core decoder. */
    ismd_audio_dts_num_output_channels_t *param = (ismd_audio_dts_num_output_channels_t *) &dec_param;
    *param = 2;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_NUM_OUTPUT_CHANNELS %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_NUM_OUTPUT_CHANNELS, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,"Setting the num output channels on DTS CORE decoder failed %d", result);
    }
  }
  // please set lfe_downmix as ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE
  /* Valid values: "ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE" | "ISMD_AUDIO_DTS_LFE_DOWNMIX_10DB" */
  {
    /* Set the LFE downmix setting of the DTS core decoder. */
    int *param = (int *) &dec_param;
    *param = ISMD_AUDIO_DTS_LFE_DOWNMIX_NONE;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_LFEDNMX %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_LFEDNMX, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR,"Setting the LFE downmix on DTS CORE decoder failed %d", result);
    }
  }

  // please set sec_audio_scale as 0      /* Valid values: 0 | 1 */
  {
    /* Set the LFE downmix setting of the DTS core decoder. */
    ismd_audio_dts_secondary_audio_scale_t *param = (ismd_audio_dts_secondary_audio_scale_t *) &dec_param;
    *param = 0;
    CLog::Log(LOGNONE, "ConfigureDTSModes ISMD_AUDIO_DTS_SEC_AUD_SCALING %d", *param);
    if ((result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_DTS_SEC_AUD_SCALING, &dec_param)) != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "Setting the secondary audio scaling on DTS CORE decoder failed %d", result);
    }
  }
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
       CLog::Log(LOGERROR, "CAESinkIntelSMD::GetISMDFormat - unknown audio media format requested: %d", audioMediaFormat);
       return ISMD_AUDIO_MEDIA_FMT_INVALID;
    }

  return ismdAudioInputFormat;
}

void CAESinkIntelSMD::ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config, int output,
    int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough)
{
  VERBOSE();
  bool bUseEDID = g_guiSettings.GetBool("videoscreen.forceedid");
  bool bLPCMMode = g_guiSettings.GetBool("audiooutput.lpcm71passthrough");

  CLog::Log(LOGINFO, "CAESinkIntelSMD::ConfigureAudioOutputParams %s sample size %d sample rate %d channels %d format %d",
      output == AUDIO_HDMI ? "HDMI" : "SPDIF", sampleSize, sampleRate, channels, format);

  SetDefaultOutputConfig(output_config);

  if(sampleRate == 24000)
    sampleRate = 48000;

  if (bPassthrough)
  {
    output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
  }
  else if (m_bAC3Encode)
  {
    output_config.out_mode = ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL;
  }

  if(output == AUDIO_HDMI)
  {
    // We need to match input to output freq on HDMI
    // We only do this if we have edid info
    // Last item - if we have audio data from EDID and are on HDMI, then validate the output
    unsigned int suggSampleRate = sampleRate;
    unsigned int suggSampleSize = sampleSize;
    int suggChannels = channels;

    if (bUseEDID)
    {
      CLog::Log(LOGINFO, "CAESinkIntelSMD::ConfigureAudioOutputParams Testing EDID for sample rate %d sample size %d", suggSampleRate, suggSampleSize);
      if (CheckEDIDSupport(bPassthrough ? format : ISMD_AUDIO_MEDIA_FMT_PCM, suggChannels, suggSampleRate, suggSampleSize))
      {
        output_config.sample_rate = suggSampleRate;
        output_config.sample_size = suggSampleSize;
        CLog::Log(LOGINFO, "CAESinkIntelSMD::ConfigureAudioOutputParams EDID results sample rate %d sample size %d", suggSampleRate, suggSampleSize);
      } else
        CLog::Log(LOGINFO, "CAESinkIntelSMD::ConfigureAudioOutputParams no EDID support found");
    }
    else
    {
      sampleRate = 48000;
      sampleSize = 16;
    }
    if(bLPCMMode && !m_bIsAllOutputs)
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


  if(output == AUDIO_IEC958)
  {
    if(sampleRate == 48000 || sampleRate == 96000 || sampleRate == 32000 || sampleRate == 44100 || sampleRate == 88200)
      output_config.sample_rate = sampleRate;
    if(sampleSize == 16 || sampleSize == 20 || sampleSize == 24)
      output_config.sample_size = sampleSize;
  } // SPDIF

  CLog::Log(LOGINFO, "CAESinkIntelSMD::ConfigureAudioOutputParams stream_delay %d sample_size %d \
ch_config %d out_mode %d sample_rate %d ch_map %d",
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

bool CAESinkIntelSMD::IsCompatible(const AEAudioFormat format, const std::string device)
{
  VERBOSE();
  // TODO(q)
  if (device != "IntelSMD")
  {
    return false;
  }
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
  // most likely TODO(q)
  CAEDeviceInfo info;
  info.m_deviceName = "IntelSMD";
  info.m_displayName = "IntelSMD";
  info.m_deviceType =  AE_DEVTYPE_HDMI;
  info.m_sampleRates.push_back(48000);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  AEChannel channels[] = {AE_CH_FL , AE_CH_FR, AE_CH_NULL};
  info.m_channels = CAEChannelInfo(channels);
  list.push_back(info);
}


#endif
