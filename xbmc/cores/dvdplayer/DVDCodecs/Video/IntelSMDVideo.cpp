/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "system.h"

#if defined(HAS_INTEL_SMD)

#include "settings/AdvancedSettings.h"
#include "IntelSMDVideo.h"
#include "../../../IntelSMDGlobals.h"
#include "DVDClock.h"
#include "threads/Atomics.h"
#include "threads/Thread.h"
#include "utils/log.h"
#include "utils/fastmemcpy.h"
#include "Application.h"

#define __MODULE_NAME__ "IntelSMDVideo"

#if 0
#define VERBOSE() CLog::Log(LOGDEBUG, "%s::%s", __MODULE_NAME__, __FUNCTION__)
#else
#define VERBOSE()
#endif

void mp_msg(int mod, int lev, const char *format, ... ){}
#define MSGT_DECVIDEO 0
#define MSGL_ERR 0
#define MSGL_V 0
#define MSGL_DBG2 0


#define USE_FFMPEG_ANNEXB

CIntelSMDVideo* CIntelSMDVideo::m_pInstance = NULL;

CIntelSMDVideo::CIntelSMDVideo()
: m_buffer(NULL)
{
  VERBOSE();
  SetDefaults();
}


CIntelSMDVideo::~CIntelSMDVideo()
{
  VERBOSE();
}

void CIntelSMDVideo::SetDefaults()
{
  VERBOSE();
  m_IsConfigured = false;
  m_width = 0;
  m_height = 0;
  m_dwidth = 0;
  m_dheight = 0;
  m_codec_type = ISMD_CODEC_TYPE_INVALID;
  m_bNeedVC1Conversion = m_bNeedWMV3Conversion = m_bNeedH264Conversion = false;
  memset(&m_vc1_converter, 0, sizeof(m_vc1_converter));
  memset(&m_H264_converter, 0, sizeof(m_H264_converter));
  m_bFlushFlag = true;
}

void CIntelSMDVideo::RemoveInstance(void)
{
  VERBOSE();
  if (m_pInstance)
  {
    delete m_pInstance;
    m_pInstance = NULL;
  }
}

CIntelSMDVideo* CIntelSMDVideo::GetInstance(void)
{
  VERBOSE();
  if (!m_pInstance)
  {
    m_pInstance = new CIntelSMDVideo();
  }
  return m_pInstance;
}

void CIntelSMDVideo::vc1_viddec_init (Vc1Viddec *viddec)
{
  VERBOSE();

  viddec->vCodec = ISMD_CODEC_TYPE_VC1;
  memset ( &viddec->SPMP_PESpacket_PayloadFormatHeader[0], 0, MAX_SIZE_PES_SEQUENCE_HEADER );
  viddec->size_SPMP_PESpacket_PayloadFormatHeader = 0;
  viddec->flag = 1;
  viddec->AP_Buffer = NULL;

  viddec->size_AP_Buffer = 0;

  // Initialize the End Of Sequence of SPMP
  viddec->SPMP_EndOfSequence = new unsigned char[4];
  if ( viddec->SPMP_EndOfSequence )
  {
    unsigned char* pData = viddec->SPMP_EndOfSequence;
    pData[0] = pData[1] = 0;
    pData[2] = 0x1;
    pData[3] = 0x0A;
  }
}

void h264_viddec_init(H264Viddec *viddec)
{
  VERBOSE();
  viddec->vCodec = ISMD_CODEC_TYPE_H264;
  viddec->first = true;
  viddec->h264_codec_priv_pushed_data_ptr = NULL;
  viddec->h264_priv_len = 0;
  viddec->sizeOfNALULength = 0;
  viddec->avi_sps_pps = NULL;
  viddec->sps_pps_len = 0;
}

bool h264_viddec_parse_codec_priv_data(H264Viddec *viddec, unsigned char *buf, int size)
{
  VERBOSE();
  unsigned int offset = 0;
  unsigned char *pdata = NULL;
  unsigned char sync_header[4];
  sync_header[0] = 0x00;
  sync_header[1] = 0x00;
  sync_header[2] = 0x00;
  sync_header[3] = 0x01;

  //printf("h264_viddec_parse_codec_priv_data extra size %d\n", size);

  if (buf != NULL)
  {
    pdata = (unsigned char*) malloc(size);// temp buffer
    viddec->h264_codec_priv_pushed_data_ptr = (unsigned char*) malloc(size);
    if (!pdata || !viddec->h264_codec_priv_pushed_data_ptr)
    {
      //printf("Failed to alloc memory!\n");
      goto end;
    }

    memcpy(pdata, buf, size);
    memset(viddec->h264_codec_priv_pushed_data_ptr, 0x00, size);

    viddec->sizeOfNALULength = (*(pdata + 4) & 0x03) + 1;
    //printf("Size of NALU length =%d\n", viddec->sizeOfNALULength);

    //read num of Sequence Parameter Sets
    unsigned char numOfSequenceParameterSets = *(pdata + 5) & 0x1F;
    //printf("  - Num Seq Parm Sets =%d\n", numOfSequenceParameterSets);

    if (!numOfSequenceParameterSets)
    {
      //printf("Num of Sequece Parms end\n");
      goto end;
    }
    offset = 6;
    unsigned int i;
    unsigned char *hptr = (unsigned char*) viddec->h264_codec_priv_pushed_data_ptr;

    memcpy(hptr, sync_header, 4);
    viddec->h264_priv_len = 4;

    for (i = 0; i < numOfSequenceParameterSets; i++)
    {
      unsigned short len = *(pdata + offset++);
      len = (len << 8) | (*(pdata + offset++));

      if(pdata + 2 + len > ((uint8_t *)pdata + size))
      {
        //printf("pdata + 2 + len > ((uint8_t *)pdata + size\n");
        goto end;
      }

      if (len != 0)
      {
        memcpy(hptr + viddec->h264_priv_len, (pdata + offset), len);
        viddec->h264_priv_len += len;
        offset += len;
      }
    }

    //read num pf Picture Parameter Sets
    unsigned char numOfPictureParameterSets = *(pdata + offset++);
    //printf("  - Num Pictures =%d\n", numOfPictureParameterSets);

    if (!numOfSequenceParameterSets)
    {
      //printf("Num of Sequece Parms end\n");
      goto end;
    }

    memcpy(hptr + viddec->h264_priv_len, sync_header, 4);
    viddec->h264_priv_len += 4;
    for (i = 0; i < numOfPictureParameterSets; i++)
    {
      unsigned short len = *(pdata + offset++);
      len = (len << 8) | (*(pdata + offset++));
      if (len)
      {
        //printf("  - Picture Parameter Set len: %d\n", len);
        memcpy(hptr + viddec->h264_priv_len, pdata + offset, len);
        viddec->h264_priv_len += len;
        offset += len;
      }
    }

    if (pdata != NULL)
      free(pdata);

    return TRUE;
  }

  end: if (pdata != NULL)
    free(pdata);

  if (viddec->h264_codec_priv_pushed_data_ptr != NULL)
  {
    free(viddec->h264_codec_priv_pushed_data_ptr);
    viddec->h264_codec_priv_pushed_data_ptr = NULL;
  }
  return FALSE;
}


