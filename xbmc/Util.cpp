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
#include "network/Network.h"
#include "threads/SystemClock.h"
#include "system.h"
#if defined(TARGET_DARWIN)
#include <sys/param.h>
#include <mach-o/dyld.h>
#endif

#if defined(TARGET_FREEBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#ifdef TARGET_POSIX
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#if defined(TARGET_ANDROID)
#include "android/bionic_supplement/bionic_supplement.h"
#endif
#include <stdlib.h>

#include "Application.h"
#include "Util.h"
#include "addons/Addon.h"
#include "filesystem/PVRDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/RSSDirectory.h"
#ifdef HAS_FILESYSTEM_RAR
#include "filesystem/RarManager.h"
#endif
#include "filesystem/MythDirectory.h"
#ifdef HAS_UPNP
#include "filesystem/UPnPDirectory.h"
#endif
#include "profiles/ProfilesManager.h"
#include "utils/RegExp.h"
#include "guilib/GraphicContext.h"
#include "guilib/TextureManager.h"
#include "utils/fstrcmp.h"
#include "storage/MediaManager.h"
#ifdef TARGET_WINDOWS
#include "utils/CharsetConverter.h"
#include <shlobj.h>
#include "WIN32Util.h"
#endif
#if defined(TARGET_DARWIN)
#include "osx/DarwinUtils.h"
#endif
#include "GUIUserMessages.h"
#include "filesystem/File.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#ifdef HAS_IRSERVERSUITE
  #include "input/windows/IRServerSuite.h"
#endif
#include "guilib/LocalizeStrings.h"
#include "utils/md5.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/Environment.h"

#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleStream.h"
#include "URL.h"
#ifdef HAVE_LIBCAP
  #include <sys/capability.h>
#endif

using namespace std;


#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const int64_t SECS_BETWEEN_EPOCHS = 11644473600LL;
static const int64_t SECS_TO_100NS = 10000000;

using namespace XFILE;
using namespace PLAYLIST;

#ifdef HAS_DX
static D3DGAMMARAMP oldramp, flashramp;
#elif defined(HAS_SDL_2D)
static uint16_t oldrampRed[256];
static uint16_t oldrampGreen[256];
static uint16_t oldrampBlue[256];
static uint16_t flashrampRed[256];
static uint16_t flashrampGreen[256];
static uint16_t flashrampBlue[256];
#endif

#if !defined(TARGET_WINDOWS)
unsigned int CUtil::s_randomSeed = time(NULL);
#endif

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  // use above to get the filename
  CStdString path(strFileNameAndPath);
  URIUtils::RemoveSlashAtEnd(path);
  CStdString strFilename = URIUtils::GetFileName(path);

  CURL url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();

#ifdef HAS_UPNP
  // UPNP
  if (url.GetProtocol() == "upnp")
    strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
#endif

  if (url.GetProtocol() == "rss")
  {
    CRSSDirectory dir;
    CFileItemList items;
    if(dir.GetDirectory(strFileNameAndPath, items) && !items.m_strTitle.IsEmpty())
      return items.m_strTitle;
  }

  // Shoutcast
  else if (url.GetProtocol() == "shout")
  {
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.GetProtocol() == "smb" && strFilename.IsEmpty())
  {
    if (url.GetHostName().IsEmpty())
    {
      strFilename = g_localizeStrings.Get(20171);
    }
    else
    {
      strFilename = url.GetHostName();
    }
  }
  // iTunes music share (DAAP)
  else if (url.GetProtocol() == "daap" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.GetProtocol() == "hdhomerun" && strFilename.IsEmpty())
    strFilename = "HDHomerun Devices";

  // Slingbox Devices
  else if (url.GetProtocol() == "sling")
    strFilename = "Slingbox";

  // ReplayTV Devices
  else if (url.GetProtocol() == "rtv")
    strFilename = "ReplayTV Devices";

  // HTS Tvheadend client
  else if (url.GetProtocol() == "htsp")
    strFilename = g_localizeStrings.Get(20256);

  // VDR Streamdev client
  else if (url.GetProtocol() == "vtp")
    strFilename = g_localizeStrings.Get(20257);
  
  // MythTV client
  else if (url.GetProtocol() == "myth")
    strFilename = g_localizeStrings.Get(20258);

  // SAP Streams
  else if (url.GetProtocol() == "sap" && strFilename.IsEmpty())
    strFilename = "SAP Streams";

  // Root file views
  else if (url.GetProtocol() == "sources")
    strFilename = g_localizeStrings.Get(744);

  // Music Playlists
  else if (path.Left(24).Equals("special://musicplaylists"))
    strFilename = g_localizeStrings.Get(136);

  // Video Playlists
  else if (path.Left(24).Equals("special://videoplaylists"))
    strFilename = g_localizeStrings.Get(136);

  else if (URIUtils::ProtocolHasParentInHostname(url.GetProtocol()) && strFilename.IsEmpty())
    strFilename = URIUtils::GetFileName(url.GetHostName());

  // now remove the extension if needed
  if (!CSettings::Get().GetBool("filelists.showextensions") && !bIsFolder)
  {
    URIUtils::RemoveExtension(strFilename);
    return strFilename;
  }
  
  // URLDecode since the original path may be an URL
  CURL::Decode(strFilename);
  return strFilename;
}

void CUtil::CleanString(const CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bRemoveExtension /* = false */, bool bCleanChars /* = true */)
{
  strTitleAndYear = strFileName;

  if (strFileName.Equals(".."))
   return;

  const CStdStringArray &regexps = g_advancedSettings.m_videoCleanStringRegExps;

  CRegExp reTags(true);
  CRegExp reYear;

  if (!reYear.RegComp(g_advancedSettings.m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, g_advancedSettings.m_videoCleanDateTimeRegExp.c_str());
  }
  else
  {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetReplaceString("\\1");
      strYear = reYear.GetReplaceString("\\2");
    }
  }

  URIUtils::RemoveExtension(strTitleAndYear);

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid string clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    int j=0;
    if ((j=reTags.RegFind(strTitleAndYear.c_str())) > 0)
      strTitleAndYear = strTitleAndYear.Mid(0, j);
  }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  if (bCleanChars)
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.Find(' ') >= 0);

    for (int i = 0; i < (int)strTitleAndYear.size(); i++)
    {
      char c = strTitleAndYear.GetAt(i);

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear.SetAt(i, ' ');
      }
    }
  }

  strTitle = strTitleAndYear.Trim();

  // append year
  if (!strYear.IsEmpty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!bRemoveExtension)
    strTitleAndYear += URIUtils::GetExtension(strFileName);
}

