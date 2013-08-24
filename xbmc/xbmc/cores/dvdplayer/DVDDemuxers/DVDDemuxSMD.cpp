  /*
 *      Copyright (C) 2005-2008 Team Boxee
 *      http://www.boxee.tv
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

#include "DVDDemuxSMD.h"

#ifdef HAS_INTEL_SMD

#include "system.h"
#include "IntelSMDGlobals.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "xbmc/cores/dvb/dvbmanager.h"
#include "DVDDemuxUtils.h"
#include "utils/log.h"
#include "cores/IntelSMDGlobals.h"
#include "cores/AudioRenderers/IntelSMDAudioRenderer.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "WindowingFactory.h"
#include "GUISettings.h"

#include <ismd_vidpproc.h>
#include <dvbpsi.h>
#include <psi.h>
#include <descriptor.h>
#include <tables/pat.h>
#include <tables/pmt.h>

#define PAT_PID 0
#define EIT_PID 0x12

#define BUFFER_SIZE 8192
#define PACKET_SIZE_188 188
#define READ_SIZE   43 * PACKET_SIZE_188

#define PAT_TIMEOUT 1000

#define ISMD_BUFFER_IS_CACHED(buf_type) ((buf_type) == ISMD_BUFFER_TYPE_PHYS_CACHED)
#define ISMD_BUFFER_MAP_BY_TYPE(buf_type, base_address, size) \
            (ISMD_BUFFER_IS_CACHED(buf_type) ? OS_MAP_IO_TO_MEM_CACHE(base_address, size) : OS_MAP_IO_TO_MEM_NOCACHE(base_address, size))

#define VIDEO_STREAM 0
#define AUDIO_STREAM 1

static void PMTCallBack(void *p_cb_data, dvbpsi_pmt_t* p_pmt)
{
  dvbpsi_pmt_es_t* p_es = p_pmt->p_first_es;
  CDVDDemuxSMD* demux = (CDVDDemuxSMD*) p_cb_data;

  CLog::Log(LOGINFO, "New active PMT");
  CLog::Log(LOGINFO, "  program_number : %d", p_pmt->i_program_number);
  CLog::Log(LOGINFO, "  version_number : %d", p_pmt->i_version);
  CLog::Log(LOGINFO, "  PCR_PID        : 0x%x (%d)", p_pmt->i_pcr_pid, p_pmt->i_pcr_pid);

  demux->SetPcrPid(p_pmt->i_pcr_pid);

  CLog::Log(LOGINFO, "    | type @ elementary_PID\n");

  while(p_es)
  {
    CLog::Log(LOGINFO, "    | 0x%02x  @ 0x%x (%d)\n", p_es->i_type, p_es->i_pid, p_es->i_pid);

    if (p_es->i_pid == demux->GetVideoPid())
    {
      switch (p_es->i_type)
      {
      case 0x02: demux->SetUpVideo(CODEC_ID_MPEG2VIDEO); break;
      case 0x1B: demux->SetUpVideo(CODEC_ID_H264); break;
      default:   CLog::Log(LOGERROR, "PMTCallBack: Unknown video stream type: %d", p_es->i_type); break;
      }
    }

    if (p_es->i_pid == demux->GetAudioPid())
     {
       switch (p_es->i_type)
       {
       case 0x81: demux->SetUpAudio(CODEC_ID_AC3); break;
       case 0x11: demux->SetUpAudio(CODEC_ID_AAC_LATM); break;
       default:   CLog::Log(LOGERROR, "PMTCallBack: Unknown audio stream type: %d", p_es->i_type); break;
       }
     }

    p_es = p_es->p_next;
  }


  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();
  tuners[0]->AddPidFilter(demux->GetVideoPid(), DvbTuner::DVB_FILTER_VIDEO);
  tuners[0]->AddPidFilter(demux->GetAudioPid(), DvbTuner::DVB_FILTER_AUDIO);

  if (tuners[0]->GetTunerType() == DvbTuner::TUNER_TYPE_DVBT)
    tuners[0]->AddPidFilter(EIT_PID, DvbTuner::DVB_FILTER_OTHER);

  tuners[0]->RemovePidFilter(demux->GetPmtPid());

  demux->SetState(CDVDDemuxSMD::DEMUX_STATE_FOUND_PMT);

  dvbpsi_DeletePMT(p_pmt);
  demux->dvbpsiPMT = NULL;
}

static void PATCallBack(void *p_cb_data, dvbpsi_pat_t* p_pat)
{
  dvbpsi_pat_program_t* p_program = p_pat->p_first_program;
  CDVDDemuxSMD* demux = (CDVDDemuxSMD*) p_cb_data;

  CLog::Log(LOGINFO, "New PAT");
  CLog::Log(LOGINFO, "  transport_stream_id : %d", p_pat->i_ts_id);
  CLog::Log(LOGINFO, "  version_number      : %d", p_pat->i_version);
  CLog::Log(LOGINFO, "    | program_number @ [NIT|PMT]_PID");

  int foundPid = 0;
  while(p_program)
  {
    if (p_program->i_number == demux->GetProgramNumber())
    {
      foundPid = p_program->i_pid;
    }

    CLog::Log(LOGINFO, "    | %14d @ 0x%x (%d)", p_program->i_number, p_program->i_pid, p_program->i_pid);
    p_program = p_program->p_next;
  }

  CLog::Log(LOGINFO, "  active              : %d", p_pat->b_current_next);

  if (foundPid != 0)
  {
    CLog::Log(LOGINFO, "*********** FOUND PID: %d", foundPid);
    demux->SetPmtPid(foundPid);

    const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();
    tuners[0]->AddPidFilter(foundPid, DvbTuner::DVB_FILTER_OTHER);

    demux->dvbpsiPMT = dvbpsi_AttachPMT(demux->GetProgramNumber(), PMTCallBack, p_cb_data);

    tuners[0]->RemovePidFilter(PAT_PID);
    dvbpsi_DeletePAT(p_pat);
    demux->dvbpsiPAT = NULL;

    demux->SetState(CDVDDemuxSMD::DEMUX_STATE_FOUND_PAT);
  }
}

void CDemuxStreamVideoSMD::GetStreamInfo(std::string& strInfo)
{
  ismd_result_t ismd_ret;
  ismd_viddec_stream_properties_t properties;

  CStdString info = "";
  CStdString videoCodec = "Unknown";

  if(m_parent->GetState() != CDVDDemuxSMD::DEMUX_STATE_RUNNING)
    return;

  ismd_ret = ismd_viddec_get_stream_properties(g_IntelSMDGlobals.GetVidDec(), &properties);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDemuxStreamVideoSMD::GetStreamInfo failed at ismd_viddec_get_stream_properties");
    return;
  }

  switch(properties.codec_type)
  {
  case ISMD_CODEC_TYPE_MPEG2:
    videoCodec = "MPEG2";
    break;
  case ISMD_CODEC_TYPE_H264:
    videoCodec = "H264";
    break;
  case ISMD_CODEC_TYPE_VC1:
    videoCodec = "VC1";
    break;
  case ISMD_CODEC_TYPE_MPEG4:
    videoCodec = "MPEG4";
    break;
  default:
    break;
  }

  double bitRate = m_parent->GetVideoBitRate();
  double fps = 0;
  int num = properties.frame_rate_num;
  int den = properties.frame_rate_den;
  if (num != 0 && den != 0)
  {
    fps = (float) num / (float) den;
  }

  info.Format("Video: %s, %d x %d%s, %.2f fps, %.2f Mb/s", videoCodec.c_str(),
      properties.coded_width,
      properties.coded_height,
      properties.is_stream_interlaced ? "i" : "p",
      fps,
      bitRate / (1024.0 * 1024.0));
  strInfo = info.c_str();
}

void CDemuxStreamAudioSMD::GetStreamInfo(std::string& strInfo)
{
  ismd_result_t ismd_ret;
  ismd_audio_stream_info_t stream_info;

  CStdString info = "";
  CStdString audioCodec = "Unknown";

  if(m_parent->GetState() != CDVDDemuxSMD::DEMUX_STATE_RUNNING)
    return;

  ismd_ret = ismd_audio_input_get_stream_info(g_IntelSMDGlobals.GetAudioProcessor(),
      g_IntelSMDGlobals.GetPrimaryAudioDevice(), &stream_info);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDemuxStreamAudioSMD::GetStreamInfo failed at ismd_audio_input_get_stream_info");
    return;
  }

  switch (stream_info.algo)
  {
  case ISMD_AUDIO_MEDIA_FMT_DD:
    audioCodec = "AC3";
    break;
  case ISMD_AUDIO_MEDIA_FMT_AAC_LOAS:
    audioCodec = "AAC LOAS/LATM";
    break;
  default:
    break;
  }

  double bitRate = m_parent->GetAudioBitRate();

  info.Format("Audio: %s, %d Hz, %d channels, %d bits, %.2f kb/s",
      audioCodec.c_str(), stream_info.sample_rate, stream_info.channel_count,
      stream_info.sample_size, bitRate / 1024.0);

  strInfo = info.c_str();
}

CDVDDemuxSMD::CDVDDemuxSMD() : CDVDDemux()
{
  m_nStreams = 0;
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
  m_nVideoPID = -1;
  m_nAudioPID = -1;
  m_nVideoCodec = CODEC_ID_NONE;
  m_nAudioCodec = CODEC_ID_NONE;
  m_pcrPid = -1;
  m_demux = -1;
  m_demux_input_port = -1;
  m_reading_thread_port_event = -1;
  m_bFirstFrame = false;
  m_bStarted = false;
  m_epgLoader = NULL;
  m_lastPatStopWatch.Reset();
}

CDVDDemuxSMD::~CDVDDemuxSMD()
{
  Dispose();
}

bool CDVDDemuxSMD::Open(CDVDInputStream* pInput)
{
  ismd_result_t result = ISMD_SUCCESS;
  std::string strFile;
  ismd_event_t  events[ISMD_EVENT_LIST_MAX];
  unsigned output_buf_size;

  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();

  if (!pInput)
    return false;

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  if(!g_IntelSMDGlobals.CreateDemuxer())
    return false;

  m_demux = g_IntelSMDGlobals.GetDemuxer();
  if (m_demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Open m_demux is -1");
    return false;
  }

  m_demux_input_port = g_IntelSMDGlobals.GetDemuxInput();
  if (m_demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Open m_demux_input_port is -1");
    return false;
  }

  //Create the port events
  result = ismd_event_alloc(&m_reading_thread_port_event);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Open ismd_event_alloc failed %d", result);
    return false;
  }

  result = ismd_port_attach(m_demux_input_port, m_reading_thread_port_event,
          (ismd_queue_event_t) (ISMD_QUEUE_EVENT_NOT_FULL
          | ISMD_QUEUE_EVENT_EMPTY), ISMD_QUEUE_WATERMARK_NONE);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD: ismd_event_alloc failed %d", result);
    return false;
  }

  AddStreams();

  m_pAudioSink = new CIntelSMDAudioRenderer();

  if (!m_epgLoader && tuners[0]->GetTunerType() == DvbTuner::TUNER_TYPE_DVBT)
  {
    m_epgLoader = new DvbEpgLoader();
    m_epgLoader->AddListener(&DVBManager::GetInstance());
  }

  return true;
}

void CDVDDemuxSMD::Dispose()
{
  CLog::Log(LOGDEBUG, "CDVDDemuxSMD::Dispose");

  ismd_result_t ismd_ret;

  ismd_event_free(m_reading_thread_port_event);
  ismd_port_detach(m_demux_input_port);

  g_IntelSMDGlobals.DeleteDemuxer();
  g_IntelSMDGlobals.DeleteVideoDecoder();
  g_IntelSMDGlobals.DeleteVideoRender();

  if (m_pAudioSink)
    SAFE_DELETE(m_pAudioSink);

  for (int i = 0; i < MAX_STREAMS; i++)
  {
    if (m_streams[i])
    {
      if (m_streams[i]->ExtraData)
        delete[] (BYTE*)(m_streams[i]->ExtraData);
      delete m_streams[i];
      m_nStreams--;
    }
    m_streams[i] = NULL;
  }

  if(m_epgLoader)
    m_epgLoader->RemoveListener(&DVBManager::GetInstance());
  SAFE_DELETE(m_epgLoader);

  m_bStarted = false;
}

void CDVDDemuxSMD::Flush()
{
  g_IntelSMDGlobals.FlushDemuxer();
  g_IntelSMDGlobals.FlushAudioDevice(g_IntelSMDGlobals.GetPrimaryAudioDevice());
  g_IntelSMDGlobals.FlushVideoDecoder();
  g_IntelSMDGlobals.FlushVideoRender();
}

void CDVDDemuxSMD::Abort()
{
  m_bStarted = false;
}

std::string CDVDDemuxSMD::GetFileName()
{
  if(m_pInput && m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}


DemuxPacket* CDVDDemuxSMD::ReadAudioData()
{
  DemuxPacket* pPacket = NULL;

  ismd_result_t ismd_ret;
  ismd_dev_t demux = -1;
  ismd_port_handle_t audio_data_port = -1;
  ismd_demux_filter_handle_t m_demux_aes_filter_handle = -1;
  ismd_buffer_handle_t buffer = -1;
  ismd_buffer_descriptor_t ismd_buf_desc;
  ismd_es_buf_attr_t *buf_attrs;
  ismd_port_status_t port_status;
  uint8_t *buf_ptr;

  demux = g_IntelSMDGlobals.GetDemuxer();
  if(demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::ReadAudioData failed at g_IntelSMDGlobals.GetDemuxer");
    return NULL;
  }

  m_demux_aes_filter_handle = g_IntelSMDGlobals.GetDemuxAudioFilter();
  if(demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::ReadAudioData failed at g_IntelSMDGlobals.GetDemuxAudioFilter");
    return NULL;
  }

  ismd_ret = ismd_demux_filter_get_output_port(demux, m_demux_aes_filter_handle, &audio_data_port);

  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::ReadAudioData failed at ismd_demux_filter_get_output_port %d", ismd_ret);
    return NULL;
  }

  ismd_port_get_status(audio_data_port, &port_status);
  if(port_status.cur_depth == 0)
    return NULL;

  ismd_port_read(audio_data_port, &buffer);

  ismd_ret = ismd_buffer_read_desc(buffer, &ismd_buf_desc);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecSMD::ReadAudioData ismd_buffer_read_desc failed. %d", ismd_ret);
    return NULL;
  }

  buf_ptr = (uint8_t *)OS_MAP_IO_TO_MEM_NOCACHE(ismd_buf_desc.phys.base,ismd_buf_desc.phys.size);

  int size = ismd_buf_desc.phys.level;

  if(size == 0)
  {
    ismd_ret = ismd_buffer_dereference(buffer);
    if (ismd_ret != ISMD_SUCCESS)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecSMD::ReadAudioData ismd_buffer_dereference failed. %d", ismd_ret);
    }
    return NULL;
  }

  buf_attrs = (ismd_es_buf_attr_t *) ismd_buf_desc.attributes;

  pPacket = CDVDDemuxUtils::AllocateDemuxPacket(size);
  pPacket->iStreamId =  AUDIO_STREAM;
  pPacket->pts = g_IntelSMDGlobals.IsmdToDvdPts(buf_attrs->original_pts);
  pPacket->dts = g_IntelSMDGlobals.IsmdToDvdPts(buf_attrs->original_pts);
  pPacket->iSize = size;
  memcpy(pPacket->pData, buf_ptr, size);

  OS_UNMAP_IO_FROM_MEM(buf_ptr, ismd_buf_desc.phys.size);

  //need to dereference this buffer as SMD driver created reference when it made the buffer so it can be freed
  ismd_ret = ismd_buffer_dereference(buffer);
  if (ismd_ret != ISMD_SUCCESS)
    CLog::Log(LOGERROR, "CDVDVideoCodecSMD::ReadAudioData ismd_buffer_dereference failed. %d", ismd_ret);

  return pPacket;
}


DemuxPacket* CDVDDemuxSMD::Read()
{
  ismd_result_t result = ISMD_SUCCESS;
  ismd_port_status_t port_status;
  ismd_buffer_handle_t buffer = ISMD_BUFFER_HANDLE_INVALID;
  ismd_buffer_descriptor_t buffer_desc;
  char data_buffer[READ_SIZE];
  int data_buffer_size = 0;
  unsigned int amount_to_read;
  unsigned int amount_read = 0;
  char *virtual_buffer_address = NULL;
  bool full = false;
  uint8_t *chunk;
  ismd_es_buf_attr_t *buf_attrs;
  char buf[READ_SIZE];
  DemuxPacket* pPacket = NULL;
  int packets_num;
  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();

  amount_to_read = READ_SIZE;
  amount_read = m_pInput->Read((BYTE *) buf, amount_to_read);

  // no use to continue if we're not tuned
  if (tuners[0]->GetTuningState() != DvbTuner::TUNING_STATE_TUNED)
  {
    Stop();
    goto end;
  }

  if(!tuners[0]->IsSignalOk())
  {
    Stop();
    goto end;
  }

  if (amount_read == 0)
  {
    //CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read time out");
    goto end;
  }

  if (amount_read == -1 && tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNED)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Read amount_read is -1 while tuning");
    goto end;
  }

  if (amount_read % 188 != 0 && tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNED)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD:Read: amount_read % 188 != 0 (%d)", amount_read);

  if(tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNED)
    Start();

  if(m_lastPatStopWatch.GetElapsedMilliseconds() > PAT_TIMEOUT)
  {
    CLog::Log(LOGWARNING, "CDVDDemuxSMD:Read: timeout for PAT. Reopening DVR");
    tuners[0]->RemoveAllPidFilters();
    m_lastPatStopWatch.StartZero();
  }

  chunk = (uint8_t*) buf;
  packets_num = amount_read / 188;
  for (int i = 0; i < packets_num; i++)
  {
    if (chunk[0] != 0x47)
    {
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read sync byte is not 0x47 (%02X)", chunk[0]);
      chunk += 188;
      continue;
    }

    uint16_t chunkPid = ((uint16_t) (chunk[1] & 0x1f) << 8) + chunk[2];

    if (chunkPid == PAT_PID && m_state == DEMUX_STATE_IDLE)
    {
      dvbpsi_PushPacket(dvbpsiPAT, chunk);
    }
    else if (chunkPid == m_pmtPid && m_state == DEMUX_STATE_FOUND_PAT)
    {
      dvbpsi_PushPacket(dvbpsiPMT, chunk);
    }
    else if (m_state == DEMUX_STATE_FOUND_PMT)
    {
      if (m_nAudioCodec == CODEC_ID_NONE)
        CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read audio codec not found");
      if (m_nVideoCodec == CODEC_ID_NONE)
        CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read video codec not found");
      if (m_pcrPid == -1)
        CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read PCR  not found");

      SetState(DEMUX_STATE_RUNNING);
      CLog::Log(LOGINFO,"CDVDDemuxSMD::Read Audio PID %d Video PID %d Audio Codec %d Video Codec %d PCR %d\n",
            m_nAudioPID, m_nVideoPID, m_nAudioCodec, m_nVideoCodec, m_pcrPid);
      StartDemuxer();
    }
    else if (chunkPid == EIT_PID && tuners[0]->GetTunerType() == DvbTuner::TUNER_TYPE_DVBT)
    {
      if(m_epgLoader)
        m_epgLoader->ProcessPacket(chunk);
    }
    else if (chunkPid == m_nVideoPID || chunkPid == m_nAudioPID)
    {
      if (chunkPid == m_nVideoPID)
        m_videoStats.AddSampleBytes(188 - 4);
      if (chunkPid == m_nAudioPID)
        m_audioStats.AddSampleBytes(188 - 4);
      OS_MEMCPY(data_buffer + data_buffer_size, chunk, 188);
      data_buffer_size += 188;
      if (data_buffer_size > BUFFER_SIZE)
      {
        CLog::Log(LOGERROR, "CDVDDemuxSMD::Read data_buffer_size is too big: %d", data_buffer_size);
        goto end;
      }
    }

    chunk += 188;
  }

  // no video/audio data was found, nothing to push into SMD
  if (data_buffer_size == 0)
  {
    goto end;
  }

  if(m_state != DEMUX_STATE_RUNNING)
  {
    goto end;
  }

  result = ismd_port_get_status(m_demux_input_port, &port_status);

  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD:Read: ismd_port_get_status failed %d", result);
    goto end;
  }
  else
  {
    full = port_status.cur_depth >= port_status.max_depth;
  }

  // if port/queue has no space
  if (full)
  {
    // wait for the port event - this will indicate when there is space
    result = ismd_event_wait(m_reading_thread_port_event, 100);
    ismd_event_acknowledge(m_reading_thread_port_event);

    if (result == ISMD_SUCCESS || result == ISMD_ERROR_TIMEOUT)
    {
      if (result == ISMD_ERROR_TIMEOUT)
      {
        CLog::Log(LOGWARNING, "CDVDDemuxSMD::Read Demux is full");
        goto end;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CDVDDemuxSMD::Read ismd_event_wait failed on reading_thread_port_event %d", result);
      goto end;
    }
  }

  // get buffer
  buffer = ISMD_BUFFER_HANDLE_INVALID;
  result = ismd_buffer_alloc(BUFFER_SIZE, &buffer);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Read ismd_buffer_alloc failed %d", result);
    goto end;
  }

  result = ismd_buffer_read_desc(buffer, &buffer_desc);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Read ismd_buffer_read_desc failed %d", result);
    ismd_buffer_dereference(buffer);
    goto end;
  }

  virtual_buffer_address = (char*)OS_MAP_IO_TO_MEM_NOCACHE(buffer_desc.phys.base, buffer_desc.phys.size);
  OS_MEMCPY(virtual_buffer_address, data_buffer, data_buffer_size);

  buf_attrs = (ismd_es_buf_attr_t *) buffer_desc.attributes;

  buffer_desc.phys.level = data_buffer_size;
  buf_attrs->original_pts = 0;
  buf_attrs->local_pts = ISMD_NO_PTS;
  buf_attrs->discontinuity = false;

  result = ismd_buffer_update_desc(buffer, &buffer_desc);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Read ismd_buffer_update_desc failed %d", result);
    ismd_buffer_dereference(buffer);
    goto end;
  }

  OS_UNMAP_IO_FROM_MEM(virtual_buffer_address, buffer_desc.phys.size);

  result = ismd_port_write(m_demux_input_port, buffer);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Read ismd_port_write failed %d", result);
    ismd_buffer_dereference(buffer);
    goto end;
  }

  // we wait until first frame show up and then unmute the renderer
  if (!m_bFirstFrame)
  {
    ismd_vidrend_stream_position_info_t info;
    ismd_vidrend_get_stream_position(g_IntelSMDGlobals.GetVidRender(), &info);
    if (info.flip_time != ISMD_NO_PTS)
    {
      g_IntelSMDGlobals.MuteVideoRender(false);
      m_bFirstFrame = true;
    }
  }

  // look for video changes
  ConfigureVideo();

end:
  pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
  pPacket->flags |= DEMUX_PACKET_FLAG_PASSTHROUGH;

  return pPacket;
}

CDemuxStream* CDVDDemuxSMD::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;

  return m_streams[iStreamId];
}

int CDVDDemuxSMD::GetNrOfStreams()
{
  return m_nStreams;
}

double CDVDDemuxSMD::GetAudioBitRate()
{
  return m_audioStats.GetBitrate();
}

double CDVDDemuxSMD::GetVideoBitRate()
{
  return m_videoStats.GetBitrate();
}

void CDVDDemuxSMD::AddStreams()
{
  // add video and audio for testing
  {
    CDemuxStreamVideoSMD* st = new CDemuxStreamVideoSMD(this);
    m_streams[VIDEO_STREAM] = st;
    st->iWidth = 0;
    st->iHeight = 0;
    st->iId = VIDEO_STREAM;
    st->codec = CODEC_ID_NONE;
    st->source = STREAM_SOURCE_DEMUX;
    m_nStreams++;
  }

  {
    CDemuxStreamAudioSMD* st = new CDemuxStreamAudioSMD(this);
    m_streams[AUDIO_STREAM] = st;
    st->iChannels = 2;
    st->iSampleRate = 48000;
    st->iBitsPerSample = 16;
    st->iId = AUDIO_STREAM;
    st->codec = CODEC_ID_NONE;
    st->source = STREAM_SOURCE_DEMUX;
    m_nStreams++;
  }
}

void CDVDDemuxSMD::SetSpeed(int speed)
{
  ismd_dev_state_t dev_speed = g_IntelSMDGlobals.DVDSpeedToSMD(speed);

  if(g_IntelSMDGlobals.IsDemuxToVideo())
  {
    g_IntelSMDGlobals.SetVideoDecoderState(dev_speed);
    g_IntelSMDGlobals.SetVideoRenderState(dev_speed);
  }

  if(g_IntelSMDGlobals.IsDemuxToAudio())
  {
    g_IntelSMDGlobals.SetAudioDeviceState(dev_speed, g_IntelSMDGlobals.GetPrimaryAudioDevice());
  }

  g_IntelSMDGlobals.SetDemuxDeviceState(dev_speed);
}

void CDVDDemuxSMD::Start()
{
  if(m_bStarted)
    return;

  CLog::Log(LOGINFO, "CDVDDemuxSMD::Start");

  if(!m_pInput)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::Start m_pInput is NULL");
    return;
  }

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_TV))
  {
    int channelIndex;
    CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInput);

    if (!input)
    {
      CLog::Log(LOGERROR, "CDVDDemuxSMD::Start input NULL");
      return;
    }

    channelIndex = input->GetProgramId();
    DvbChannelPtr channel = DVBManager::GetInstance().GetChannels().GetChannelByIndex(channelIndex);
    m_nProgramNumber = channel->GetServiceId();
    m_nVideoPID = channel->GetVideoPid();
    const std::vector<DvbAudioInfo>& audioInfo = channel->GetAudioPids();
    if(audioInfo.size() > 0)
      m_nAudioPID = audioInfo[0].pid;
    else
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::Start no audio found in DVB stream");

    dvbpsiPAT = dvbpsi_AttachPAT(PATCallBack, this);
    dvbpsiPMT = NULL;
    m_pmtPid = 0;
  }

  AddPidFilter(PAT_PID);

  m_videoStats.Start();
  m_audioStats.Start();

  m_lastPatStopWatch.StartZero();

  m_bStarted = true;
}

void CDVDDemuxSMD::StartDemuxer()
{
  CLog::Log(LOGINFO, "CDVDDemuxSMD::StartDemuxer");

  Flush();

  g_IntelSMDGlobals.SetBaseTime(g_IntelSMDGlobals.GetCurrentTime());
  g_IntelSMDGlobals.SetDemuxDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime());
  g_IntelSMDGlobals.SetAudioDeviceBaseTime(g_IntelSMDGlobals.GetBaseTime(), g_IntelSMDGlobals.GetPrimaryAudioDevice());
  g_IntelSMDGlobals.SetVideoRenderBaseTime(g_IntelSMDGlobals.GetBaseTime());

  SendNewSegment();

  // we mute video render until first frame is received
  if(m_nVideoCodec == CODEC_ID_H264)
    g_IntelSMDGlobals.MuteVideoRender(true);

  SetSpeed(DVD_PLAYSPEED_NORMAL);
}

void CDVDDemuxSMD::Stop()
{
  if(!m_bStarted)
    return;

  CLog::Log(LOGINFO, "CDVDDemuxSMD::Stop");

  SetSpeed(DVD_PLAYSPEED_PAUSE);

  if(m_nAudioPID != -1)
    RemovePidFilter(m_nAudioPID);
  if(m_nVideoPID != -1)
    RemovePidFilter(m_nVideoPID);

  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();
  if (tuners[0]->GetTunerType() == DvbTuner::TUNER_TYPE_DVBT)
    RemovePidFilter(EIT_PID);

  m_nAudioPID = -1;
  m_nVideoPID = -1;

  CDemuxStreamVideo* st = (CDemuxStreamVideo *)m_streams[VIDEO_STREAM];
  st->iWidth = 0;
  st->iHeight = 0;

  SetState(DEMUX_STATE_IDLE);

  m_bFirstFrame = false;
  m_bStarted = false;

  m_lastPatStopWatch.Reset();

  gdl_flip(GDL_VIDEO_PLANE, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);
}

bool CDVDDemuxSMD::SendNewSegment()
{
  ismd_newsegment_tag_t newsegment_data;
  ismd_buffer_handle_t carrier_buffer = ISMD_BUFFER_HANDLE_INVALID;
  ismd_result_t ismd_ret;
  ismd_newsegment_tag_t newseg;

  newsegment_data.linear_start = 0;
  newsegment_data.start = ISMD_NO_PTS;
  newsegment_data.stop = ISMD_NO_PTS;
  newsegment_data.requested_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.applied_rate = ISMD_NORMAL_PLAY_RATE;
  newsegment_data.rate_valid = true;

  ismd_ret = ismd_buffer_alloc(0, &carrier_buffer);
  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SendNewSegment failed at ismd_buffer_alloc, creating carrier buffer %d", ismd_ret);
    return false;
  }

  ismd_ret = ismd_tag_set_newsegment(carrier_buffer, newsegment_data);
  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SendNewSegment failed at ismd_tag_set_newsegment %d", ismd_ret);
    ismd_buffer_dereference(carrier_buffer);
    return false;
  }

  ismd_ret = ismd_port_write(g_IntelSMDGlobals.GetDemuxInput(), carrier_buffer);
  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SendNewSegment failed at ismd_port_write, writing carrier buffer %d", ismd_ret);
    ismd_buffer_dereference(carrier_buffer);
    return false;
  }

  return true;
}

void CDVDDemuxSMD::SetUpAudio(CodecID codecID)
{
  ismd_result_t ismd_ret;
  ismd_dev_t demux;
  ismd_demux_pid_list_t pid_list;
  unsigned list_size;

  if(!m_pAudioSink)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetAudio audio sink in not open");
    return;
  }

  if(m_nAudioPID == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetAudio no audio PID while setting audio");
    return;
  }

  demux = g_IntelSMDGlobals.GetDemuxer();
  if(demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetAudio failed at g_IntelSMDGlobals.GetDemuxer");
    return;
  }

  ismd_demux_filter_handle_t audio_filter = g_IntelSMDGlobals.GetDemuxAudioFilter();

  if(audio_filter == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetAudio failed at g_IntelSMDGlobals.GetDemuxAudioFilter");
    return;
  }

  ismd_ret = ismd_demux_filter_get_mapped_pids(demux, audio_filter, &pid_list, &list_size);
  if (ismd_ret != ISMD_SUCCESS)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpAudio failed at ismd_demux_filter_get_mapped_pids %d", ismd_ret);

  if(list_size > 1)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpAudio more than 1 audio PID mapped: %d", list_size);

  if(list_size == 1)
  {
    if(pid_list.pid_array[0].pid_type != ISMD_DEMUX_PID_TYPE_PES)
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpAudio pid_type != ISMD_DEMUX_PID_TYPE_PES (%d)",
          pid_list.pid_array[0].pid_type);

    CLog::Log(LOGINFO, "CDVDDemuxSMD::SetUpAudio removing audio pid %d", pid_list.pid_array[0].pid_val);

    ismd_ret = ismd_demux_filter_unmap_from_pids(demux, audio_filter, pid_list, list_size);
    if (ismd_ret != ISMD_SUCCESS)
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpAudio failed at ismd_demux_filter_unmap_from_pids %d", ismd_ret);
  }

  ismd_ret = ismd_demux_filter_flush(demux, audio_filter);
  if (ismd_ret != ISMD_SUCCESS)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpAudio failed at ismd_demux_filter_flush %d", ismd_ret);

  ismd_ret = ismd_demux_filter_map_to_pid(demux, g_IntelSMDGlobals.GetDemuxAudioFilter(), m_nAudioPID, ISMD_DEMUX_PID_TYPE_PES);
  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetUpAudio failed at ismd_demux_filter_map_to_pids %d", ismd_ret);
    return;
  }

  if(codecID == m_nAudioCodec)
    return;

  m_nAudioCodec = codecID;

  AudioMediaFormat mediaFormat = AUDIO_MEDIA_FMT_PCM;

  bool bPassthrough = false;
  int audioOutputMode = g_guiSettings.GetInt("audiooutput.mode");

  switch(m_nAudioCodec)
  {
  case CODEC_ID_AC3:
    mediaFormat = AUDIO_MEDIA_FMT_DD;
    if(g_guiSettings.GetBool("audiooutput.ac3passthrough") &&
                (AUDIO_HDMI == audioOutputMode || AUDIO_IEC958 == audioOutputMode))
      bPassthrough = true;
    break;
  case CODEC_ID_AAC_LATM:
    mediaFormat = AUDIO_MEDIA_FMT_AAC_LATM;
    break;
  default:
    break;
  }

  m_pAudioSink->Initialize(NULL, 2, NULL, 48000, 16, false, "DVB", false, bPassthrough, true, mediaFormat);

  if(!g_IntelSMDGlobals.ConnectDemuxerToAudio(g_IntelSMDGlobals.GetPrimaryAudioDevice()))
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetAudio ConnectDemuxerToAudio failed");
    return;
  }
}

void CDVDDemuxSMD::SetUpVideo(CodecID codecID)
{
  ismd_result_t ismd_ret;
  ismd_dev_t demux;
  ismd_demux_pid_list_t pid_list;
  unsigned   list_size;

  CLog::Log(LOGDEBUG, "CDVDDemuxSMD::SetVideo pid %d codec %d", m_nVideoPID, m_nVideoCodec);

  if (m_nVideoPID == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetVideo no video PID while setting video");
    return;
  }

  demux = g_IntelSMDGlobals.GetDemuxer();
  if(demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetVideo failed at g_IntelSMDGlobals.GetDemuxer");
    return;
  }

  ismd_demux_filter_handle_t video_filter = g_IntelSMDGlobals.GetDemuxVideoFilter();

  if(video_filter == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetVideo failed at g_IntelSMDGlobals.GetDemuxVideoFilter");
    return;
  }

  ismd_ret = ismd_demux_filter_get_mapped_pids(demux, video_filter, &pid_list,
      &list_size);
  if (ismd_ret != ISMD_SUCCESS)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpVideo failed at ismd_demux_filter_get_mapped_pids %d", ismd_ret);

  if (list_size > 1)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpVideo more than 1 video PID mapped: %d", list_size);

  if(list_size == 1)
  {
    if (pid_list.pid_array[0].pid_type != ISMD_DEMUX_PID_TYPE_PES)
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpVideo pid_type != ISMD_DEMUX_PID_TYPE_PES (%d)",
        pid_list.pid_array[0].pid_type);

    CLog::Log(LOGINFO, "CDVDDemuxSMD::SetUpVideo removing video pid %d",
      pid_list.pid_array[0].pid_val);

    ismd_ret = ismd_demux_filter_unmap_from_pids(demux, video_filter, pid_list, list_size);
    if (ismd_ret != ISMD_SUCCESS)
      CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpVideo failed at ismd_demux_filter_unmap_from_pids %d", ismd_ret);
  }

  ismd_ret = ismd_demux_filter_flush(demux, video_filter);
  if (ismd_ret != ISMD_SUCCESS)
    CLog::Log(LOGWARNING, "CDVDDemuxSMD::SetUpVideo failed at ismd_demux_filter_flush %d", ismd_ret);

  ismd_ret = ismd_demux_filter_map_to_pid(demux, g_IntelSMDGlobals.GetDemuxVideoFilter(), m_nVideoPID, ISMD_DEMUX_PID_TYPE_PES);
  if(ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetUpVideo failed at ismd_demux_filter_map_to_pids %d", ismd_ret);
    return;
  }

  if(codecID == m_nVideoCodec)
    return;

  m_nVideoCodec = codecID;

  // build the pipeline
  ismd_codec_type_t smdCodec = ISMD_CODEC_TYPE_INVALID;
  if(m_nVideoCodec == CODEC_ID_H264)
    smdCodec = ISMD_CODEC_TYPE_H264;
  else if(m_nVideoCodec == CODEC_ID_MPEG2VIDEO)
    smdCodec = ISMD_CODEC_TYPE_MPEG2;

  g_IntelSMDGlobals.CreateVideoDecoder(smdCodec);

  if(!g_IntelSMDGlobals.ConnectDemuxerToVideo())
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetVideo ConnectDemuxerToVideo failed");
    return;
  }

  if(!g_IntelSMDGlobals.CreateVideoRender(GDL_VIDEO_PLANE))
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetVideo CreateVideoRender failed");
    return;
  }

  unsigned int flags = 0;
  flags |= CONF_FLAGS_SMD_DECODING;
  int screen_width = g_Windowing.GetWidth();
  int screen_height = g_Windowing.GetHeight();
  g_renderManager.PreInit();
  g_renderManager.Configure(screen_width, screen_height, screen_width, screen_height, 60, flags);

  g_IntelSMDGlobals.ConnectDecoderToRenderer();
}

void CDVDDemuxSMD::ConfigureVideo()
{
  ismd_viddec_stream_properties_t prop;
  unsigned int flags = CONF_FLAGS_SMD_DECODING;
  ismd_dev_t viddec = g_IntelSMDGlobals.GetVidDec();

  if(viddec == -1)
    return;

  CDemuxStreamVideo* st = (CDemuxStreamVideo *)m_streams[VIDEO_STREAM];
  if(st->iWidth != 0 && st->iHeight != 0)
    return;

  ismd_result_t res = ismd_viddec_get_stream_properties(viddec, &prop);

  if (res == ISMD_SUCCESS && prop.coded_width && prop.coded_height)
  {
    CDemuxStreamVideo* st = (CDemuxStreamVideo *)m_streams[VIDEO_STREAM];
    if(st->iWidth != prop.display_width || st->iHeight != prop.display_height)
    {
      st->iWidth = prop.display_width;
      st->iHeight = prop.display_height;

      g_renderManager.Configure(prop.coded_width, prop.coded_height, prop.display_width, prop.display_height, 60, flags);
    }
  }
}

void CDVDDemuxSMD::SetPcrPid(int pcrPID)
{
  ismd_result_t ismd_ret;
  ismd_dev_t demux;
  demux = g_IntelSMDGlobals.GetDemuxer();

  if(demux == -1)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetPcrPid failed at g_IntelSMDGlobals.GetDemuxer");
    return;
  }

  m_pcrPid = pcrPID;

  ismd_ret = ismd_demux_ts_set_pcr_pid(demux, m_pcrPid);
  if (ismd_ret != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CDVDDemuxSMD::SetPcrPid failed at ismd_demux_ts_set_pcr_pid");
    return;
  }
}

void CDVDDemuxSMD::AddPidFilter(int pid)
{
  // Add a filter to the tuner
  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();
  tuners[0]->AddPidFilter(pid, DvbTuner::DVB_FILTER_OTHER);
}

void CDVDDemuxSMD::RemovePidFilter(int pid)
{
  // Add a filter to the tuner
  const std::vector<DvbTunerPtr>& tuners = DVBManager::GetInstance().GetTuners();
  tuners[0]->RemovePidFilter(pid);
}

void CDVDDemuxSMD::SetState(ParsingState state)
{
  m_state = state;

  if(m_state == DEMUX_STATE_FOUND_PAT)
    m_lastPatStopWatch.Stop();
}

#endif
