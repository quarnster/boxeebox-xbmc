/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include <config.h>
#include "system.h"
#ifdef HAS_INTEL_SMD
 
#include "DVDAudioCodecSMD.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "DVDPlayerAudio.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "Settings.h"
#include "utils/log.h"
#include "IntelSMDGlobals.h"

CDVDAudioCodecSMD::CDVDAudioCodecSMD(void)
{
  m_Codec = CODEC_ID_NONE;
  
  m_bHardwareDecode = false;
  m_DTSHDDetection = 0;
  m_bIsDTSHD = false;
  m_bBitstreamDTSHD = false;
  m_pStateDTS = NULL;
  
  m_frameBytes = 0;
  m_iOutputSize = 0;

  m_sample_size = 0;
  m_channel_count = 0;
  m_sample_rate = 0;
}

CDVDAudioCodecSMD::~CDVDAudioCodecSMD(void)
{
  Dispose();
}

bool CDVDAudioCodecSMD::IsPassthroughAudioCodec( int Codec )
{
  bool bSupportsAC3Out = false;
  bool bSupportsEAC3Out = false;
  bool bSupportsDTSOut = false;
  bool bSupportsDTSHDOut = false;
  bool bSupportsTrueHDOut = false;

  if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI ||
      g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_SPDIF)
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }
  if( g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI )
  {
    bSupportsEAC3Out = g_guiSettings.GetBool("audiooutput.eac3passthrough");
    bSupportsTrueHDOut = g_guiSettings.GetBool("audiooutput.truehdpassthrough");
    bSupportsDTSHDOut = g_guiSettings.GetBool("audiooutput.dtshdpassthrough");
  }

  // On ISMD we support hardware downmix of EAC3 to AC3
  bSupportsEAC3Out = bSupportsAC3Out;

  // NOTE - DTS-HD is not a codec setting and is always detected after the first two frames
  if ((Codec == CODEC_ID_AC3 && bSupportsAC3Out)
    || (Codec == CODEC_ID_EAC3 && bSupportsEAC3Out)
    || (Codec == CODEC_ID_DTS && bSupportsDTSOut)
    || (Codec == CODEC_ID_TRUEHD && bSupportsTrueHDOut))
  {

    return true;
  }

  return false;
}
bool CDVDAudioCodecSMD::IsHWDecodeAudioCodec( int Codec )
{
  return g_IntelSMDGlobals.CheckCodecHWDecode( Codec );
}

bool CDVDAudioCodecSMD::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsHWDecode = IsHWDecodeAudioCodec( hints.codec );
  bool bSupportsPassthrough = IsPassthroughAudioCodec( hints.codec );

  // Detect DTS-HD streams; allow up to 3 regular DTS frames before we give up and treat as DTS only
  if( hints.codec == CODEC_ID_DTS && g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL_HDMI)
  {
    m_DTSHDDetection = 3;
    m_bBitstreamDTSHD = g_guiSettings.GetBool("audiooutput.dtshdpassthrough");

    if (!m_dllDTS.Load())
      return false;
    m_pStateDTS = m_dllDTS.dts_init(0);
    if(m_pStateDTS == NULL)
      return false;
  }

  // Caller has requested bitstream only; if we don't support bitstream for this format
  // exit now, otherwise we'll end up hardware decoding
  if( hints.bitstream && !bSupportsPassthrough )
  {
    return false;
  }
  
  // Format isn't supported
  if( !bSupportsPassthrough && !bSupportsHWDecode )
  {
    return false;
  }
  
  if( !bSupportsPassthrough )
    m_bHardwareDecode = true;
  
  // init stream info from hints until we can get these from SMD
  m_sample_size = hints.bitspersample;
  m_channel_count = hints.channels;
  m_sample_rate = hints.samplerate;
  m_Codec = hints.codec;

  CLog::Log(LOGINFO, "CDVDAudioCodecSMD::Open successful. codec %d bSupportsHWDecode %d bSupportsPassthrough %d\n",
        hints.codec, bSupportsHWDecode, bSupportsPassthrough);

  return true;    
}