void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  // Check if the filename is a fully qualified URL such as protocol://path/to/file
  CURL plItemUrl(strFilename);
  if (!plItemUrl.GetProtocol().IsEmpty())
    return;

  // If the filename starts "x:", "\\" or "/" it's already fully qualified so return
  if (strFilename.size() > 1)
#ifdef TARGET_POSIX
    if ( (strFilename[1] == ':') || (strFilename[0] == '/') )
#else
    if ( strFilename[1] == ':' || (strFilename[0] == '\\' && strFilename[1] == '\\'))
#endif
      return;

  // add to base path and then clean
  strFilename = URIUtils::AddFileToFolder(strBasePath, strFilename);

  // get rid of any /./ or \.\ that happen to be there
  strFilename.Replace("\\.\\", "\\");
  strFilename.Replace("/./", "/");

  // now find any "\\..\\" and remove them via GetParentPath
  int pos;
  while ((pos = strFilename.Find("/../")) > 0)
  {
    CStdString basePath = strFilename.Left(pos+1);
    strFilename = strFilename.Mid(pos+4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
  while ((pos = strFilename.Find("\\..\\")) > 0)
  {
    CStdString basePath = strFilename.Left(pos+1);
    strFilename = strFilename.Mid(pos+4);
    basePath = URIUtils::GetParentPath(basePath);
    strFilename = URIUtils::AddFileToFolder(basePath, strFilename);
  }
}

#ifdef UNIT_TESTING
bool CUtil::TestGetQualifiedFilename()
{
  CStdString file = "../foo"; GetQualifiedFilename("smb://", file);
  if (file != "foo") return false;
  file = "C:\\foo\\bar"; GetQualifiedFilename("smb://", file);
  if (file != "C:\\foo\\bar") return false;
  file = "../foo/./bar"; GetQualifiedFilename("smb://my/path", file);
  if (file != "smb://my/foo/bar") return false;
  file = "smb://foo/bar/"; GetQualifiedFilename("upnp://", file);
  if (file != "smb://foo/bar/") return false;
  return true;
}

bool CUtil::TestMakeLegalPath()
{
  CStdString path;
#ifdef TARGET_WINDOWS
  path = "C:\\foo\\bar"; path = MakeLegalPath(path);
  if (path != "C:\\foo\\bar") return false;
  path = "C:\\foo:\\bar\\"; path = MakeLegalPath(path);
  if (path != "C:\\foo_\\bar\\") return false;
#elif
  path = "/foo/bar/"; path = MakeLegalPath(path);
  if (path != "/foo/bar/") return false;
  path = "/foo?/bar"; path = MakeLegalPath(path);
  if (path != "/foo_/bar") return false;
#endif
  path = "smb://foo/bar"; path = MakeLegalPath(path);
  if (path != "smb://foo/bar") return false;
  path = "smb://foo/bar?/"; path = MakeLegalPath(path);
  if (path != "smb://foo/bar_/") return false;
  return true;
}
#endif

void CUtil::RunShortcut(const char* szShortcutPath)
{
}

void CUtil::GetHomePath(CStdString& strPath, const CStdString& strTarget)
{
  strPath = CEnvironment::getenv(strTarget);

#ifdef TARGET_WINDOWS
  if (strPath.find("..") != std::string::npos)
  {
    //expand potential relative path to full path
    CStdStringW strPathW;
    g_charsetConverter.utf8ToW(strPath, strPathW, false);
    CWIN32Util::AddExtraLongPathPrefix(strPathW);
    const unsigned int bufSize = GetFullPathNameW(strPathW, 0, NULL, NULL);
    if (bufSize != 0)
    {
      wchar_t * buf = new wchar_t[bufSize];
      if (GetFullPathNameW(strPathW, bufSize, buf, NULL) <= bufSize-1)
      {
        std::wstring expandedPathW(buf);
        CWIN32Util::RemoveExtraLongPathPrefix(expandedPathW);
        g_charsetConverter.wToUTF8(expandedPathW, strPath);
      }

      delete [] buf;
    }
  }
#endif

  if (strPath.IsEmpty())
  {
    CStdString strHomePath = ResolveExecutablePath();
#if defined(TARGET_DARWIN)
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size =2*MAXPATHLEN;

    result = GetDarwinExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      #if defined(TARGET_DARWIN_IOS)
        strcat(given_path, "/XBMCData/XBMCHome/");
      #else
        // Assume local path inside application bundle.
        strcat(given_path, "../Resources/XBMC/");
      #endif

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
      {
        strPath = real_path;
        return;
      }
    }
#endif
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != string::npos)
      strPath = strHomePath.Left(last_sep);
    else
      strPath = strHomePath;
  }

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  /* Change strPath accordingly when target is XBMC_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  CStdString installPath = INSTALL_PATH;
  CStdString binInstallPath = BIN_INSTALL_PATH;
  if (!strTarget.compare("XBMC_HOME") && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    CStdString tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }
#endif
}

bool CUtil::IsPVR(const CStdString& strFile)
{
  return strFile.Left(4).Equals("pvr:");
}

bool CUtil::IsHTSP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("htsp:");
}

bool CUtil::IsLiveTV(const CStdString& strFile)
{
  if (strFile.Left(14).Equals("pvr://channels"))
    return true;

  if(URIUtils::IsTuxBox(strFile)
  || URIUtils::IsVTP(strFile)
  || URIUtils::IsHDHomeRun(strFile)
  || URIUtils::IsHTSP(strFile)
  || strFile.Left(4).Equals("sap:"))
    return true;

  if (URIUtils::IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool CUtil::IsTVRecording(const CStdString& strFile)
{
  return strFile.Left(15).Equals("pvr://recording");
}

bool CUtil::IsPicture(const CStdString& strFile)
{
  return URIUtils::HasExtension(strFile,
                  g_advancedSettings.m_pictureExtensions + "|.tbn|.dds");
}

bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps)
{
  if (strFileOrFolder.IsEmpty())
    return false;

  CRegExp regExExcludes(true);  // case insensitive regex

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strFileOrFolder) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strFileOrFolder.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!URIUtils::IsRemote(strURL)) return ;
  if (URIUtils::IsDVD(strURL)) return ;

  CURL url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = URIUtils::GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

CStdString CUtil::GetFileMD5(const CStdString& strPath)
{
  CFile file;
  CStdString result;
  if (file.Open(strPath))
  {
    XBMC::XBMC_MD5 md5;
    char temp[1024];
    int pos=0;
    int read=1;
    while (read > 0)
    {
      read = file.Read(temp,1024);
      pos += read;
      md5.append(temp,read);
    }
    md5.getDigest(result);
    file.Close();
  }

  return result;
}

bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = URIUtils::GetFileName(strFileName);
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  URIUtils::RemoveSlashAtEnd(strDescription);

  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !g_mediaManager.IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( URIUtils::IsDVD(strPath) )
  {
#ifdef HAS_DVD_DRIVE
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "DefaultXboxDVD.png";
      return ;
    }
#endif
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsISO9660(strPath) )
  {
#ifdef HAS_DVD_DRIVE
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "DefaultVCD.png";
      return ;
    }
#endif
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( URIUtils::IsCDDA(strPath) )
  {
    strIcon = "DefaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  CStdString searchPath = CProfilesManager::Get().GetDatabaseFolder();
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".tmp", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    XFILE::CFile::Delete(items[i]->GetPath());
  }
}

void CUtil::ClearSubtitles()
{
  //delete cached subs
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/",items);
  for( int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      if ( items[i]->GetPath().Find("subtitle") >= 0 || items[i]->GetPath().Find("vobsub_queue") >= 0 )
      {
        CLog::Log(LOGDEBUG, "%s - Deleting temporary subtitle %s", __FUNCTION__, items[i]->GetPath().c_str());
        CFile::Delete(items[i]->GetPath());
      }
    }
  }
}

void CUtil::ClearTempFonts()
{
  CStdString searchPath = "special://temp/fonts/";

  if (!CFile::Exists(searchPath))
    return;

  CFileItemList items;
  CDirectory::GetDirectory(searchPath, items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_BYPASS_CACHE);

  for (int i=0; i<items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CFile::Delete(items[i]->GetPath());
  }
}

static const char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", NULL};

int64_t CUtil::ToInt64(uint32_t high, uint32_t low)
{
  int64_t n;
  n = high;
  n <<= 32;
  n += low;
  return n;
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  CStdString searchPath = URIUtils::GetDirectory(fn_template);
  CStdString mask = URIUtils::GetExtension(fn_template);

  CStdString name;
  name.Format(fn_template.c_str(), 0);

  CFileItemList items;
  if (!CDirectory::GetDirectory(searchPath, items, mask, DIR_FLAG_NO_FILE_DIRS))
    return name;

  items.SetFastLookup(true);
  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(fn_template.c_str(), i);
    if (!items.Get(name))
      return name;
  }
  return "";
}

CStdString CUtil::GetNextPathname(const CStdString &path_template, int max)
{
  if (!path_template.Find("%04d"))
    return "";
  
  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(path_template.c_str(), i);
    if (!CFile::Exists(name))
      return name;
  }
  return "";
}

void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (int64_t)stat->st_size;

#ifndef TARGET_POSIX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef TARGET_POSIX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef TARGET_POSIX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::Stat64ToStat(struct stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef TARGET_POSIX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (time_t)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (time_t)(stat->st_ctime & 0xFFFFFFFF);
}

#ifdef TARGET_WINDOWS
void CUtil::Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef TARGET_POSIX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
#ifndef TARGET_POSIX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}
#endif

bool CUtil::CreateDirectoryEx(const CStdString& strPath)
{
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // we currently only allow HD and smb, nfs and afp paths
  if (!URIUtils::IsHD(strPath) && !URIUtils::IsSmb(strPath) && !URIUtils::IsNfs(strPath) && !URIUtils::IsAfp(strPath))
  {
    CLog::Log(LOGERROR,"%s called with an unsupported path: %s", __FUNCTION__, strPath.c_str());
    return false;
  }

  CStdStringArray dirs = URIUtils::SplitPath(strPath);
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
  {
    dir = URIUtils::AddFileToFolder(dir, *it);
    CDirectory::Create(dir);
  }

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath)) return false;
  return true;
}

CStdString CUtil::MakeLegalFileName(const CStdString &strFile, int LegalType)
{
  CStdString result = strFile;

  result.Replace('/', '_');
  result.Replace('\\', '_');
  result.Replace('?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    result.Replace(':', '_');
    result.Replace('*', '_');
    result.Replace('?', '_');
    result.Replace('\"', '_');
    result.Replace('<', '_');
    result.Replace('>', '_');
    result.Replace('|', '_');
    result.TrimRight(".");
    result.TrimRight(" ");
  }
  return result;
}

// legalize entire path
CStdString CUtil::MakeLegalPath(const CStdString &strPathAndFile, int LegalType)
{
  if (URIUtils::IsStack(strPathAndFile))
    return MakeLegalPath(CStackDirectory::GetFirstStackedFile(strPathAndFile));
  if (URIUtils::IsMultiPath(strPathAndFile))
    return MakeLegalPath(CMultiPathDirectory::GetFirstPath(strPathAndFile));
  if (!URIUtils::IsHD(strPathAndFile) && !URIUtils::IsSmb(strPathAndFile) && !URIUtils::IsNfs(strPathAndFile) && !URIUtils::IsAfp(strPathAndFile))
    return strPathAndFile; // we don't support writing anywhere except HD, SMB, NFS and AFP - no need to legalize path

  bool trailingSlash = URIUtils::HasSlashAtEnd(strPathAndFile);
  CStdStringArray dirs = URIUtils::SplitPath(strPathAndFile);
  // we just add first token to path and don't legalize it - possible values: 
  // "X:" (local win32), "" (local unix - empty string before '/') or
  // "protocol://domain"
  CStdString dir(dirs.front());
  URIUtils::AddSlashAtEnd(dir);
  for (CStdStringArray::iterator it = dirs.begin() + 1; it != dirs.end(); it ++)
    dir = URIUtils::AddFileToFolder(dir, MakeLegalFileName(*it, LegalType));
  if (trailingSlash) URIUtils::AddSlashAtEnd(dir);
  return dir;
}

CStdString CUtil::ValidatePath(const CStdString &path, bool bFixDoubleSlashes /* = false */)
{
  CStdString result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (URIUtils::IsURL(path) && 
     (path.Find('%') >= 0 ||
      path.Left(4).Equals("apk:") ||
      path.Left(4).Equals("zip:") ||
      path.Left(4).Equals("rar:") ||
      path.Left(6).Equals("stack:") ||
      path.Left(7).Equals("bluray:") ||
      path.Left(10).Equals("multipath:") ))
    return result;

  // check the path for incorrect slashes
#ifdef TARGET_WINDOWS
  if (URIUtils::IsDOSPath(path))
  {
    result.Replace('/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double back slashes (but ignore the \\ of unc-paths)
      for (int x = 1; x < result.GetLength() - 1; x++)
      {
        if (result[x] == '\\' && result[x+1] == '\\')
          result.Delete(x);
      }
    }
  }
  else if (path.Find("://") >= 0 || path.Find(":\\\\") >= 0)
#endif
  {
    result.Replace('\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    if (bFixDoubleSlashes)
    {
      // Fixup for double forward slashes(/) but don't touch the :// of URLs
      for (int x = 2; x < result.GetLength() - 1; x++)
      {
        if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
          result.Delete(x);
      }
    }
  }
  return result;
}

bool CUtil::IsUsingTTFSubtitles()
{
  return URIUtils::HasExtension(CSettings::Get().GetString("subtitles.font"), ".ttf");
}

#ifdef UNIT_TESTING
bool CUtil::TestSplitExec()
{
  CStdString function;
  vector<CStdString> params;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo")
    return false;
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\"")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\test\\foo\\")
    return false;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")", function, params);
  if (function != "ActivateWindow" || params.size() != 2 || params[0] != "Video" || params[1] != "C:\\\\test\\\\foo\\")
    return false;
  CUtil::SplitExecFunction("SetProperty(Foo,\"\")", function, params);
  if (function != "SetProperty" || params.size() != 2 || params[0] != "Foo" || params[1] != "")
   return false;
  CUtil::SplitExecFunction("SetProperty(foo,ba(\"ba black )\",sheep))", function, params);
  if (function != "SetProperty" || params.size() != 2 || params[0] != "foo" || params[1] != "ba(\"ba black )\",sheep)")
    return false;
  return true;
}
#endif

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &function, vector<CStdString> &parameters)
{
  CStdString paramString;

  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    paramString = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    function = execString.Left(iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  function.Trim();
  if( function.Left(5).Equals("xbmc.", false) )
    function.Delete(0, 5);

  SplitParams(paramString, parameters);
}

void CUtil::SplitParams(const CStdString &paramString, std::vector<CStdString> &parameters)
{
  bool inQuotes = false;
  bool lastEscaped = false; // only every second character can be escaped
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  CStdString parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
    lastEscaped = escaped;
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
      }
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signfies the end of this parameter
        if (whiteSpacePos)
          parameter = parameter.Left(whiteSpacePos);
        // trim off start and end quotes
        if (parameter.GetLength() > 1 && parameter[0] == '"' && parameter[parameter.GetLength() - 1] == '"')
          parameter = parameter.Mid(1,parameter.GetLength() - 2);
        else if (parameter.GetLength() > 3 && parameter[parameter.GetLength() - 1] == '"')
        {
          // check name="value" style param.
          int quotaPos = parameter.Find('"');
          if (quotaPos > 1 && quotaPos < parameter.GetLength() - 1 && parameter[quotaPos - 1] == '=')
          {
            parameter.Delete(parameter.GetLength() - 1);
            parameter.Delete(quotaPos);
          }
        }
        parameters.push_back(parameter);
        parameter.Empty();
        whiteSpacePos = 0;
        continue;
      }
    }
    if ((ch == '"' || ch == '\\') && escaped)
    { // escaped quote or backslash
      parameter[parameter.size()-1] = ch;
      continue;
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.IsEmpty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "%s(%s) - end of string while searching for ) or \"", __FUNCTION__, paramString.c_str());
  if (whiteSpacePos)
    parameter = parameter.Left(whiteSpacePos);
  // trim off start and end quotes
  if (parameter.GetLength() > 1 && parameter[0] == '"' && parameter[parameter.GetLength() - 1] == '"')
    parameter = parameter.Mid(1,parameter.GetLength() - 2);
  else if (parameter.GetLength() > 3 && parameter[parameter.GetLength() - 1] == '"')
  {
    // check name="value" style param.
    int quotaPos = parameter.Find('"');
    if (quotaPos > 1 && quotaPos < parameter.GetLength() - 1 && parameter[quotaPos - 1] == '=')
    {
      parameter.Delete(parameter.GetLength() - 1);
      parameter.Delete(quotaPos);
    }
  }
  if (!parameter.IsEmpty() || parameters.size())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.IsEmpty())
    return -1;

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURL checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();
  if (checkURL.GetProtocol() == "tuxbox")
    return 1;
  if (checkURL.GetProtocol() == "plugin")
    return 1;
  if (checkURL.GetProtocol() == "multipath")
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    CStdString strName = share.strName;

    // special cases for dvds
    if (URIUtils::IsOnDVD(share.strPath))
    {
      if (URIUtils::IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    if (strPath.Equals(strName))
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURL urlDest(strPath);
  urlDest.SetOptions("");
  CStdString strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!URIUtils::HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURL url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }

    // doesnt match a name, so try the source path
    vector<CStdString> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURL urlShare(vecPaths[j]);
      urlShare.SetOptions("");
      CStdString strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);
      if (!URIUtils::HasSlashAtEnd(strShare))
        strShare += "/";
      int iLenShare = strShare.size();

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource: no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialSource(const CStdString &strSpecial)
{
  if (!strSpecial.IsEmpty() && strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      return URIUtils::AddFileToFolder("special://home/", strSpecial.Mid(5));
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      return URIUtils::AddFileToFolder("special://subtitles/", strSpecial.Mid(10));
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      return URIUtils::AddFileToFolder("special://userdata/", strSpecial.Mid(9));
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      return URIUtils::AddFileToFolder("special://database/", strSpecial.Mid(9));
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      return URIUtils::AddFileToFolder("special://thumbnails/", strSpecial.Mid(11));
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      return URIUtils::AddFileToFolder("special://recordings/", strSpecial.Mid(11));
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      return URIUtils::AddFileToFolder("special://screenshots/", strSpecial.Mid(12));
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
      return URIUtils::AddFileToFolder("special://musicplaylists/", strSpecial.Mid(15));
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
      return URIUtils::AddFileToFolder("special://videoplaylists/", strSpecial.Mid(15));
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      return URIUtils::AddFileToFolder("special://cdrips/", strSpecial.Mid(7));
    // this one will be removed post 2.0
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      return URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), strSpecial.Mid(10));
  }
  return strSpecial;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<CStdString> vec;
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "music"));
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<CStdString> vec;
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "video"));
  vec.push_back(URIUtils::AddFileToFolder(CSettings::Get().GetString("system.playlistspath"), "mixed"));
  return XFILE::CMultiPathDirectory::ConstructMultiPath(vec);
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb-");
  CUtil::DeleteDirectoryCache("sp-"); // overkill as it will delete video smartplaylists, but as we can't differentiate based on URL...
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb-");
  CUtil::DeleteDirectoryCache("sp-"); // overkill as it will delete music smartplaylists, but as we can't differentiate based on URL...
}

