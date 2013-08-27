#pragma once
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

#include "Interfaces/AESink.h"
#include "Utils/AEDeviceInfo.h"
#include <stdint.h>

#include <ismd_audio.h>
#include <gdl_pd.h>

#include "threads/CriticalSection.h"

class CAESinkIntelSMD : public IAESink
{
public:
  virtual const char *  GetName() { return "IntelSMD"; }

                        CAESinkIntelSMD();
  virtual               ~CAESinkIntelSMD();

  virtual bool          Initialize  (AEAudioFormat &format, std::string &device);
  virtual void          Deinitialize();
  virtual bool          IsCompatible(const AEAudioFormat format, const std::string device);

  virtual double        GetDelay        ();
  virtual double        GetCacheTime    ();
  virtual double        GetCacheTotal   () { return 0.0; }
  virtual unsigned int  AddPackets      (uint8_t *data, unsigned int frames, bool hasAudio);
  virtual void          Drain           ();
  virtual bool          HasVolume() { return true; }
  virtual void          SetVolume(float volume);
  virtual bool          SoftSuspend();
  virtual bool          SoftResume();

  static CCriticalSection m_SMDAudioLock;

  static  void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

private:
  unsigned int SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size, ismd_pts_t pts = ISMD_NO_PTS);

  void ConfigureDolbyModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);
  void ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode);
  void ConfigureDolbyTrueHDModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);
  void ConfigureDTSModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle);

  ismd_audio_format_t GetISMDFormat(AEDataFormat format);
//  AUDIO_CODEC_VENDOR  GetAudioCodecVendor(AEAudioFormat format);

  void SetDefaultOutputConfig(ismd_audio_output_config_t& output_config);
  void ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config,
      int output, int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough);

  float m_fCurrentVolume;
  bool m_bPause;
  bool m_bIsAllocated;

  bool m_bAC3Encode;

  bool m_bIsHDMI;
  bool m_bIsSPDIF;
  bool m_bIsAnalog;
  bool m_bIsAllOutputs;

  ismd_dev_t        m_audioDevice;
  ismd_dev_handle_t m_audioDeviceInput;

  enum AEChannel* m_channelMap;
  unsigned int m_uiBitsPerSample;
  unsigned int m_uiChannels;
  AEDataFormat m_audioMediaFormat;
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
