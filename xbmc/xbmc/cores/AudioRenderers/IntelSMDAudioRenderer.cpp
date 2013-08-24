#include "system.h"
#include "Settings.h"
#include "GUISettings.h"
#include "IntelSMDAudioRenderer.h"
#include <cmath>

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

#include "IntelSMDGlobals.h"
#include "AudioContext.h"
#include "cores/dvdplayer/DVDClock.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#include "AdvancedSettings.h"
#include "Application.h"


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
  #include "ffmpeg/libavcodec/avcodec.h"
#endif
}

using namespace std;

CCriticalSection CIntelSMDAudioRenderer::m_SMDAudioLock;

static const unsigned int s_edidRates[] = {32000,44100,48000,88200,96000,176400,192000};
static const unsigned int s_edidSampleSizes[] = {16,20,24,32};
CIntelSMDAudioRenderer::edidHint* CIntelSMDAudioRenderer::m_edidTable = NULL;

ismd_dev_t tmpDevice = -1;

CIntelSMDAudioRenderer::CIntelSMDAudioRenderer()
{
  m_bIsAllocated = false;
  m_bFlushFlag = true;
  m_dwChunkSize = 0;
  m_dwBufferLen = 0;
  m_lastPts = ISMD_NO_PTS;
  m_lastSync = ISMD_NO_PTS;
  
  m_channelMap = NULL;
  m_uiBitsPerSample = 0;
  m_uiChannels = 0;
  m_audioMediaFormat = AUDIO_MEDIA_FMT_PCM;
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
  m_nCurrentVolume = 0;
}