void CUtil::DeleteDirectoryCache(const CStdString &prefix)
{
  CStdString searchPath = "special://temp/";
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(searchPath, items, ".fi", DIR_FLAG_NO_FILE_DIRS))
    return;

  for (int i = 0; i < items.Size(); ++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    CStdString fileName = URIUtils::GetFileName(items[i]->GetPath());
    if (fileName.Left(prefix.GetLength()) == prefix)
      XFILE::CFile::Delete(items[i]->GetPath());
  }
}


void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories)
{
  CFileItemList myItems;
  int flags = DIR_FLAG_DEFAULTS;
  if (!bUseFileDirectories)
    flags |= DIR_FLAG_NO_FILE_DIRS;
  CDirectory::GetDirectory(strPath,myItems,strMask,flags);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->GetPath(),items,strMask,bUseFileDirectories);
    else
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",DIR_FLAG_NO_FILE_DIRS);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->GetPath().Equals(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->GetPath(),item);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size();
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize))
    return false;

  char cDelim = '\0';
  size_t nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos != CStdString::npos )
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos != CStdString::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }
  while( iTextMaxLength < iStrInputSize )
  {
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos != CStdString::npos )
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    if ( nPos == CStdString::npos ) break;
    if ( nGreaterDelim > nPos ) StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsWriteFileOperations(const CStdString& strPath)
{
  // currently only hd, smb, nfs, afp and dav support delete and rename
  if (URIUtils::IsHD(strPath))
    return true;
  if (URIUtils::IsSmb(strPath))
    return true;
  if (CUtil::IsTVRecording(strPath))
    return CPVRDirectory::SupportsWriteFileOperations(strPath);
  if (URIUtils::IsNfs(strPath))
    return true;
  if (URIUtils::IsAfp(strPath))
    return true;
  if (URIUtils::IsDAV(strPath))
    return true;
  if (URIUtils::IsMythTV(strPath))
  {
    /*
     * Can't use CFile::Exists() to check whether the myth:// path supports file operations because
     * it hits the directory cache on the way through, which has the Live Channels and Guide
     * items cached.
     */
    return CMythDirectory::SupportsWriteFileOperations(strPath);
  }
  if (URIUtils::IsStack(strPath))
    return SupportsWriteFileOperations(CStackDirectory::GetFirstStackedFile(strPath));
  if (URIUtils::IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsWriteFileOperations(strPath);

  return false;
}

bool CUtil::SupportsReadFileOperations(const CStdString& strPath)
{
  if (URIUtils::IsVideoDb(strPath))
    return false;

  return true;
}

CStdString CUtil::GetDefaultFolderThumb(const CStdString &folderThumb)
{
  if (g_TextureManager.HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(vector<CStdString>& vecTheme)
{
  CStdString strPath = URIUtils::AddFileToFolder(g_graphicsContext.GetMediaDir(), "media");
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension = URIUtils::GetExtension(pItem->GetPath());
      if ((strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr")) ||
          (strExtension == ".xbt" && pItem->GetLabel().CompareNoCase("Textures.xbt")))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::InitRandomSeed()
{
  // Init random seed
  int64_t now;
  now = CurrentHostCounter();
  unsigned int seed = (unsigned int)now;
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

#ifdef TARGET_POSIX
bool CUtil::RunCommandLine(const CStdString& cmdLine, bool waitExit)
{
  CStdStringArray args;

  StringUtils::SplitString(cmdLine, ",", args);

  // Strip quotes and whitespace around the arguments, or exec will fail.
  // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
  // But it's still limited, for example quotes inside the strings are not expanded, etc.
  // TODO: Maybe some python library routine can parse this more properly ?
  for (size_t i=0; i<args.size(); i++)
  {
    CStdString &s = args[i];
    CStdString stripd = s.Trim();
    if (stripd[0] == '"' || stripd[0] == '\'')
    {
      s = s.TrimLeft();
      s = s.Right(s.size() - 1);
    }
    if (stripd[stripd.size() - 1] == '"' || stripd[stripd.size() - 1] == '\'')
    {
      s = s.TrimRight();
      s = s.Left(s.size() - 1);
    }
  }

  return Command(args, waitExit);
}

//
// FIXME, this should be merged with the function below.
//
bool CUtil::Command(const CStdStringArray& arrArgs, bool waitExit)
{
#ifdef _DEBUG
  printf("Executing: ");
  for (size_t i=0; i<arrArgs.size(); i++)
    printf("%s ", arrArgs[i].c_str());
  printf("\n");
#endif

  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    if (!waitExit)
    {
      // fork again in order not to leave a zombie process
      child = fork();
      if (child == -1)
        _exit(2);
      else if (child != 0)
        _exit(0);
    }
    close(0);
    close(1);
    close(2);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      for (size_t i=0; i<arrArgs.size(); i++)
        args[i] = (char *)arrArgs[i].c_str();
      execvp(args[0], args);
    }
  }
  else
  {
    waitpid(child, &n, 0);
  }

  return (waitExit) ? (WEXITSTATUS(n) == 0) : true;
}

bool CUtil::SudoCommand(const CStdString &strCommand)
{
  CLog::Log(LOGDEBUG, "Executing sudo command: <%s>", strCommand.c_str());
  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0); // close stdin to avoid sudo request password
    close(1);
    close(2);
    CStdStringArray arrArgs;
    StringUtils::SplitString(strCommand, " ", arrArgs);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      args[0] = (char *)"/usr/bin/sudo";
      args[1] = (char *)"-S";
      for (size_t i=0; i<arrArgs.size(); i++)
      {
        args[i+2] = (char *)arrArgs[i].c_str();
      }
      execvp("/usr/bin/sudo", args);
    }
  }
  else
    waitpid(child, &n, 0);

  return WEXITSTATUS(n) == 0;
}
#endif

int CUtil::LookupRomanDigit(char roman_digit)
{
  switch (roman_digit)
  {
    case 'i':
    case 'I':
      return 1;
    case 'v':
    case 'V':
      return 5;
    case 'x':
    case 'X':
      return 10;
    case 'l':
    case 'L':
      return 50;
    case 'c':
    case 'C':
      return 100;
    case 'd':
    case 'D':
      return 500;
    case 'm':
    case 'M':
      return 1000;
    default:
      return 0;
  }
}

int CUtil::TranslateRomanNumeral(const char* roman_numeral)
{
  
  int decimal = -1;

  if (roman_numeral && roman_numeral[0])
  {
    int temp_sum  = 0,
        last      = 0,
        repeat    = 0,
        trend     = 1;
    decimal = 0;
    while (*roman_numeral)
    {
      int digit = CUtil::LookupRomanDigit(*roman_numeral);
      int test  = last;
      
      // General sanity checks

      // numeral not in LUT
      if (!digit)
        return -1;
      
      while (test > 5)
        test /= 10;
      
      // N = 10^n may not precede (N+1) > 10^(N+1)
      if (test == 1 && digit > last * 10)
        return -1;
      
      // N = 5*10^n may not precede (N+1) >= N
      if (test == 5 && digit >= last) 
        return -1;

      // End general sanity checks

      if (last < digit)
      {
        // smaller numerals may not repeat before a larger one
        if (repeat) 
          return -1;

        temp_sum += digit;
        
        repeat  = 0;
        trend   = 0;
      }
      else if (last == digit)
      {
        temp_sum += digit;
        repeat++;
        trend = 1;
      }
      else
      {
        if (!repeat)
          decimal += 2 * last - temp_sum;
        else
          decimal += temp_sum;
        
        temp_sum = digit;

        trend   = 1;
        repeat  = 0;
      }
      // Post general sanity checks

      // numerals may not repeat more than thrice
      if (repeat == 3)
        return -1;

      last = digit;
      roman_numeral++;
    }

    if (trend)
      decimal += temp_sum;
    else
      decimal += 2 * last - temp_sum;
  }
  return decimal;
}

CStdString CUtil::ResolveExecutablePath()
{
  CStdString strExecutablePath;
#ifdef TARGET_WINDOWS
  static const size_t bufSize = MAX_PATH * 2;
  wchar_t* buf = new wchar_t[bufSize];
  buf[0] = 0;
  ::GetModuleFileNameW(0, buf, bufSize);
  buf[bufSize-1] = 0;
  g_charsetConverter.wToUTF8(buf,strExecutablePath);
  delete[] buf;
#elif defined(TARGET_DARWIN)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinExecutablePath(given_path, &path_size);
  strExecutablePath = given_path;
#elif defined(TARGET_FREEBSD)                                                                                                                                                                   
  char buf[PATH_MAX];
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = getpid();

  buflen = sizeof(buf) - 1;
  if(sysctl(mib, 4, buf, &buflen, NULL, 0) < 0)
    strExecutablePath = "";
  else
    strExecutablePath = buf;
#else
  /* Get our PID and build the name of the link in /proc */
  pid_t pid = getpid();
  char linkname[64]; /* /proc/<pid>/exe */
  snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);

  /* Now read the symbolic link */
  char buf[PATH_MAX + 1];
  buf[0] = 0;

  int ret = readlink(linkname, buf, sizeof(buf) - 1);
  if (ret != -1)
    buf[ret] = 0;

  strExecutablePath = buf;
#endif
  return strExecutablePath;
}

