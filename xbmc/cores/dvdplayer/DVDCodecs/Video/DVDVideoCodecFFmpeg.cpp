/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDVideoPPFFmpeg.h"
#if defined(TARGET_POSIX) || defined(TARGET_WINDOWS)
#include "utils/CPUInfo.h"
#endif
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "boost/shared_ptr.hpp"
#include "threads/Atomics.h"

#ifndef TARGET_POSIX
#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))
#else
#include <math.h>
#define RINT lrint
#endif

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/RenderFormats.h"

#ifdef HAVE_LIBVDPAU
#include "VDPAU.h"
#endif
#ifdef HAS_DX
#include "DXVA.h"
#endif
#ifdef HAVE_LIBVA
#include "VAAPI.h"
#endif
#ifdef TARGET_DARWIN_OSX
#include "VDA.h"
#endif

using namespace boost;

enum PixelFormat CDVDVideoCodecFFmpeg::GetFormat( struct AVCodecContext * avctx
                                                , const PixelFormat * fmt )
{
  CDVDVideoCodecFFmpeg* ctx  = (CDVDVideoCodecFFmpeg*)avctx->opaque;

  if(!ctx->IsHardwareAllowed())
    return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);

  const PixelFormat * cur = fmt;
  while(*cur != PIX_FMT_NONE)
  {
#ifdef HAVE_LIBVDPAU
    if(VDPAU::CDecoder::IsVDPAUFormat(*cur) && CSettings::Get().GetBool("videoplayer.usevdpau"))
    {
      CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::GetFormat - Creating VDPAU(%ix%i)", avctx->width, avctx->height);
      VDPAU::CDecoder* vdp = new VDPAU::CDecoder();
      if(vdp->Open(avctx, *cur, ctx->m_uSurfacesCount))
      {
        ctx->SetHardware(vdp);
        return *cur;
      }
      else
        vdp->Release();
    }
#endif
#ifdef HAS_DX
  if(DXVA::CDecoder::Supports(*cur) && CSettings::Get().GetBool("videoplayer.usedxva2"))
  {
    DXVA::CDecoder* dec = new DXVA::CDecoder();
    if(dec->Open(avctx, *cur, ctx->m_uSurfacesCount))
    {
      ctx->SetHardware(dec);
      return *cur;
    }
    else
      dec->Release();
  }
#endif
#ifdef HAVE_LIBVA
    // mpeg4 vaapi decoding is disabled
    if(*cur == PIX_FMT_VAAPI_VLD && CSettings::Get().GetBool("videoplayer.usevaapi") 
    && (avctx->codec_id != AV_CODEC_ID_MPEG4 || g_advancedSettings.m_videoAllowMpeg4VAAPI)) 
    {
      if (ctx->GetHardware() != NULL)
      {
        ctx->SetHardware(NULL);
      }

      VAAPI::CDecoder* dec = new VAAPI::CDecoder();
      if(dec->Open(avctx, *cur, ctx->m_uSurfacesCount))
      {
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif

#ifdef TARGET_DARWIN_OSX
    if (*cur == AV_PIX_FMT_VDA_VLD && CSettings::Get().GetBool("videoplayer.usevda"))
    {
      VDA::CDecoder* dec = new VDA::CDecoder();
      if(dec->Open(avctx, *cur, ctx->m_uSurfacesCount))
      {
        ctx->SetHardware(dec);
        return *cur;
      }
      else
        dec->Release();
    }
#endif
    cur++;
  }
  return ctx->m_dllAvCodec.avcodec_default_get_format(avctx, fmt);
}

CDVDVideoCodecFFmpeg::CDVDVideoCodecFFmpeg() : CDVDVideoCodec()
{
  m_pCodecContext = NULL;
  m_pFrame = NULL;
  m_pFilterGraph  = NULL;
  m_pFilterIn     = NULL;
  m_pFilterOut    = NULL;
#if defined(LIBAVFILTER_AVFRAME_BASED)
  m_pFilterFrame  = NULL;
#else
  m_pBufferRef    = NULL;
#endif

  m_iPictureWidth = 0;
  m_iPictureHeight = 0;

  m_uSurfacesCount = 0;

  m_iScreenWidth = 0;
  m_iScreenHeight = 0;
  m_iOrientation = 0;
  m_bSoftware = false;
  m_isHi10p = false;
  m_pHardware = NULL;
  m_iLastKeyframe = 0;
  m_dts = DVD_NOPTS_VALUE;
  m_started = false;
}

CDVDVideoCodecFFmpeg::~CDVDVideoCodecFFmpeg()
{
  Dispose();
}

bool CDVDVideoCodecFFmpeg::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  AVCodec* pCodec;

  if(!m_dllAvUtil.Load()
  || !m_dllAvCodec.Load()
  || !m_dllSwScale.Load()
  || !m_dllPostProc.Load()
  || !m_dllAvFilter.Load()
  ) return false;

  m_dllAvCodec.avcodec_register_all();
  m_dllAvFilter.avfilter_register_all();

  m_bSoftware     = hints.software;
  m_iOrientation  = hints.orientation;

  for(std::vector<ERenderFormat>::iterator it = options.m_formats.begin(); it != options.m_formats.end(); ++it)
  {
    m_formats.push_back((PixelFormat)CDVDCodecUtils::PixfmtFromEFormat(*it));
    if(*it == RENDER_FMT_YUV420P)
      m_formats.push_back(PIX_FMT_YUVJ420P);
  }
  m_formats.push_back(PIX_FMT_NONE); /* always add none to get a terminated list in ffmpeg world */

  pCodec = NULL;
  m_pCodecContext = NULL;

  if (hints.codec == AV_CODEC_ID_H264)
  {
    switch(hints.profile)
    {
      case FF_PROFILE_H264_HIGH_10:
      case FF_PROFILE_H264_HIGH_10_INTRA:
      case FF_PROFILE_H264_HIGH_422:
      case FF_PROFILE_H264_HIGH_422_INTRA:
      case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      case FF_PROFILE_H264_HIGH_444_INTRA:
      case FF_PROFILE_H264_CAVLC_444:
      // this is needed to not open the decoders
      m_bSoftware = true;
      // this we need to enable multithreading for hi10p via advancedsettings
      m_isHi10p = true;
      break;
    }
  }

  if(pCodec == NULL)
    pCodec = m_dllAvCodec.avcodec_find_decoder(hints.codec);

  if(pCodec == NULL)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to find codec %d", hints.codec);
    return false;
  }

  CLog::Log(LOGNOTICE,"CDVDVideoCodecFFmpeg::Open() Using codec: %s",pCodec->long_name ? pCodec->long_name : pCodec->name);

  if(m_pCodecContext == NULL)
    m_pCodecContext = m_dllAvCodec.avcodec_alloc_context3(pCodec);

  m_pCodecContext->opaque = (void*)this;
  m_pCodecContext->debug_mv = 0;
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->codec_tag = hints.codec_tag;
  /* Only allow slice threading, since frame threading is more
   * sensitive to changes in frame sizes, and it causes crashes
   * during HW accell - so we unset it in this case.
   *
   * When we detect Hi10p and user did not disable hi10pmultithreading
   * via advancedsettings.xml we keep the ffmpeg default thread type.
   * */
  if(m_isHi10p && !g_advancedSettings.m_videoDisableHi10pMultithreading)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Keep default threading for Hi10p: %d",
                        m_pCodecContext->thread_type);
  }
  else
    m_pCodecContext->thread_type = FF_THREAD_SLICE;