#define NUMBYTES_START_CODE 4

/*
\brief Search H264 SPS/PPS in a buffer, and copy it to a new buffer if needed.

\param data_buffer H264 data that may contains SPS/PPS
\param sps_pps Place to store SPS/PPS data. If set to NULL, this function only checks SPS/PPS.

\retval TRUE This buffer has SPS/PPS
\retval FALSE This buffer does not have SPS/PPS
*/
bool ismd_gst_h264_viddec_search_sps_pps(unsigned char *data_buffer,
    unsigned int size, unsigned char **sps_pps, unsigned int *sps_pps_size)
{
  unsigned char *sps = NULL, *pps = NULL;
  int sps_len = 0, pps_len = 0;
  bool in_sps = FALSE, in_pps = FALSE;
  unsigned char *data = NULL;
  int len = 0;
  if (NULL == data_buffer)
  {
    return FALSE;
  }
  data = data_buffer;
  len = size;
  while (len > 0)
  {
    if (!in_sps && !in_pps && sps != NULL && pps != NULL)
    {
      break;
    }
    if (len >= 4 && data[0] == 0 && data[1] == 0 && data[2] == 1)
    {//start code
      in_sps = FALSE;
      in_pps = FALSE;
      switch (data[3] & 0x1F)
      {
      case 7: //SPS
        sps = data;
        sps_len = 0;
        in_sps = TRUE;
        break;
      case 8: //PPS
        pps = data;
        pps_len = 0;
        in_pps = TRUE;
        break;
      default:
        break;
      }
    }
    if (in_sps)
    {
      ++sps_len;
    }
    if (in_pps)
    {
      ++pps_len;
    }
    ++data;
    --len;
  }
  mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "SPS = %d/%d, PPS = %d/%d\n", sps ? sps
      - data_buffer : -1, sps_len, pps ? pps - data_buffer : -1, pps_len);
  if (sps != NULL && pps != NULL)
  {
    if (sps_pps != NULL)
    {
      unsigned char *cp;
      *sps_pps = (unsigned char* )malloc(sps_len + pps_len);
      cp = *sps_pps;
      memcpy(cp, sps, sps_len);
      memcpy(cp + sps_len, pps, pps_len);
      if (sps_pps_size != NULL)
        *sps_pps_size = sps_len + pps_len;
    }
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

unsigned int find_num_nalus(int NALUByteSize, unsigned char *buf, int size)
{

  unsigned char *data;
  data = buf;
  unsigned int sizeOfBuffer = size;
  unsigned int offset = 0;
  unsigned char numNALUs = 0;

  while (sizeOfBuffer > offset)
  {
    unsigned int NALULen = 0;
    switch (NALUByteSize)
    {
    case 1:
      NALULen = data[offset++];
      break;

    case 2:
      NALULen = data[offset++];
      NALULen = (NALULen << 8) | data[offset++];
      break;

    case 3:
      NALULen = data[offset++]; // bits 20 - 14
      NALULen = (NALULen << 8) | data[offset++]; // bits 13 -  8
      NALULen = (NALULen << 8) | data[offset++]; // bits  7 -  0
      break;

    case 4:
      NALULen = data[offset++]; // bits 31 - 21
      NALULen = (NALULen << 8) | data[offset++]; // bits 20 - 14
      NALULen = (NALULen << 8) | data[offset++]; // bits 13 -  8
      NALULen = (NALULen << 8) | data[offset++]; // bits  7 -  0
      break;

    default:
      break;
    }
    offset += NALULen;
    numNALUs++;
  }

  mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "Total Number of NALUs found: %d\n",
      numNALUs);
  return numNALUs;
}