bool CIntelSMDAudioRenderer::Initialize(IAudioCallback* pCallback,
                                   int iChannels,
                                   enum PCMChannels* channelMap,
                                   unsigned int uiSamplesPerSec,
                                   unsigned int uiBitsPerSample,
                                   bool bResample,
                                   const char* strAudioCodec,
                                   bool bIsMusic,
                                   bool bPassthrough,
                                   bool bTimed,
                                   AudioMediaFormat audioMediaFormat)
{
  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer Initialize name %s iChannels %d uiSamplesPerSec %d uiBitsPerSample %d audioMediaFormat %d bPassthrough %d bTimed %d\n",
      strAudioCodec, iChannels, uiSamplesPerSec, uiBitsPerSample, audioMediaFormat, bPassthrough, bTimed);

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t result;

  g_lic_settings.Load();
  g_lic_settings.dump_audio_licnense_file();

  CStdString strName(strAudioCodec);

  ismd_audio_processor_t  audioProcessor = -1;
  ismd_audio_format_t     ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;
  AUDIO_CODEC_VENDOR decoding_vendor = AUDIO_VENDOR_NONE;

  m_dwChunkSize = 8192;
  m_dwBufferLen = 8192;

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
  int inputChannelConfig = AUDIO_CHAN_CONFIG_INVALID;

  m_bIsHDMI = (AUDIO_HDMI == audioOutputMode);
  m_bIsSPDIF = (AUDIO_IEC958 == audioOutputMode);
  m_bIsAnalog = (AUDIO_ANALOG == audioOutputMode);
  m_bIsAllOutputs = (audioOutputMode == AUDIO_ALL_OUTPUTS);
  m_bAC3Encode = false;

  bool bIsUIAudio = strName.Equals("gui", false);
  bool bIsDVB = strName.Equals("dvb", false);
  bool bHardwareDecoder = false;

  if(bIsUIAudio)
    bIsMusic = false;

  if(m_bIsAllOutputs)
    m_bIsHDMI = m_bIsSPDIF = m_bIsAnalog = true;

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize DVB %d UI %d Music %d", bIsDVB, bIsUIAudio, bIsMusic);
  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize Outputs: HDMI %d SPDIF %d I2C %d", m_bIsHDMI, m_bIsSPDIF, m_bIsAnalog);

  audioProcessor = g_IntelSMDGlobals.GetAudioProcessor();
  if(audioProcessor == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize audioProcessor is not valid");
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

  m_bTimed = bTimed;

  m_audioDevice = g_IntelSMDGlobals.CreateAudioInput(m_bTimed);
  if(m_audioDevice == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize failed to create audio input");
    return false;
  }

  if(!bIsUIAudio)
    g_IntelSMDGlobals.SetPrimaryAudioDevice(m_audioDevice);

  m_audioDeviceInput = g_IntelSMDGlobals.GetAudioDevicePort(m_audioDevice);
  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize failed to create audio input port");
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

  ismdAudioInputFormat = GetISMDFormat(audioMediaFormat);
  decoding_vendor = GetAudioCodecVendor(audioMediaFormat);

  // Can we do hardware decode?
  bHardwareDecoder = (ismd_audio_codec_available(ismdAudioInputFormat) == ISMD_SUCCESS);
  if(ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_PCM)
    bHardwareDecoder = false;

  // sets some defaults
  if(uiBitsPerSample == 0)
    uiBitsPerSample = 16;
  if (iChannels == 0)
    iChannels = 2;

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize ismdAudioInputFormat %d Hardware decoding (non PCM) %d\n",
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

  // Configure modes for CODECS
  switch( audioMediaFormat )
  {
    case AUDIO_MEDIA_FMT_DD:
      CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize DD detected");
      bHDMIPassthrough = bSPDIFPassthrough = bAC3pass;
      ConfigureDolbyModes(g_IntelSMDGlobals.GetAudioProcessor(), m_audioDevice);
      break;
    case AUDIO_MEDIA_FMT_DD_PLUS:
      CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize DD Plus detected");

      // check special case for DD+->DD using DDCO
      bHDMIPassthrough = m_bIsHDMI && bEAC3pass;
      if(!bEAC3pass && bAC3pass)
      {
        m_bAC3Encode = true;
        CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize EAC3->AC3 transcoding is on");
        if( iChannels > 6 )
          iChannels = 6;
        bHDMIPassthrough = false;
      }
      ConfigureDolbyPlusModes(g_IntelSMDGlobals.GetAudioProcessor(), m_audioDevice, m_bAC3Encode);
      break;
    case AUDIO_MEDIA_FMT_DTS:
    case AUDIO_MEDIA_FMT_DTS_LBR:
      CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize DTS detected");
      bHDMIPassthrough = bSPDIFPassthrough = bDTSPass;
      ConfigureDTSModes(g_IntelSMDGlobals.GetAudioProcessor(), m_audioDevice);
      break;
    case AUDIO_MEDIA_FMT_DTS_HD:
    case AUDIO_MEDIA_FMT_DTS_HD_MA:
    case AUDIO_MEDIA_FMT_DTS_HD_HRA:
      CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize DTS-HD detected");
      bHDMIPassthrough = m_bIsHDMI && bDTSHDPass;
      break;
    case AUDIO_MEDIA_FMT_TRUE_HD:
      CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer::Initialize TrueHD detected");
      bHDMIPassthrough = m_bIsHDMI && bTruehdPass;
      ConfigureDolbyTrueHDModes(g_IntelSMDGlobals.GetAudioProcessor(), m_audioDevice);
      break;
    case AUDIO_MEDIA_FMT_PCM:
      switch( iChannels )
      {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
          m_dwChunkSize = 8 * 1024;
          break;
        case 3:
        case 5:
        case 6:
        case 7:
          m_dwChunkSize = iChannels * 1024;
          break;
      };
      if(!m_bTimed) // for music
        m_dwChunkSize = 0;
      inputChannelConfig = BuildChannelConfig( channelMap, iChannels );
      break;
    default:
      break;
  }

  if (ismdAudioInputFormat == ISMD_AUDIO_MEDIA_FMT_PCM)
  {
    CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize PCM sample size %d sample rate %d channel config 0x%8x",
            uiBitsPerSample, uiSamplesPerSec, inputChannelConfig);
    result = ismd_audio_input_set_pcm_format(audioProcessor, m_audioDevice, uiBitsPerSample, uiSamplesPerSec, inputChannelConfig);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize - ismd_audio_input_set_pcm_format: %d", result);
      return false;
    }


    // when playing 1 channel PCM, we want to upmix this to 2 channels
    if(iChannels == 1)
    {
      unsigned int input_channel;
      unsigned int output_channel;
      ismd_audio_channel_mix_config_t mix_config;

      // mute all outputs on all inputs
      for(input_channel = 0; input_channel < AUDIO_MAX_INPUT_CHANNELS; input_channel++)
      {
        mix_config.input_channels_map[input_channel].input_channel_id = input_channel;
        for(output_channel = 0; output_channel < AUDIO_MAX_OUTPUT_CHANNELS; output_channel++)
          mix_config.input_channels_map[input_channel].output_channels_gain[output_channel] = -1450; // mute
      }

      // unmute 0->0 and 0->2 (L and R)
      mix_config.input_channels_map[0].output_channels_gain[0] = 0;
      mix_config.input_channels_map[0].output_channels_gain[2] = 0;

      ismd_audio_input_set_channel_mix(audioProcessor, m_audioDevice, mix_config);
    }
  }

  if(m_bIsAllOutputs)
  {
    bHDMIPassthrough = bSPDIFPassthrough = false;

    if(audioMediaFormat == AUDIO_MEDIA_FMT_DD && bAC3pass)
      bSPDIFPassthrough = true;
  }

  if(bHDMIPassthrough || bSPDIFPassthrough)
  {
    m_dwBufferLen = 8 * 1024;
    m_dwChunkSize = 0;
  }

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize HDMI passthrough %d SPDIF passthrough %d buffer size %d chunk size %d",
      bHDMIPassthrough, bSPDIFPassthrough, m_dwBufferLen, m_dwChunkSize);

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize testing audio license: channels %d vendor %d", iChannels, decoding_vendor);
  g_lic_settings.ModifyAudioChannelsByLicense(iChannels, decoding_vendor);
  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize after license test: channels %d", iChannels);

  // Configure output ports
  if(bIsUIAudio)
  {
    uiBitsPerSample = 16;
    uiSamplesPerSec = 48000;
  }

  unsigned int outputFreq = 48000;

  // I2S. Nothing to touch here. we always use defaults

  // SPDIF
  if(m_bIsSPDIF)
  {
    ismd_audio_output_t OutputSPDIF = g_IntelSMDGlobals.GetSPDIFOutput();
    ismd_audio_output_config_t spdif_output_config;
    unsigned int samplesPerSec = uiSamplesPerSec;
    unsigned int bitsPerSec = uiBitsPerSample;
    if(m_bIsAllOutputs)
    {
      samplesPerSec = 48000;
      bitsPerSec = 16;
    }

    ConfigureAudioOutputParams(spdif_output_config, AUDIO_IEC958, bitsPerSec,
        samplesPerSec, iChannels, ismdAudioInputFormat, bSPDIFPassthrough);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputSPDIF, spdif_output_config))
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize ConfigureAudioOutput SPDIF failed %d", result);
      return false;
    }
    outputFreq = spdif_output_config.sample_rate;
  }

  // HDMI
  if(m_bIsHDMI && !m_bIsAllOutputs)
  {
    ismd_audio_output_t OutputHDMI = g_IntelSMDGlobals.GetHDMIOutput();
    ismd_audio_output_config_t hdmi_output_config;
    ConfigureAudioOutputParams(hdmi_output_config, AUDIO_HDMI, uiBitsPerSample,
      uiSamplesPerSec, iChannels, ismdAudioInputFormat, bHDMIPassthrough);
    if(!g_IntelSMDGlobals.ConfigureAudioOutput(OutputHDMI, hdmi_output_config))
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize ConfigureAudioOutput HDMI failed %d", result);
      return false;
    }
    outputFreq = hdmi_output_config.sample_rate;
  }

  // Configure the master clock frequency

  if(m_bIsAllOutputs)
    outputFreq = 48000;

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize ConfigureMasterClock %d", outputFreq);
  g_IntelSMDGlobals.ConfigureMasterClock(outputFreq);

  if(bIsUIAudio)
      ismd_audio_input_set_as_secondary(audioProcessor, m_audioDevice);
  else if(!bIsMusic || (bIsMusic && (bHDMIPassthrough || bSPDIFPassthrough)))
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
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize  ismd_audio_input_set_as_primary failed %d", result);
      return false;
    }
  }

  if(!g_IntelSMDGlobals.EnableAudioInput(m_audioDevice))
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize  EnableAudioInput");
    return false;
  }

  // enable outputs
  if (m_bIsHDMI)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetHDMIOutput()))
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize  EnableAudioOutput HDMI failed");
      return false;
    }
  }

  if (m_bIsSPDIF)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetSPDIFOutput()))
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize  EnableAudioOutput SPDIF failed");
      return false;
    }
  }

  if (m_bIsAnalog)
  {
    if(!g_IntelSMDGlobals.EnableAudioOutput(g_IntelSMDGlobals.GetI2SOutput()))
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize  EnableAudioOutput I2S failed");
      return false;
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

  m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
  g_IntelSMDGlobals.SetMasterVolume(m_nCurrentVolume);

  if(m_pChunksCollected)
    delete m_pChunksCollected;

  m_bPause = false;
  m_pChunksCollected = new unsigned char[m_dwBufferLen];
  m_first_pts = ISMD_NO_PTS;
  m_lastPts = ISMD_NO_PTS;
  m_ChunksCollectedSize = 0;

  m_bIsAllocated = true;

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Initialize done");

  return true;
}

