/*
 * XBMC Media Center
 * Linux OpenGL Renderer
 * Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
 *
 * Based on XBoxRenderer by Frodo/jcmarshall
 * Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
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
#include "system.h"

#ifdef HAS_INTEL_SMD

#include "IntelSMDRenderer.h"
#include "xbmc/cores/IntelSMDGlobals.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "xbmc/threads/SingleLock.h"
#include "../dvdplayer/DVDClock.h"
#include "guilib/GUITexture.h"
#include "guilib/TransformMatrix.h"
#include "xbmc/settings/GUISettings.h"
#include "Application.h"

#include <ismd_core_protected.h>
#include <ismd_vidrend.h>
namespace ismd {
#include <ismd_vidpproc.h>
}
#include <libgdl.h>


#define ROUND_UP(num, amt) ((num%amt) ? (num+amt) - (num%amt) : num)
#define ROUND_DOWN(num, amt) ((num%amt) ? num - (num%amt): num)
#define CACHE_LINE_SIZE 64

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

#if 0
static int64_t pts_dtoi(double pts)
{
  pts_union u;
  u.pts_d = pts;
  return u.pts_i;
}

static double pts_itod(int64_t pts)
{
  pts_union u;
  u.pts_i = pts;
  return u.pts_d;
}
#endif

static void cpy_I420_to_NV12(uint8_t *I420_Y, int strid_I420_Y,
    uint8_t *I420_U, int strid_I420_U, uint8_t *I420_V, int strid_I420_V,
    uint8_t *NV12_Y, int strid_NV12_Y, uint8_t *NV12_UV, int strid_NV12_UV,
    int w, int h, int x)
{
  int i, src_offset, dst_offset, src_v_offset;
  //copy Y plane
  src_offset = dst_offset = x;
  for (i = 0; i < h; i++)
  {
    memcpy(&NV12_Y[dst_offset], &I420_Y[src_offset], w);
    dst_offset += strid_NV12_Y;
    src_offset += strid_I420_Y;
  }

  //copy U/V plane interleaved: 420 pseudo-planar
  //new UV copy implementation to use fast memcpy
  //TODO: test and handle the case that the address may not cache size aligned
  {
    unsigned int Width = (unsigned int) w;
    unsigned char temp_line[Width];
    unsigned int word_num;
    unsigned int u_word, v_word, uv_word1, uv_word2;
    unsigned char *u_ptr, *v_ptr, *out_data_uv;
    w = w >> 1;
    h = h >> 1;
    src_v_offset = src_offset = dst_offset = (x >> 1);
    out_data_uv = NV12_UV + dst_offset;
    u_ptr = I420_U + src_offset;
    v_ptr = I420_V + src_offset;
    for (i = 0; i < h; i++)
    {
      for (word_num = 0; word_num < Width >> 3; word_num++)
      {
        u_word = *(unsigned int *) (u_ptr + (word_num * 4));
        v_word = *(unsigned int *) (v_ptr + (word_num * 4));
        uv_word1 = (((u_word >> 0) & 0xFF) << 0)
            | (((v_word >> 0) & 0xFF) << 8) | (((u_word >> 8) & 0xFF) << 16)
            | (((v_word >> 8) & 0xFF) << 24);
        uv_word2 = (((u_word >> 16) & 0xFF) << 0) | (((v_word >> 16) & 0xFF)
            << 8) | (((u_word >> 24) & 0xFF) << 16) | (((v_word >> 24) & 0xFF)
            << 24);
        *(unsigned int *) (temp_line + (8 * word_num)) = uv_word1;
        *(unsigned int *) (temp_line + ((8 * word_num) + 4)) = uv_word2;
      }
      memcpy(out_data_uv, temp_line, Width);
      out_data_uv += strid_NV12_UV;
      u_ptr += strid_I420_U;
      v_ptr += strid_I420_V;
    }
  }

  return;
}

#if 0
static void uv_to_nv12(void *dest_uv, int dest_uv_stride,

void *src_u, void *src_v, int src_u_stride, int src_v_stride,

int width, /* number of u bytes per row */
int height)
{
  int row;
  int round_bytes_to_copy;
  uintptr_t dest_row;
  uintptr_t src_u_row;
  uintptr_t src_v_row;

  assert((uintptr_t)dest_uv % 32 == 0);
  assert(dest_uv_stride % 32 == 0);
  assert(src_u_stride == src_v_stride);
  assert(width >= 16);

  //printf("\t %d: %d %d %d\n", __LINE__,     dest_uv_stride,     src_u_stride,     src_v_stride     );
  //write_buffer_to_file("u_uv_to_nv12",src_u, src_u_stride, height, width);
  //write_buffer_to_file("v_uv_to_nv12",src_v, src_v_stride, height, width);

  round_bytes_to_copy = (width / 16) * 16;

  dest_row = (uintptr_t)dest_uv;
  src_u_row = (uintptr_t)src_u;
  src_v_row = (uintptr_t)src_v;

  for (row = 0; row < height; row++) {

    /* process the row */
    asm (
        "movl %0, %%edi # dest_row\n\t"
        "movl %1, %%eax # src_u_row\n\t"
        "movl %2, %%ebx # src_v_row\n\t"
        "movl %3, %%ecx # round bytes to copy\n\t"
        "movl %4, %%edx # total bytes to copy\n\t"
        "movl %%ecx, %%esi\n\t"

        "next_chunk:\n\t"
        /* read the next 16 U/V bytes */
        "movdqu -16(%%eax, %%ecx, 1), %%xmm1\n\t"
        "movdqu -16(%%ebx, %%ecx, 1), %%xmm2\n\t"

        /* generate the next 16 UV pairs */
        "movdqa %%xmm1, %%xmm3\n\t"
        "movdqa %%xmm2, %%xmm4\n\t"

        "punpckhbw %%xmm2, %%xmm1\n\t"
        "punpcklbw %%xmm4, %%xmm3\n\t"

        /* store the results */
        "movntdq %%xmm1, -16(%%edi, %%ecx, 2)\n\t"
        "movntdq %%xmm3, -32(%%edi, %%ecx, 2)\n\t"

        "subl $16, %%ecx\n\t"
        "jz done_with_chunks\n\t"

        "jmp next_chunk\n\t"
        "done_with_chunks:\n\t"

        "sfence\n\t"

        /* if width == chunked_bytes, we're done */
        "cmpl %%edx, %%esi\n\t"
        "je done_uv\n\t"
        /* else, we need to handle the remaining few bytes */

        /* read the last 16 U/V bytes */
        "movdqu -16(%%eax, %%edx, 1), %%xmm1\n\t"
        "movdqu -16(%%ebx, %%edx, 1), %%xmm2\n\t"

        /* generate the last 16 UV pairs */
        "movdqa %%xmm1, %%xmm3\n\t"
        "movdqa %%xmm2, %%xmm4\n\t"

        "punpckhbw %%xmm2, %%xmm1\n\t"
        "punpcklbw %%xmm4, %%xmm3\n\t"

        /* store the results */
        "movdqu %%xmm1, -16(%%edi, %%edx, 2)\n\t"
        "movdqu %%xmm3, -32(%%edi, %%edx, 2)\n\t"
        "clflush -1(%%edi, %%edx, 2)\n\t"
        "clflush -32(%%edi, %%edx, 2)\n\t"

        "done_uv:\n\t"

        :
        : "g"(dest_row),
          "g"(src_u_row),
          "g"(src_v_row),
          "g"(round_bytes_to_copy),
          "g"(width)
          : "eax", "ebx", "ecx", "edx", "edi", "esi"
    );

    dest_row += dest_uv_stride;
    src_u_row += src_u_stride;
    src_v_row += src_v_stride;
  }
}

