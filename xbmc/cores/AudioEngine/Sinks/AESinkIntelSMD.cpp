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
#include <sstream>
extern "C"
{
#include <ismd_core.h>
#include <ismd_audio.h>
//#include <ismd_audio_ac3.h>
#include <ismd_audio_ddplus.h>
//#include <ismd_audio_truehd.h>
//#include <ismd_audio_dts.h>
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
CAESinkIntelSMD::edidHint* CAESinkIntelSMD::m_edidTable = NULL;

ismd_dev_t tmpDevice = -1;

bool m_codecAvailable[ISMD_AUDIO_MEDIA_FMT_COUNT];
bool m_checkedCodecs = false;

static const unsigned int s_edidRates[] = {32000,44100,48000,88200,96000,176400,192000};
static const unsigned int s_edidSampleSizes[] = {16,20,24,32};

ismd_audio_format_t mapGDLAudioFormat( gdl_hdmi_audio_fmt_t f )
{
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

// Functions to determine whether a device is enabled by name
static inline bool isHDMI(std::string &device)
{
  if (device == "HDMI" || device == "All")
    return true;

  return false;
}

static inline bool isSPDIF(std::string &device)
{
  if (device == "SPDIF" || device == "All")
    return true;

  return false;
}

static inline bool isAnalog(std::string &device)
{
  if (device == "Analog" || device == "All")
    return true;

  return false;
}

// Used to convert the input sample rate to the supported output sample rate
static inline int getOutputSampleRate(int deviceType, int inputSampleRate)
{
  // We need to match input to output freq on HDMI
  // We only do this if we have edid info
  // Last item - if we have audio data from EDID and are on HDMI, then validate the output
  // default to 48000
  int outSampleRate = 48000;
  
  if (deviceType == AE_DEVTYPE_PCM)
    return outSampleRate;

  switch(inputSampleRate) {
    case 12000:
    case 24000:
    case 32000:
    // TODO: Unable to use 44100 and 88200 clocks
    //case 44100:
    //case 88200:
    case 96000:
    case 192000:
      //if (deviceType != AE_DEVTYPE_HDMI)
        outSampleRate = inputSampleRate;
      break;
    case 176400:
      if (deviceType == AE_DEVTYPE_PCM)
        outSampleRate = inputSampleRate;
      //else if (deviceType == AE_DEVTYPE_IEC958)
        //outSampleRate = 88200;
      break;
    default:
      break;
  }

  return outSampleRate;
}

// Determines the supported sample size
// TODO: Fix so non-16bit samples are supported
static inline int getOutputSampleSize(int deviceType, int inputSampleSize)
{
  // default to 16bit sample size
  int outSampleSize = 4;
  
  //if (inputSampleSize >= 8)// && deviceType == AE_DEVTYPE_HDMI)
  //  outSampleSize = 8;
  //else if (inputSampleSize >= 6)
  //  outSampleSize = 6;
  //else if (inputSampleSize >= 5 && deviceType == AE_DEVTYPE_HDMI)
  //  outSampleSize = 5;
	
  return outSampleSize;
}

// Converts input format to supported output format
static inline AEDataFormat getAEDataFormat(int deviceType, AEDataFormat inputMediaFormat)
{
  // default to 16 little endian
  AEDataFormat outputAEDataFormat = AE_FMT_S16LE;

  switch (inputMediaFormat) {
    case AE_FMT_S32BE:
    case AE_FMT_S32LE:
    case AE_FMT_S32NE:
      //if (deviceType == AE_DEVTYPE_HDMI)
        outputAEDataFormat = AE_FMT_S32LE;
      //else
      //  outputAEDataFormat = AE_FMT_S24LE4;
      break;
    case AE_FMT_S24BE3:
    case AE_FMT_S24LE3:
    case AE_FMT_S24NE3:
      outputAEDataFormat = AE_FMT_S24LE3;
    case AE_FMT_S24BE4:
    case AE_FMT_S24LE4:
    case AE_FMT_S24NE4:
      outputAEDataFormat = AE_FMT_S24LE4;
      break;
    default:
      break;
  }
   
  return outputAEDataFormat;
}

// Determines device type based on the device name
static inline int getDeviceType(std::string &device)
{
  if (device == "SPDIF")
    return AE_DEVTYPE_IEC958;
  else if (device == "HDMI")
    return AE_DEVTYPE_HDMI;

  return AE_DEVTYPE_PCM;
}

CAESinkIntelSMD::CAESinkIntelSMD()
{
  VERBOSE2();
  m_bIsAllocated = false;
  m_latency = 0.025;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
  m_dSampleRate = 0.0;
  m_frameSize = 0;

  m_audioDevice = -1;
  m_audioDeviceInput = -1;

  //m_fCurrentVolume = 0;

  if (!m_checkedCodecs)
  {
    m_checkedCodecs = true;
    // set invalid codec to false
    m_codecAvailable[0] = false;
    for(int i = 1; i < ISMD_AUDIO_MEDIA_FMT_COUNT; ++i)
    {
      m_codecAvailable[i] = (ismd_audio_codec_available((ismd_audio_format_t)i) == ISMD_SUCCESS);
      CLog::Log(LOGDEBUG, "%s: ismd_audio_format %d codec available: %d", __DEBUG_ID__, i, m_codecAvailable[i]);
    }

    //Turn off DTS hw decoding as that currently is not working properly
    m_codecAvailable[ISMD_AUDIO_MEDIA_FMT_DTS] = false;
  }
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

  bool bIsHDMI = isHDMI(device);
  bool bIsSPDIF = isSPDIF(device);
  bool bIsAnalog = isAnalog(device);
  int deviceType = getDeviceType(device);

  ismd_result_t result;
  AEDataFormat inputDataFormat = format.m_dataFormat;

  bool bIsCodecAvailable = m_codecAvailable[GetISMDFormat(inputDataFormat)];
  bool bSPDIFPassthrough = false;
  bool bHDMIPassthrough = false;
  bool bIsRawCodec = AE_IS_RAW(inputDataFormat);
  // TODO(q)
  m_dwChunkSize = 4*1024;
  m_dwBufferLen = m_dwChunkSize;
  static enum AEChannel map[8] = { AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR };
  format.m_sampleRate = getOutputSampleRate(deviceType, format.m_sampleRate);
  format.m_dataFormat = getAEDataFormat(deviceType, format.m_dataFormat);

  if (!bIsRawCodec || !bIsCodecAvailable)    
    inputDataFormat = format.m_dataFormat;

  int channels = format.m_channelLayout.Count();
  // can not support more than 2 channels on anything other than HDMI bitstreaming
  if (channels > 2 && (bIsSPDIF || bIsAnalog || !bIsRawCodec))
    channels = 2;
  // support for more than 8 channels not supported
  else if (channels > 8)
    channels = 8;

  format.m_channelLayout.Reset();
  if (bIsRawCodec)
  {
    for (int i = 0; i < channels; ++i)
      format.m_channelLayout += AE_CH_RAW;
  }
  // TODO: This currently handles Mono,Stereo, 5.1, 7.1 correctly
  // Handle the other cases (i.e. 6.1 DTS)
  else
  {
    for (int i = 0; i < channels; ++i)
      format.m_channelLayout += map[i];
  }
  format.m_frameSize = getOutputSampleSize(deviceType, format.m_frameSize);
  format.m_frames = m_dwChunkSize/format.m_frameSize;
  format.m_frameSamples = format.m_frames*channels;
  m_frameSize = format.m_frameSize;


  ismd_audio_processor_t  audioProcessor = -1;
  ismd_audio_format_t     ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;

  //TODO: Handle non normal channel configs (3 channel, etc).
  int inputChannelConfig = AUDIO_CHAN_CONFIG_2_CH;
  if (format.m_channelLayout.Count() == 1)
    inputChannelConfig = AUDIO_CHAN_CONFIG_1_CH;
  else if (format.m_channelLayout.Count() == 6)
    inputChannelConfig = AUDIO_CHAN_CONFIG_6_CH;
  else if (format.m_channelLayout.Count() == 8)
    inputChannelConfig = AUDIO_CHAN_CONFIG_8_CH;

  audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if(audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "%s audioProcessor is not valid", __DEBUG_ID__);
    return false;
  }

  // disable all outputs
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

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

  ismdAudioInputFormat = GetISMDFormat(inputDataFormat);
  
  // Can we do hardware decode?
  bool bHardwareDecoder = m_codecAvailable[GetISMDFormat(inputDataFormat)];
  if(ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_PCM)
    bHardwareDecoder = false;

  int uiBitsPerSample = m_frameSize*4;

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
  
  // Are we doing DD+ -> DD mode
  bool bAC3Encode = false;
  switch( ismdAudioInputFormat )
  {
    case ISMD_AUDIO_MEDIA_FMT_DD:
      CLog::Log(LOGDEBUG, "%s: Initialize DD detected", __DEBUG_ID__);
      bHDMIPassthrough = bSPDIFPassthrough = true;
      break;
    case ISMD_AUDIO_MEDIA_FMT_DD_PLUS:
      CLog::Log(LOGDEBUG, "%s: Initialize DD Plus detected", __DEBUG_ID__);

      // check special case for DD+->DD using DDCO
      bHDMIPassthrough = true;
      if(bIsSPDIF && ISMD_SUCCESS == ismd_audio_codec_available((ismd_audio_format_t) ISMD_AUDIO_ENCODE_FMT_AC3))
      {
        CLog::Log(LOGDEBUG, "%s: Initialize EAC3->AC3 transcoding is on", __DEBUG_ID__);
        bHDMIPassthrough = false;
        bAC3Encode = true;
      }
      ConfigureDolbyPlusModes(audioProcessor, m_audioDevice, bAC3Encode);
      break;
    case ISMD_AUDIO_MEDIA_FMT_DTS:
    case ISMD_AUDIO_MEDIA_FMT_DTS_LBR:
      CLog::Log(LOGDEBUG, "%s: Initialize DTS detected", __DEBUG_ID__);
      bHDMIPassthrough = bSPDIFPassthrough = true;
      break;
    case ISMD_AUDIO_MEDIA_FMT_DTS_HD:
    case ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA:
    case ISMD_AUDIO_MEDIA_FMT_DTS_HD_HRA:
      CLog::Log(LOGDEBUG, "%s: Initialize DTS-HD detected", __DEBUG_ID__);
      bHDMIPassthrough = true;
      break;
    case ISMD_AUDIO_MEDIA_FMT_TRUE_HD:
      CLog::Log(LOGDEBUG, "%s: Initialize TrueHD detected", __DEBUG_ID__);
      bHDMIPassthrough = true;
      break;
    case ISMD_AUDIO_MEDIA_FMT_PCM:
      result = ismd_audio_input_set_pcm_format(audioProcessor, m_audioDevice, uiBitsPerSample, format.m_sampleRate, inputChannelConfig);
      if (result != ISMD_SUCCESS)
      {
        CLog::Log(LOGERROR, "%s - ismd_audio_input_set_pcm_format: %d", __DEBUG_ID__, result);
    //    return false;
      }
      break;
    default:
      break;
  }
  
  
  // I2S. Nothing to touch here. we always use defaults

  // SPIDF
   if(bIsSPDIF)
   {
     ismd_audio_output_t OutputSPDIF = g_IntelSMDGlobals.GetSPDIFOutput();
     ismd_audio_output_config_t spdif_output_config;
     unsigned int samplesPerSec = format.m_sampleRate;
     unsigned int bitsPerSec = uiBitsPerSample;

     ConfigureAudioOutputParams(spdif_output_config, AE_DEVTYPE_IEC958, bitsPerSec,
         samplesPerSec, format.m_channelLayout.Count(), ismdAudioInputFormat, bSPDIFPassthrough);
     if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputSPDIF, spdif_output_config))
     {
       CLog::Log(LOGERROR, "%s ConfigureAudioOutput SPDIF failed %d", __DEBUG_ID__, result);
 //      return false;
     }
     
     //format.m_sampleRate = spdif_output_config.sample_rate;
   }

  // HDMI
  if(bIsHDMI)
  {
    ismd_audio_output_t OutputHDMI = g_IntelSMDGlobals.GetHDMIOutput();
    ismd_audio_output_config_t hdmi_output_config;
    ConfigureAudioOutputParams(hdmi_output_config, AE_DEVTYPE_HDMI, uiBitsPerSample,
      format.m_sampleRate, format.m_channelLayout.Count(), ismdAudioInputFormat, bHDMIPassthrough);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputHDMI, hdmi_output_config))
    {
      CLog::Log(LOGERROR, "%s ConfigureAudioOutput HDMI failed %d", __DEBUG_ID__, result);
      return false;
    }
    //format.m_sampleRate = hdmi_output_config.sample_rate;
  }

  // Configure the master clock frequency

  CLog::Log(LOGINFO, "%s ConfigureMasterClock %d", __DEBUG_ID__, format.m_sampleRate);
  g_IntelSMDGlobals.ConfigureMasterClock(format.m_sampleRate);
  
  bSPDIFPassthrough = bIsSPDIF && bSPDIFPassthrough;
  bHDMIPassthrough = bIsHDMI && bHDMIPassthrough;
  ismd_audio_input_pass_through_config_t passthrough_config;
  memset(&passthrough_config, 0, sizeof(&passthrough_config));
  if (bSPDIFPassthrough || bHDMIPassthrough)
  {
    passthrough_config.is_pass_through = TRUE;
    //passthrough_config.supported_format_count = 1;
    //passthrough_config.supported_formats[0] = ismdAudioInputFormat;
  }

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
  if (bIsHDMI)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput()))
    {
      CLog::Log(LOGERROR, "%s  EnableAudioOutput HDMI failed", __DEBUG_ID__);
     //   return false;
    }
  }
  
  if (bIsSPDIF)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput()))
    {
      CLog::Log(LOGERROR, "%s  EnableAudioOutput SPDIF failed", __DEBUG_ID__);
//     return false;
    }
  }
  
  if (bIsAnalog)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput()))
    {
      CLog::Log(LOGERROR, "%s  EnableAudioOutput I2S failed", __DEBUG_ID__);
 //      return false;
    }
  }

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);