CIntelSMDAudioRenderer::~CIntelSMDAudioRenderer()
{
  Deinitialize();
}

static const int _default_channel_layouts[] =
  { AUDIO_CHAN_CONFIG_2_CH,     // if channel count is 0, assume stereo
    AUDIO_CHAN_CONFIG_1_CH,     // for PCM 1 channel we map to 2 channels
    AUDIO_CHAN_CONFIG_2_CH,
    0xFFFFF210,                 // 3CH: Left, Center, Right
    0xFFFF4320,                 // 4CH: Left, Right, Left Sur, Right Sur
    0xFFF43210,                 // 5CH: Left, Center, Right, Left Sur, Right Sur
    AUDIO_CHAN_CONFIG_6_CH,
    0xF6543210,                 // 7CH: Left, Center, Right, Left Sur, Right Sur, Left Rear Sur, Right Rear Sur
    AUDIO_CHAN_CONFIG_8_CH
  };
//static 
int CIntelSMDAudioRenderer::BuildChannelConfig( enum PCMChannels* channelMap, int iChannels )
{
  int res = 0;

  // SMD will have major issues should this ever occur.
  if( iChannels > 8 )
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::BuildChannelConfig received more than 8 channels? (%d)", iChannels);
    return 0;
  }

  if( !channelMap )
  {
    // if we don't have a mapping, it's guesswork. how do 4ch or 5ch streams work? who knows.
    // use some defaults from intel and try some other things here
    // mono and stereo are probably always right, though AUDIO_CHAN_CONFIG_1_CH is configured as center-only

    res = _default_channel_layouts[iChannels];

    if( iChannels > 2 )
      CLog::Log(LOGWARNING, "CIntelSMDAudioRenderer::BuildChannelConfig - using a default configuration for %d channels; your channel mapping may be wrong\n", iChannels);
  }
  else
  {
    int i;
    bool bBackToSur = false;     // if we are outputing 5.1 we may have to remap the back channels to surround
    bool bUsingSides = false;    // if we use the side channels this is true
    bool bUsingOffC = false;     // if we use left/right of center channels this is true
    for( i = 0; i < iChannels; i++ )
    {
      int val = -1;

      // PCM remap - we assume side channels are surrounds, but also support left/right of center being surrounds
      //  we can't, however, handle both channel sets being present in a stream.
      switch( channelMap[i] )
      {
        case PCM_FRONT_LEFT:    val = ISMD_AUDIO_CHANNEL_LEFT; break;
        case PCM_FRONT_RIGHT:   val = ISMD_AUDIO_CHANNEL_RIGHT; break;
        case PCM_FRONT_CENTER:  val = (iChannels > 1) ? ISMD_AUDIO_CHANNEL_CENTER : ISMD_AUDIO_CHANNEL_LEFT; break;
        case PCM_LOW_FREQUENCY: val = ISMD_AUDIO_CHANNEL_LFE; break;
        case PCM_BACK_LEFT:
        {
          if( iChannels == 8 )
            val = ISMD_AUDIO_CHANNEL_LEFT_REAR_SUR;
          else
          {
            val = ISMD_AUDIO_CHANNEL_LEFT_SUR;
            bBackToSur = true;
          }
          break;
        }
        case PCM_BACK_RIGHT:
        {
          if( iChannels == 8 )
            val = ISMD_AUDIO_CHANNEL_RIGHT_REAR_SUR;
          else
          {
            val = ISMD_AUDIO_CHANNEL_RIGHT_SUR;
            bBackToSur = true;
          }
          break;
        }
        case PCM_FRONT_LEFT_OF_CENTER:  val = ISMD_AUDIO_CHANNEL_LEFT_SUR; bUsingOffC = true; break;
        case PCM_FRONT_RIGHT_OF_CENTER: val = ISMD_AUDIO_CHANNEL_RIGHT_SUR; bUsingOffC = true; break;
        case PCM_BACK_CENTER:           break;   // not supported
        case PCM_SIDE_LEFT:     val = ISMD_AUDIO_CHANNEL_LEFT_SUR; bUsingSides = true; break;
        case PCM_SIDE_RIGHT:    val = ISMD_AUDIO_CHANNEL_RIGHT_SUR; bUsingSides = true; break;
        case PCM_TOP_FRONT_LEFT:        break;   // not supported
        case PCM_TOP_FRONT_RIGHT:       break;   // not supported
        case PCM_TOP_FRONT_CENTER:      break;   // not supported
        case PCM_TOP_CENTER:            break;   // not supported
        case PCM_TOP_BACK_LEFT:         break;   // not supported
        case PCM_TOP_BACK_RIGHT:        break;   // not supported
        case PCM_TOP_BACK_CENTER:       break;   // not supported
        default: break;
      };

      if( val == -1 )
      {
        CLog::Log(LOGWARNING, "CIntelSMDAudioRenderer::BuildChannelConfig - invalid channel %d\n", channelMap[i] );
        break;
      }

      if( bUsingSides && bUsingOffC )
      {
        CLog::Log(LOGWARNING, "CIntelSMDAudioRenderer::BuildChannelConfig - given both side and off center channels - aborting");
        break;
      }

      res |= (val << (4*i));
    }

    if( i < iChannels )
    {
      // Here we have a problem. The source content has a channel layout that we couldn't convert, so at this point we are better off using a
      // default channel layout. We'll never get the layout to be accurate anyways, and at least this way SMD is seeing the right amount of PCM...
      CLog::Log(LOGWARNING, "CIntelSMDAudioRenderer::BuildChannelConfig - switching back to default channel layout since not all channels were found");
      res = _default_channel_layouts[iChannels];
    }
    else
    {
      if( bBackToSur )
      {
        CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::BuildChannelConfig - remapping back channels to surround speakers for 5.1 audio out");
      }
      // mark the remaining slots invalid
      for( int i = iChannels; i < 8; i++ )
      {
        res |= (ISMD_AUDIO_CHANNEL_INVALID << (i*4));
      }
    }
  }

  CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer::BuildChannelConfig - %d channels layed out as 0x%08x\n", iChannels, res);
  return res;
}