static void uv_to_nv12_2(
    void *dest_uv,
    int dest_uv_stride,

    void *src_u,
    void *src_v,
    int src_u_stride,
    int src_v_stride,

    int width, /* number of u bytes per row */
    int height)
{
  int row;
  int round_bytes_to_copy;
  unsigned char* dest_row;
  unsigned char* src_u_row;
  unsigned char* src_v_row;

  assert((uintptr_t)dest_uv % 32 == 0);
  assert(dest_uv_stride % 32 == 0);
  assert(src_u_stride == src_v_stride);
  assert(width >= 16);

  round_bytes_to_copy = (width / 16) * 16;

  dest_row = (unsigned char*)dest_uv;
  src_u_row = (unsigned char*)src_u;
  src_v_row = (unsigned char*)src_v;

  for (row = 0; row < height; row++)
  {
    for(int i = 0; i < width / 2; i++)
    {
      *(dest_row + 2 * i + 0) = src_u_row[i];
      *(dest_row + 2 * i + 1) = src_v_row[i];
    }

    dest_row += dest_uv_stride;
    src_u_row += src_u_stride;
    src_v_row += src_v_stride;
  }
}
#endif

CIntelSMDRenderer::CIntelSMDRenderer()
{
  printf("%s\n", __FUNCTION__);

  SetDefaults();
}

