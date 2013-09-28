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

#include "threads/SystemClock.h"
#include "CacheStrategy.h"
#ifdef TARGET_POSIX
#include "PlatformInclude.h"
#endif
#include "Util.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "SpecialProtocol.h"
#ifdef TARGET_WINDOWS
#include "PlatformDefs.h" //for PRIdS, PRId64
#endif

using namespace XFILE;

CCacheStrategy::CCacheStrategy() : m_bEndOfInput(false)
{
}


CCacheStrategy::~CCacheStrategy()
{
}

void CCacheStrategy::EndOfInput() {
  m_bEndOfInput = true;
}

bool CCacheStrategy::IsEndOfInput()
{
  return m_bEndOfInput;
}

void CCacheStrategy::ClearEndOfInput()
{
  m_bEndOfInput = false;
}

CSimpleFileCache::CSimpleFileCache()
  : m_hCacheFileRead(NULL)
  , m_hCacheFileWrite(NULL)
  , m_hDataAvailEvent(NULL)
  , m_nStartPosition(0)
  , m_nWritePosition(0)
  , m_nReadPosition(0) {
}

CSimpleFileCache::~CSimpleFileCache()
{
  Close();
}

int CSimpleFileCache::Open()
{
  Close();

  m_hDataAvailEvent = new CEvent;

  CStdString fileName = CSpecialProtocol::TranslatePath(CUtil::GetNextFilename("special://temp/filecache%03d.cache", 999));
  if(fileName.empty())
  {
    CLog::Log(LOGERROR, "%s - Unable to generate a new filename", __FUNCTION__);
    Close();
    return CACHE_RC_ERROR;
  }

  m_hCacheFileWrite = CreateFile(fileName.c_str()
            , GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE
            , NULL
            , CREATE_ALWAYS
            , FILE_ATTRIBUTE_NORMAL
            , NULL);

  if(m_hCacheFileWrite == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGERROR, "%s - failed to create file %s with error code %d", __FUNCTION__, fileName.c_str(), GetLastError());
    Close();
    return CACHE_RC_ERROR;
  }

  m_hCacheFileRead = CreateFile(fileName.c_str()
            , GENERIC_READ, FILE_SHARE_WRITE
            , NULL
            , OPEN_EXISTING
            , FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE
            , NULL);

  if(m_hCacheFileRead == INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGERROR, "%s - failed to open file %s with error code %d", __FUNCTION__, fileName.c_str(), GetLastError());
    Close();
    return CACHE_RC_ERROR;
  }

  return CACHE_RC_OK;
}

void CSimpleFileCache::Close()
{
  if (m_hDataAvailEvent)
    delete m_hDataAvailEvent;

  m_hDataAvailEvent = NULL;

  if (m_hCacheFileWrite)
    CloseHandle(m_hCacheFileWrite);

  m_hCacheFileWrite = NULL;

  if (m_hCacheFileRead)
    CloseHandle(m_hCacheFileRead);

  m_hCacheFileRead = NULL;
}

int CSimpleFileCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  DWORD iWritten=0;
  if (!WriteFile(m_hCacheFileWrite, pBuffer, iSize, &iWritten, NULL))
  {
    CLog::Log(LOGERROR, "%s - failed to write to file. err: %u",
                          __FUNCTION__, GetLastError());
    return CACHE_RC_ERROR;
  }

  m_nWritePosition += iWritten;

  // when reader waits for data it will wait on the event.
  m_hDataAvailEvent->Set();

  return iWritten;
}

int64_t CSimpleFileCache::GetAvailableRead()
{
  return m_nWritePosition - m_nReadPosition;
}

int CSimpleFileCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  int64_t iAvailable = GetAvailableRead();
  if ( iAvailable <= 0 ) {
    return m_bEndOfInput? 0 : CACHE_RC_WOULD_BLOCK;
  }

  if (iMaxSize > (size_t)iAvailable)
    iMaxSize = (size_t)iAvailable;

  DWORD iRead = 0;
  if (!ReadFile(m_hCacheFileRead, pBuffer, iMaxSize, &iRead, NULL)) {
    CLog::Log(LOGERROR,"CSimpleFileCache::ReadFromCache - failed to read %"PRIdS" bytes.", iMaxSize);
    return CACHE_RC_ERROR;
  }
  m_nReadPosition += iRead;

  if (iRead > 0)
    m_space.Set();

  return iRead;
}

int64_t CSimpleFileCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  if( iMillis == 0 || IsEndOfInput() )
    return GetAvailableRead();

  XbmcThreads::EndTime endTime(iMillis);
  while (!IsEndOfInput())
  {
    int64_t iAvail = GetAvailableRead();
    if (iAvail >= iMinAvail)
      return iAvail;

    if (!m_hDataAvailEvent->WaitMSec(endTime.MillisLeft()))
      return CACHE_RC_TIMEOUT;
  }
  return GetAvailableRead();
}

int64_t CSimpleFileCache::Seek(int64_t iFilePosition)
{
  int64_t iTarget = iFilePosition - m_nStartPosition;

  if (iTarget < 0)
  {
    CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek, request seek before start of cache.");
    return CACHE_RC_ERROR;
  }

  int64_t nDiff = iTarget - m_nWritePosition;
  if (nDiff > 500000 || (nDiff > 0 && WaitForData((unsigned int)(iTarget - m_nReadPosition), 5000) == CACHE_RC_TIMEOUT))
  {
    CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek - Attempt to seek past read data");
    return CACHE_RC_ERROR;
  }

  LARGE_INTEGER pos;
  pos.QuadPart = iTarget;

  if(!SetFilePointerEx(m_hCacheFileRead, pos, NULL, FILE_BEGIN))
    return CACHE_RC_ERROR;

  m_nReadPosition = iTarget;
  m_space.Set();

  return iFilePosition;
}