bool CIntelSMDAudioRenderer::Deinitialize()
{
  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Deinitialize");

  if(!m_bIsAllocated)
    return true;

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

  return true;
}

void CIntelSMDAudioRenderer::Flush() 
{
  CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer %s", __FUNCTION__);

  g_IntelSMDGlobals.FlushAudioDevice(m_audioDevice);

  if(m_bTimed)
    m_bFlushFlag = true;

  m_lastPts = ISMD_NO_PTS;
  m_lastSync = ISMD_NO_PTS;
}

void CIntelSMDAudioRenderer::Resync(double pts)
{

}

bool CIntelSMDAudioRenderer::Pause()
{
  CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer %s\n", __FUNCTION__);

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

//***********************************************************************************************
bool CIntelSMDAudioRenderer::Resume()
{
  CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer %s\n", __FUNCTION__);

  if (!m_bIsAllocated)
    return false;

  if(!m_bFlushFlag)
  {
    g_IntelSMDGlobals.SetAudioDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime(), m_audioDevice);
    g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_PLAY, m_audioDevice);
  }
  else
    CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::Resume flush flag is set, ignoring request\n");

  m_bPause = false;

  return true;
}

//***********************************************************************************************
bool CIntelSMDAudioRenderer::Stop()
{
  CLog::Log(LOGDEBUG, "CIntelSMDAudioRenderer %s\n", __FUNCTION__);

  if(m_audioDevice == -1)
    return true;

  g_IntelSMDGlobals.SetAudioDeviceState(ISMD_DEV_STATE_STOP, m_audioDevice);
  g_IntelSMDGlobals.FlushAudioDevice(m_audioDevice);

  return true;
}