CIntelSMDRenderer::~CIntelSMDRenderer()
{
  printf("%s\n", __FUNCTION__);

  UnInit();
}

void CIntelSMDRenderer::SetDefaults()
{
  //printf("%s\n", __FUNCTION__);

  m_bConfigured = false;

  m_sourceWidth = 0;
  m_sourceHeight = 0;
  m_destWidth = 0;
  m_destHeight = 0;
  m_fps = 0;

  m_startTime = 0;
  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = NUM_BUFFERS;
  m_PTS = DVD_NOPTS_VALUE;
  m_bCropping = false;
  m_bFlushFlag = true;
  m_bRunning = false;

  m_bUsingSMDecoder = true;
  m_bNullRendering = false;

  for(int b = 0; b < NUM_BUFFERS; b++)
    for (int p = 0; p < MAX_PLANES; p++)
      m_YUVMemoryTexture[b][p] = NULL;

  m_resolution = g_guiSettings.m_LookAndFeelResolution;

  m_lastAspectNum = 0;
  m_lastAspectDenom = 0;
  m_aspectTransition = 0;

}

unsigned int CIntelSMDRenderer::PreInit()
{
  printf("%s\n", __FUNCTION__);

  UnInit();

  return true;
}

void CIntelSMDRenderer::UnInit()
{
  //printf("%s\n", __FUNCTION__);

  if(!m_bUsingSMDecoder && !m_bNullRendering)
  {
    ReleaseYUVBuffers();
    g_IntelSMDGlobals.DeleteVideoRender();
    g_IntelSMDGlobals.ResetClock();
  }

  // clear video plane to remove any video leftovers
  gdl_flip(GDL_PLANE_ID_UPP_B, GDL_SURFACE_INVALID, GDL_FLIP_ASYNC);

  SetDefaults();
}

void CIntelSMDRenderer::SetSpeed(int speed)
{
  if(m_bUsingSMDecoder)
    return;

  printf("CIntelSMDRenderer::SetSpeed %d\n", speed);

  if(speed == DVD_PLAYSPEED_PAUSE)
  {
    g_IntelSMDGlobals.SetVideoRenderState(ISMD_DEV_STATE_PAUSE);
  }
  else
  {
    if(!m_bFlushFlag)
    {
      g_IntelSMDGlobals.SetVideoRenderBaseTime(g_IntelSMDGlobals.GetBaseTime());
      g_IntelSMDGlobals.SetVideoRenderState(ISMD_DEV_STATE_PLAY);
    }
    else
      printf("CIntelSMDRenderer::SetSpeed flush flag is set, ignoring request\n");
  }
}

void CIntelSMDRenderer::Flush()
{
  printf("CIntelSMDRenderer::Flush\n");

  m_bRunning = false;
  m_bFlushFlag = true;
}

void CIntelSMDRenderer::AllocateYUVBuffers()
{
  printf("%s\n", __FUNCTION__);

  ReleaseYUVBuffers();


  // Initialize the video to black (in yuv values)
  for(int b = 0; b < NUM_BUFFERS; b++)
    for (int p = 0; p < MAX_PLANES; p++)
    {
      if(p == 0)
      {
        m_YUVMemoryTexture[b][p] = new unsigned char[m_sourceWidth * m_sourceHeight];
        memset(m_YUVMemoryTexture[b][p], 16, m_sourceWidth * m_sourceHeight);
      }
      else
      {
        m_YUVMemoryTexture[b][p] = new unsigned char[m_sourceWidth / 2 * m_sourceHeight / 2];
        memset(m_YUVMemoryTexture[b][p], 128, m_sourceWidth / 2 * m_sourceHeight / 2);
      }
    }
}

void CIntelSMDRenderer::ReleaseYUVBuffers()
{
  printf("%s\n", __FUNCTION__);

  for(int b = 0; b < NUM_BUFFERS; b++)
    for (int p = 0; p < MAX_PLANES; p++)
    {
      delete [] m_YUVMemoryTexture[b][p];
      m_YUVMemoryTexture[b][p] = NULL;
    }
}