CStdString CUtil::GetFrameworksPath(bool forPython)
{
  CStdString strFrameworksPath;
#if defined(TARGET_DARWIN)
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinFrameworkPath(forPython, given_path, &path_size);
  strFrameworksPath = given_path;
#endif
  return strFrameworksPath;
}

void CUtil::ScanForExternalSubtitles(const CStdString& strMovie, std::vector<CStdString>& vecSubtitles )
{
  unsigned int startTimer = XbmcThreads::SystemClockMillis();
  
  // new array for commons sub dirs
  const char * common_sub_dirs[] = {"subs",
    "Subs",
    "subtitles",
    "Subtitles",
    "vobsubs",
    "Vobsubs",
    "sub",
    "Sub",
    "vobsub",
    "Vobsub",
    "subtitle",
    "Subtitle",
    NULL};
  
  vector<CStdString> vecExtensionsCached;
  
  CFileItem item(strMovie, false);
  if ( item.IsInternetStream()
    || item.IsHDHomeRun()
    || item.IsSlingbox()
    || item.IsPlayList()
    || item.IsLiveTV()
    || !item.IsVideo())
    return;
  
  vector<CStdString> strLookInPaths;
  
  CStdString strMovieFileName;
  CStdString strPath;
  
  URIUtils::Split(strMovie, strPath, strMovieFileName);
  CStdString strMovieFileNameNoExt(URIUtils::ReplaceExtension(strMovieFileName, ""));
  strLookInPaths.push_back(strPath);
  
  if (!CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() && !CSettings::Get().GetString("subtitles.custompath").empty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !URIUtils::IsHD(CSettings::Get().GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    else if (!CDirectory::Exists(CSettings::Get().GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(-1); // disabled
    }
    
    CMediaSettings::Get().SetAdditionalSubtitleDirectoryChecked(1);
  }
  
  if (strMovie.Left(6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURL url(strMovie);
    CStdString strArchive = url.GetHostName();
    URIUtils::Split(strArchive, strPath, strMovieFileName);
    strLookInPaths.push_back(strPath);
  }
  
  int iSize = strLookInPaths.size();
  for (int i=0; i<iSize; ++i)
  {
    CStdStringArray directories;
    int nTokens = StringUtils::SplitString( strLookInPaths[i], "/", directories );
    if (nTokens == 1)
      StringUtils::SplitString( strLookInPaths[i], "\\", directories );

    // if it's inside a cdX dir, add parent path
    if (directories.size() >= 2 && directories[directories.size()-2].size() == 3 && directories[directories.size()-2].Left(2).Equals("cd")) // SplitString returns empty token as last item, hence size-2
    {
      CStdString strPath2;
      URIUtils::GetParentPath(strLookInPaths[i], strPath2);
      strLookInPaths.push_back(strPath2);
    }
  }

  // checking if any of the common subdirs exist ..
  iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      CStdString strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j]);
      URIUtils::AddSlashAtEnd(strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for common subdirs
  
  // check if there any cd-directories in the paths we have added so far
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    CStdString cdDir;
    cdDir.Format("cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2 = URIUtils::AddFileToFolder(strLookInPaths[i],cdDir);
      URIUtils::AddSlashAtEnd(strPath2);
      bool pathAlreadyAdded = false;
      for (unsigned int i=0; i<strLookInPaths.size(); i++)
      {
        // if movie file is inside cd-dir, this directory can exist in vector already
        if (strLookInPaths[i].Equals( strPath2 ) )
          pathAlreadyAdded = true;
      }
      if (CDirectory::Exists(strPath2) && !pathAlreadyAdded) 
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs
  
  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (CMediaSettings::Get().GetAdditionalSubtitleDirectoryChecked() == 1)
  {
    strPath = CSettings::Get().GetString("subtitles.custompath");
    URIUtils::AddSlashAtEnd(strPath);
    strLookInPaths.push_back(strPath);
  }
  
  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;
  
  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;
      
      CDirectory::GetDirectory(strLookInPaths[step], items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",DIR_FLAG_NO_FILE_DIRS);
      int fnl = strMovieFileNameNoExt.size();
      
      for (int j = 0; j < items.Size(); j++)
      {
        URIUtils::Split(items[j]->GetPath(), strPath, strItem);
        
        if (strItem.Left(fnl).Equals(strMovieFileNameNoExt))
        {
          // is this a rar or zip-file
          if (URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem))
          {
            // zip-file name equals strMovieFileNameNoExt, don't check in zip-file
            ScanArchiveForSubtitles( items[j]->GetPath(), "", vecSubtitles );
          }
          else    // not a rar/zip file
          {
            for (int i = 0; sub_exts[i]; i++)
            {
              //Cache subtitle with same name as movie
              if (URIUtils::HasExtension(strItem, sub_exts[i]))
              {
                vecSubtitles.push_back( items[j]->GetPath() ); 
                CLog::Log(LOGINFO, "%s: found subtitle file %s\n", __FUNCTION__, items[j]->GetPath().c_str() );
              }
            }
          }
        }
        else
        {
          // is this a rar or zip-file
          if (URIUtils::IsRAR(strItem) || URIUtils::IsZIP(strItem))
          {
            // check strMovieFileNameNoExt in zip-file
            ScanArchiveForSubtitles( items[j]->GetPath(), strMovieFileNameNoExt, vecSubtitles );
          }
        }
      }
    }
  }

  iSize = vecSubtitles.size();
  for (int i = 0; i < iSize; i++)
  {
    if (URIUtils::HasExtension(vecSubtitles[i], ".smi"))
    {
      //Cache multi-language sami subtitle
      CDVDSubtitleStream* pStream = new CDVDSubtitleStream();
      if(pStream->Open(vecSubtitles[i]))
      {
        CDVDSubtitleTagSami TagConv;
        TagConv.LoadHead(pStream);
        if (TagConv.m_Langclass.size() >= 2)
        {
          for (unsigned int k = 0; k < TagConv.m_Langclass.size(); k++)
          {
            strDest.Format("special://temp/subtitle.%s.%d.smi", TagConv.m_Langclass[k].Name, i);
            if (CFile::Cache(vecSubtitles[i], strDest))
            {
              CLog::Log(LOGINFO, " cached subtitle %s->%s\n", vecSubtitles[i].c_str(), strDest.c_str());
              vecSubtitles.push_back(strDest);
            }
          }
        }
      }
      delete pStream;
    }
  }
  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(XbmcThreads::SystemClockMillis() - startTimer));
}