unsigned char *H264_viddec_convert(H264Viddec *viddec, unsigned char *buf,
    int size, bool need_seq_hdr, unsigned int *outbuf_size)
{
  unsigned int buf_offset = 0;
  unsigned char *outbuf = NULL;

  if (outbuf_size == NULL)
    return FALSE;
  else
    *outbuf_size = size;
  // If this is the first data buffer, we need to parse the codec_data and
  // inject it into the first data buffer for the sake of the H.264 video
  // decoder.
  if (viddec->first || need_seq_hdr)
  {

    if (buf != NULL && viddec->first)
    {
      mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "---> *** FIRST BUFFER size: %d\n", size);
      //            h264_viddec_parse_codec_priv_data(viddec, codec_data, codec_data_size);
    }
    if (viddec->h264_codec_priv_pushed_data_ptr && viddec->h264_priv_len)
    {
      mp_msg(MSGT_DECVIDEO, MSGL_V, "---> *** NAL Num Bytes: %d\n",
          viddec->sizeOfNALULength);
      if (viddec->sizeOfNALULength < 3)
      {
        unsigned int numNALUs = 0;
        //
        // We need to allocate a larger buffer in case the NALU length is 1 or 2 bytes
        // because there is not enough bytes in the H.264 stream to overwrite the
        // NALU size with the start code.  The size of this buffer will be:
        //
        // h264 private header length (SPS/PPS info) + original buffer data +
        // num of NAL units in GST buffer * 4-byte start code -
        // num of NAL units in GST buffer * orig num bytes used to carry size info
        //
        numNALUs = find_num_nalus(viddec->sizeOfNALULength, buf, size);
        *outbuf_size = viddec->h264_priv_len + size + numNALUs
            * NUMBYTES_START_CODE - numNALUs * viddec->sizeOfNALULength;
        outbuf = (unsigned char *)malloc(*outbuf_size);
      }
      else
      {
        *outbuf_size = viddec->h264_priv_len + size;
        outbuf = (unsigned char *)malloc(*outbuf_size);
      }

      if (outbuf == NULL)
      {
        mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to create Buffer\n");
        return FALSE;
      }
      else
      {
        mp_msg(MSGT_DECVIDEO, MSGL_V,
            "First Buffer: %d bytes header + %d bytes data\n",
            viddec->h264_priv_len, size);
      }

      memcpy(outbuf, viddec->h264_codec_priv_pushed_data_ptr,
          viddec->h264_priv_len);

      viddec->pvt_data_avail = TRUE;
      viddec->calculated_offset = TRUE;
    }
    else
    {
      //AVI+H264: sometimes only one SPS/PPS is available at the beginning of ES stream,
      //save it for seek operation.
      if (viddec->avi_sps_pps == NULL)
      {
        //No SPS/PPS stored, need to find and cache one.
        if (!ismd_gst_h264_viddec_search_sps_pps(buf, size,
            &viddec->avi_sps_pps, &viddec->sps_pps_len))
        {
          mp_msg(MSGT_DECVIDEO, MSGL_DBG2,
              "AVI: SPS/PPS not found, probably this is an error in video stream.\n");
        }
      }
      else
      {
        //Already have a SPS/PPS, check if there is SPS/PPS in buf
        if (!ismd_gst_h264_viddec_search_sps_pps(buf, size, NULL, NULL))
        {
          mp_msg(MSGT_DECVIDEO, MSGL_DBG2,
              "AVI: No SPS/PPS, use cached buf size is %d.\n",
              viddec->sps_pps_len);
          buf_offset = viddec->sps_pps_len;
          *outbuf_size = size + viddec->sps_pps_len;
          outbuf = (unsigned char *)malloc(*outbuf_size);
          if (outbuf == NULL)
          {
            mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to create Buffer\n");
            return FALSE;
          }
          memcpy(outbuf, viddec->avi_sps_pps, viddec->sps_pps_len);
          memcpy(outbuf + buf_offset, buf, size);
        }
      }

      //In this case we will send it as it is
      //In cases where the header is FOURCC etc,
      //we get proper H264 packets with these details and caps structure will
      //no contain these details. So sending it as it is works fine.
      viddec->pvt_data_avail = FALSE;
      mp_msg(MSGT_DECVIDEO, MSGL_V, "..... NO h264_private data\n");
    }
  }
  if (viddec->pvt_data_avail && viddec->sizeOfNALULength < 3 && !(viddec->first
      || need_seq_hdr))
  {
    unsigned int numNALUs = 0;
    numNALUs = find_num_nalus(viddec->sizeOfNALULength, buf, size);
    *outbuf_size = size + numNALUs * NUMBYTES_START_CODE - numNALUs
        * viddec->sizeOfNALULength;
    outbuf = (unsigned char *)malloc(*outbuf_size);
    if (outbuf == NULL)
    {
      mp_msg(MSGT_DECVIDEO, MSGL_ERR, "Failed to create Buffer\n");
      return FALSE;
    }
  }

  // Inject a start code sequence in each frame.  For NALU length of 3 or 4 bytes, we
  // can just overwrite the first 3 or 4 bytes respectively as the H.264 spec allows
  // 0x 00 00 01 or 0x 00 00 00 01 as valid start codes.  For NALU length = 1 or 2 bytes,
  // we can't overwrite as there are not enough bytes in stream needed for a valid
  // start code (valid start code must be as least 3 bytes: 0x 00 00 01).  For the
  // latter cases, we need to do out-of-place data transformation.
  if (viddec->pvt_data_avail == TRUE)
  {
    unsigned char *data;
    unsigned char filler[4] = { 0x00, 0x00, 0x00, 0x01 };
    data = buf;
    unsigned int sizeOfBuffer = size;
    unsigned int offset = 0;
    unsigned int totalSize = 0;
    mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "---> NALU unit -> sizeOfBuffer=%d\n",
        sizeOfBuffer);

    while (sizeOfBuffer > offset)
    {
      unsigned int NALULen = 0;
      switch (viddec->sizeOfNALULength)
      {
      case 1:
        // For 1 and 2 byte NALU Length payloads, OUT-OF-PLACE data transformation
        // (start-code injection) is required as there are not enough bytes
        // in the original stream to overlay the start code sequence.  We re-write
        // everything into an newly allocated outbuf buffer.
        NALULen = data[offset++];
        mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "NALU len: %d, case %d\n", NALULen,
            viddec->sizeOfNALULength);

        if (viddec->first || need_seq_hdr)
        {
          memcpy(outbuf + viddec->h264_priv_len + buf_offset, filler, 4);
        }
        else
        {
          memcpy(outbuf + buf_offset, filler, 4);
        }
        buf_offset += 4;
        if (viddec->first || need_seq_hdr)
        {
          memcpy(outbuf + viddec->h264_priv_len + buf_offset, &data[offset],
              NALULen);
        }
        else
        {
          memcpy(outbuf + buf_offset, &data[offset], NALULen);
        }
        buf_offset += NALULen;
        totalSize += 1;
        break;

      case 2:
        NALULen = data[offset++];
        NALULen = (NALULen << 8) | data[offset++];
        mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "NALU len: %d, case %d\n", NALULen,
            viddec->sizeOfNALULength);

        if (viddec->first || need_seq_hdr)
        {
          memcpy(outbuf + viddec->h264_priv_len + buf_offset, filler, 4);
        }
        else
        {
          memcpy(outbuf + buf_offset, filler, 4);
        }
        buf_offset += 4;
        if (viddec->first || need_seq_hdr)
        {
          memcpy(outbuf + viddec->h264_priv_len + buf_offset, &data[offset],
              NALULen);
        }
        else
        {
          memcpy(outbuf + buf_offset, &data[offset], NALULen);
        }
        buf_offset += NALULen;
        totalSize += 2;

        break;

      case 3:
        NALULen = data[offset++]; // bits 20 - 14
        NALULen = (NALULen << 8) | data[offset++]; // bits 13 -  8
        NALULen = (NALULen << 8) | data[offset++]; // bits  7 -  0

        // For 3-byte NALU Length payloads, IN-PLACE start code
        // injection can be done as we can conveniently use the
        // 3 bytes that contained the NALU length information to put .
        // the start code sequence.
        data[offset - 3] = 0x00;
        data[offset - 2] = 0x00;
        data[offset - 1] = 0x01;
        totalSize += 3;
        break;

      case 4:
        NALULen = data[offset++]; // bits 31 - 21
        NALULen = (NALULen << 8) | data[offset++]; // bits 20 - 14
        NALULen = (NALULen << 8) | data[offset++]; // bits 13 -  8
        NALULen = (NALULen << 8) | data[offset++]; // bits  7 -  0

        // For 4-byte NALU Length payloads, IN-PLACE start code
        // injection can be done as we can conveniently use the
        // 4 bytes that contained the NALU length information to put .
        // the start code sequence.
        data[offset - 4] = 0x00;
        data[offset - 3] = 0x00;
        data[offset - 2] = 0x00;
        data[offset - 1] = 0x01;
        totalSize += 4;
        break;

      default:
        mp_msg(MSGT_DECVIDEO, MSGL_V, "---> Size not 1, 2, 3 or 4 (%d)!!!!\n",
            viddec->sizeOfNALULength);
        //offset = sizeOfBuffer +1;
        break;
      }
      offset += NALULen;
      totalSize += NALULen;
    }
    mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "---> Total Size: %d Diff: %d\n",
        totalSize, sizeOfBuffer - totalSize);

    // For first frame only of 3 or 4-byte NALU Len payloads:
    // Copy rest of data into buffer that has header info at front of buffer
    if ((viddec->first || need_seq_hdr) && (viddec->sizeOfNALULength == 3
        || viddec->sizeOfNALULength == 4))
    {
      unsigned char *dptr;
      dptr = outbuf;
      mp_msg(MSGT_DECVIDEO, MSGL_DBG2,
          "---> copying %d bytes at outbuf->data[%d] for data\n", size,
          viddec->h264_priv_len);
      memcpy(dptr + viddec->h264_priv_len, data, sizeOfBuffer);
    }
  }

  if (viddec->pvt_data_avail && (viddec->first || need_seq_hdr))
  {
    viddec->first = false;
    mp_msg(MSGT_DECVIDEO, MSGL_V, "---> SEQHDR sent!\n");
    return outbuf;
  }
  else
  {
    viddec->first = false;
    if (buf_offset)
    {
      // If buf_offset is non-zero, it means we did out-of-place data transformation
      // (1,2 byte NALU lens) so just send the new buffer with the start codes written in.
      mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "---> sending modified buf: %d bytes\n",
          buf_offset);
      return outbuf;
    }
    else
    {
      // If buf_offset is 0, it means we did in-place data transformation
      // (3, 4 byte NALU len) so just send the original buffer with the start
      // codes overlayed into the NALU len bytes.
      mp_msg(MSGT_DECVIDEO, MSGL_DBG2, "---> sending original buf: %d bytes\n",
          size);
      return buf;
    }
  }
}

