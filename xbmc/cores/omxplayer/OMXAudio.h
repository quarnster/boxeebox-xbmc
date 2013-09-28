/*
 *      Copyright (c) 2002 d7o3g4q and RUNTiME
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef __OPENMAXAUDIORENDER_H__
#define __OPENMAXAUDIORENDER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/AudioEngine/Utils/AERemap.h"
#include "cores/IAudioCallback.h"
#include "linux/PlatformDefs.h"
#include "DVDStreamInfo.h"

#include "OMXClock.h"
#include "OMXCore.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"

#include "threads/CriticalSection.h"

#define AUDIO_BUFFER_SECONDS 3
#define VIS_PACKET_SIZE 512

#define OMX_IS_RAW(x)       \
(                           \
  (x) == AE_FMT_AC3   ||    \
  (x) == AE_FMT_DTS         \
)

class COMXAudio
{
public:
  void UnRegisterAudioCallback();
  void RegisterAudioCallback(IAudioCallback* pCallback);
  unsigned int GetChunkLen();
  float GetDelay();
  float GetCacheTime();
  float GetCacheTotal();
  COMXAudio();
  bool Initialize(AEAudioFormat format, OMXClock *clock, CDVDStreamInfo &hints, bool bUsePassthrough, bool bUseHWDecode);
  bool PortSettingsChanged();
  ~COMXAudio();

  unsigned int AddPackets(const void* data, unsigned int len);
  unsigned int AddPackets(const void* data, unsigned int len, double dts, double pts);
  unsigned int GetSpace();
  bool Deinitialize();

  void SetVolume(float nVolume);
  void SetMute(bool bOnOff);
  void SetDynamicRangeCompression(long drc);
  float GetDynamicRangeAmplification() { return 20.0f * log10f(m_amplification * m_attenuation); }
  bool ApplyVolume();
  int SetPlaySpeed(int iSpeed);
  void SubmitEOS();
  bool IsEOS();
  void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  void Flush();

  void Process();

  void SetCodingType(AEDataFormat dataFormat);
  static bool CanHWDecode(AVCodecID codec);

  static void PrintChannels(OMX_AUDIO_CHANNELTYPE eChannelMapping[]);
  void PrintPCM(OMX_AUDIO_PARAM_PCMMODETYPE *pcm, std::string direction);
  void PrintDDP(OMX_AUDIO_PARAM_DDPTYPE *ddparm);
  void PrintDTS(OMX_AUDIO_PARAM_DTSTYPE *dtsparam);
  unsigned int SyncDTS(BYTE* pData, unsigned int iSize);
  unsigned int SyncAC3(BYTE* pData, unsigned int iSize);

  bool BadState() { return !m_Initialized; };
  unsigned int GetAudioRenderingLatency();
  float GetMaxLevel(double &pts);
  void VizPacket(const void* data, unsigned int len, double pts);

private:
  IAudioCallback* m_pCallback;
  bool          m_Initialized;
  float         m_CurrentVolume;
  bool          m_Mute;
  long          m_drc;
  bool          m_Passthrough;
  bool          m_HWDecode;
  unsigned int  m_BytesPerSec;
  unsigned int  m_BufferLen;
  unsigned int  m_ChunkLen;
  unsigned int  m_OutputChannels;
  unsigned int  m_BitsPerSample;
  float         m_maxLevel;
  float         m_amplification;
  float         m_attenuation;
  float         m_desired_attenuation;
  COMXCoreComponent *m_omx_clock;
  OMXClock       *m_av_clock;
  bool          m_settings_changed;
  bool          m_setStartTime;
  bool          m_LostSync;
  int           m_SampleRate;
  OMX_AUDIO_CODINGTYPE m_eEncoding;
  uint8_t       *m_extradata;
  int           m_extrasize;
  // stuff for visualisation
  double        m_last_pts;
  int           m_vizBufferSize;
  uint8_t       *m_vizBuffer;
  int           m_vizRemapBufferSize;
  uint8_t       *m_vizRemapBuffer;
  CAERemap      m_vizRemap;
  bool          m_submitted_eos;
  bool          m_failed_eos;
  typedef struct {
    int num_samples;
    float samples[VIS_PACKET_SIZE];
    double pts;
  } vizblock_t;
  std::queue<vizblock_t> m_vizqueue;

  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_output;
  OMX_AUDIO_PARAM_PCMMODETYPE m_pcm_input;
  OMX_AUDIO_PARAM_DTSTYPE     m_dtsParam;
  WAVEFORMATEXTENSIBLE        m_wave_header;
  AEAudioFormat m_format;
protected:
  COMXCoreComponent m_omx_render_analog;
  COMXCoreComponent m_omx_render_hdmi;
  COMXCoreComponent m_omx_splitter;
  COMXCoreComponent m_omx_mixer;
  COMXCoreComponent m_omx_decoder;
  COMXCoreTunel     m_omx_tunnel_clock_analog;
  COMXCoreTunel     m_omx_tunnel_clock_hdmi;
  COMXCoreTunel     m_omx_tunnel_mixer;
  COMXCoreTunel     m_omx_tunnel_decoder;
  COMXCoreTunel     m_omx_tunnel_splitter_analog;
  COMXCoreTunel     m_omx_tunnel_splitter_hdmi;
  DllAvUtil         m_dllAvUtil;

  OMX_AUDIO_CHANNELTYPE m_input_channels[OMX_AUDIO_MAXCHANNELS];
  OMX_AUDIO_CHANNELTYPE m_output_channels[OMX_AUDIO_MAXCHANNELS];

  CAEChannelInfo    m_channelLayout;

  static CAEChannelInfo    GetChannelLayout(AEAudioFormat format);

  static void CheckOutputBufferSize(void **buffer, int *oldSize, int newSize);
  CCriticalSection m_critSection;
};
#endif

