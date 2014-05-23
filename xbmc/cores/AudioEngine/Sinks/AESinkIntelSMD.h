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
  //virtual bool          IsCompatible(const AEAudioFormat &format, const std::string &device);

  virtual double        GetDelay        ();
  virtual double        GetCacheTime    ();
  virtual double        GetCacheTotal   ();
  virtual unsigned int  AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void          Drain           ();
  virtual double GetLatency() { return m_latency; };
  //virtual bool          HasVolume() { return true; }
  //virtual void          SetVolume(float volume);
  virtual bool          SoftSuspend();
  virtual bool          SoftResume();

  static  void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

private:
  unsigned int SendDataToInput(unsigned char* buffer_data, unsigned int buffer_size);
  ismd_audio_format_t GetISMDFormat(AEDataFormat format);
  void SetDefaultOutputConfig(ismd_audio_output_config_t& output_config);
  void ConfigureAudioOutputParams(ismd_audio_output_config_t& output_config, int output, int sampleSize, int sampleRate, int channels, ismd_audio_format_t format, bool bPassthrough, bool bAC3Encode);
  void ConfigureDolbyPlusModes(ismd_audio_processor_t proc_handle, ismd_dev_t input_handle, bool bAC3Encode);

  //float m_fCurrentVolume;
  bool m_bPause;
  bool m_bIsAllocated;
  double m_latency;

  ismd_dev_t        m_audioDevice;
  ismd_dev_handle_t m_audioDeviceInput;
  
  unsigned int m_dwChunkSize;
  unsigned int m_dwBufferLen;
  double m_dSampleRate;
  unsigned int m_frameSize;

  static CCriticalSection m_SMDAudioLock;

  static bool LoadEDID();
  static void UnloadEDID();
  static bool CheckEDIDSupport( ismd_audio_format_t &format, int& iChannels, unsigned int& uiSampleRate, unsigned int& uiSampleSize, bool& bAC3Encode );
  static void GetEDIDInfo(int &maxChannels, std::vector<AEDataFormat> &formats, std::vector<unsigned int> &rates);
  static void DumpEDID();
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