void
CIntelSMDVideo::vc1_viddec_SPMP_PESpacket_PayloadFormatHeader (Vc1Viddec *viddec, unsigned char *pCodecData, int width, int height)
{
  VERBOSE();
  if ( viddec->size_SPMP_PESpacket_PayloadFormatHeader > 0 )
  {
    viddec->size_SPMP_PESpacket_PayloadFormatHeader = 0;
    memset ( viddec->SPMP_PESpacket_PayloadFormatHeader, 0, MAX_SIZE_PES_SEQUENCE_HEADER );
  }

  unsigned char pSeqHeaderRbdu[8];

  unsigned char* pSequenceHdr =  pCodecData ;
  // These are little-endian values in the RCV file (valid C DWORD's). 16-bit
  // values which go into the SeqHeaderRbdu are therefore extracted bytewise
  pSeqHeaderRbdu[ 0 ] = ( unsigned char ) ( ( width  >> 8 ) & 0xFF );
  pSeqHeaderRbdu[ 1 ] = ( unsigned char ) ( ( width  >> 0 ) & 0xFF );
  pSeqHeaderRbdu[ 2 ] = ( unsigned char ) ( ( height >> 8 ) & 0xFF );
  pSeqHeaderRbdu[ 3 ] = ( unsigned char ) ( ( height >> 0 ) & 0xFF );
  //
  // The SEQ header
  //
  memcpy ( &pSeqHeaderRbdu[ 4 ], pSequenceHdr, 4 );
  // start code for header: 0x0000010F
  // Annex E.5, SMPTE-421M-FDS1.doc
  viddec->SPMP_PESpacket_PayloadFormatHeader[0] = viddec->SPMP_PESpacket_PayloadFormatHeader[1] = 0;
  viddec->SPMP_PESpacket_PayloadFormatHeader[2] = 0x01;
  viddec->SPMP_PESpacket_PayloadFormatHeader[3] = 0x0F;

  unsigned char byte_written = vc1_viddec_encapsulate_and_write_ebdu (
      &viddec->SPMP_PESpacket_PayloadFormatHeader[4], 12, &pSeqHeaderRbdu[0], 8 );

  viddec->size_SPMP_PESpacket_PayloadFormatHeader = 4 + byte_written;

}