//  m_fCurrentVolume = g_settings.m_fVolumeLevel;
  //g_IntelSMDGlobals.SetMasterVolume(m_fCurrentVolume);

  m_bPause = false;
  m_dSampleRate = format.m_sampleRate;
  m_bIsAllocated = true;

  // set latency when using passthrough since we are not using a timed audio interface
  if (bSPDIFPassthrough || bHDMIPassthrough || bAC3Encode)
    m_latency = 0.45;

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
  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  m_bIsAllocated = false;

  return;
}

void CAESinkIntelSMD::ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode)
{
  ismd_result_t result;
  ismd_audio_decoder_param_t param;

  ismd_audio_ddplus_output_configuration_t *output_config_ddplus = (ismd_audio_ddplus_output_configuration_t *) &param;
  *output_config_ddplus = (ismd_audio_ddplus_output_configuration_t)ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_RAW_MODE;
  if(bAC3Encode) // if the output will be rencode to AC3, output all 5 channels
    *output_config_ddplus = ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION_3_2_1_LCRlrTs;
  CLog::Log(LOGNONE, "ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION %d", *output_config_ddplus);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle, ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION, &param);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "%s ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION failed %d", __DEBUG_ID__, result);
  }
}

double CAESinkIntelSMD::GetCacheTotal()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0.0;
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

  return ((double) (maxDepth * m_dwBufferLen))/((double) (m_frameSize)*m_dSampleRate);
}