bool CIntelSMDRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, CRect &rect)
{
  CLog::Log(LOGINFO, "CIntelSMDRenderer::Configure width %d height %d d_width %d d_height %d fps %f flags %d\n",
      width, height, d_width, d_height, fps, flags);

  if(width == 0 || height == 0 || d_width == 0 || d_height == 0)
    return true;

  // print overscan values
  RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
  int left = g_settings.m_ResInfo[iRes].Overscan.left;
  int top = g_settings.m_ResInfo[iRes].Overscan.top;
  int right = g_settings.m_ResInfo[iRes].Overscan.right;
  int bottom = g_settings.m_ResInfo[iRes].Overscan.bottom;
  CLog::Log(LOGINFO, "CIntelSMDRenderer configure overscan values left %d top %d right %d bottom %d", left, top, right, bottom);

  // if only fps change, no need to reconfigure everything
  if(m_sourceWidth == width &&
      m_sourceHeight == height &&
      m_destWidth == d_width &&
      m_destHeight == d_height &&
      m_iFlags == flags)
  {
    if(m_fps != fps && fps > 20)
    {
      m_fps = fps;
      ChooseBestResolution(m_fps);
    }
    return true;
  }

  m_sourceWidth = width;
  m_sourceHeight = height;
  m_destWidth = d_width;
  m_destHeight = d_height;
  m_iFlags = flags;
  m_fps = fps;

  // if (flags & CONF_FLAGS_EXTERN_IMAGE)
  //   m_bNullRendering = true;
  // else
  //   m_bNullRendering = false;

  if (flags & CONF_FLAGS_SMD_DECODING)
    m_bUsingSMDecoder = true;
  else
    m_bUsingSMDecoder = false;

  if (!m_bUsingSMDecoder && !m_bNullRendering)
  {
    AllocateYUVBuffers();
    g_IntelSMDGlobals.CreateVideoRender(GDL_VIDEO_PLANE);
  }

  if (m_bUsingSMDecoder)
    CLog::Log(LOGINFO, "Video rendering using SMD decoder");
  else if (m_bNullRendering)
    CLog::Log(LOGINFO, "Video rendering using NULL renderer");
  else
    CLog::Log(LOGINFO, "Video rendering using software decoder");

  bool keep43ar = g_guiSettings.GetBool("ota.keep43ar");

  // if (g_application.IsPlayingLiveTV() && !keep43ar)
  // {
  //   g_stSettings.m_currentVideoSettings.m_ViewMode = VIEW_MODE_STRETCH_16x9;
  // }

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  if(!m_bNullRendering && (flags & CONF_FLAGS_FULLSCREEN))
    ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  if (!m_bNullRendering && !m_bConfigured)
  {
    ConfigureGDLPlane(GDL_VIDEO_PLANE);
    ConfigureVideoProc();
    ConfigureDeinterlace();
    ConfigureVideoFilters();
  }

  m_bConfigured = true;

  return true;
}