unsigned char *
CIntelSMDVideo::Vc1_viddec_convert_SPMP (Vc1Viddec *viddec, unsigned char *buf, int size,
    bool need_seq_hdr, unsigned int *outbuf_size)
{
  VERBOSE();

  unsigned char *outbuf = NULL;
  unsigned char* pCurrent; // points to next writing position of the new buffer
  int max_bufsize = 0;

  // Calculate maximum buffer size, make room for encapsulation
  // SMPTE-421M-FDSI doc, Annex-E
  max_bufsize = ((size + 3) / 3) * 4;

  if (viddec->flag || need_seq_hdr)
  {
    // Allocate extra buffer to include header
    max_bufsize += viddec->size_SPMP_PESpacket_PayloadFormatHeader;
  }

  // allocate new buffer
  outbuf = new unsigned char[max_bufsize];

  if (outbuf)
  {
    pCurrent = outbuf;
    *outbuf_size = 0;

    if (viddec->flag || need_seq_hdr)
    {
      // copy sequence header to buffer
      memcpy(pCurrent, viddec->SPMP_PESpacket_PayloadFormatHeader,
          viddec->size_SPMP_PESpacket_PayloadFormatHeader);
      // update buffer data size
      *outbuf_size += viddec->size_SPMP_PESpacket_PayloadFormatHeader;
      // update pointer
      pCurrent += viddec->size_SPMP_PESpacket_PayloadFormatHeader;
      viddec->flag = 0;
    }
    // time to convert the data

    // Start Code Prefix for frame
    pCurrent[0] = pCurrent[1] = 0;
    pCurrent[2] = 0x01;
    pCurrent[3] = 0x0D;
    pCurrent += 4;
    *outbuf_size += 4;

    int byte_written = vc1_viddec_encapsulate_and_write_ebdu(pCurrent,
        max_bufsize - *outbuf_size, buf, size);

    pCurrent += byte_written;
    *outbuf_size += byte_written;


  }

  return outbuf;
}


int CIntelSMDVideo::vc1_viddec_encapsulate_and_write_ebdu ( unsigned char* pDes, unsigned int SizeDes, unsigned char* pRbdu, unsigned int SizeRBDU )
{
  VERBOSE();
  int    ZeroRun = 0;
  unsigned int i;
  unsigned int j = 0;

  const unsigned char Escape  = 0x03;
  const unsigned char StopBit = 0x80;  // Byte with a stop bit in the MSB.


  // loop through source
  for ( i = 0; i < SizeRBDU; i++ ) {
    if ( ( ZeroRun == 2 ) && ( pRbdu[ i ] <= 0x03 ) ) {
      pDes[j++] = Escape;

      if( j >= SizeDes ) {
        j = 0;
        break;
      }
      ZeroRun = 0;
    }

    pDes[j++] = pRbdu[i];

    if( j >= SizeDes ) {
      j = 0;
      break;
    }

    if ( pRbdu[ i ] == 0x0 ) {
      ZeroRun++;
    } else {
      ZeroRun = 0;
    }

  }
  //
  // Stop bit should not be added for a NULL RBDU
  //
  if ( !SizeRBDU ) return j;
  //
  // Add a stop bit at the end of the BDU.
  //
  pDes[j++] = StopBit;
  return j;
}