double CAESinkIntelSMD::GetDelay()
{
  return GetCacheTime();//+m_dwBufferLen/(2*2*m_dSampleRate);
}

double CAESinkIntelSMD::GetCacheTime()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0.0;
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

  return ((double) (curDepth * m_dwBufferLen))/((double)(m_frameSize)*m_dSampleRate);
}

unsigned int CAESinkIntelSMD::SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size)
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
  len *= m_frameSize;


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

  //ismd_pts_t local_pts = ISMD_NO_PTS;
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
    unsigned int dataSent = SendDataToInput(pBuffer, bytes_to_copy);

    if(dataSent != bytes_to_copy)
    {
      CLog::Log(LOGERROR, "%s SendDataToInput failed. req %d actual %d", __DEBUG_ID__, len, dataSent);
      return 0;
    }

    pBuffer += dataSent; // Update buffer pointer
    len -= dataSent; // Update remaining data len
  }

  return (total - len)/m_frameSize; // frames used
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
//  if (!m_bIsAllocated )
//    return;

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
     case AE_FMT_FLOAT:
     case AE_FMT_DOUBLE:
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
     case AE_FMT_AC3:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DD;
       break;
     case AE_FMT_EAC3:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DD_PLUS;
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
  //bool bLPCMMode = false; //TODO(q) g_guiSettings.GetBool("audiooutput.lpcm71passthrough");

  CLog::Log(LOGINFO, "%s %s sample size %d sample rate %d channels %d format %d", __DEBUG_ID__,
      output == AE_DEVTYPE_HDMI ? "HDMI" : "SPDIF", sampleSize, sampleRate, channels, format);

  SetDefaultOutputConfig(output_config);
  
  output_config.sample_rate = sampleRate;
  output_config.sample_size = sampleSize;
  if(output == AE_DEVTYPE_HDMI)
  {
    if (bPassthrough)
      output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
    //if(bLPCMMode)
   // {
      if(channels == 8)
        output_config.ch_config = ISMD_AUDIO_7_1;
      else if(channels == 6)
        output_config.ch_config = ISMD_AUDIO_5_1;
    //}

    //if(sampleRate == 48000 || sampleRate == 96000 || sampleRate == 32000 || sampleRate == 44100
    //    || sampleRate == 88200 || sampleRate == 176400 || sampleRate == 192000)
      //output_config.sample_rate = sampleRate;
    //if(sampleSize == 16 || sampleSize == 20 || sampleSize == 24 || sampleSize == 32)
     // output_config.sample_size = sampleSize;
  } // HDMI
  else if (output == AE_DEVTYPE_IEC958)
  {
    if (bPassthrough && (format == ISMD_AUDIO_MEDIA_FMT_DTS || format == ISMD_AUDIO_MEDIA_FMT_DD))
      output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
    else if (format == ISMD_AUDIO_MEDIA_FMT_DD_PLUS || format == ISMD_AUDIO_MEDIA_FMT_TRUE_HD)
      output_config.out_mode = ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL;
  }

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