int CIntelSMDRenderer::GetImage(YV12Image *image, double pts, int source, bool readonly)
{
  if (!image)
    return -1;

  //printf("Get Image PTS = %f Configure %d\n", pts, m_bConfigured);

  if (!m_bConfigured)
    return -1;

  if(m_bUsingSMDecoder || m_bNullRendering)
  {
    return 0;
  }

  m_PTS = pts;

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return -1;

  YUVMEMORYPLANES &planes = m_YUVMemoryTexture[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;

  for(int i = 0; i < MAX_PLANES; i++)
  {
    if(i == 0)
      image->stride[i] = m_sourceWidth;
    else
      image->stride[i] = m_sourceWidth / 2;
    image->plane[i] = planes[i];
  }

  return source;
}

void CIntelSMDRenderer::ReleaseImage(int source, bool preserve)
{
  if(!m_bUsingSMDecoder)
  {
    RenderYUVBUffer(m_YUVMemoryTexture[m_iYV12RenderBuffer]);
  }

  m_iYV12RenderBuffer = NextYV12Texture();
}

void CIntelSMDRenderer::Reset()
{
  //printf("%s\n", __FUNCTION__);

  UnInit();
}

void CIntelSMDRenderer::Update(bool bPauseDrawing)
{
  //printf("%s\n", __FUNCTION__);
  if (!m_bConfigured)
    return;

  ManageDisplay();
}

void CIntelSMDRenderer::RenderYUVBUffer(YUVMEMORYPLANES plane)
{
  ismd_result_t result;
  ismd_pts_t ismd_pts;
  ismd_buffer_handle_t renderBuf = ISMD_ERROR_INVALID_HANDLE;

  ismd_port_handle_t video_input_port_proc;
  video_input_port_proc = g_IntelSMDGlobals.GetVidProcInput();

  if(video_input_port_proc == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDRenderer::RenderYUVBUffer video_input_port_proc == -1");
    return;
  }

  if(plane[0] == NULL || plane[1] == NULL || plane[2] == NULL)
  {
    CLog::Log(LOGERROR, "CIntelSMDRenderer::RenderYUVBUffer invalid plane data");
    return;
  }

  ismd_pts = g_IntelSMDGlobals.DvdToIsmdPts(m_PTS);

  unsigned int destHeight = ROUND_DOWN(m_sourceHeight, 16);
  unsigned int destWidth = ROUND_DOWN(m_sourceWidth, 16);

  int height_to_alloc = (destHeight * 2);

  result = ismd_frame_buffer_alloc(ROUND_UP(destWidth, CACHE_LINE_SIZE), height_to_alloc, &renderBuf);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismd_gst_element: request for downstream allocation: ismd buffer allocation failed");
    return;
  }

  ismd_buffer_descriptor_t desc;
  result = ismd_buffer_read_desc(renderBuf, &desc);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismdismd_buffer_read_desc failed");
    ismd_buffer_dereference(renderBuf);
    return;
  }

  unsigned int systemStride = SYSTEM_STRIDE;
  unsigned char* ptr = (unsigned char*) OS_MAP_IO_TO_MEM_NOCACHE (desc.phys.base, height_to_alloc * systemStride);

  if(ptr == NULL)
  {
    CLog::Log(LOGERROR, "IntelSMDRenderer::RenderYUVBUffer OS_MAP_IO_TO_MEM_NOCACHE return null");
    return;
  }

  // new method to copy I420 to NV12, doesn't crash on 720p
  cpy_I420_to_NV12(plane[0], m_sourceWidth, plane[1], m_sourceWidth / 2, plane[2], m_sourceWidth / 2,
      ptr, systemStride, ptr + (destHeight * systemStride), systemStride, destWidth, destHeight, 0);

#if 0
  // Copy Y data
  for (int line = 0; line < m_sourceHeight; line++)
  {
    OS_MEMCPY(ptr + (line * SYSTEM_STRIDE), plane[0] + (line * m_sourceWidth), m_sourceWidth);
  }

  uv_to_nv12(ptr + (m_sourceHeight * SYSTEM_STRIDE), SYSTEM_STRIDE, plane[1], plane[2], m_sourceWidth / 2, m_sourceWidth / 2,
   m_sourceWidth, m_sourceHeight);
