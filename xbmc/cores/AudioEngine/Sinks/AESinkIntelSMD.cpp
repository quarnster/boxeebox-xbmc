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
#include "Utils/AEUtil.h"
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

bool m_checkedCodecs = false;

static const unsigned int s_edidRates[] = {32000,44100,48000,88200,96000,176400,192000};
static const unsigned int s_edidSampleSizes[] = {16,20,24,32};
static enum AEChannel s_chMap[8] = { AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_FC, AE_CH_LFE, AE_CH_SL, AE_CH_SR };

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

static bool tryGetAEDataFormat(ismd_audio_format_t ismd_type, AEDataFormat &aeFormat)
{
   switch (ismd_type) {
     case ISMD_AUDIO_MEDIA_FMT_PCM:
       aeFormat = AE_FMT_LPCM;
       break;
     case ISMD_AUDIO_MEDIA_FMT_AAC:
       aeFormat = AE_FMT_AAC;
       break;
     case ISMD_AUDIO_MEDIA_FMT_DD:
       aeFormat = AE_FMT_AC3;
       break;
     case ISMD_AUDIO_MEDIA_FMT_DD_PLUS:
       aeFormat = AE_FMT_EAC3;
       break;
     case ISMD_AUDIO_MEDIA_FMT_TRUE_HD:
       aeFormat = AE_FMT_TRUEHD;
       break;
     case ISMD_AUDIO_MEDIA_FMT_DTS:
       aeFormat = AE_FMT_DTS;
       break;
     case ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA:
       aeFormat = AE_FMT_DTSHD;
       break;
     default:
       return false;
       break;
   }

  return true;
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
  
  //if (deviceType == AE_DEVTYPE_PCM)
  //  return outSampleRate;

  switch(inputSampleRate) {
    //case 12000:
   // case 24000:
    case 32000:
    case 96000:
    case 192000:
      //if (deviceType != AE_DEVTYPE_HDMI)
        outSampleRate = inputSampleRate;
      break;
    // TODO: Unable to use 44100 and 88200 clocks
    case 44100:
    case 88200:
    case 176400:
      //outSampleRate = inputSampleRate;
      //if (deviceType == AE_DEVTYPE_PCM)
      //  outSampleRate = 48000;
      //else if (deviceType == AE_DEVTYPE_IEC958)
      //  outSampleRate = 88200;
      break;
    default:
      break;
  }

  return outSampleRate;
}