#if defined(TARGET_DARWIN_IOS)
  // ffmpeg with enabled neon will crash and burn if this is enabled
  m_pCodecContext->flags &= CODEC_FLAG_EMU_EDGE;
#else
  if (pCodec->id != AV_CODEC_ID_H264 && pCodec->capabilities & CODEC_CAP_DR1
      && pCodec->id != AV_CODEC_ID_VP8
     )
    m_pCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
#endif

  // if we don't do this, then some codecs seem to fail.
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;

  if( hints.extradata && hints.extrasize > 0 )
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata = (uint8_t*)m_dllAvUtil.av_mallocz(hints.extrasize + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  // advanced setting override for skip loop filter (see avcodec.h for valid options)
  // TODO: allow per video setting?
  if (g_advancedSettings.m_iSkipLoopFilter != 0)
  {
    m_pCodecContext->skip_loop_filter = (AVDiscard)g_advancedSettings.m_iSkipLoopFilter;
  }

  // set any special options
  for(std::vector<CDVDCodecOption>::iterator it = options.m_keys.begin(); it != options.m_keys.end(); ++it)
  {
    if (it->m_name == "surfaces")
      m_uSurfacesCount = std::atoi(it->m_value.c_str());
    else
      m_dllAvUtil.av_opt_set(m_pCodecContext, it->m_name.c_str(), it->m_value.c_str(), 0);
  }

  int num_threads = std::min(8 /*MAX_THREADS*/, g_cpuInfo.getCPUCount());
  if( num_threads > 1 && !hints.software && m_pHardware == NULL // thumbnail extraction fails when run threaded
  && ( pCodec->id == AV_CODEC_ID_H264
    || pCodec->id == AV_CODEC_ID_MPEG4 ))
    m_pCodecContext->thread_count = num_threads;

  if (m_dllAvCodec.avcodec_open2(m_pCodecContext, pCodec, NULL) < 0)
  {
    CLog::Log(LOGDEBUG,"CDVDVideoCodecFFmpeg::Open() Unable to open codec");
    return false;
  }

  m_pFrame = m_dllAvCodec.avcodec_alloc_frame();
  if (!m_pFrame) return false;

#if defined(LIBAVFILTER_AVFRAME_BASED)
  m_pFilterFrame = m_dllAvUtil.av_frame_alloc();
  if (!m_pFilterFrame) return false;
#endif

  UpdateName();
  return true;
}

