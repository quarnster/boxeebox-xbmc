#pragma once

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
#include "DVDAudioCodec.h"
#include <config.h>
#include <ismd_audio.h>
#ifdef HAS_DVD_LIBDTS_CODEC
#include "DllLibDts.h"
#endif

class CDVDAudioCodecSMD : public CDVDAudioCodec
{
public:
  CDVDAudioCodecSMD();
  virtual ~CDVDAudioCodecSMD();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual enum PCMChannels* GetChannelMap();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual unsigned char GetFlags();
  virtual const char* GetName()  { return "smd"; }
  
private:
  
  static bool IsPassthroughAudioCodec( int Codec );
  static bool IsHWDecodeAudioCodec( int Codec );

  static bool DetectDTSHD(BYTE* pData, int iSize);

void GetStreamInfo();

  int m_Codec;
  
  bool m_bHardwareDecode;
  
  int m_DTSHDDetection;
  bool m_bIsDTSHD;
  bool m_bBitstreamDTSHD;
  
  DllLibDts m_dllDTS;
  dts_state_t* m_pStateDTS;
  
  BYTE m_frameCache[65536];
  int m_frameBytes;
  
  int m_iOutputSize;

  int m_sample_size;
  int m_channel_count;
  int m_sample_rate;
};