int CUtil::ScanArchiveForSubtitles( const CStdString& strArchivePath, const CStdString& strMovieFileNameNoExt, std::vector<CStdString>& vecSubtitles )
{
  int nSubtitlesAdded = 0;
  CFileItemList ItemList;
 
  // zip only gets the root dir
  if (URIUtils::HasExtension(strArchivePath, ".zip"))
  {
   CStdString strZipPath;
   URIUtils::CreateArchivePath(strZipPath,"zip",strArchivePath,"");
   if (!CDirectory::GetDirectory(strZipPath,ItemList,"",DIR_FLAG_NO_FILE_DIRS))
    return false;
  }
  else
  {
 #ifdef HAS_FILESYSTEM_RAR
   // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
   // so now we dont have to find any subdirs anymore, all files in the rar is checked.
   if( !g_RarManager.GetFilesInRar(ItemList, strArchivePath, false, "") )
    return false;
 #else
   return false;
 #endif
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
   CStdString strPathInRar = ItemList[it]->GetPath();
   CStdString strExt = URIUtils::GetExtension(strPathInRar);
   
   CLog::Log(LOGDEBUG, "ScanArchiveForSubtitles:: Found file %s", strPathInRar.c_str());
   // always check any embedded rar archives
   // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
   if (URIUtils::IsRAR(strPathInRar) || URIUtils::IsZIP(strPathInRar))
   {
    CStdString strRarInRar;
    if (strExt == ".rar")
      URIUtils::CreateArchivePath(strRarInRar, "rar", strArchivePath, strPathInRar);
    else
      URIUtils::CreateArchivePath(strRarInRar, "zip", strArchivePath, strPathInRar);
    ScanArchiveForSubtitles(strRarInRar,strMovieFileNameNoExt,vecSubtitles);
   }
   // done checking if this is a rar-in-rar

   // check that the found filename matches the movie filename
   int fnl = strMovieFileNameNoExt.size();
   if (fnl && !URIUtils::GetFileName(strPathInRar).Left(fnl).Equals(strMovieFileNameNoExt))
     continue;

   int iPos=0;
    while (sub_exts[iPos])
    {
     if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
     {
      CStdString strSourceUrl;
      if (URIUtils::HasExtension(strArchivePath, ".rar"))
       URIUtils::CreateArchivePath(strSourceUrl, "rar", strArchivePath, strPathInRar);
      else
       strSourceUrl = strPathInRar;
      
       CLog::Log(LOGINFO, "%s: found subtitle file %s\n", __FUNCTION__, strSourceUrl.c_str() );
       vecSubtitles.push_back( strSourceUrl );
       nSubtitlesAdded++;
     }
     
     iPos++;
    }
  }

  return nSubtitlesAdded;
}