#endif


  OS_UNMAP_IO_FROM_MEM(ptr, height_to_alloc * systemStride);

  ismd_frame_attributes_t *attr = (ismd_frame_attributes_t *) (desc.attributes);
  attr->discontinuity = false;
  attr->cont_rate = 0;
  attr->cont_size.width = destWidth;
  attr->cont_size.height = destHeight;
  attr->dest_size.width = destWidth;
  attr->dest_size.height = destHeight;
  attr->pixel_format = ISMD_PF_NV12;
  attr->color_space = ISMD_SRC_COLOR_SPACE_BT601;
  attr->polarity = ISMD_POLARITY_FRAME;   // FIXME: can we tell if it's interlaced?
  attr->gamma_type = ISMD_GAMMA_HDTV; // default as per as per ISO/IEC 13818-2
  attr->repeated = 0;
  attr->y = 0;
  attr->u = attr->y + (systemStride * destHeight);
  attr->v = attr->y + (systemStride * destHeight);
  attr->original_pts = ismd_pts;
  attr->local_pts = ismd_pts;

  //printf("PTS  = %.2f\n", g_IntelSMDGlobals.IsmdToDvdPts(ismd_pts) / 1000000);

  // commit the changes to the descriptor
  result = ismd_buffer_update_desc (renderBuf, &desc);
  if (result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "failed to update ISMD descriptor!");
    ismd_buffer_dereference(renderBuf);
    return;
  }

  if(m_bFlushFlag)
  {
    ismd_pts_t start = ismd_pts;

    if(start == ISMD_NO_PTS && g_IntelSMDGlobals.GetAudioStartPts() != ISMD_NO_PTS)
      start = g_IntelSMDGlobals.GetAudioStartPts();

    if (!m_bUsingSMDecoder)
    {
      g_IntelSMDGlobals.FlushVideoRender();

      if (g_IntelSMDGlobals.GetFlushFlag())
      {
        //printf("Setting base time from renderer\n");
        g_IntelSMDGlobals.SetBaseTime(g_IntelSMDGlobals.GetCurrentTime());
        g_IntelSMDGlobals.SetFlushFlag(false);
      }
    }

    // Add a new_segment buffer to the next video buffer
    g_IntelSMDGlobals.SetVideoStartPts(start);
    g_IntelSMDGlobals.SetVideoRenderBaseTime(g_IntelSMDGlobals.GetBaseTime());
    g_IntelSMDGlobals.CreateStartPacket(start, renderBuf);
    g_IntelSMDGlobals.SetVideoRenderState(ISMD_DEV_STATE_PLAY);
    m_bFlushFlag = false;
    m_bRunning = true;
  }

  ismd_pts_t syncPTS = g_IntelSMDGlobals.Resync();
  if(syncPTS != ISMD_NO_PTS)
  {
    if(syncPTS != g_IntelSMDGlobals.GetVideoStartPts())
    {
      g_IntelSMDGlobals.SetVideoStartPts(syncPTS);
      g_IntelSMDGlobals.CreateStartPacket(syncPTS, renderBuf);
    }
  }

  result = ISMD_ERROR_UNSPECIFIED;

  int retry = 0;

  while(m_bRunning && retry < 5)
  {
    result = ismd_port_write(video_input_port_proc, renderBuf);

    if(result == ISMD_SUCCESS)
      break;

    if(g_IntelSMDGlobals.GetVideoRenderState() != ISMD_DEV_STATE_STOP)
    {
      retry++;
      Sleep(100);
    } else
      break;
  }

  if(result != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "CIntelSMDRenderer::RenderYUVBUffer failed. %d", result);
    ismd_buffer_dereference(renderBuf);
  }

  return;
}

void CIntelSMDRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured)
  {
    // if(g_application.GetCurrentPlayer() == EPC_FLASHPLAYER)
    //     g_graphicsContext.Clear();

    return;
  }

  if(m_bNullRendering)
  {
    g_graphicsContext.Clear();
    Render(flags);
    return;
  }

  if(clear)
    g_graphicsContext.Clear();
  // else if (g_application.GetCurrentPlayer() == EPC_FLASHPLAYER)
  //   g_graphicsContext.Clear();

  ManageDisplay();

  if(!clear)
  {
    g_graphicsContext.AddTransform(TransformMatrix());
    CGUITextureGLES::DrawQuad(m_destRect, alpha);
    g_graphicsContext.RemoveTransform();
  }

  ConfigureVideoProc();

  Render(flags);
}

int CIntelSMDRenderer::NextYV12Texture()
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  else
    return -1;
}

unsigned int CIntelSMDRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  printf("%s\n", __FUNCTION__);
  return 0;
}

void CIntelSMDRenderer::Render(DWORD flags)
{
  return;
}

bool CIntelSMDRenderer::SupportsBrightness()
{
  return false;
}

bool CIntelSMDRenderer::SupportsContrast()
{
  return false;
}

bool CIntelSMDRenderer::SupportsGamma()
{
  return false;
}

int CIntelSMDRenderer::ConfigureGDLPlane(gdl_plane_id_t plane)
{
  CLog::Log(LOGINFO, "CIntelSMDRenderer::ConfigureGDLPlane %d", plane);

  gdl_ret_t rc = GDL_SUCCESS;

  rc = gdl_plane_reset(plane);

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_begin(plane);
  }

  gdl_vid_policy_t policy = GDL_VID_POLICY_RESIZE;

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_set_attr(GDL_PLANE_VID_MISMATCH_POLICY, &policy);
  }

  if (GDL_SUCCESS == rc)
  {
    rc = gdl_plane_config_end(GDL_FALSE);
  }
  else
  {
    gdl_plane_config_end(GDL_TRUE);
  }

  if (GDL_SUCCESS != rc)
  {
    fprintf(stderr, "GDL configuration failed! GDL error code is 0x%x\n", rc);
  }

  return rc;
}