// Converts input format to supported output format
static inline AEDataFormat getAEDataFormat(int deviceType, AEDataFormat inputMediaFormat, unsigned int frameSize)
{
  // default to 16 little endian
  AEDataFormat outputAEDataFormat = AE_FMT_S16LE;

  switch (inputMediaFormat) {
    case AE_FMT_S24BE3:
    case AE_FMT_S24LE3:
    case AE_FMT_S24NE3:
    case AE_FMT_S24BE4:
    case AE_FMT_S24LE4:
    case AE_FMT_S24NE4:
      outputAEDataFormat = AE_FMT_S24NE4;
    case AE_FMT_S32BE:
    case AE_FMT_S32LE:
    case AE_FMT_S32NE:
      outputAEDataFormat = AE_FMT_S32LE;
      break;
    case AE_FMT_FLOAT:
    case AE_FMT_DOUBLE:
      // if using SPDIF/HDMI then attempt to detect 24 bit audio
      // this uses the frame size to attempt to detect mono, stereo, 5.1, or 7.1 24 bit sources
      // Currently 24 bit is disabled until it is determined why audio crackling is heard on certain setups.
      //if ((deviceType == AE_DEVTYPE_IEC958 || deviceType == AE_DEVTYPE_HDMI) && (frameSize == 3 || frameSize == 6 || frameSize == 18 || frameSize == 24))
      //  outputAEDataFormat = AE_FMT_S24NE4;
      //else
        outputAEDataFormat = AE_FMT_S16LE;
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
   
    for(int i = 1; i < ISMD_AUDIO_MEDIA_FMT_COUNT; ++i)
      CLog::Log(LOGDEBUG, "%s: ismd_audio_format %d codec available: %d", __DEBUG_ID__, i, ismd_audio_codec_available((ismd_audio_format_t)i) == ISMD_SUCCESS);
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

  CLog::Log(LOGDEBUG, "%s: device: %s, data format: %s, sample rate: %d, channel count: %d, frame size: %d", __DEBUG_ID__, device.c_str(), CAEUtil::DataFormatToStr(format.m_dataFormat), format.m_sampleRate, format.m_channelLayout.Count(), format.m_frameSize);

  CSingleLock lock(m_SMDAudioLock);

  bool bIsHDMI = isHDMI(device);
  bool bIsSPDIF = isSPDIF(device);
  bool bIsAnalog = isAnalog(device);
  int deviceType = getDeviceType(device);

  ismd_result_t result;
  AEDataFormat inputDataFormat = format.m_dataFormat;

  bool bSPDIFPassthrough = false;
  bool bHDMIPassthrough = false;
  bool bIsRawCodec = AE_IS_RAW(inputDataFormat);

  format.m_sampleRate = getOutputSampleRate(deviceType, format.m_sampleRate);
  format.m_dataFormat = getAEDataFormat(deviceType, format.m_dataFormat, format.m_frameSize);

  int channels = format.m_channelLayout.Count();
  // can not support more than 2 channels on anything other than HDMI
  if (channels > 2 && (bIsSPDIF || bIsAnalog))
    channels = 2;
  // support for more than 8 channels not supported
  else if (channels > 8)
    channels = 8;


  ismd_audio_processor_t  audioProcessor = -1;
  ismd_audio_format_t     ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;

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

  unsigned int uiBitsPerSample = CAEUtil::DataFormatToBits(format.m_dataFormat);
  unsigned int uiUsedBitsPerSample = CAEUtil::DataFormatToUsedBits(format.m_dataFormat);

  // Are we doing DD+ -> DD mode
  bool bAC3Encode = false;
  if (bIsHDMI)
  {
    unsigned int sampleRate = format.m_sampleRate;
    unsigned int bitsPerSample = uiUsedBitsPerSample;
    if (format.m_encodedRate != 0)
      sampleRate = format.m_encodedRate;

    unsigned int suggSampleRate = sampleRate;
    if (!CheckEDIDSupport(ismdAudioInputFormat, channels, suggSampleRate, bitsPerSample, bAC3Encode))
    {
      if (suggSampleRate != sampleRate)
        format.m_sampleRate = suggSampleRate;

      if (bitsPerSample != uiUsedBitsPerSample)
      {
        if (uiUsedBitsPerSample == 24)
        {
          format.m_dataFormat = AE_FMT_S24NE4;
          uiUsedBitsPerSample = bitsPerSample;
        }
        else if (uiUsedBitsPerSample == 32)
        {
          format.m_dataFormat = AE_FMT_S32LE;
          uiUsedBitsPerSample = bitsPerSample;
        }
        else
        {
          format.m_dataFormat = AE_FMT_S16LE;
          uiUsedBitsPerSample = 16;
        }
        
        uiBitsPerSample = CAEUtil::DataFormatToBits(format.m_dataFormat);
      }
      //format.m_frameSize = uiBitsPerSample/4;
    }  
  }
  else if (bIsSPDIF && ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_DD_PLUS && ISMD_SUCCESS == ismd_audio_codec_available((ismd_audio_format_t) ISMD_AUDIO_ENCODE_FMT_AC3))
  {
    bAC3Encode = true;
  }

  unsigned int outputSampleRate = format.m_sampleRate;
  
  // for raw codecs, send as PCM passthrough
  if (!bAC3Encode && bIsRawCodec && ismdAudioInputFormat != ISMD_AUDIO_MEDIA_FMT_DD && ismdAudioInputFormat != ISMD_AUDIO_MEDIA_FMT_TRUE_HD)
  {
    format.m_dataFormat = AE_FMT_S16NE;
    ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_PCM;
    bHDMIPassthrough = bSPDIFPassthrough = true;
  }

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
      format.m_channelLayout += s_chMap[i];
  }

  //TODO: Handle non normal channel configs (3 channel, etc).
  int inputChannelConfig = AUDIO_CHAN_CONFIG_2_CH;
  if (format.m_channelLayout.Count() == 1)
    inputChannelConfig = AUDIO_CHAN_CONFIG_1_CH;
  else if (format.m_channelLayout.Count() == 6)
    inputChannelConfig = AUDIO_CHAN_CONFIG_6_CH;
  else if (format.m_channelLayout.Count() == 8)
    inputChannelConfig = AUDIO_CHAN_CONFIG_8_CH;

  format.m_frameSize = channels * (CAEUtil::DataFormatToBits(format.m_dataFormat) >> 3);

  // if standard audio keep buffer small so delay is short
  if (!bIsRawCodec)
  {
    // try to keep roughly 5ms buffer using multiples of a 1024 buffer size
    int numBuffers = ((0.005*((double)(format.m_sampleRate*format.m_frameSize))) / 1024.0) + 0.5;
    if (numBuffers == 0)
      numBuffers = 1;
    else if (numBuffers > 8)
      numBuffers = 8;
    m_dwChunkSize = numBuffers*1024;
  }
  else
  {
    m_dwChunkSize = 8*1024;
  }

  m_dwBufferLen = m_dwChunkSize;

  format.m_frames = m_dwChunkSize/format.m_frameSize;
  format.m_frameSamples = format.m_frames*channels;
  m_frameSize = format.m_frameSize;

  CLog::Log(LOGINFO, "%s ismdAudioInputFormat %d\n", __DEBUG_ID__,
      ismdAudioInputFormat);

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
  
  switch( ismdAudioInputFormat )
  {
    case ISMD_AUDIO_MEDIA_FMT_DD:
      CLog::Log(LOGDEBUG, "%s: Initialize DD detected", __DEBUG_ID__);
      bHDMIPassthrough = bSPDIFPassthrough = true;
      break;
    case ISMD_AUDIO_MEDIA_FMT_DD_PLUS:
      CLog::Log(LOGDEBUG, "%s: Initialize DD Plus detected", __DEBUG_ID__);

      bHDMIPassthrough = true;
      // check special case for DD+->DD using DDCO 
      if(bAC3Encode)
      {
        CLog::Log(LOGDEBUG, "%s: Initialize EAC3->AC3 transcoding is on", __DEBUG_ID__);
        bHDMIPassthrough = false;
        bAC3Encode = true; 
        ConfigureDolbyPlusModes(audioProcessor, m_audioDevice, bAC3Encode);       
      }
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
      outputSampleRate = format.m_encodedRate;
      channels = 2;
      break;
    case ISMD_AUDIO_MEDIA_FMT_TRUE_HD:
      CLog::Log(LOGDEBUG, "%s: Initialize TrueHD detected", __DEBUG_ID__);
      bHDMIPassthrough = true;
      outputSampleRate = format.m_encodedRate;
      channels = 2;
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

     ConfigureAudioOutputParams(spdif_output_config, AE_DEVTYPE_IEC958, uiUsedBitsPerSample,
        outputSampleRate, channels, ismdAudioInputFormat, bSPDIFPassthrough, bAC3Encode);
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
    ConfigureAudioOutputParams(hdmi_output_config, AE_DEVTYPE_HDMI, uiUsedBitsPerSample,
      outputSampleRate, channels, ismdAudioInputFormat, bHDMIPassthrough, bAC3Encode);
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
    passthrough_config.supported_format_count = 1;
    passthrough_config.supported_formats[0] = ismdAudioInputFormat;
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
  if (bAC3Encode || ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_DD)
    m_latency = 0.675;//0.45;

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
  
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput());
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput());
  g_IntelSMDGlobals.DisableAudioOutput(g_IntelSMDGlobals.GetI2SOutput());

  //g_IntelSMDGlobals.BuildAudioOutputs();
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

  CLog::Log(LOGDEBUG, "%s ConfigureDolbyPlusModes ISMD_AUDIO_DDPLUS_OUTPUT_CONFIGURATION %d", __DEBUG_ID__, *output_config_ddplus);
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

  //CSingleLock lock(m_SMDAudioLock);

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

  //CSingleLock lock(m_SMDAudioLock);

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
  while (counter < 100)
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