//***********************************************************************************************
long CIntelSMDAudioRenderer::GetCurrentVolume() const
{
  return m_nCurrentVolume;
}

//***********************************************************************************************
void CIntelSMDAudioRenderer::Mute(bool bMute)
{
  g_IntelSMDGlobals.Mute(bMute);
}

//***********************************************************************************************
bool CIntelSMDAudioRenderer::SetCurrentVolume(long nVolume)
{
  // If we fail or if we are doing passthrough, don't set
  if (!m_bIsAllocated )
    return false;

  m_nCurrentVolume = nVolume;
  return g_IntelSMDGlobals.SetMasterVolume(nVolume);
}


//***********************************************************************************************
unsigned int CIntelSMDAudioRenderer::GetSpace()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetSpace - inputPort == -1");
    return 0;
  }

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetSpace - error getting port status: %d", smd_ret);
    return 0;
  }

  //printf("max depth = %d cur depth = %d\n", portStatus.max_depth,  portStatus.cur_depth);
  unsigned int result = (maxDepth - curDepth) * m_dwBufferLen;
  return result;
}

unsigned int CIntelSMDAudioRenderer::SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size, ismd_pts_t local_pts)
{
  ismd_result_t smd_ret;
  ismd_buffer_handle_t ismdBuffer;
  ismd_es_buf_attr_t *buf_attrs;
  ismd_buffer_descriptor_t ismdBufferDesc;

  //printf("audio packet size %d\n", buffer_size);

  if(m_dwBufferLen < buffer_size)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput data size %d is bigger that smd buffer size %d\n",
        buffer_size, m_dwBufferLen);
    return 0;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput - inputPort == -1");
    return 0;
  }

  smd_ret = ismd_buffer_alloc(m_dwBufferLen, &ismdBuffer);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput - error allocating buffer: %d", smd_ret);
    return 0;
  }

  smd_ret = ismd_buffer_read_desc(ismdBuffer, &ismdBufferDesc);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput - error reading descriptor: %d", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    return 0;
  }

  unsigned char* buf_ptr = (uint8_t *) OS_MAP_IO_TO_MEM_NOCACHE(ismdBufferDesc.phys.base, buffer_size);
  if(buf_ptr == NULL)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput - unable to mmap buffer %d", ismdBufferDesc.phys.base);
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
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput - error updating descriptor: %d", smd_ret);
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
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::SendDataToInput failed to write buffer %d\n", smd_ret);
    ismd_buffer_dereference(ismdBuffer);
    buffer_size = 0;
  }

  return buffer_size;
}