void CDVDVideoCodecFFmpeg::Dispose()
{
  if (m_pFrame) m_dllAvUtil.av_free(m_pFrame);
  m_pFrame = NULL;

#if defined(LIBAVFILTER_AVFRAME_BASED)
  m_dllAvUtil.av_frame_free(&m_pFilterFrame);
#endif

  if (m_pCodecContext)
  {
    if (m_pCodecContext->codec) m_dllAvCodec.avcodec_close(m_pCodecContext);
    if (m_pCodecContext->extradata)
    {
      m_dllAvUtil.av_free(m_pCodecContext->extradata);
      m_pCodecContext->extradata = NULL;
      m_pCodecContext->extradata_size = 0;
    }
    m_dllAvUtil.av_free(m_pCodecContext);
    m_pCodecContext = NULL;
  }
  SAFE_RELEASE(m_pHardware);

  FilterClose();

  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
  m_dllAvFilter.Unload();
  m_dllPostProc.Unload();
}

void CDVDVideoCodecFFmpeg::SetDropState(bool bDrop)
{
  if( m_pCodecContext )
  {
    // i don't know exactly how high this should be set
    // couldn't find any good docs on it. think it varies
    // from codec to codec on what it does

    //  2 seem to be to high.. it causes video to be ruined on following images
    if( bDrop )
    {
      m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      m_pCodecContext->skip_loop_filter = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_loop_filter = AVDISCARD_DEFAULT;
    }
  }
}

unsigned int CDVDVideoCodecFFmpeg::SetFilters(unsigned int flags)
{
  m_filters_next.Empty();

  if(m_pHardware)
    return 0;

  if(flags & FILTER_ROTATE)
  {
    switch(m_iOrientation)
    {
      case 90:
        m_filters_next += "transpose=1";
        break;
      case 180:
        m_filters_next += "vflip,hflip";
        break;
      case 270:  
        m_filters_next += "transpose=2";
        break;
      default:
        break;
      }
  }

  if(flags & FILTER_DEINTERLACE_YADIF)
  {
    if(flags & FILTER_DEINTERLACE_HALFED)
      m_filters_next = "yadif=0:-1";
    else
      m_filters_next = "yadif=1:-1";

    if(flags & FILTER_DEINTERLACE_FLAGGED)
      m_filters_next += ":1";

    flags &= ~FILTER_DEINTERLACE_ANY | FILTER_DEINTERLACE_YADIF;
  }

  return flags;
}

union pts_union
{
  double  pts_d;
  int64_t pts_i;
};

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