unsigned char *
CIntelSMDVideo::Vc1_viddec_convert_AP (Vc1Viddec *viddec, unsigned char *buf,int size,
    bool need_seq_hdr,unsigned int *outbuf_size)
{
  VERBOSE();

  unsigned char *outbuf = NULL;
  unsigned char* pCurrent; // points to next writing position of the new buffer
  int max_bufsize = 0;

  // Allocate the output buffer with maximum possible size
  // Sequence Header + Frame Start Code + Frame Data
  max_bufsize = viddec->size_AP_Buffer + 4 + size;
  outbuf = new unsigned char[max_bufsize];
  if (outbuf == NULL)
  {
    return outbuf;
  }

  pCurrent = outbuf;
  *outbuf_size = 0;

  // Handle the Sequence Header
  if (viddec->flag || need_seq_hdr)
  {
    // First, do a Sequence Start Code detection
    if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01 && buf[3] == 0x0F)
    {

      // The buffer from demux already has Sequence Header
      // In this case, the buffer should has below layout
      // (Sequence Start Code + Sequence Header Data) + (Frame Start Code + Frame Data)
      // TODO: Is it possible that no Frame Start Code in this case?
      // TODO: Is it possible that no Frame Start Code in this case?

      viddec->flag = 0;

      // Do nothing, just go to COPY_DATA
      goto COPY_DATA;

    }
    else
    {


      // no Sequence Header detected, use AP_Buffer
      memcpy(pCurrent, viddec->AP_Buffer, viddec->size_AP_Buffer);
      // update buffer data size
      *outbuf_size += viddec->size_AP_Buffer;
      // update pointer
      pCurrent += viddec->size_AP_Buffer;

      viddec->flag = 0;
    }
  }
  else
  {
    // During the playback, Sequence Header will be send from demux occasionally in some contents
    if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01 && buf[3] == 0x0F)
    {

      // Do nothing, just go to COPY_DATA
      goto COPY_DATA;
    }
  }

  // Handle the Frame Start Code
  if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01 && buf[3] == 0x0D)
  {

    // the buffer from demux already has Frame Start Code, do nothing
    goto COPY_DATA;

  }
  else
  {

    // Insert the Frame Start Code
    pCurrent[0] = pCurrent[1] = 0;
    pCurrent[2] = 0x01;
    pCurrent[3] = 0x0D;

    pCurrent += 4;
    *outbuf_size += 4;

  }

  COPY_DATA:
  memcpy(pCurrent, buf, size);
  pCurrent += size;
  *outbuf_size += size;


  return outbuf;
}

void CIntelSMDVideo::Reset()
{
  VERBOSE();

  m_bFlushFlag = true;

  CSingleLock lock(m_bufferLock);
  if (m_buffer)
  {
    delete m_buffer;
    m_buffer = NULL;
  }

  g_IntelSMDGlobals.FlushVideoDecoder();
  g_IntelSMDGlobals.FlushVideoRender();
}

bool CIntelSMDVideo::OpenDecoder(CodecID ffmpegCodedId, ismd_codec_type_t codec_type, int extradata_size, void *extradata)
{
  VERBOSE();

  if (m_IsConfigured)
    CloseDecoder();

  Reset();

  m_codec_type = codec_type;

  if(!g_IntelSMDGlobals.CreateVideoDecoder(m_codec_type))
  {
    CLog::Log(LOGERROR, "CIntelSMDVideo::OpenDecoder CreateVideoDecoder %d failed.", m_codec_type);
    return false;
  }

  if(!g_IntelSMDGlobals.CreateVideoRender(GDL_VIDEO_PLANE))
  {
    CLog::Log(LOGERROR, "CIntelSMDVideo::OpenDecoder CreateVideoRender failed");
    return false;
  }

  if(!g_IntelSMDGlobals.ConnectDecoderToRenderer())
  {
    CLog::Log(LOGERROR, "CIntelSMDRenderer::Configure ConnectDecoderToRenderer failed");
    return false;
  }

  if(m_codec_type == ISMD_CODEC_TYPE_H264 && extradata_size > 0)
  {
    h264_viddec_init (&m_H264_converter);

    m_bNeedH264Conversion = h264_viddec_parse_codec_priv_data(&m_H264_converter,(unsigned char *)extradata, extradata_size);
    printf("h264_viddec_parse_codec_priv_data %d\n", m_bNeedH264Conversion);
  }

  if (ffmpegCodedId == CODEC_ID_WMV3 &&  extradata_size >= 4){
    CLog::Log(LOGERROR, "CIntelSMDVideo::OpenDecoder WMV3 stream annex e");
    vc1_viddec_init(&m_vc1_converter);
    vc1_viddec_SPMP_PESpacket_PayloadFormatHeader(&m_vc1_converter,(unsigned char*)extradata,m_width, m_height);
    m_bNeedWMV3Conversion = true;
  }

  if (ffmpegCodedId == CODEC_ID_VC1 && extradata_size > 0)
  {
    CLog::Log(LOGERROR, "CIntelSMDVideo::OpenDecoder VC1 stream annex e");
    printf("CIntelSMDVideo::OpenDecoder VC1 stream annex e\n");
    vc1_viddec_init(&m_vc1_converter);
    m_vc1_converter.size_AP_Buffer = extradata_size;
    //mp_msg(MSGT_DECVIDEO,MSGL_V,"sequence header size: 0X %x\n",Vc1_converter.size_AP_Buffer);
    m_vc1_converter.AP_Buffer = new unsigned char [m_vc1_converter.size_AP_Buffer];
    memcpy(m_vc1_converter.AP_Buffer,(unsigned char *)extradata, m_vc1_converter.size_AP_Buffer);
    m_bNeedVC1Conversion = true;
  }

  m_IsConfigured = true;

  g_IntelSMDGlobals.SetVideoDecoderState(ISMD_DEV_STATE_PAUSE);
  g_IntelSMDGlobals.SetVideoRenderState(ISMD_DEV_STATE_PAUSE);

  // we now wait for the first packet to start
  m_bFlushFlag = true;

  CLog::Log(LOGINFO, "%s: codec opened", __MODULE_NAME__);

  return true;
}