//***********************************************************************************************
unsigned int CIntelSMDAudioRenderer::AddPackets(const void* data, unsigned int len, double pts, double duration)
{
  CSingleLock lock(m_SMDAudioLock);

  ismd_pts_t local_pts = ISMD_NO_PTS;
  unsigned char* pBuffer = NULL;
  unsigned int total = 0;
  unsigned int dataSent = 0;
  if (!m_bIsAllocated)
  {
    CLog::Log(LOGERROR,"CIntelSMDAudioRenderer::AddPackets - sanity failed. no valid play handle!");
    return len;
  }

  // don't do anything if demuxer is connected directly to decoder
  if(g_IntelSMDGlobals.IsDemuxToAudio())
    return len;

  if (m_bTimed)
  {
    local_pts = g_IntelSMDGlobals.DvdToIsmdPts(pts);
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

  // What's the maximal write size - either a chunk if provided, or the full buffer
  unsigned int block_size = (m_dwChunkSize ? m_dwChunkSize : m_dwBufferLen);

  //printf("len %d chunksize %d m_bTimed %d\n", len, m_dwChunkSize, m_bTimed);

  while (len && len >= m_dwChunkSize) // We want to write at least one chunk at a time
  {
    unsigned int bytes_to_copy = len > block_size ? block_size : len;

    //printf("bytes_to_copy %d block size %d m_ChunksCollectedSize %d\n", bytes_to_copy, block_size, m_ChunksCollectedSize);

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
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::AddPackets SendDataToInput failed. req %d actual %d", len, dataSent);
      return 0;
    }

    pBuffer += dataSent; // Update buffer pointer
    len -= dataSent; // Update remaining data len

    if( local_pts != ISMD_NO_PTS )
      m_lastPts = local_pts;
  }

  return total - len; // Bytes used
}

//***********************************************************************************************
float CIntelSMDAudioRenderer::GetDelay()
{
  return 0.0f;
}

float CIntelSMDAudioRenderer::GetCacheTime()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated)
  {
    return 0.0f;
  }

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetCacheTime - inputPort == -1");
    return 0;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);

  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetCacheTime - error getting port status: %d", smd_ret);
    return 0;
  }

  float result = ((float) (curDepth * m_dwBufferLen));

  return result;
}

unsigned int CIntelSMDAudioRenderer::GetChunkLen()
{
  return m_dwChunkSize;
}

int CIntelSMDAudioRenderer::SetPlaySpeed(int iSpeed)
{
  return 0;
}

void CIntelSMDAudioRenderer::RegisterAudioCallback(IAudioCallback *pCallback)
{
  m_pCallback = pCallback;
}

void CIntelSMDAudioRenderer::UnRegisterAudioCallback()
{
  m_pCallback = NULL;
}

void CIntelSMDAudioRenderer::WaitCompletion()
{
  unsigned int curDepth = 0;
  unsigned int maxDepth = 0;

  if (!m_bIsAllocated || m_bPause)
    return;

  if(m_audioDeviceInput == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetSpace - inputPort == -1");
    return;
  }

  CSingleLock lock(m_SMDAudioLock);

  ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
  if (smd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::WaitCompletion - error getting port status: %d", smd_ret);
    return;
  }

  while (curDepth != 0)
  {
    usleep(5000);

    ismd_result_t smd_ret = g_IntelSMDGlobals.GetPortStatus(m_audioDeviceInput, curDepth, maxDepth);
    if (smd_ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::WaitCompletion - error getting port status: %d", smd_ret);
      return;
    }
  } 
}

void CIntelSMDAudioRenderer::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
  return;
}