void CAESinkIntelSMD::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  VERBOSE();

  LoadEDID();
  // most likely TODO(q)- now messed up by quasar?
  // And now even more messed up by Keyser :)
  CAEDeviceInfo info;
  info.m_channels.Reset();
  info.m_dataFormats.clear();
  info.m_sampleRates.clear();
  
  info.m_deviceType =  AE_DEVTYPE_PCM; 
  info.m_deviceName = "All";
  info.m_displayName = "All Outputs";
  info.m_displayNameExtra = "HDMI/SPDIF/Analog";
  info.m_channels += AE_CH_FL;
  info.m_channels += AE_CH_FR;
  info.m_sampleRates.push_back(48000);  
  info.m_dataFormats.push_back(AE_FMT_S8);
  info.m_dataFormats.push_back(AE_FMT_S16BE);
  info.m_dataFormats.push_back(AE_FMT_S16NE);
  info.m_dataFormats.push_back(AE_FMT_S32BE);
  info.m_dataFormats.push_back(AE_FMT_S32LE);
  info.m_dataFormats.push_back(AE_FMT_S32NE);
  info.m_dataFormats.push_back(AE_FMT_S24BE4);
  info.m_dataFormats.push_back(AE_FMT_S24LE4);
  info.m_dataFormats.push_back(AE_FMT_S24NE4);
  info.m_dataFormats.push_back(AE_FMT_S24BE3);
  info.m_dataFormats.push_back(AE_FMT_S24LE3);
  info.m_dataFormats.push_back(AE_FMT_S24NE3);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  list.push_back(info);

  info.m_channels.Reset();
  info.m_dataFormats.clear();
  info.m_sampleRates.clear();
  
  info.m_deviceType =  AE_DEVTYPE_HDMI; 
  info.m_deviceName = "HDMI";
  info.m_displayName = "HDMI";
  info.m_displayNameExtra = "HDMI Output";
  info.m_channels += AE_CH_FL;
  info.m_channels += AE_CH_FR;
  info.m_channels += AE_CH_FC;
  info.m_channels += AE_CH_LFE;
  info.m_channels += AE_CH_BL;
  info.m_channels += AE_CH_BR;
  info.m_channels += AE_CH_SL;
  info.m_channels += AE_CH_SR;
  info.m_sampleRates.push_back(48000);  
  info.m_dataFormats.push_back(AE_FMT_S8);
  info.m_dataFormats.push_back(AE_FMT_S16BE);
  info.m_dataFormats.push_back(AE_FMT_S16NE);
  info.m_dataFormats.push_back(AE_FMT_S32BE);
  info.m_dataFormats.push_back(AE_FMT_S32LE);
  info.m_dataFormats.push_back(AE_FMT_S32NE);
  info.m_dataFormats.push_back(AE_FMT_S24BE4);
  info.m_dataFormats.push_back(AE_FMT_S24LE4);
  info.m_dataFormats.push_back(AE_FMT_S24NE4);
  info.m_dataFormats.push_back(AE_FMT_S24BE3);
  info.m_dataFormats.push_back(AE_FMT_S24LE3);
  info.m_dataFormats.push_back(AE_FMT_S24NE3);
  info.m_dataFormats.push_back(AE_FMT_LPCM);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  info.m_dataFormats.push_back(AE_FMT_AC3);
  info.m_dataFormats.push_back(AE_FMT_DTS);
  info.m_dataFormats.push_back(AE_FMT_EAC3);
  info.m_dataFormats.push_back(AE_FMT_DTSHD);
  info.m_dataFormats.push_back(AE_FMT_TRUEHD);
  list.push_back(info);

  info.m_channels.Reset();
  info.m_dataFormats.clear();
  info.m_sampleRates.clear();
  
  info.m_deviceType = AE_DEVTYPE_IEC958; 
  info.m_deviceName = "SPDIF";
  info.m_displayName = "SPDIF";
  info.m_displayNameExtra = "Toslink Output";
  info.m_channels += AE_CH_FL;
  info.m_channels += AE_CH_FR;
  info.m_sampleRates.push_back(12000);
  info.m_sampleRates.push_back(24000);
  info.m_sampleRates.push_back(32000);
  //info.m_sampleRates.push_back(44100);
  info.m_sampleRates.push_back(48000);
  //info.m_sampleRates.push_back(88200);
  info.m_sampleRates.push_back(96000);
  info.m_sampleRates.push_back(192000);
  info.m_dataFormats.push_back(AE_FMT_S8);
  info.m_dataFormats.push_back(AE_FMT_S16BE);
  info.m_dataFormats.push_back(AE_FMT_S16NE);
  info.m_dataFormats.push_back(AE_FMT_S24BE4);
  info.m_dataFormats.push_back(AE_FMT_S24LE4);
  info.m_dataFormats.push_back(AE_FMT_S24NE4);
  info.m_dataFormats.push_back(AE_FMT_S24BE3);
  info.m_dataFormats.push_back(AE_FMT_S24LE3);
  info.m_dataFormats.push_back(AE_FMT_S24NE3);
  info.m_dataFormats.push_back(AE_FMT_LPCM);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  info.m_dataFormats.push_back(AE_FMT_AC3);
  info.m_dataFormats.push_back(AE_FMT_DTS);
  info.m_dataFormats.push_back(AE_FMT_EAC3);
  list.push_back(info);

  info.m_channels.Reset();
  info.m_dataFormats.clear();
  info.m_sampleRates.clear();
  
  info.m_deviceType = AE_DEVTYPE_PCM; 
  info.m_deviceName = "Analog";
  info.m_displayName = "Analog";
  info.m_displayNameExtra = "RCA Outputs";
  info.m_channels += AE_CH_FL;
  info.m_channels += AE_CH_FR;
  info.m_sampleRates.push_back(12000);
  info.m_sampleRates.push_back(24000);
  info.m_sampleRates.push_back(32000);
  //info.m_sampleRates.push_back(44100);
  info.m_sampleRates.push_back(48000);
  //info.m_sampleRates.push_back(88200);
  info.m_sampleRates.push_back(96000);
  //info.m_sampleRates.push_back(176400);
  info.m_sampleRates.push_back(192000);
  info.m_dataFormats.push_back(AE_FMT_S8);
  info.m_dataFormats.push_back(AE_FMT_S16BE);
  info.m_dataFormats.push_back(AE_FMT_S16NE);
  info.m_dataFormats.push_back(AE_FMT_S24BE4);
  info.m_dataFormats.push_back(AE_FMT_S24LE4);
  info.m_dataFormats.push_back(AE_FMT_S24NE4);
  info.m_dataFormats.push_back(AE_FMT_S24BE3);
  info.m_dataFormats.push_back(AE_FMT_S24LE3);
  info.m_dataFormats.push_back(AE_FMT_S24NE3);
  list.push_back(info);
}