void CIntelSMDVideo::CloseDecoder(void)
{
  VERBOSE();

  Sleep(100);

  g_IntelSMDGlobals.DeleteVideoDecoder();
  g_IntelSMDGlobals.DeleteVideoRender();

  if (m_vc1_converter.AP_Buffer)
    delete m_vc1_converter.AP_Buffer;

  if (m_vc1_converter.SPMP_EndOfSequence)
    delete m_vc1_converter.SPMP_EndOfSequence;

  if(m_H264_converter.h264_codec_priv_pushed_data_ptr)
    free(m_H264_converter.h264_codec_priv_pushed_data_ptr);

  SetDefaults();

  CLog::Log(LOGDEBUG, "%s: codec closed", __MODULE_NAME__);
}

int CIntelSMDVideo::AddInput(unsigned char *pData, size_t size, double dts, double pts)
{
  VERBOSE();
  bool filtered = false;
  unsigned int demuxer_bytes = size;
  uint8_t *demuxer_content = pData;

  unsigned int outbuf_size = 0;
  uint8_t *outbuf = NULL;

  if (m_bNeedH264Conversion)
  {
    outbuf = H264_viddec_convert(&m_H264_converter, demuxer_content,
        demuxer_bytes, m_bFlushFlag, &outbuf_size);
    if (outbuf)
    {
      filtered = true;
      demuxer_content = outbuf;
      demuxer_bytes = outbuf_size;
    }
  }
  else if (m_bNeedWMV3Conversion)
  {
    outbuf = Vc1_viddec_convert_SPMP(&m_vc1_converter, demuxer_content,
        demuxer_bytes, m_bFlushFlag, &outbuf_size);
    if (outbuf)
    {
      filtered = true;
      demuxer_content = outbuf;
      demuxer_bytes = outbuf_size;
    }
  }
  else if (m_bNeedVC1Conversion)
  {
    outbuf = Vc1_viddec_convert_AP(&m_vc1_converter, demuxer_content,
        demuxer_bytes, m_bFlushFlag, &outbuf_size);
    if (outbuf)
    {
      filtered = true;
      demuxer_content = outbuf;
      demuxer_bytes = outbuf_size;
    }
  }

  //printf("PTS = %.2f\n", demuxer_pts / DVD_TIME_BASE);

  CSingleLock lock(m_bufferLock);
  ismd_result_t ismd_ret = ISMD_SUCCESS;
  ismd_buffer_descriptor_t buffer_desc;
  ismd_es_buf_attr_t *buf_attrs;

  unsigned char *ptr;

  if (pts == DVD_NOPTS_VALUE)
  {
    if (dts != DVD_NOPTS_VALUE)
      pts = dts;
    else
      pts = 0;
  }


  if (!m_buffer)
  {
    m_buffer = new CISMDBuffer;
    m_buffer->firstpts = DVD_NOPTS_VALUE;
  }
  if (m_bFlushFlag)
    m_buffer->m_bFlush = m_bFlushFlag;

  if (m_buffer->firstpts == DVD_NOPTS_VALUE && pts != DVD_NOPTS_VALUE)
    m_buffer->firstpts = pts;

  m_buffer->pts = pts;
  m_buffer->dts = dts;

  outbuf = demuxer_content;
  outbuf_size = demuxer_bytes;
  while (outbuf_size > 0)
  {

    //recommend using 32 kB buffers as these are already allocated in the memory map,
    //break data into smaller pieces if over 32 kB
    const int bufSize = 32*1024;
    unsigned int copy = bufSize;

    ismd_buffer_handle_t buffer_handle;
    ismd_ret = ismd_buffer_alloc(bufSize, &buffer_handle);
    if (ismd_ret != ISMD_SUCCESS)
    {
      printf("CIntelSMDVideo::WriteToInputPort ismd_buffer_alloc failed <%d>, %d\n", ismd_ret, bufSize);
      goto cleanup;
    }

    ismd_ret = ismd_buffer_read_desc(buffer_handle, &buffer_desc);
    if (ismd_ret != ISMD_SUCCESS)
    {
      printf("CIntelSMDVideo::WriteToInputPort ismd_buffer_read_desc failed <%d>\n", ismd_ret);
      goto cleanup;
    }

    if (copy > outbuf_size)
      copy = outbuf_size;

    ptr = (unsigned char*) OS_MAP_IO_TO_MEM_NOCACHE(buffer_desc.phys.base, buffer_desc.phys.size);
    OS_MEMCPY(ptr, outbuf, copy);
    OS_UNMAP_IO_FROM_MEM(ptr, buffer_desc.phys.size);

    buffer_desc.phys.level = copy;

    buf_attrs = (ismd_es_buf_attr_t *) buffer_desc.attributes;

    buf_attrs->original_pts = g_IntelSMDGlobals.DvdToIsmdPts(pts);
    buf_attrs->local_pts = g_IntelSMDGlobals.DvdToIsmdPts(pts);


    ismd_ret = ismd_buffer_update_desc(buffer_handle, &buffer_desc);
    if (ismd_ret != ISMD_SUCCESS)
    {
      printf("-- ismd_buffer_update_desc failed <%d>\n", ismd_ret);
      goto cleanup;
    }
    m_buffer->m_buffers.push(buffer_handle);
  cleanup:
    if (ismd_ret != ISMD_SUCCESS)
    {
      ismd_buffer_dereference(buffer_handle);
      break;
    }
    outbuf_size -= copy;
    outbuf += copy;
  }


  if (filtered)
  {
    if (m_bNeedH264Conversion)
    {
      if (demuxer_content != pData)
        free(demuxer_content);
    }
    else if (demuxer_content != NULL)
      free(demuxer_content);
  }


  unsigned int curDepth;
  unsigned int maxDepth;
  g_IntelSMDGlobals.GetPortStatus(g_IntelSMDGlobals.GetVidDecInput(), curDepth, maxDepth);
  unsigned int queueLen =  curDepth + m_buffer->m_buffers.size();

  const unsigned int maxQueue = ISMD_VIDEO_BUFFER_QUEUE;
  ismd_vidrend_stream_position_info_t position;
  ismd_vidrend_get_stream_position(g_IntelSMDGlobals.GetVidRender(), &position);

  if (
     !m_bFlushFlag &&
      queueLen < maxQueue &&
      position.segment_time != ISMD_NO_PTS && // Just keep emitting frames until we figure out where we are
      ismd_ret == ISMD_SUCCESS)
  {
    // Report that the frame is "dropped" just to queue up frames a bit
    return VC_BUFFER;
  }
  if (m_bFlushFlag)
  {
    m_bFlushFlag = false;
  }

  if (!m_buffer->m_buffers.empty())
  {
    return VC_BUFFER|VC_PICTURE;
  }
  else
  {
    return VC_ERROR;
  }
}