int CIntelSMDRenderer::ConfigureVideoFilters()
{
  ismd_result_t res;

  ismd_dev_t video_proc = g_IntelSMDGlobals.GetVidProc();

  bool deringing_filter = g_guiSettings.GetBool("videoplayback.deringing");
  bool gaussian_filter = g_guiSettings.GetBool("videoplayback.gaussian");

  if (deringing_filter)
    res = ismd::ismd_vidpproc_deringing_enable(video_proc);
  else
    res = ismd::ismd_vidpproc_deringing_disable(video_proc);

  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismd_vidpproc_gaussian_enable failed. %d", res);
    return 0;
  }

  if (gaussian_filter)
    res = ismd::ismd_vidpproc_gaussian_enable(video_proc);
  else
    res = ismd::ismd_vidpproc_gaussian_disable(video_proc);

  if (res != ISMD_SUCCESS)
  {
    CLog::Log(LOGERROR, "ismd_vidpproc_gaussian_enable failed. %d", res);
    return 0;
  }

  CLog::Log(LOGINFO, "CIntelSMDRenderer Setting deringing_filter %d\n", deringing_filter);
  CLog::Log(LOGINFO, "CIntelSMDRenderer Setting gaussian filter %d\n", gaussian_filter);

  return 1;
}

int CIntelSMDRenderer::ConfigureDeinterlace()
{
  ismd_result_t ret;

  int iResolution = g_graphicsContext.GetVideoResolution();
  ismd_dev_t video_proc = g_IntelSMDGlobals.GetVidProc();
  int deinterlace_policy = g_guiSettings.GetInt("videoplayback.interlacemode");
  bool interlace_display = g_settings.m_ResInfo[iResolution].dwFlags & D3DPRESENTFLAG_INTERLACED;
  ismd::ismd_vidpproc_deinterlace_policy_t smd_policy = ismd::NONE;

  switch(deinterlace_policy)
  {
  case INTERLACE_MODE_AUTO:
    smd_policy = ismd::AUTO;
    break;
  case INTERLACE_MODE_VIDEO:
    smd_policy = ismd::VIDEO;
    break;
  case INTERLACE_MODE_FILM:
    smd_policy = ismd::FILM;
    break;
  case INTERLACE_MODE_SPATIAL_ONLY:
    smd_policy = ismd::SPATIAL_ONLY;
    break;
  case INTERLACE_MODE_TOP_FIELD_ONLY:
    smd_policy = ismd::TOP_FIELD_ONLY;
    break;
  case INTERLACE_MODE_SCALE_ONLY:
    smd_policy = ismd::NONE;
    break;
  case INTERLACE_MODE_NEVER:
    smd_policy = ismd::NEVER;
    break;
  }

  // if(g_application.IsPlayingLiveTV())
  // {
  //   smd_policy = ismd::VIDEO;
  // }

  if(interlace_display)
  {
    if(deinterlace_policy == INTERLACE_MODE_AUTO)
      smd_policy = ismd::NONE;
  }

  CLog::Log(LOGINFO, "Setting Interlace mode: screen %d user %d smd %d", interlace_display, deinterlace_policy, smd_policy);

  ret = ismd::ismd_vidpproc_set_deinterlace_policy(video_proc, smd_policy);

  if (ret != ISMD_SUCCESS)
  {
    printf("ismd_vidpproc_set_deinterlace_policy failed\n");
  }

  return 1;
}