int CDVDVideoCodecFFmpeg::Decode(uint8_t* pData, int iSize, double dts, double pts)
{
  int iGotPicture = 0, len = 0;

  if (!m_pCodecContext)
    return VC_ERROR;

  if(pData)
    m_iLastKeyframe++;

  shared_ptr<CSingleLock> lock;
  if(m_pHardware)
  {
    CCriticalSection* section = m_pHardware->Section();
    if(section)
      lock = shared_ptr<CSingleLock>(new CSingleLock(*section));

    int result;
    if(pData)
      result = m_pHardware->Check(m_pCodecContext);
    else
      result = m_pHardware->Decode(m_pCodecContext, NULL);

    if(result)
      return result;
  }

  if(m_pFilterGraph)
  {
    int result = 0;
    if(pData == NULL)
      result = FilterProcess(NULL);
    if(result)
      return result;
  }

  m_dts = dts;
  m_pCodecContext->reordered_opaque = pts_dtoi(pts);

  AVPacket avpkt;
  m_dllAvCodec.av_init_packet(&avpkt);
  avpkt.data = pData;
  avpkt.size = iSize;
  /* We lie, but this flag is only used by pngdec.c.
   * Setting it correctly would allow CorePNG decoding. */
  avpkt.flags = AV_PKT_FLAG_KEY;
  len = m_dllAvCodec.avcodec_decode_video2(m_pCodecContext, m_pFrame, &iGotPicture, &avpkt);

  if(m_iLastKeyframe < m_pCodecContext->has_b_frames + 2)
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;

  if (len < 0)
  {
    CLog::Log(LOGERROR, "%s - avcodec_decode_video returned failure", __FUNCTION__);
    return VC_ERROR;
  }

  if (!iGotPicture)
    return VC_BUFFER;

  if(m_pFrame->key_frame)
  {
    m_started = true;
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;
  }

  /* put a limit on convergence count to avoid huge mem usage on streams without keyframes */
  if(m_iLastKeyframe > 300)
    m_iLastKeyframe = 300;

  /* h264 doesn't always have keyframes + won't output before first keyframe anyway */
  if(m_pCodecContext->codec_id == AV_CODEC_ID_H264
  || m_pCodecContext->codec_id == AV_CODEC_ID_SVQ3)
    m_started = true;

  if(m_pHardware == NULL)
  {
    bool need_scale = std::find( m_formats.begin()
                               , m_formats.end()
                               , m_pCodecContext->pix_fmt) == m_formats.end();

    bool need_reopen  = false;
    if(!m_filters.Equals(m_filters_next))
      need_reopen = true;

    if(m_pFilterIn)
    {
      if(m_pFilterIn->outputs[0]->format != m_pCodecContext->pix_fmt
      || m_pFilterIn->outputs[0]->w      != m_pCodecContext->width
      || m_pFilterIn->outputs[0]->h      != m_pCodecContext->height)
        need_reopen = true;
    }

    // try to setup new filters
    if (need_reopen || (need_scale && m_pFilterGraph == NULL))
    {
      m_filters = m_filters_next;

      if(FilterOpen(m_filters, need_scale) < 0)
        FilterClose();
    }
  }

  int result;
  if(m_pHardware)
    result = m_pHardware->Decode(m_pCodecContext, m_pFrame);
  else if(m_pFilterGraph)
    result = FilterProcess(m_pFrame);
  else
    result = VC_PICTURE | VC_BUFFER;

  if(result & VC_FLUSHED)
    Reset();

  return result;
}

void CDVDVideoCodecFFmpeg::Reset()
{
  m_started = false;
  m_iLastKeyframe = m_pCodecContext->has_b_frames;
  m_dllAvCodec.avcodec_flush_buffers(m_pCodecContext);

  if (m_pHardware)
    m_pHardware->Reset();

  m_filters = "";
  FilterClose();
}