void CSimpleFileCache::Reset(int64_t iSourcePosition, bool clearAnyway)
{
  LARGE_INTEGER pos;
  if (!clearAnyway && IsCachedPosition(iSourcePosition))
  {
    pos.QuadPart = m_nReadPosition = iSourcePosition - m_nStartPosition;
    SetFilePointerEx(m_hCacheFileRead, pos, NULL, FILE_BEGIN);
    return;
  }

  pos.QuadPart = 0;

  SetFilePointerEx(m_hCacheFileWrite, pos, NULL, FILE_BEGIN);
  SetFilePointerEx(m_hCacheFileRead, pos, NULL, FILE_BEGIN);
  m_nStartPosition = iSourcePosition;
  m_nReadPosition = 0;
  m_nWritePosition = 0;
}

void CSimpleFileCache::EndOfInput()
{
  CCacheStrategy::EndOfInput();
  m_hDataAvailEvent->Set();
}

int64_t CSimpleFileCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  if (iFilePosition >= m_nStartPosition && iFilePosition <= m_nStartPosition + m_nWritePosition)
    return m_nStartPosition + m_nWritePosition;
  return iFilePosition;
}

int64_t CSimpleFileCache::CachedDataEndPos()
{
  return m_nStartPosition + m_nWritePosition;
}

bool CSimpleFileCache::IsCachedPosition(int64_t iFilePosition)
{
  return iFilePosition >= m_nStartPosition && iFilePosition <= m_nStartPosition + m_nWritePosition;
}

CCacheStrategy *CSimpleFileCache::CreateNew()
{
  return new CSimpleFileCache();
}


CSimpleDoubleCache::CSimpleDoubleCache(CCacheStrategy *impl)
{
  assert(NULL != impl);
  m_pCache = impl;
  m_pCacheOld = NULL;
}

CSimpleDoubleCache::~CSimpleDoubleCache()
{
  delete m_pCache;
  delete m_pCacheOld;
}

int CSimpleDoubleCache::Open()
{
  return m_pCache->Open();
}

void CSimpleDoubleCache::Close()
{
  m_pCache->Close();
  if (m_pCacheOld)
  {
    delete m_pCacheOld;
    m_pCacheOld = NULL;
  }
}

int CSimpleDoubleCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  return m_pCache->WriteToCache(pBuffer, iSize);
}

int CSimpleDoubleCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  return m_pCache->ReadFromCache(pBuffer, iMaxSize);
}

int64_t CSimpleDoubleCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  return m_pCache->WaitForData(iMinAvail, iMillis);
}

int64_t CSimpleDoubleCache::Seek(int64_t iFilePosition)
{
  return m_pCache->Seek(iFilePosition);
}

void CSimpleDoubleCache::Reset(int64_t iSourcePosition, bool clearAnyway)
{
  if (!clearAnyway && m_pCache->IsCachedPosition(iSourcePosition)
      && (!m_pCacheOld || !m_pCacheOld->IsCachedPosition(iSourcePosition)
          || m_pCache->CachedDataEndPos() >= m_pCacheOld->CachedDataEndPos()))
  {
    m_pCache->Reset(iSourcePosition, clearAnyway);
    return;
  }
  if (!m_pCacheOld)
  {
    CCacheStrategy *pCacheNew = m_pCache->CreateNew();
    if (pCacheNew->Open() != CACHE_RC_OK)
    {
      delete pCacheNew;
      m_pCache->Reset(iSourcePosition, clearAnyway);
      return;
    }
    pCacheNew->Reset(iSourcePosition, clearAnyway);
    m_pCacheOld = m_pCache;
    m_pCache = pCacheNew;
    return;
  }
  m_pCacheOld->Reset(iSourcePosition, clearAnyway);
  CCacheStrategy *tmp = m_pCacheOld;
  m_pCacheOld = m_pCache;
  m_pCache = tmp;
}

void CSimpleDoubleCache::EndOfInput()
{
  m_pCache->EndOfInput();
}

bool CSimpleDoubleCache::IsEndOfInput()
{
  return m_pCache->IsEndOfInput();
}

void CSimpleDoubleCache::ClearEndOfInput()
{
  m_pCache->ClearEndOfInput();
}

int64_t CSimpleDoubleCache::CachedDataEndPos()
{
  return m_pCache->CachedDataEndPos();
}

int64_t CSimpleDoubleCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  int64_t ret = m_pCache->CachedDataEndPosIfSeekTo(iFilePosition);
  if (m_pCacheOld)
    return std::max(ret, m_pCacheOld->CachedDataEndPosIfSeekTo(iFilePosition));
  return ret;
}

bool CSimpleDoubleCache::IsCachedPosition(int64_t iFilePosition)
{
  return m_pCache->IsCachedPosition(iFilePosition) || (m_pCacheOld && m_pCacheOld->IsCachedPosition(iFilePosition));
}

CCacheStrategy *CSimpleDoubleCache::CreateNew()
{
  return new CSimpleDoubleCache(m_pCache->CreateNew());
}