/*! \brief in a vector of subtitles finds the corresponding .sub file for a given .idx file
 */
bool CUtil::FindVobSubPair( const std::vector<CStdString>& vecSubtitles, const CStdString& strIdxPath, CStdString& strSubPath )
{
  if (URIUtils::HasExtension(strIdxPath, ".idx"))
  {
    CStdString strIdxFile;
    CStdString strIdxDirectory;
    URIUtils::Split(strIdxPath, strIdxDirectory, strIdxFile);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      CStdString strSubFile;
      CStdString strSubDirectory;
      URIUtils::Split(vecSubtitles[j], strSubDirectory, strSubFile);
      if (URIUtils::IsInArchive(vecSubtitles[j]))
        CURL::Decode(strSubDirectory);
      if (URIUtils::HasExtension(strSubFile, ".sub") &&
          (URIUtils::ReplaceExtension(strIdxFile,"").Equals(URIUtils::ReplaceExtension(strSubFile,"")) ||
           strSubDirectory.Mid(6, strSubDirectory.length()-11).Equals(URIUtils::ReplaceExtension(strIdxPath,""))))
      {
        strSubPath = vecSubtitles[j];
        return true;
      }
    }
  }
  return false;
}

/*! \brief checks if in the vector of subtitles the given .sub file has a corresponding idx and hence is a vobsub file
 */