bool CDVDVideoCodecFFmpeg::GetPictureCommon(DVDVideoPicture* pDvdVideoPicture)
{
  pDvdVideoPicture->iWidth = m_pFrame->width;
  pDvdVideoPicture->iHeight = m_pFrame->height;

#if !defined(LIBAVFILTER_AVFRAME_BASED)
  if(m_pBufferRef)
  {
    pDvdVideoPicture->iWidth  = m_pBufferRef->video->w;
    pDvdVideoPicture->iHeight = m_pBufferRef->video->h;
  }
#endif

  /* crop of 10 pixels if demuxer asked it */
  if(m_pCodecContext->coded_width  && m_pCodecContext->coded_width  < (int)pDvdVideoPicture->iWidth
                                   && m_pCodecContext->coded_width  > (int)pDvdVideoPicture->iWidth  - 10)
    pDvdVideoPicture->iWidth = m_pCodecContext->coded_width;

  if(m_pCodecContext->coded_height && m_pCodecContext->coded_height < (int)pDvdVideoPicture->iHeight
                                   && m_pCodecContext->coded_height > (int)pDvdVideoPicture->iHeight - 10)
    pDvdVideoPicture->iHeight = m_pCodecContext->coded_height;

  double aspect_ratio;

  /* use variable in the frame */
  AVRational pixel_aspect = m_pFrame->sample_aspect_ratio;
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  if (m_pBufferRef)
    pixel_aspect = m_pBufferRef->video->sample_aspect_ratio;
#endif

  if (pixel_aspect.num == 0)
    aspect_ratio = 0;
  else
    aspect_ratio = av_q2d(pixel_aspect) * pDvdVideoPicture->iWidth / pDvdVideoPicture->iHeight;

  if (aspect_ratio <= 0.0)
    aspect_ratio = (float)pDvdVideoPicture->iWidth / (float)pDvdVideoPicture->iHeight;

  /* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
  pDvdVideoPicture->iDisplayHeight = pDvdVideoPicture->iHeight;
  pDvdVideoPicture->iDisplayWidth  = ((int)RINT(pDvdVideoPicture->iHeight * aspect_ratio)) & -3;
  if (pDvdVideoPicture->iDisplayWidth > pDvdVideoPicture->iWidth)
  {
    pDvdVideoPicture->iDisplayWidth  = pDvdVideoPicture->iWidth;
    pDvdVideoPicture->iDisplayHeight = ((int)RINT(pDvdVideoPicture->iWidth / aspect_ratio)) & -3;
  }


  pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if (!m_pFrame)
    return false;

  AVDictionaryEntry * entry = m_dllAvUtil.av_dict_get(m_dllAvCodec.av_frame_get_metadata(m_pFrame), "stereo_mode", NULL, 0);
  if(entry && entry->value)
  {
    strncpy(pDvdVideoPicture->stereo_mode, (const char*)entry->value, sizeof(pDvdVideoPicture->stereo_mode));
    pDvdVideoPicture->stereo_mode[sizeof(pDvdVideoPicture->stereo_mode)-1] = '\0';
  }

  pDvdVideoPicture->iRepeatPicture = 0.5 * m_pFrame->repeat_pict;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  pDvdVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pDvdVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST: 0;

  pDvdVideoPicture->chroma_position = m_pCodecContext->chroma_sample_location;
  pDvdVideoPicture->color_primaries = m_pCodecContext->color_primaries;
  pDvdVideoPicture->color_transfer = m_pCodecContext->color_trc;
  if(m_pCodecContext->color_range == AVCOL_RANGE_JPEG
  || m_pCodecContext->pix_fmt     == PIX_FMT_YUVJ420P)
    pDvdVideoPicture->color_range = 1;
  else
    pDvdVideoPicture->color_range = 0;

  pDvdVideoPicture->qscale_table = m_pFrame->qscale_table;
  pDvdVideoPicture->qscale_stride = m_pFrame->qstride;

  switch (m_pFrame->qscale_type) {
  case FF_QSCALE_TYPE_MPEG1:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG1;
    break;
  case FF_QSCALE_TYPE_MPEG2:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_MPEG2;
    break;
  case FF_QSCALE_TYPE_H264:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_H264;
    break;
  default:
    pDvdVideoPicture->qscale_type = DVP_QSCALE_UNKNOWN;
  }

  pDvdVideoPicture->dts = m_dts;
  m_dts = DVD_NOPTS_VALUE;
  if (m_pFrame->reordered_opaque)
    pDvdVideoPicture->pts = pts_itod(m_pFrame->reordered_opaque);
  else
    pDvdVideoPicture->pts = DVD_NOPTS_VALUE;

  if(!m_started)
    pDvdVideoPicture->iFlags |= DVP_FLAG_DROPPED;

  return true;
}

bool CDVDVideoCodecFFmpeg::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if(m_pHardware)
    return m_pHardware->GetPicture(m_pCodecContext, m_pFrame, pDvdVideoPicture);

  if(!GetPictureCommon(pDvdVideoPicture))
    return false;

  {
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->data[i]      = m_pFrame->data[i];
    for (int i = 0; i < 4; i++)
      pDvdVideoPicture->iLineSize[i] = m_pFrame->linesize[i];
  }

  pDvdVideoPicture->iFlags |= pDvdVideoPicture->data[0] ? 0 : DVP_FLAG_DROPPED;
  pDvdVideoPicture->extended_format = 0;

  PixelFormat pix_fmt;
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  if(m_pBufferRef)
    pix_fmt = (PixelFormat)m_pBufferRef->format;
  else
#endif
    pix_fmt = (PixelFormat)m_pFrame->format;

  pDvdVideoPicture->format = CDVDCodecUtils::EFormatFromPixfmt(pix_fmt);
  return true;
}

int CDVDVideoCodecFFmpeg::FilterOpen(const CStdString& filters, bool scale)
{
  int result;
  AVBufferSinkParams *buffersink_params;

  if (m_pFilterGraph)
    FilterClose();

  if (filters.IsEmpty() && !scale)
    return 0;

  if (m_pHardware)
  {
    CLog::Log(LOGWARNING, "CDVDVideoCodecFFmpeg::FilterOpen - skipped opening filters on hardware decode");
    return 0;
  }

  if (!(m_pFilterGraph = m_dllAvFilter.avfilter_graph_alloc()))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - unable to alloc filter graph");
    return -1;
  }

  AVFilter* srcFilter = m_dllAvFilter.avfilter_get_by_name("buffer");
  AVFilter* outFilter = m_dllAvFilter.avfilter_get_by_name("buffersink"); // should be last filter in the graph for now

  CStdString args;

  args.Format("%d:%d:%d:%d:%d:%d:%d",
    m_pCodecContext->width,
    m_pCodecContext->height,
    m_pCodecContext->pix_fmt,
    m_pCodecContext->time_base.num,
    m_pCodecContext->time_base.den,
    m_pCodecContext->sample_aspect_ratio.num,
    m_pCodecContext->sample_aspect_ratio.den);

  if ((result = m_dllAvFilter.avfilter_graph_create_filter(&m_pFilterIn, srcFilter, "src", args, NULL, m_pFilterGraph)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: src");
    return result;
  }

  buffersink_params = m_dllAvFilter.av_buffersink_params_alloc();
  buffersink_params->pixel_fmts = &m_formats[0];
#ifdef FF_API_OLD_VSINK_API
  if ((result = m_dllAvFilter.avfilter_graph_create_filter(&m_pFilterOut, outFilter, "out", NULL, (void*)buffersink_params->pixel_fmts, m_pFilterGraph)) < 0)
#else
  if ((result = m_dllAvFilter.avfilter_graph_create_filter(&m_pFilterOut, outFilter, "out", NULL, buffersink_params, m_pFilterGraph)) < 0)
#endif
  {
    m_dllAvUtil.av_freep(&buffersink_params);
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_create_filter: out");
    return result;
  }
  m_dllAvUtil.av_freep(&buffersink_params);

  if (!filters.empty())
  {
    AVFilterInOut* outputs = m_dllAvFilter.avfilter_inout_alloc();
    AVFilterInOut* inputs  = m_dllAvFilter.avfilter_inout_alloc();

    outputs->name    = m_dllAvUtil.av_strdup("in");
    outputs->filter_ctx = m_pFilterIn;
    outputs->pad_idx = 0;
    outputs->next    = NULL;

    inputs->name    = m_dllAvUtil.av_strdup("out");
    inputs->filter_ctx = m_pFilterOut;
    inputs->pad_idx = 0;
    inputs->next    = NULL;

#if defined(HAVE_AVFILTER_GRAPH_PARSE_PTR)
    if ((result = m_dllAvFilter.avfilter_graph_parse_ptr(m_pFilterGraph, (const char*)m_filters.c_str(), &inputs, &outputs, NULL)) < 0)
#else
    if ((result = m_dllAvFilter.avfilter_graph_parse(m_pFilterGraph, (const char*)m_filters.c_str(), &inputs, &outputs, NULL)) < 0)
#endif
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_parse");
      return result;
    }

    m_dllAvFilter.avfilter_inout_free(&outputs);
    m_dllAvFilter.avfilter_inout_free(&inputs);
  }
  else
  {
    if ((result = m_dllAvFilter.avfilter_link(m_pFilterIn, 0, m_pFilterOut, 0)) < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_link");
      return result;
    }
  }

  if ((result = m_dllAvFilter.avfilter_graph_config(m_pFilterGraph, NULL)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterOpen - avfilter_graph_config");
    return result;
  }

  return result;
}

void CDVDVideoCodecFFmpeg::FilterClose()
{
#if !defined(LIBAVFILTER_AVFRAME_BASED)
  if(m_pBufferRef)
  {
    m_dllAvFilter.avfilter_unref_buffer(m_pBufferRef);
    m_pBufferRef = NULL;
  }
#endif

  if (m_pFilterGraph)
  {
    m_dllAvFilter.avfilter_graph_free(&m_pFilterGraph);

    // Disposed by above code
    m_pFilterIn   = NULL;
    m_pFilterOut  = NULL;
  }
}

int CDVDVideoCodecFFmpeg::FilterProcess(AVFrame* frame)
{
  int result;

  if (frame)
  {
#if defined(LIBAVFILTER_AVFRAME_BASED)
    // API changed in:
    // ffmpeg: commit 7e350379f87e7f74420b4813170fe808e2313911 (28 Nov 2012)
    //         not released (post 1.2)
    // libav: commit 7e350379f87e7f74420b4813170fe808e2313911 (28 Nov 2012)
    //        release v9 (5 January 2013)
    result = m_dllAvFilter.av_buffersrc_add_frame(m_pFilterIn, frame);
#else
    // API changed in:
    // ffmpeg: commit 7bac2a78c2241df4bcc1665703bb71afd9a3e692 (28 Apr 2012)
    //         release 0.11 (25 May 2012)
    result = m_dllAvFilter.av_buffersrc_add_frame(m_pFilterIn, frame, 0);
#endif
    if (result < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_buffersrc_add_frame");
      return VC_ERROR;
    }
  }

#if defined(LIBAVFILTER_AVFRAME_BASED)
  result = m_dllAvFilter.av_buffersink_get_frame(m_pFilterOut, m_pFilterFrame);

  if(result  == AVERROR(EAGAIN) || result == AVERROR_EOF)
    return VC_BUFFER;
  else if(result < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_buffersink_get_frame");
    return VC_ERROR;
  }

  m_dllAvUtil.av_frame_unref(m_pFrame);
  m_dllAvUtil.av_frame_move_ref(m_pFrame, m_pFilterFrame);

  return VC_PICTURE;
#else
  int frames;

  if(m_pBufferRef)
  {
    m_dllAvFilter.avfilter_unref_buffer(m_pBufferRef);
    m_pBufferRef = NULL;
  }

  if ((frames = m_dllAvFilter.av_buffersink_poll_frame(m_pFilterOut)) < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - av_buffersink_poll_frame");
    return VC_ERROR;
  }

  if (frames > 0)
  {

    result = m_dllAvFilter.av_buffersink_get_buffer_ref(m_pFilterOut, &m_pBufferRef, 0);
    if(!m_pBufferRef || result < 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecFFmpeg::FilterProcess - cur_buf");
      return VC_ERROR;
    }

    if(frame == NULL)
      m_pFrame->reordered_opaque = 0;
    else
      m_pFrame->repeat_pict      = -(frames - 1);

    m_pFrame->interlaced_frame = m_pBufferRef->video->interlaced;
    m_pFrame->top_field_first  = m_pBufferRef->video->top_field_first;

    memcpy(m_pFrame->linesize, m_pBufferRef->linesize, 4*sizeof(int));
    memcpy(m_pFrame->data    , m_pBufferRef->data    , 4*sizeof(uint8_t*));

    if(frames > 1)
      return VC_PICTURE;
    else
      return VC_PICTURE | VC_BUFFER;
  }

  return VC_BUFFER;
#endif
}

unsigned CDVDVideoCodecFFmpeg::GetConvergeCount()
{
  if(m_pHardware)
    return m_iLastKeyframe;
  else
    return 0;
}

unsigned CDVDVideoCodecFFmpeg::GetAllowedReferences()
{
  if(m_pHardware)
    return m_pHardware->GetAllowedReferences();
  else
    return 0;
}