int CIntelSMDRenderer::ConfigureVideoProc()
{
  ismd_result_t ret = ISMD_ERROR_NO_RESOURCES;

  int screenWidth = g_graphicsContext.GetWidth();
  int screenHeight = g_graphicsContext.GetHeight();
  const CRect& viewWindow = g_graphicsContext.GetViewWindow();

  // ok, get the dest rect
  int destx = (int)m_destRect.x1;
  int desty = (int)m_destRect.y1;
  unsigned int width = (unsigned int)m_destRect.Width();
  unsigned int height = (unsigned int)m_destRect.Height();

  int aspect_ratio_num = 100;
  int aspect_ratio_denom = 100;
  
  ismd_dev_t video_proc = g_IntelSMDGlobals.GetVidProc();

  if(video_proc == -1)
  {
    CLog::Log(LOGERROR, "CIntelSMDRenderer::ConfigureVideoProc failed to retrieve video proc");
    return 0;
  }

  // now, see if we need to crop. basically if the destination window
  // for video is outside of the viewWindow (which is the overscan adjusted screen)
  // then cropping is required
  if( destx < viewWindow.x1 ||
      desty < viewWindow.y1 ||
      width > viewWindow.Width()||
      height > viewWindow.Height() )
  {
    int croppedX1 = (int)m_sourceRect.x1, 
        croppedY1 = (int)m_sourceRect.y1,
        croppedWidth = (int)m_sourceRect.Width(),
        croppedHeight = (int)m_sourceRect.Height();
    
    // core image from the source rect is being blown out of the viewing area
    // usually this happens due to zoom or overscan adjustments
    // what we do here is calculate any scaling ratio applied to the source rect, so
    // we can crop accordingly
    
    // adjust the destination window
    if( destx < viewWindow.x1 || width > viewWindow.Width() )
    {
      float scale = m_destRect.Width() / m_sourceRect.Width();
      croppedWidth = (int) (viewWindow.Width() / scale);
      croppedX1 += (int)((m_sourceRect.Width() - croppedWidth) / 2);
      
      width = (unsigned int)viewWindow.Width();
      destx = (int)viewWindow.x1;
    }
    if( desty < viewWindow.y1 || height > viewWindow.Height() )
    {
      float scale = m_destRect.Height() / m_sourceRect.Height();
      croppedHeight = (int)(viewWindow.Height() / scale);
      croppedY1 += (int)((m_sourceRect.Height() - croppedHeight) / 2);

      height = (unsigned int)viewWindow.Height();
      desty = (int)viewWindow.y1;
    }

    /*
    printf("croppedX1 %d croppedY1 %d croppedWidth %d croppedHeight %d sW %d sH %d\n",
     croppedX1, croppedY1, croppedWidth, croppedHeight, screenWidth, screenHeight);
    */
    
    if( croppedX1 < 0 ) croppedX1 = 0;
    if( croppedY1 < 0 ) croppedY1 = 0;
    if( croppedX1 + croppedWidth > m_sourceRect.Width() )
      croppedWidth = (int)m_sourceRect.Width() - croppedX1;
    if( croppedY1 + croppedHeight > m_sourceRect.Height() )
      croppedHeight = (int)m_sourceRect.Height() - croppedY1;
    
    ret = ismd::ismd_vidpproc_set_crop_input(video_proc,
                  (unsigned int)croppedX1,
                  (unsigned int)croppedY1,
                  (unsigned int)croppedWidth,
                  (unsigned int)croppedHeight);
    if (ret != ISMD_SUCCESS)
    {
      printf("ismd_vidpproc_set_crop_input failed\n");
      return 0;
    }
    
    m_bCropping = true;
  }
  else if (m_bCropping)
  {
    ismd::ismd_vidpproc_disable_crop_input( video_proc );
    m_bCropping = false;
  }
  
  if( destx < 0 ) destx = 0;
  if( desty < 0 ) desty = 0;
  if( destx + width > (unsigned int) screenWidth ) width = screenWidth - destx;
  if( desty + height > (unsigned int) screenHeight ) height = screenHeight - desty;
  
  if( desty & 1 && height & 1)
  {
    desty--;
    height++;
  }
  else if( height & 1 ) height--;
  else if( desty & 1 ) desty--;
  
  /*
  printf("width = %d height = %d aspect ratio = %dx%d, destx = %d desty = %d\n",
      width, height, aspect_ratio_num, aspect_ratio_denom, destx, desty );

  printf("source (%f,%f) (%f,%f), dest (%f,%f) (%f,%f)\n",
     m_sourceRect.x1, m_sourceRect.y1, m_sourceRect.x2, m_sourceRect.y2,
      m_destRect.x1, m_destRect.y1, m_destRect.x2, m_destRect.y2);
      */

  ret = ismd::ismd_vidpproc_set_dest_params2(video_proc,
            width,
            height,
            aspect_ratio_num,
            aspect_ratio_denom,
            (unsigned int)destx,
            (unsigned int)desty);
  if(ret != ISMD_SUCCESS)
  {
    printf("ismd_vidpproc_set_dest_params2 failed\n");
    return 0;
  }

  return 1;
}

#endif