bool CUtil::IsVobSub( const std::vector<CStdString>& vecSubtitles, const CStdString& strSubPath )
{
  if (URIUtils::HasExtension(strSubPath, ".sub"))
  {
    CStdString strSubFile;
    CStdString strSubDirectory;
    URIUtils::Split(strSubPath, strSubDirectory, strSubFile);
    if (URIUtils::IsInArchive(strSubPath))
      CURL::Decode(strSubDirectory);
    for (unsigned int j=0; j < vecSubtitles.size(); j++)
    {
      CStdString strIdxFile;
      CStdString strIdxDirectory;
      URIUtils::Split(vecSubtitles[j], strIdxDirectory, strIdxFile);
      if (URIUtils::HasExtension(strIdxFile, ".idx") &&
          (URIUtils::ReplaceExtension(strIdxFile,"").Equals(URIUtils::ReplaceExtension(strSubFile,"")) ||
           strSubDirectory.Mid(6, strSubDirectory.length()-11).Equals(URIUtils::ReplaceExtension(vecSubtitles[j],""))))
        return true;
    }
  }
  return false;
}

bool CUtil::CanBindPrivileged()
{
#ifdef TARGET_POSIX

  if (geteuid() == 0)
    return true; //root user can always bind to privileged ports

#ifdef HAVE_LIBCAP

  //check if CAP_NET_BIND_SERVICE is enabled, this allows non-root users to bind to privileged ports
  bool canbind = false;
  cap_t capabilities = cap_get_proc();
  if (capabilities)
  {
    cap_flag_value_t value;
    if (cap_get_flag(capabilities, CAP_NET_BIND_SERVICE, CAP_EFFECTIVE, &value) == 0)
      canbind = value;

    cap_free(capabilities);
  }

  return canbind;

#else //HAVE_LIBCAP

  return false;

#endif //HAVE_LIBCAP

#else //TARGET_POSIX

  return true;

#endif //TARGET_POSIX
}

bool CUtil::ValidatePort(int port)
{
  // check that it's a valid port
#ifdef TARGET_POSIX
  if (!CUtil::CanBindPrivileged() && (port < 1024 || port > 65535))
    return false;
  else
#endif
  if (port <= 0 || port > 65535)
    return false;

  return true;
}

int CUtil::GetRandomNumber()
{
#ifdef TARGET_WINDOWS
  unsigned int number;
  if (rand_s(&number) == 0)
    return (int)number;
#else
  return rand_r(&s_randomSeed);
#endif

  return rand();
}

