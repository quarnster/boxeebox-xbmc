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

#include "DVDDemux.h"
#include "Thread.h"

#ifdef HAS_INTEL_SMD

#include "../../../dvb/dvbepgloader.h"
#include "IntelSMDGlobals.h"
#include "cores/AudioRenderers/IntelSMDAudioRenderer.h"
#include "BitstreamStats.h"
#include "Stopwatch.h"

struct dvbpsi_decoder_s;

#define MAX_STREAMS 100

class CDVDDemuxSMD;

class CDemuxStreamVideoSMD
  : public CDemuxStreamVideo
{
  CDVDDemuxSMD *m_parent;
public:
  CDemuxStreamVideoSMD(CDVDDemuxSMD *parent)
  : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};


class CDemuxStreamAudioSMD
  : public CDemuxStreamAudio
{
    CDVDDemuxSMD *m_parent;
public:
  CDemuxStreamAudioSMD(CDVDDemuxSMD *parent)
  : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};


class CDVDDemuxSMD : public CDVDDemux
{
public:
  typedef enum { DEMUX_STATE_IDLE, DEMUX_STATE_FOUND_PAT, DEMUX_STATE_FOUND_PMT, DEMUX_STATE_RUNNING } ParsingState;

  CDVDDemuxSMD();
  virtual ~CDVDDemuxSMD();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset() {}
  void Flush();
  void Abort();
  void SetSpeed(int iSpeed);
  virtual std::string GetFileName();

  DemuxPacket* Read();

  virtual bool SeekTime(int time, bool backwords = false, double* startpts = NULL) { return true; }
  virtual int GetStreamLength() { return 0; }
  virtual CDemuxStream* GetStream(int iStreamId);
  virtual int GetNrOfStreams();

  virtual void SetUpAudio(CodecID codec);
  virtual void SetUpVideo(CodecID codec);
  virtual void SetPcrPid(int pid);

  int GetVideoPid() { return m_nVideoPID; }
  int GetAudioPid() { return m_nAudioPID; }

  CodecID GetVideoCodec() { return m_nVideoCodec; }
  CodecID GetAudioCodec() { return m_nAudioCodec; }

  virtual bool IsAudioPassthrough() { return true; }
  virtual bool IsVideoPassthrough() { return true; }

  virtual IAudioRenderer* GetAudioDevice() { return m_pAudioSink; }

  virtual void Start();
  virtual void Stop();

  virtual double GetAudioBitRate();
  virtual double GetVideoBitRate();

  int GetProgramNumber() { return m_nProgramNumber; }

  void SetState(ParsingState state);
  ParsingState GetState() { return m_state; }
  void SetPmtPid(int pid ) { m_pmtPid = pid; }
  int GetPmtPid() { return m_pmtPid; }

  struct dvbpsi_decoder_s* dvbpsiPAT;
  struct dvbpsi_decoder_s* dvbpsiPMT;

protected:
  void AddStreams();
  void WaitForBufferEvent();
  bool SendNewSegment();
  DemuxPacket* ReadAudioData();
  void ConfigureVideo();

  void AddPidFilter(int pid);
  void RemovePidFilter(int pid);
  bool ParseDataFrame(char* buf);

  void StartDemuxer();

  CDVDInputStream* m_pInput;
  int m_nVideoPID;
  int m_nAudioPID;
  CodecID m_nVideoCodec;
  CodecID m_nAudioCodec;
  int m_nProgramNumber;
  int m_nStreams;
  CDemuxStream* m_streams[MAX_STREAMS];
  CIntelSMDAudioRenderer* m_pAudioSink;

  int m_pmtPid;
  int m_pcrPid;
  ParsingState m_state;

  DvbEpgLoader* m_epgLoader;

  BitstreamStats m_videoStats;
  BitstreamStats m_audioStats;

  ismd_dev_t          m_demux;
  ismd_port_handle_t  m_demux_input_port;
  ismd_event_t        m_reading_thread_port_event;

  bool m_bStarted;
  bool m_bFirstFrame;

  CStopWatch m_lastPatStopWatch;
};

#endif

