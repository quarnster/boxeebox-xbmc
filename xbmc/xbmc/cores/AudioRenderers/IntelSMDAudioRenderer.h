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

#ifndef __INTEL_SMD_AUDIO_RENDERER_H__
#define __INTEL_SMD_AUDIO_RENDERER_H__

#include "system.h"

#ifdef HAS_INTEL_SMD

#include "IAudioRenderer.h"
#include "IAudioCallback.h"
#include "../ssrc.h"
#include "utils/SingleLock.h"
#include <ismd_audio.h>
#include <gdl_pd.h>
#include <queue>

#include "LicenseConfig.h"

extern void RegisterAudioCallback(IAudioCallback* pCallback);
extern void UnRegisterAudioCallback();


class CIntelSMDAudioRenderer : public IAudioRenderer
{
public:
  virtual void UnRegisterAudioCallback();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback);
  virtual unsigned int GetChunkLen();
  virtual float GetDelay();
  virtual float GetCacheTime();
  CIntelSMDAudioRenderer();
  virtual bool Initialize(IAudioCallback* pCallback, int iChannels, enum PCMChannels* channelMap,
                          unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample,
                          bool bResample, const char* strAudioCodec = "", bool bIsMusic=false,
                          bool bPassthrough = false, bool bTimed = AUDIO_NOT_TIMED,
                          AudioMediaFormat audioMediaFormat = AUDIO_MEDIA_FMT_PCM);
  virtual ~CIntelSMDAudioRenderer();

  virtual unsigned int AddPackets(const void* data, unsigned int len, double pts = DVD_NOPTS_VALUE, double duration = 0);
  virtual unsigned int GetSpace();
  virtual bool Deinitialize();
  virtual bool Pause();
  virtual bool Stop();
  virtual bool Resume();

  virtual long GetCurrentVolume() const;
  virtual void Mute(bool bMute);
  virtual bool SetCurrentVolume(long nVolume);
  virtual int SetPlaySpeed(int iSpeed);
  virtual void WaitCompletion();
  virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);

  virtual void Flush();
  virtual void Resync(double pts);
  virtual bool IsTimed() { return m_bTimed; }
  virtual void DisablePtsCorrection(bool bDisable) { m_bDisablePtsCorrection = bDisable; }

  static CCriticalSection m_SMDAudioLock;

private:
  static int BuildChannelConfig( enum PCMChannels* channelMap, int iChannels );
  unsigned int SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size, ismd_pts_t pts = ISMD_NO_PTS);

  void ConfigureDolbyModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);
  void ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode);
  void ConfigureDolbyTrueHDModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);
  void ConfigureDTSModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);

  ismd_audio_format_t GetISMDFormat(AudioMediaFormat format);
  AUDIO_CODEC_VENDOR  GetAudioCodecVendor(AudioMediaFormat format);

  void SetDefaultOutputConfig(ismd_audio_output_config_t& output_config);
  void ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config,
      int output, int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough);

  IAudioCallback* m_pCallback;
  long m_nCurrentVolume;
  bool m_bPause;
  bool m_bIsAllocated;

  bool m_bAC3Encode;

  bool m_bIsHDMI;
  bool m_bIsSPDIF;
  bool m_bIsAnalog;
  bool m_bIsAllOutputs;

  ismd_dev_t        m_audioDevice;
  ismd_dev_handle_t m_audioDeviceInput;

  enum PCMChannels* m_channelMap;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  AudioMediaFormat m_audioMediaFormat;
  unsigned int m_uiSamplesPerSec;
  bool m_bTimed;
  bool m_bFlushFlag;
  unsigned int m_dwChunkSize;
  unsigned int m_dwBufferLen;

  ismd_pts_t m_lastPts;
  ismd_pts_t m_lastSync;
  bool m_bDisablePtsCorrection;

  // data for small chunks collection
  ismd_pts_t m_first_pts;
  unsigned int m_ChunksCollectedSize;
  unsigned char *m_pChunksCollected;


  /*
  static ismd_audio_format_t m_inputActualFormat;
  static ismd_audio_format_t m_inputAudioFormat;
  static int m_ioBitsPerSample;
  static int m_inputSamplesPerSec;
  static unsigned int m_uiBytesPerSecond;
  static int m_inputChannelConfig;
  static ismd_audio_channel_config_t m_outputChannelConfig;
  static int m_outputChannelMap;
  static ismd_audio_output_mode_t m_outputOutMode;
  static unsigned int m_outputSampleRate;
  static bool m_bAllOutputs;
  static bool m_bIsInit;
  static ismd_audio_downmix_mode_t m_DownmixMode;
  static int m_nSampleRate;
  */

  static CIntelSMDAudioRenderer* m_owner;

  bool LoadEDID();
  void UnloadEDID();
  bool CheckEDIDSupport( ismd_audio_format_t format, int& iChannels, unsigned int& uiSampleRate, unsigned int& uiSampleSize );
  void DumpEDID();
  static ismd_audio_format_t MapGDLAudioFormat( gdl_hdmi_audio_fmt_t f );

  typedef struct _edidCaps
  {
    ismd_audio_format_t format;
    int channels;
    unsigned char sample_rates; // 7 bit field corresponding to m_edidRates
    unsigned char sample_sizes; // 4 bit field corresponding to m_edidSampleSizes, PCM only
    struct _edidCaps* next;
  } edidHint;
  static edidHint* m_edidTable;
};

#endif

#endif 