bool CAESinkIntelSMD::LoadEDID()
{
  UnloadEDID();

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

    hint->format = mapGDLAudioFormat( (gdl_hdmi_audio_fmt_t)ctrl.data._get_caps.cap.format );
    if( ISMD_AUDIO_MEDIA_FMT_INVALID == hint->format )
    {
      delete hint;
      ctrl.data._get_caps.index++;
      continue;
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
  while( m_edidTable )
  {
    edidHint* nxt = m_edidTable->next;
    delete[] m_edidTable;
    m_edidTable = nxt;
  }
}

void CAESinkIntelSMD::DumpEDID()
{
  std::string formats[] = {"invalid","LPCM","DVDPCM","BRPCM","MPEG","AAC","AACLOAS","DD","DD+","TrueHD","DTS-HD","DTS-HD-HRA","DTS-HD-MA","DTS","DTS-LBR","WM9"};
  CLog::Log(LOGINFO,"%s HDMI Audio sink supports the following formats:\n", __DEBUG_ID__);
  for( edidHint* cur = m_edidTable; cur; cur = cur->next )
  {
    std::ostringstream line;
    line << "* Format: " << formats[cur->format] << " Max channels: " << cur->channels << " Samplerates: ";

    for( size_t z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( cur->sample_rates & (1<<z) )
      {
        line << s_edidRates[z] << " ";
      }         
    }
    if( ISMD_AUDIO_MEDIA_FMT_PCM == cur->format )
    {
      line << "Sample Sizes: ";
      for( size_t z = 0; z< sizeof(s_edidSampleSizes); z++ )
      {
        if( cur->sample_sizes & (1<<z) )
        {
          line << s_edidSampleSizes[z] << " ";
        }
      }
    }
    CLog::Log(LOGINFO, "%s %s", __DEBUG_ID__, line.str().c_str());
  }
}

#endif
