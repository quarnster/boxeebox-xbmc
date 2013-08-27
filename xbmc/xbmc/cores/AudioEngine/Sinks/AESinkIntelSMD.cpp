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
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
  
  m_audioDevice = -1;
  m_audioDeviceInput = -1;

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

  bool bUseEDID = g_guiSettings.GetBool("videoscreen.forceedid");
  int inputChannelConfig = AUDIO_CHAN_CONFIG_2_CH;

  audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if(audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize audioProcessor is not valid");
    return false;
  }

  // disable all outputs
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  // g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  // g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  if(bUseEDID)
  {
    UnloadEDID();
    (void)LoadEDID();
  }

  m_audioDevice = g_IntelSMDGlobals.CreateAudioInput(false);
  if(m_audioDevice == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize failed to create audio input");
    return false;
  }

  g_IntelSMDGlobals.SetPrimaryAudioDevice(m_audioDevice);
  m_audioDeviceInput = g_IntelSMDGlobals.GetAudioDevicePort(m_audioDevice);
  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize failed to create audio input port");
    return false;
  }

  ismdAudioInputFormat = GetISMDFormat(format.m_dataFormat);

  // Can we do hardware decode?
  bool bHardwareDecoder = (ismd_audio_codec_available(ismdAudioInputFormat) == ISMD_SUCCESS);
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

  // TODO(q) never actually take anything other than pcm
  result = ismd_audio_input_set_pcm_format(audioProcessor, m_audioDevice, uiBitsPerSample, format.m_sampleRate, inputChannelConfig);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize - ismd_audio_input_set_pcm_format: %d", result);
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

//     ConfigureAudioOutputParams(spdif_output_config, AUDIO_IEC958, bitsPerSec,
//         samplesPerSec, format.m_channelLayout.Count(), ismdAudioInputFormat, bSPDIFPassthrough);
//     if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputSPDIF, spdif_output_config))
//     {
//       CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize ConfigureAudioOutput SPDIF failed %d", result);
// //      return false;
//     }
//     format.m_sampleRate = spdif_output_config.sample_rate;
//   }

  // HDMI
//  if(m_bIsHDMI && !m_bIsAllOutputs)
  {
    ismd_audio_output_t OutputHDMI = g_IntelSMDGlobals.GetHDMIOutput();
    ismd_audio_output_config_t hdmi_output_config;
    ConfigureAudioOutputParams(hdmi_output_config, AUDIO_HDMI, uiBitsPerSample,
      format.m_sampleRate, format.m_channelLayout.Count(), ismdAudioInputFormat, false);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputHDMI, hdmi_output_config))
    {
      CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize ConfigureAudioOutput HDMI failed %d", result);
//      return false;
    }
   format.m_sampleRate = hdmi_output_config.sample_rate;
  }

  // Configure the master clock frequency

  CLog::Log(LOGINFO, "CAESinkIntelSMD::Initialize ConfigureMasterClock %d", format.m_sampleRate);
  g_IntelSMDGlobals.ConfigureMasterClock(format.m_sampleRate);

  ismd_audio_input_pass_through_config_t passthrough_config;
  memset(&passthrough_config, 0, sizeof(&passthrough_config));
  result = ismd_audio_input_set_as_primary(audioProcessor, m_audioDevice, passthrough_config);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  ismd_audio_input_set_as_primary failed %d", result);
//      return false;
  }

  if(!g_IntelSMDGlobals.EnableAudioInput(m_audioDevice))
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioInput");
//    return false;
  }

  // enable outputs
  if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput()))
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput HDMI failed");
//      return false;
  }

//   if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput()))
//   {
//     CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput SPDIF failed");
// //      return false;
//   }

//   if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput()))
//   {
//     CLog::Log(LOGERROR, "CAESinkIntelSMD::Initialize  EnableAudioOutput I2S failed");
// //      return false;
//   }

  g_IntelSMDGlobals.SetBaseTime(0);
  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);

  m_fCurrentVolume = g_settings.m_fVolumeLevel;
  g_IntelSMDGlobals.SetMasterVolume(m_fCurrentVolume);

  m_bPause = false;
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
    CLog::Log(LOGERROR, "CAESinkIntelSMD::GetCacheTotal - inputPort == -1");
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CAESinkIntelSMD::GetCacheTotal - error getting port status: %d", smd_ret);
    return 0;
  }

  return ((double) (maxDepth * m_dwBufferLen)/(2*2*48000.0));

}
double CAESinkIntelSMD::GetDelay()
{
  return m_dwBufferLen/(2*2*48000.0);
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

  return ((double) (curDepth * m_dwBufferLen)/(2*2*48000.0));

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

  static unsigned int s = 0; 
  if (s++ % 8 == 0) {
    unsigned int curDepth = 0;
    unsigned int maxDepth = 0;
    g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
    printf("cur: %d, max: %d\n", curDepth, maxDepth);
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
        usleep(1000);
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
//    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput - error allocating buffer: %d", smd_ret);
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

  counter = 0;
  while (counter < 1000)
  {
    smd_ret = ismd_port_write(m_audioDeviceInput, ismdBuffer);
    if (smd_ret != ISMD_SUCCESS)
    {
      if(g_IntelSMDGlobals.GetAudioDeviceState(m_audioDevice) != ISMD_DEV_STATE_STOP)
      {
        counter++;
        usleep(1000);
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
//    CLog::Log(LOGERROR, "CAESinkIntelSMD::SendDataToInput failed to write buffer %d\n", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    buffer_size = 0;
  }

  return buffer_size;
}


unsigned int CAESinkIntelSMD::AddPackets(uint8_t* data, unsigned int len, bool hasAudio)
{
  VERBOSE();
  // Len is in frames, we want it in bytes
  len *= 4;

  // What's the maximal write size - either a chunk if provided, or the full buffer
  unsigned int block_size = (m_dwChunkSize ? m_dwChunkSize : m_dwBufferLen);

//  CLog::Log(LOGDEBUG, "CAESinkIntelSMD::AddPackets: %d %d %d %d", len, block_size, m_dwChunkSize, m_dwBufferLen);

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

  pBuffer = (unsigned char*)data;
  total = len;

  //printf("len %d chunksize %d m_bTimed %d\n", len, m_dwChunkSize, m_bTimed);

  while (len && len >= m_dwChunkSize) // We want to write at least one chunk at a time
  {
    unsigned int bytes_to_copy = len > block_size ? block_size : len;
//    printf("bytes_to_copy %d block size %d m_ChunksCollectedSize %d\n", bytes_to_copy, block_size, m_ChunksCollectedSize);
    dataSent = SendDataToInput(pBuffer, bytes_to_copy, local_pts);

    if(dataSent != bytes_to_copy)
    {
//      CLog::Log(LOGERROR, "CAESinkIntelSMD::AddPackets SendDataToInput failed. req %d actual %d", len, dataSent);
      return 0;
    }

    pBuffer += dataSent; // Update buffer pointer
    len -= dataSent; // Update remaining data len
  }

  return (total - len)/4; // frames used
}

void CAESinkIntelSMD::Drain()
{
  VERBOSE();
  CSingleLock lock(m_SMDAudioLock);
  g_IntelSMDGlobals.FlushAudioDevice(m_audioDevice);

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

  if (!m_bPause)
    return true;

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);
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
