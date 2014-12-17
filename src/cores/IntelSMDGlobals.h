
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

#pragma once

extern "C"
{
#include <ismd_core.h>
#include <ismd_viddec.h>
#include <ismd_vidrend.h>
#include <ismd_audio.h>
#include <libgdl.h>
#include <pal_soc_info.h>
}

#include <queue>
#include "xbmc/threads/SingleLock.h"
#include "dvdplayer/DVDClock.h"

extern char *dbg_id(const char *fmt, ...);
#define __DEBUG_ID__ dbg_id("%20s::%-32s - %04d", __MODULE_NAME__, __FUNCTION__, __LINE__)

#define GDL_VIDEO_PLANE           GDL_PLANE_ID_UPP_B  // bottom
#define GDL_GRAPHICS_PLANE        GDL_PLANE_ID_UPP_D  // on top for overlays
#define GDL_FLASH_GRAPHICS_PLANE  GDL_PLANE_ID_UPP_C  // middle for pages with embedded video

#define SMD_CLOCK_FREQ 90000

#define ISMD_VIDEO_BUFFER_QUEUE 30

enum InterlaceMode
{
  INTERLACE_MODE_AUTO = 0,
  INTERLACE_MODE_VIDEO,
  INTERLACE_MODE_FILM,
  INTERLACE_MODE_SPATIAL_ONLY,
  INTERLACE_MODE_TOP_FIELD_ONLY,
  INTERLACE_MODE_SCALE_ONLY,
  INTERLACE_MODE_NEVER,
};

// arbitrary value to allow communication of DTSHD support
#define SMD_CODEC_DTSHD 0xf0f0f0f0

class CISMDBuffer {
public:
  CISMDBuffer() : m_bFlush(false) {
  }
  ~CISMDBuffer() {
    while (!m_buffers.empty())
    {
      ismd_buffer_dereference(m_buffers.front());
      m_buffers.pop();
    }
  }
  double firstpts;
  double pts;
  double dts;
  bool m_bFlush;
  std::queue<ismd_buffer_handle_t> m_buffers;
};

class CIntelSMDGlobals
{
public:
  CIntelSMDGlobals();
  virtual ~CIntelSMDGlobals();

  ismd_clock_t GetSMDClock() { return m_main_clock; }
  ismd_dev_t GetVidDec() { return m_viddec; }
  ismd_dev_t GetVidProc() { return m_video_proc; }
  ismd_dev_t GetVidRender() { return m_video_render; }
  ismd_port_handle_t GetVidDecInput() { return m_viddec_input_port; }
  ismd_port_handle_t GetVidDecOutput() { return m_viddec_output_port; }
  ismd_port_handle_t GetVidProcInput() { return m_video_input_port_proc; }
  ismd_port_handle_t GetVidProcOutput() { return m_video_output_port_proc; }
  ismd_port_handle_t GetVidRenderInput() { return m_video_input_port_renderer; }
  ismd_port_handle_t GetVidRenderOutput() { return m_video_output_port_renderer; }
  bool SetVideoRenderBaseTime(ismd_time_t time);

  bool  InitAudio();
  bool  DeInitAudio();
  bool  CreateAudioProcessor();
  bool  DeleteAudioProcessor();
  ismd_audio_processor_t GetAudioProcessor() { return m_audioProcessor; }
  bool  ConfigureMasterClock(unsigned int frequency);

  ismd_audio_output_t GetHDMIOutput() { return m_audioOutputHDMI; }
  ismd_audio_output_t GetSPDIFOutput() { return m_audioOutputSPDIF; }
  ismd_audio_output_t GetI2SOutput() { return m_audioOutputI2S0; }

  bool  BuildAudioOutputs();
  bool  DisableAudioOutput(ismd_audio_output_t audio_output);
  bool  EnableAudioOutput(ismd_audio_output_t audio_output);
  bool  ConfigureAudioOutput(ismd_audio_output_t output, ismd_audio_output_config_t output_config);

  bool  EnableAudioInput(ismd_dev_t input);
  bool  DisableAudioInput(ismd_dev_t input);

  ismd_dev_t          CreateAudioInput(bool timed);
  bool                RemoveAudioInput(ismd_dev_t device);

  ismd_dev_t          GetPrimaryAudioDevice() { return m_primaryAudioInput; }
  void                SetPrimaryAudioDevice(ismd_dev_t device) { m_primaryAudioInput = device; }

  ismd_port_handle_t  GetAudioDevicePort(ismd_dev_t device);

  ismd_time_t         GetCurrentTime();
  bool                SetCurrentTime(ismd_time_t time);

  ismd_result_t    GetPortStatus(ismd_port_handle_t port, unsigned int& curDepth, unsigned int& maxDepth);
  ismd_dev_state_t GetRenderState() { return m_RenderState; }

  bool CreateMainClock();
  void DestroyMainClock();

  void CreateStartPacket(ismd_pts_t start_pts, ismd_buffer_handle_t buffer_handle);
  void SendStartPacket(ismd_pts_t start_pts, ismd_port_handle_t port, ismd_buffer_handle_t buffer = 0);
  ismd_pts_t DvdToIsmdPts(double pts);
  double IsmdToDvdPts(ismd_pts_t pts);

  bool CreateVideoRender(gdl_plane_id_t plane);
  bool DeleteVideoRender();
  bool CreateVideoDecoder(ismd_codec_type_t codec_type);
  bool DeleteVideoDecoder();
  bool ConnectDecoderToRenderer();

  bool SetAudioDeviceState(ismd_dev_state_t state, ismd_dev_t device);
  bool SetVideoDecoderState(ismd_dev_state_t state);
  bool SetVideoRenderState(ismd_dev_state_t state);

  ismd_dev_state_t GetVideoRenderState() { return m_RenderState; }
  ismd_dev_state_t GetAudioDeviceState(ismd_dev_t device);

  bool FlushAudioDevice(ismd_dev_t device);
  bool FlushVideoDecoder();
  bool FlushVideoRender();

  bool PrintRenderStats();

  static bool SetMasterVolume(float nVolume);
  static void Mute(bool bMute);

protected:

  bool SetClockPrimary();

  ismd_audio_output_t   AddAudioOutput(int output);
  bool                  RemoveAudioOutput(ismd_audio_output_t output);
  bool                  RemoveAllAudioOutput();

  // clock
  ismd_clock_t            m_main_clock;

  // Video
  ismd_dev_t              m_bufmon;
  ismd_dev_t              m_viddec;
  ismd_dev_t              m_video_proc;
  ismd_dev_t              m_video_render;
  ismd_port_handle_t      m_viddec_input_port;
  ismd_port_handle_t      m_viddec_output_port;
  ismd_port_handle_t      m_video_input_port_proc;
  ismd_port_handle_t      m_video_output_port_proc;
  ismd_port_handle_t      m_video_input_port_renderer;
  ismd_port_handle_t      m_video_output_port_renderer;

  ismd_dev_state_t        m_RenderState;

  ismd_audio_processor_t  m_audioProcessor;

  ismd_dev_t              m_primaryAudioInput; // for AV main playback

  // Audio outputs
  ismd_audio_output_t     m_audioOutputHDMI;
  ismd_audio_output_t     m_audioOutputSPDIF;
  ismd_audio_output_t     m_audioOutputI2S0;

  CCriticalSection        m_Lock;
};


extern CIntelSMDGlobals g_IntelSMDGlobals;