#define MAKE_DWORD(x) ((unsigned int)((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))
bool CDVDAudioCodecSMD::DetectDTSHD(BYTE* pData, int iSize)
{
  if( iSize < 4 ) return false;
  
  unsigned int syncword = MAKE_DWORD( &pData[0] );
  return ( 0x64582025 == syncword );
}

void CDVDAudioCodecSMD::Dispose()
{
  if( m_pStateDTS )
  {
    m_dllDTS.dts_free(m_pStateDTS);
    m_pStateDTS = NULL;
  }
}

int CDVDAudioCodecSMD::Decode(BYTE* pData, int iSize)
{
  int len = 0;
  
  //printf("CDVDAudioCodecSMD decode\n");

  // Copy this frame over in all cases
  memcpy(m_frameCache + m_frameBytes, pData, iSize);
  m_frameBytes += iSize;
  
  if( CODEC_ID_DTS == m_Codec && !m_bIsDTSHD && m_DTSHDDetection > 0 && m_bBitstreamDTSHD)
  {
    int ignore1=0, ignore2=0, ignore3=0, flags=0;
    int iFrameSize = m_dllDTS.dts_syncinfo(m_pStateDTS, pData, &flags, &ignore1, &ignore2, &ignore3);

    if( iFrameSize < iSize )
      m_bIsDTSHD = DetectDTSHD( pData + iFrameSize, iSize-iFrameSize );
    m_DTSHDDetection--;
    
    if( !m_bIsDTSHD )
    {
      // Don't let audio initialize; do this by indicating we haven't yet decoded a frame
      len = iSize;
      m_iOutputSize = 0;
    }
    else
    {
      // We detected DTS-HD
      // output all frames
      m_iOutputSize = m_frameBytes;
      len = iSize;
    }
  }
  else
  {
    // Output the frame as-is
    m_iOutputSize = m_frameBytes;
    len = iSize;
  }

  // make sure audio decoding started otherwise SMD will block
  if(g_IntelSMDGlobals.GetAudioStartPts() != ISMD_NO_PTS)
    GetStreamInfo();

  return len;
}

int CDVDAudioCodecSMD::GetData(BYTE** dst)
{
  int size;
  if(m_iOutputSize)
  {
    *dst = m_frameCache;
    size = m_iOutputSize;

    m_iOutputSize = 0;
    m_frameBytes = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecSMD::Reset()
{
}

int CDVDAudioCodecSMD::GetChannels()
{
  return m_channel_count;
}

enum PCMChannels* CDVDAudioCodecSMD::GetChannelMap()
{
  // We always present stereo ch mapping
  static enum PCMChannels map[2] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT };
  return map;
}

int CDVDAudioCodecSMD::GetSampleRate()
{
  return m_sample_rate;
}

int CDVDAudioCodecSMD::GetBitsPerSample()
{
  return m_sample_size;
}

// See DVDPlayerAudio.h
unsigned char CDVDAudioCodecSMD::GetFlags()
{
  unsigned char flags = 0;
  if( m_bHardwareDecode ) flags |= FFLAG_HWDECODE;
  else                    flags |= FFLAG_PASSTHROUGH;

  if( CODEC_ID_TRUEHD == m_Codec || m_bIsDTSHD )
    flags |= FFLAG_HDFORMAT;

  return flags; 
}

void CDVDAudioCodecSMD::GetStreamInfo()
{
  ismd_result_t result;
  ismd_audio_processor_t audio_processor;
  ismd_dev_t audio_device;
  ismd_audio_stream_info_t audio_info;

  audio_processor = g_IntelSMDGlobals.GetAudioProcessor();
  audio_device = g_IntelSMDGlobals.GetPrimaryAudioDevice();

  if(audio_processor == -1 || audio_device == -1)
    return;

  result = ismd_audio_input_get_stream_info(
      audio_processor, audio_device,
      &audio_info);

  if(result != ISMD_SUCCESS)
    return;

  // make sure we got something meaningful
  if(audio_info.sample_size != 0)
    m_sample_size = audio_info.sample_size;
  if(audio_info.channel_count != 0)
    m_channel_count = audio_info.channel_count;
  if(audio_info.sample_rate != 0)
    m_sample_rate = audio_info.sample_rate;
}

#endif // HAS_INTEL_SMD