// Based on ismd examples
bool CIntelSMDAudioRenderer::LoadEDID()
{
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
void CIntelSMDAudioRenderer::UnloadEDID()
{
  while( m_edidTable )
  {
    edidHint* nxt = m_edidTable->next;
    delete[] m_edidTable;
    m_edidTable = nxt;
  }
}
void CIntelSMDAudioRenderer::DumpEDID()
{
  char* formats[] = {"invalid","LPCM","DVDPCM","BRPCM","MPEG","AAC","AACLOAS","DD","DD+","TrueHD","DTS-HD","DTS-HD-HRA","DTS-HD-MA","DTS","DTS-LBR","WM9"};
  CLog::Log(LOGINFO,"HDMI Audio sink supports the following formats:\n");
  for( edidHint* cur = m_edidTable; cur; cur = cur->next )
  {
    CStdString line;
    line.Format("* Format: %s Max channels: %d Samplerates: ", formats[cur->format], cur->channels);

    for( int z = 0; z < sizeof(s_edidRates); z++ )
    {
      if( cur->sample_rates & (1<<z) )
      {
        line.AppendFormat("%d ",s_edidRates[z]);
      }         
    }
    if( ISMD_AUDIO_MEDIA_FMT_PCM == cur->format )
    {
      line.AppendFormat("Sample Sizes: ");
      for( int z = 0; z< sizeof(s_edidSampleSizes); z++ )
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

bool CIntelSMDAudioRenderer::CheckEDIDSupport( ismd_audio_format_t format, int& iChannels, unsigned int& uiSampleRate, unsigned int& uiSampleSize )
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
            CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize - your audio sink indicates it doesn't support 48khz"); 
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
              CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::Initialize - your audio sink indicates it doesn't support 16bit/sample");
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
ismd_audio_format_t CIntelSMDAudioRenderer::MapGDLAudioFormat( gdl_hdmi_audio_fmt_t f )
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

void CIntelSMDAudioRenderer::ConfigureDolbyModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
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

void CIntelSMDAudioRenderer::ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode)
{
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

void CIntelSMDAudioRenderer::ConfigureDolbyTrueHDModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
  ismd_result_t result;
  ismd_audio_decoder_param_t param;

  int drcmode = g_guiSettings.GetInt("audiooutput.dd_truehd_drc");
  int drc_percent = g_guiSettings.GetInt("audiooutput.dd_truehd_drc_percentage");

  //Dolby TrueHD DRC is on by default. We require the ability to set it off and auto mode.
  // This command sets the DRC mode: 0 off, 1 auto (default), 2 forced.
  ismd_audio_truehd_drc_enable_t *drc_control =
      (ismd_audio_truehd_drc_enable_t *) &param;

  if (drcmode == DD_TRUEHD_DRC_AUTO)
    *drc_control = 1; // Set to auto
  else if (drcmode == DD_TRUEHD_DRC_OFF)
    *drc_control = 0;
  else if (drcmode == DD_TRUEHD_DRC_ON)
    *drc_control = 2;
  CLog::Log(LOGNONE, "ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_ENABLE %d", *drc_control);
  result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
      ISMD_AUDIO_TRUEHD_DRC_ENABLE, &param);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_ENABLE failed %d", result);
  }

  if (drcmode == DD_TRUEHD_DRC_ON)
  {
    ismd_audio_truehd_drc_boost_t *dobly_drc_percent = (ismd_audio_truehd_drc_enable_t *) &param;
    *dobly_drc_percent = drc_percent;

    CLog::Log(LOGNONE, "ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_BOOST %d", *dobly_drc_percent);
    result = ismd_audio_input_set_decoder_param(proc_handle, input_handle,
        ISMD_AUDIO_TRUEHD_DRC_BOOST, &param);
    if (result != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CIntelSMDGlobals::ConfigureDolbyTrueHDModes ISMD_AUDIO_TRUEHD_DRC_BOOST failed %d", result);
    }
  }

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

void CIntelSMDAudioRenderer::ConfigureDTSModes(
    ismd_audio_processor_t proc_handle, ismd_dev_t input_handle)
{
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


ismd_audio_format_t CIntelSMDAudioRenderer::GetISMDFormat(AudioMediaFormat audioMediaFormat)
{
  ismd_audio_format_t ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_INVALID;

  switch (audioMediaFormat)
    {
     case AUDIO_MEDIA_FMT_PCM:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_PCM;
       break;
     case AUDIO_MEDIA_FMT_DVD_PCM:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DVD_PCM;
       break;
     case AUDIO_MEDIA_FMT_BLURAY_PCM:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_BLURAY_PCM;
       break;
     case AUDIO_MEDIA_FMT_MPEG:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_MPEG;
       break;
     case AUDIO_MEDIA_FMT_AAC:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_AAC;
       break;
     case AUDIO_MEDIA_FMT_AAC_LOAS:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_AAC_LOAS;
       break;
     case AUDIO_MEDIA_FMT_AAC_LATM:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_AAC_LOAS;
       break;
     case AUDIO_MEDIA_FMT_DD:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DD;
       break;
     case AUDIO_MEDIA_FMT_DD_PLUS:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DD_PLUS;
       break;
     case AUDIO_MEDIA_FMT_TRUE_HD:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_TRUE_HD;
       break;
     case AUDIO_MEDIA_FMT_DTS_HD:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_HD;
       break;
     case AUDIO_MEDIA_FMT_DTS_HD_HRA:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_HD_HRA;
       break;
     case AUDIO_MEDIA_FMT_DTS_HD_MA:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_HD_MA;
       break;
     case AUDIO_MEDIA_FMT_DTS:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS;
       break;
     case AUDIO_MEDIA_FMT_DTS_LBR:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_DTS_LBR;
       break;
     case AUDIO_MEDIA_FMT_WM9:
       ismdAudioInputFormat = ISMD_AUDIO_MEDIA_FMT_WM9;
       break;
     default:
       CLog::Log(LOGERROR, "CIntelSMDAudioRenderer::GetISMDFormat - unknown audio media format requested");
       return ISMD_AUDIO_MEDIA_FMT_INVALID;
    }

  return ismdAudioInputFormat;
}

AUDIO_CODEC_VENDOR  CIntelSMDAudioRenderer::GetAudioCodecVendor(AudioMediaFormat format)
{
  AUDIO_CODEC_VENDOR decoding_vendor = AUDIO_VENDOR_NONE;

  switch(format)
  {
    case AUDIO_MEDIA_FMT_DD:
      decoding_vendor = AUDIO_VENDOR_DOLBY;
      break;
    case AUDIO_MEDIA_FMT_DD_PLUS:
      decoding_vendor = AUDIO_VENDOR_DOLBY;
      break;
    case AUDIO_MEDIA_FMT_TRUE_HD:
      decoding_vendor = AUDIO_VENDOR_DOLBY;
      break;
    case AUDIO_MEDIA_FMT_DTS_HD:
      decoding_vendor = AUDIO_VENDOR_DTS;
      break;
    case AUDIO_MEDIA_FMT_DTS_HD_HRA:
      decoding_vendor = AUDIO_VENDOR_DTS;
      break;
    case AUDIO_MEDIA_FMT_DTS_HD_MA:
      decoding_vendor = AUDIO_VENDOR_DTS;
      break;
    case AUDIO_MEDIA_FMT_DTS:
      decoding_vendor = AUDIO_VENDOR_DTS;
      break;
    case AUDIO_MEDIA_FMT_DTS_LBR:
      decoding_vendor = AUDIO_VENDOR_DTS;
      break;
    default:
      break;
  }

  return decoding_vendor;
}

void CIntelSMDAudioRenderer::ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config, int output,
    int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough)
{
  bool bUseEDID = g_guiSettings.GetBool("videoscreen.forceedid");
  bool bLPCMMode = g_guiSettings.GetBool("audiooutput.lpcm71passthrough");

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::ConfigureAudioOutputParams %s sample size %d sample rate %d channels %d format %d",
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
      CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::ConfigureAudioOutputParams Testing EDID for sample rate %d sample size %d", suggSampleRate, suggSampleSize);
      if (CheckEDIDSupport(bPassthrough ? format : ISMD_AUDIO_MEDIA_FMT_PCM, suggChannels, suggSampleRate, suggSampleSize))
      {
        output_config.sample_rate = suggSampleRate;
        output_config.sample_size = suggSampleSize;
        CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::ConfigureAudioOutputParams EDID results sample rate %d sample size %d", suggSampleRate, suggSampleSize);
      } else
        CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::ConfigureAudioOutputParams no EDID support found");
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

  CLog::Log(LOGINFO, "CIntelSMDAudioRenderer::ConfigureAudioOutputParams stream_delay %d sample_size %d \
ch_config %d out_mode %d sample_rate %d ch_map %d",
    output_config.stream_delay, output_config.sample_size, output_config.ch_config,
    output_config.out_mode, output_config.sample_rate, output_config.ch_map);
}

void CIntelSMDAudioRenderer::SetDefaultOutputConfig(ismd_audio_output_config_t& output_config)
{
  output_config.ch_config = ISMD_AUDIO_STEREO;
  output_config.ch_map = 0;
  output_config.out_mode = ISMD_AUDIO_OUTPUT_PCM;
  output_config.sample_rate = 48000;
  output_config.sample_size = 16;
  output_config.stream_delay = 0;
}

#endif