unsigned int CAESinkIntelSMD::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
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
  
  int milliseconds = (int)(GetCacheTime() * 1000.0);
  if (milliseconds)
    Sleep(++milliseconds); // add 1 for the possible truncation used in the conversion to int

  CLog::Log(LOGDEBUG, "%s delay:%dms now:%dms", __DEBUG_ID__, milliseconds, (int)(GetCacheTime() * 1000.0));
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
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA;
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
    int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough, bool bAC3Encode)
{
  VERBOSE();
  //bool bLPCMMode = false; //TODO(q) g_guiSettings.GetBool("audiooutput.lpcm71passthrough");

  CLog::Log(LOGINFO, "%s %s sample size %d sample rate %d channels %d format %d", __DEBUG_ID__,
      output == AE_DEVTYPE_HDMI ? "HDMI" : "SPDIF", sampleSize, sampleRate, channels, format);

  SetDefaultOutputConfig(output_config);
  
  output_config.sample_rate = sampleRate;
  output_config.sample_size = sampleSize;
  if (bPassthrough)
      output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
  else if (bAC3Encode)
    output_config.out_mode = ISMD_AUDIO_OUTPUT_ENCODED_DOLBY_DIGITAL;

  if(output == AE_DEVTYPE_HDMI)
  {
    //if (bPassthrough)
    //  output_config.out_mode = ISMD_AUDIO_OUTPUT_PASSTHROUGH;
    //if(bLPCMMode)
   // {
      if(channels == 8)
        output_config.ch_config = ISMD_AUDIO_7_1;
      else if(channels == 6)
        output_config.ch_config = ISMD_AUDIO_5_1;
    //}

  } // HDMI
  else if (output == AE_DEVTYPE_IEC958)
  {
    // limit output sample rate for SPDIF
    if (sampleSize > 24)
      output_config.sample_size = 24;
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

void CAESinkIntelSMD::GetEDIDInfo(int &maxChannels, std::vector<AEDataFormat> &formats, std::vector<unsigned int> &rates)
{
  // put default info.. in case no EDID was reported
  maxChannels = 2;
  rates.push_back(48000);
  formats.push_back(AE_FMT_S16LE);
  formats.push_back(AE_FMT_LPCM);

  for( edidHint* cur = m_edidTable; cur; cur = cur->next )
  {
    AEDataFormat aeFormat = AE_FMT_INVALID;
    if (tryGetAEDataFormat(cur->format, aeFormat))
    {
      std::vector<AEDataFormat>::iterator it = std::find(formats.begin(), formats.end(), aeFormat);
      if (it == formats.end())
        formats.push_back(aeFormat);

      if (aeFormat == AE_FMT_LPCM)
      {
        if (cur->channels > maxChannels)
          maxChannels = cur->channels;
      }
      else if (aeFormat == AE_FMT_AC3) // if we support AC3 we can do EAC3 via DDCO
      {
        formats.push_back(AE_FMT_EAC3);
      }
    }

    for( size_t z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( cur->sample_rates & (1<<z) )
      {
        std::vector<unsigned int>::iterator it = std::find(rates.begin(), rates.end(), s_edidRates[z]);
        if (it == rates.end())
          rates.push_back(s_edidRates[z]);
      }         
    }

    for( size_t z = 0; z < sizeof(s_edidSampleSizes); z++ )
    {
      if( cur->sample_sizes & (1<<z) )
      {
        aeFormat = AE_FMT_INVALID;
        if (s_edidSampleSizes[z] == 24)
          aeFormat = AE_FMT_S24NE4;
        else if (s_edidSampleSizes[z] == 32)
          aeFormat = AE_FMT_S32LE;
   
        if (aeFormat != AE_FMT_INVALID)
        {
          std::vector<AEDataFormat>::iterator it = std::find(formats.begin(), formats.end(), aeFormat);
          if (it == formats.end())
            formats.push_back(aeFormat);
        }
      }         
    }

  }
}

void CAESinkIntelSMD::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  VERBOSE();

  LoadEDID();

  int maxChannels;
  std::vector<AEDataFormat> formats;
  std::vector<unsigned int> rates;

  GetEDIDInfo(maxChannels, formats, rates);
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

  // support the rates available on HDMI, SPDIF and Analog will already support them
  std::vector<unsigned int>::const_iterator it, itEnd = rates.end();
  for (it = rates.begin(); it != itEnd; ++it)
    info.m_sampleRates.push_back(*it);

  info.m_dataFormats.push_back(AE_FMT_S16LE);  
  list.push_back(info);

  info.m_channels.Reset();
  info.m_dataFormats.clear();
  //info.m_sampleRates.clear();
  
  info.m_deviceType =  AE_DEVTYPE_HDMI; 
  info.m_deviceName = "HDMI";
  info.m_displayName = "HDMI";
  info.m_displayNameExtra = "HDMI Output";
  for (int i = 0; i < maxChannels; ++i)
    info.m_channels += s_chMap[i];

  //info.m_sampleRates.push_back(48000);  
  std::vector<AEDataFormat>::const_iterator itFormat, itFormatEnd = formats.end();
  for (itFormat = formats.begin(); itFormat != itFormatEnd; ++itFormat)
    info.m_dataFormats.push_back(*itFormat);

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
  //info.m_sampleRates.push_back(12000);
  //info.m_sampleRates.push_back(24000);
  info.m_sampleRates.push_back(32000);
  info.m_sampleRates.push_back(44100);
  info.m_sampleRates.push_back(48000);
  info.m_sampleRates.push_back(88200);
  info.m_sampleRates.push_back(96000);
  info.m_sampleRates.push_back(176400);
  info.m_sampleRates.push_back(192000);
  info.m_dataFormats.push_back(AE_FMT_S24NE4);
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
  //info.m_sampleRates.push_back(12000);
  //info.m_sampleRates.push_back(24000);
  info.m_sampleRates.push_back(32000);
  info.m_sampleRates.push_back(44100);
  info.m_sampleRates.push_back(48000);
  info.m_sampleRates.push_back(88200);
  info.m_sampleRates.push_back(96000);
  info.m_sampleRates.push_back(176400);
  info.m_sampleRates.push_back(192000);
  info.m_dataFormats.push_back(AE_FMT_S16LE);
  list.push_back(info);
}

bool CAESinkIntelSMD::LoadEDID()
{
  UnloadEDID();

  bool ret = false;
  gdl_init();

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

bool CAESinkIntelSMD::CheckEDIDSupport( ismd_audio_format_t &format, int& iChannels, unsigned int& uiSampleRate, unsigned int& uiSampleSize, bool& bAC3Encode )
{
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
    for( int z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( s_edidRates[z] == uiSampleRate )
      {
        if( !(cur->sample_rates & (1<<z)) )
        {
          // For now, try force resample to 48khz and worry if that isn't supported
          if( !(cur->sample_rates & 4) )
            CLog::Log(LOGERROR, "%s your audio sink indicates it doesn't support 48khz", __DEBUG_ID__);
          suggSampleRate = 48000;
          bSuggested = true;
        }
        break;
      }
    }

    // Last, see if the sample size is supported for PCM
    if( ISMD_AUDIO_MEDIA_FMT_PCM == format )
    {
      for( int y = 0; y < sizeof(s_edidSampleSizes); y++ )
      {
        if( s_edidSampleSizes[y] == uiSampleSize )
        {
          if( !(cur->sample_sizes & (1<<y)) )
          {
            // For now, try force resample to 16khz and worry if that isn't supported
            if( !(cur->sample_rates & 1) )
              CLog::Log(LOGERROR, "%s your audio sink indicates it doesn't support 16bit/sample", __DEBUG_ID__);
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

  if (!bFormatSupported && format == ISMD_AUDIO_MEDIA_FMT_DD_PLUS && ISMD_SUCCESS == ismd_audio_codec_available((ismd_audio_format_t) ISMD_AUDIO_ENCODE_FMT_AC3))
  {
    bool bAC3Support = false;
    // check for AC3 support
    for( cur = m_edidTable; cur; cur = cur->next )
    {
      // Check by format and channels
      if( cur->format != ISMD_AUDIO_MEDIA_FMT_DD )
      {
        bAC3Support = true;
        break;
      }
    }

    if (bAC3Support)
    {
      bAC3Encode = true;
      bFormatSupported = true;
      bFullMatch = true;
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


#endif