bool CIntelSMDVideo::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  VERBOSE();
  CSingleLock lock(m_bufferLock);
  if (m_buffer == NULL || m_buffer->m_buffers.empty())
    return false;

  pDvdVideoPicture->dts = m_buffer->dts;
  pDvdVideoPicture->pts = m_buffer->pts;

  pDvdVideoPicture->data[0] = NULL;
  pDvdVideoPicture->iLineSize[0] = 0;
  pDvdVideoPicture->data[1] = NULL;
  pDvdVideoPicture->iLineSize[1] = 0;
  pDvdVideoPicture->data[2] = NULL;
  pDvdVideoPicture->iLineSize[2] = 0;

  pDvdVideoPicture->iRepeatPicture = 0;
  pDvdVideoPicture->iDuration = 0;
  pDvdVideoPicture->color_range = 0;
  pDvdVideoPicture->color_matrix = 0;
  pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;
  //pDvdVideoPicture->iFlags = 0;
  pDvdVideoPicture->format = RENDER_FMT_ISMD;
  pDvdVideoPicture->ismdbuf = m_buffer;

  m_buffer = NULL;

  ismd_viddec_stream_properties_t prop;

  ismd_result_t res = ismd_viddec_get_stream_properties(g_IntelSMDGlobals.GetVidDec(), &prop);
  if(res == ISMD_SUCCESS)
  {
//        printf("cw %d ch %d dw %d dh %d asp %d x %d\n",
//            prop.coded_width, prop.coded_height, prop.display_width, prop.display_height,
//            prop.sample_aspect_ratio.numerator, prop.sample_aspect_ratio.denominator );
    if( prop.coded_width && prop.coded_height )
    {
      pDvdVideoPicture->iWidth = prop.coded_width;
      pDvdVideoPicture->iHeight = prop.coded_height;
      pDvdVideoPicture->iDisplayWidth = prop.display_width;
      pDvdVideoPicture->iDisplayHeight = prop.display_height;
      pDvdVideoPicture->iFlags |= prop.is_stream_interlaced ? DVP_FLAG_INTERLACED : 0;

      if( prop.sample_aspect_ratio.numerator && prop.sample_aspect_ratio.denominator )
      {
        float neww = (1.0f * prop.coded_width * prop.sample_aspect_ratio.numerator) / prop.sample_aspect_ratio.denominator;
        pDvdVideoPicture->iDisplayWidth = ((unsigned int)neww) & ~3;
      }

      // update our stored state with whatever the decoder gave us
      m_width = pDvdVideoPicture->iWidth;
      m_height = pDvdVideoPicture->iHeight;
      m_dwidth = pDvdVideoPicture->iDisplayWidth;
      m_dheight = pDvdVideoPicture->iDisplayHeight;
    }
    else
    {
      // use the stored values if we don't get data from SMD
      pDvdVideoPicture->iWidth = m_width;
      pDvdVideoPicture->iHeight = m_height;
      pDvdVideoPicture->iDisplayWidth = m_dwidth;
      pDvdVideoPicture->iDisplayHeight = m_dheight;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CIntelSMDVideo::GetPicture ismd_viddec_get_stream_properties failed %d\n", res);
    return false;
  }

  return true;
}

bool CIntelSMDVideo::ClearPicture(DVDVideoPicture *pDvdVideoPicture)
{
  if (pDvdVideoPicture->format != RENDER_FMT_ISMD)
    return false;
  delete pDvdVideoPicture->ismdbuf;
  return true;
}
#endif
