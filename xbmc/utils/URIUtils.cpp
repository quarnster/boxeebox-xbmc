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
#include "URIUtils.h"
#include "Application.h"
#include "FileItem.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MythDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "URL.h"
#include "StringUtils.h"

#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using namespace XFILE;

bool URIUtils::IsInPath(const CStdString &uri, const CStdString &baseURI)
{
  CStdString uriPath = CSpecialProtocol::TranslatePath(uri);
  CStdString basePath = CSpecialProtocol::TranslatePath(baseURI);
  return (strncmp(uriPath.c_str(), basePath.c_str(), basePath.GetLength()) == 0);
}

/* returns filename extension including period of filename */
CStdString URIUtils::GetExtension(const CStdString& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return GetExtension(url.GetFileName());
  }

  size_t period = strFileName.find_last_of("./\\");
  if (period == string::npos || strFileName[period] != '.')
    return CStdString();

  return strFileName.substr(period);
}

bool URIUtils::HasExtension(const CStdString& strFileName)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return HasExtension(url.GetFileName());
  }

  size_t iPeriod = strFileName.find_last_of("./\\");
  return iPeriod != string::npos && strFileName[iPeriod] == '.';
}

bool URIUtils::HasExtension(const CStdString& strFileName, const CStdString& strExtensions)
{
  if (IsURL(strFileName))
  {
    CURL url(strFileName);
    return HasExtension(url.GetFileName(), strExtensions);
  }

  // Search backwards so that '.' can be used as a search terminator.
  CStdString::const_reverse_iterator itExtensions = strExtensions.rbegin();
  while (itExtensions != strExtensions.rend())
  {
    // Iterate backwards over strFileName untill we hit a '.' or a mismatch
    for (CStdString::const_reverse_iterator itFileName = strFileName.rbegin();
         itFileName != strFileName.rend(), itExtensions != strExtensions.rend(),
         tolower(*itFileName) == *itExtensions;
         ++itFileName, ++itExtensions)
    {
      if (*itExtensions == '.')
        return true; // Match
    }

    // No match. Look for more extensions to try.
    while (itExtensions != strExtensions.rend() && *itExtensions != '|')
      ++itExtensions;

    while (itExtensions != strExtensions.rend() && *itExtensions == '|')
      ++itExtensions;
  }

  return false;
}

void URIUtils::RemoveExtension(CStdString& strFileName)
{
  if(IsURL(strFileName))
  {
    CURL url(strFileName);
    strFileName = url.GetFileName();
    RemoveExtension(strFileName);
    url.SetFileName(strFileName);
    strFileName = url.Get();
    return;
  }

  int iPos = strFileName.ReverseFind(".");
  // Extension found
  if (iPos > 0)
  {
    CStdString strExtension = GetExtension(strFileName);
    strExtension.ToLower();
    strExtension += "|";

    CStdString strFileMask;
    strFileMask = g_advancedSettings.m_pictureExtensions;
    strFileMask += "|" + g_advancedSettings.m_musicExtensions;
    strFileMask += "|" + g_advancedSettings.m_videoExtensions;
#if defined(TARGET_DARWIN)
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg|.app|.applescript|.workflow";
#else
    strFileMask += "|.py|.xml|.milk|.xpr|.xbt|.cdg";
#endif
    strFileMask += "|";

    if (strFileMask.Find(strExtension) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

CStdString URIUtils::ReplaceExtension(const CStdString& strFile,
                                      const CStdString& strNewExtension)
{
  if(IsURL(strFile))
  {
    CURL url(strFile);
    url.SetFileName(ReplaceExtension(url.GetFileName(), strNewExtension));
    return url.Get();
  }

  CStdString strChangedFile;
  CStdString strExtension = GetExtension(strFile);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
  return strChangedFile;
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const CStdString URIUtils::GetFileName(const CStdString& strFileNameAndPath)
{
  if(IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return GetFileName(url.GetFileName());
  }

  /* find any slashes */
  const int slash1 = strFileNameAndPath.find_last_of('/');
  const int slash2 = strFileNameAndPath.find_last_of('\\');

  /* select the last one */
  int slash;
  if(slash2>slash1)
    slash = slash2;
  else
    slash = slash1;

  return strFileNameAndPath.substr(slash+1);
}

void URIUtils::Split(const CStdString& strFileNameAndPath,
                     CStdString& strPath, CStdString& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

CStdStringArray URIUtils::SplitPath(const CStdString& strPath)
{
  CURL url(strPath);

  // silly CStdString can't take a char in the constructor
  CStdString sep(1, url.GetDirectorySeparator());

  // split the filename portion of the URL up into separate dirs
  CStdStringArray dirs;
  StringUtils::SplitString(url.GetFileName(), sep, dirs);
  
  // we start with the root path
  CStdString dir = url.GetWithoutFilename();
  
  if (!dir.IsEmpty())
    dirs.insert(dirs.begin(), dir);

  // we don't need empty token on the end
  if (dirs.size() > 1 && dirs.back().IsEmpty())
    dirs.erase(dirs.end() - 1);

  return dirs;
}

void URIUtils::GetCommonPath(CStdString& strParent, const CStdString& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent = strParent.Left(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!HasSlashAtEnd(strParent))
  {
    strParent = GetDirectory(strParent);
    AddSlashAtEnd(strParent);
  }
}

bool URIUtils::ProtocolHasParentInHostname(const CStdString& prot)
{
  return prot.Equals("zip")
      || prot.Equals("rar")
      || prot.Equals("apk")
      || prot.Equals("bluray")
      || prot.Equals("udf");
}

bool URIUtils::ProtocolHasEncodedHostname(const CStdString& prot)
{
  return ProtocolHasParentInHostname(prot)
      || prot.Equals("musicsearch")
      || prot.Equals("image");
}

bool URIUtils::ProtocolHasEncodedFilename(const CStdString& prot)
{
  CStdString prot2 = CURL::TranslateProtocol(prot);

  // For now assume only (quasi) http internet streams use URL encoding
  return prot2 == "http"  ||
         prot2 == "https";
}

CStdString URIUtils::GetParentPath(const CStdString& strPath)
{
  CStdString strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

bool URIUtils::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURL url(strPath);
  CStdString strFile = url.GetFileName();
  if ( URIUtils::ProtocolHasParentInHostname(url.GetProtocol()) && strFile.IsEmpty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strPath,items);
    items[0]->m_strDVDLabel = GetDirectory(items[0]->GetPath());
    if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
      GetParentPath(items[0]->m_strDVDLabel, strParent);
    else
      strParent = items[0]->m_strDVDLabel;
    for( int i=1;i<items.Size();++i)
    {
      items[i]->m_strDVDLabel = GetDirectory(items[i]->GetPath());
      if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
        items[i]->SetPath(GetParentPath(items[i]->m_strDVDLabel));
      else
        items[i]->SetPath(items[i]->m_strDVDLabel);

      GetCommonPath(strParent,items[i]->GetPath());
    }
    return true;
  }
  else if (url.GetProtocol() == "multipath")
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.GetProtocol() == "plugin")
  {
    if (!url.GetOptions().IsEmpty())
    {
      url.SetOptions("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetFileName().IsEmpty())
    {
      url.SetFileName("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetHostName().IsEmpty())
    {
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return true;  // already at root
  }
  else if (url.GetProtocol() == "special")
  {
    if (HasSlashAtEnd(strFile) )
      strFile = strFile.Left(strFile.size() - 1);
    if(strFile.ReverseFind('/') < 0)
      return false;
  }
  else if (strFile.size() == 0)
  {
    if (url.GetHostName().size() > 0)
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile = strFile.Left(strFile.size() - 1);
  }

  int iPos = strFile.ReverseFind('/');
#ifndef TARGET_POSIX
  if (iPos < 0)
  {
    iPos = strFile.ReverseFind('\\');
  }
#endif
  if (iPos < 0)
  {
    url.SetFileName("");
    strParent = url.Get();
    return true;
  }

  strFile = strFile.Left(iPos);

  AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  strParent = url.Get();
  return true;
}

CStdString URIUtils::SubstitutePath(const CStdString& strPath)
{
  for (CAdvancedSettings::StringMapping::iterator i = g_advancedSettings.m_pathSubstitutions.begin();
      i != g_advancedSettings.m_pathSubstitutions.end(); i++)
  {
    if (strncmp(strPath.c_str(), i->first.c_str(), HasSlashAtEnd(i->first.c_str()) ? i->first.size()-1 : i->first.size()) == 0)
    {
      if (strPath.size() > i->first.size())
        return URIUtils::AddFileToFolder(i->second, strPath.Mid(i->first.size()));
      else
        return i->second;
    }
  }
  return strPath;
}

bool URIUtils::IsRemote(const CStdString& strFile)
{
  if (IsCDDA(strFile) || IsISO9660(strFile))
    return false;

  if (IsSpecial(strFile))
    return IsRemote(CSpecialProtocol::TranslatePath(strFile));

  if(IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    vector<CStdString> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  CURL url(strFile);
  if(ProtocolHasParentInHostname(url.GetProtocol()))
    return IsRemote(url.GetHostName());

  if (!url.IsLocal())
    return true;

  return false;
}

bool URIUtils::IsOnDVD(const CStdString& strFile)
{
#ifdef TARGET_WINDOWS
  if (strFile.Mid(1,1) == ":")
    return (GetDriveType(strFile.Left(3)) == DRIVE_CDROM);
#endif

  if (strFile.Left(4).CompareNoCase("dvd:") == 0)
    return true;

  if (strFile.Left(4).CompareNoCase("udf:") == 0)
    return true;

  if (strFile.Left(8).CompareNoCase("iso9660:") == 0)
    return true;

  if (strFile.Left(5).CompareNoCase("cdda:") == 0)
    return true;

  return false;
}

bool URIUtils::IsOnLAN(const CStdString& strPath)
{
  if(IsMultiPath(strPath))
    return IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath));

  if(IsStack(strPath))
    return IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath));

  if(IsSpecial(strPath))
    return IsOnLAN(CSpecialProtocol::TranslatePath(strPath));

  if(IsDAAP(strPath))
    return true;
  
  if(IsPlugin(strPath))
    return false;

  if(IsTuxBox(strPath))
    return true;

  if(IsUPnP(strPath))
    return true;

  CURL url(strPath);
  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsOnLAN(url.GetHostName());

  if(!IsRemote(strPath))
    return false;

  CStdString host = url.GetHostName();
  if(host.length() == 0)
    return false;

  // assume a hostname without dot's
  // is local (smb netbios hostnames)
  if(host.find('.') == string::npos)
    return true;

  unsigned long address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    CStdString ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address != INADDR_NONE)
  {
    // check if we are on the local subnet
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;

    if (g_application.getNetwork().HasInterfaceForIP(address))
      return true;
  }

  return false;
}

bool URIUtils::IsMultiPath(const CStdString& strPath)
{
  return strPath.Left(10).Equals("multipath:");
}

bool URIUtils::IsHD(const CStdString& strFileName)
{
  CURL url(strFileName);

  if (IsSpecial(strFileName))
    return IsHD(CSpecialProtocol::TranslatePath(strFileName));

  if(IsStack(strFileName))
    return IsHD(CStackDirectory::GetFirstStackedFile(strFileName));

  if (ProtocolHasParentInHostname(url.GetProtocol()))
    return IsHD(url.GetHostName());

  return url.GetProtocol().IsEmpty() || url.GetProtocol() == "file";
}

bool URIUtils::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile;
  strFileLow.MakeLower();
  if (strFileLow.Find("video_ts.ifo") != -1 && IsOnDVD(strFile))
    return true;

#if defined(TARGET_WINDOWS)
  if (strFile.Left(6).Equals("dvd://"))
    return true;

  if(strFile.Mid(1) != ":\\"
  && strFile.Mid(1) != ":")
    return false;

  if(GetDriveType(strFile.c_str()) == DRIVE_CDROM)
    return true;
#else
  if (strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool URIUtils::IsStack(const CStdString& strFile)
{
  return strFile.Left(6).Equals("stack:");
}

bool URIUtils::IsRAR(const CStdString& strFile)
{
  CStdString strExtension = GetExtension(strFile);

  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001"))
    return true;

  if (strExtension.CompareNoCase(".cbr") == 0)
    return true;

  if (strExtension.CompareNoCase(".rar") == 0)
    return true;

  return false;
}

bool URIUtils::IsInArchive(const CStdString &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile) || IsInAPK(strFile);
}

bool URIUtils::IsInAPK(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "apk" && url.GetFileName() != "";
}

bool URIUtils::IsInZIP(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "zip" && url.GetFileName() != "";
}

bool URIUtils::IsInRAR(const CStdString& strFile)
{
  CURL url(strFile);

  return url.GetProtocol() == "rar" && url.GetFileName() != "";
}

bool URIUtils::IsAPK(const CStdString& strFile)
{
  return HasExtension(strFile, ".apk");
}

bool URIUtils::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  return HasExtension(strFile, ".zip|.cbz");
}

bool URIUtils::IsArchive(const CStdString& strFile)
{
  return HasExtension(strFile, ".zip|.rar|.apk|.cbz|.cbr");
}

bool URIUtils::IsSpecial(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(8).Equals("special:");
}

bool URIUtils::IsPlugin(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("plugin");
}

bool URIUtils::IsScript(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("script");
}

bool URIUtils::IsAddonsPath(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("addons");
}

bool URIUtils::IsSourcesPath(const CStdString& strPath)
{
  CURL url(strPath);
  return url.GetProtocol().Equals("sources");
}

bool URIUtils::IsCDDA(const CStdString& strFile)
{
  return strFile.Left(5).Equals("cdda:");
}

bool URIUtils::IsISO9660(const CStdString& strFile)
{
  return strFile.Left(8).Equals("iso9660:");
}

bool URIUtils::IsSmb(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(4).Equals("smb:");
}

bool URIUtils::IsURL(const CStdString& strFile)
{
  return strFile.Find("://") >= 0;
}

bool URIUtils::IsFTP(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(4).Equals("ftp:")  ||
         strFile2.Left(5).Equals("ftps:");
}

bool URIUtils::IsDAV(const CStdString& strFile)
{
  CStdString strFile2(strFile);

  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);

  return strFile2.Left(4).Equals("dav:")  ||
         strFile2.Left(5).Equals("davs:");
}

bool URIUtils::IsInternetStream(const CURL& url, bool bStrictCheck /* = false */)
{
  CStdString strProtocol = url.GetProtocol();
  
  if (strProtocol.IsEmpty())
    return false;

  // there's nothing to stop internet streams from being stacked
  if (strProtocol == "stack")
    return IsInternetStream(CStackDirectory::GetFirstStackedFile(url.Get()));

  CStdString strProtocol2 = url.GetTranslatedProtocol();

  // Special case these
  if (strProtocol  == "ftp"   || strProtocol  == "ftps"   ||
      strProtocol  == "dav"   || strProtocol  == "davs")
    return bStrictCheck;

  if (strProtocol2 == "http"  || strProtocol2 == "https"  ||
      strProtocol2 == "tcp"   || strProtocol2 == "udp"    ||
      strProtocol2 == "rtp"   || strProtocol2 == "sdp"    ||
      strProtocol2 == "mms"   || strProtocol2 == "mmst"   ||
      strProtocol2 == "mmsh"  || strProtocol2 == "rtsp"   ||
      strProtocol2 == "rtmp"  || strProtocol2 == "rtmpt"  ||
      strProtocol2 == "rtmpe" || strProtocol2 == "rtmpte" ||
      strProtocol2 == "rtmps")
    return true;

  return false;
}

bool URIUtils::IsDAAP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("daap:");
}

bool URIUtils::IsUPnP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("upnp:");
}

bool URIUtils::IsTuxBox(const CStdString& strFile)
{
  return strFile.Left(7).Equals("tuxbox:");
}

bool URIUtils::IsMythTV(const CStdString& strFile)
{
  return strFile.Left(5).Equals("myth:");
}

bool URIUtils::IsHDHomeRun(const CStdString& strFile)
{
  return strFile.Left(10).Equals("hdhomerun:");
}

bool URIUtils::IsSlingbox(const CStdString& strFile)
{
  return strFile.Left(6).Equals("sling:");
}

bool URIUtils::IsVTP(const CStdString& strFile)
{
  return strFile.Left(4).Equals("vtp:");
}

bool URIUtils::IsHTSP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("htsp:");
}

bool URIUtils::IsLiveTV(const CStdString& strFile)
{
  CStdString strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  if(IsTuxBox(strFile)
  || IsVTP(strFile)
  || IsHDHomeRun(strFile)
  || IsSlingbox(strFile)
  || IsHTSP(strFile)
  || strFile.Left(4).Equals("sap:")
  ||(strFileWithoutSlash.Right(4).Equals(".pvr") && !strFileWithoutSlash.Left(16).Equals("pvr://recordings")))
    return true;

  if (IsMythTV(strFile) && CMythDirectory::IsLiveTV(strFile))
    return true;

  return false;
}

bool URIUtils::IsPVRRecording(const CStdString& strFile)
{
  CStdString strFileWithoutSlash(strFile);
  RemoveSlashAtEnd(strFileWithoutSlash);

  return strFileWithoutSlash.Right(4).Equals(".pvr") &&
         strFile.Left(16).Equals("pvr://recordings");
}

bool URIUtils::IsMusicDb(const CStdString& strFile)
{
  return strFile.Left(8).Equals("musicdb:");
}

bool URIUtils::IsNfs(const CStdString& strFile)
{
  CStdString strFile2(strFile);
  
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  
  return strFile2.Left(4).Equals("nfs:");
}

bool URIUtils::IsAfp(const CStdString& strFile)
{
  CStdString strFile2(strFile);
  
  if (IsStack(strFile))
    strFile2 = CStackDirectory::GetFirstStackedFile(strFile);
  
  return strFile2.Left(4).Equals("afp:");
}


bool URIUtils::IsVideoDb(const CStdString& strFile)
{
  return strFile.Left(8).Equals("videodb:");
}

bool URIUtils::IsBluray(const CStdString& strFile)
{
  return strFile.Left(7).Equals("bluray:");
}

bool URIUtils::IsAndroidApp(const CStdString &path)
{
  return path.Left(11).Equals("androidapp:");
}

bool URIUtils::IsLibraryFolder(const CStdString& strFile)
{
  CURL url(strFile);
  return url.GetProtocol().Equals("library");
}

bool URIUtils::IsDOSPath(const CStdString &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

void URIUtils::AddSlashAtEnd(CStdString& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    CStdString file = url.GetFileName();
    if(!file.IsEmpty() && file != strFolder)
    {
      AddSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
    }
    return;
  }

  if (!HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

bool URIUtils::HasSlashAtEnd(const CStdString& strFile, bool checkURL /* = false */)
{
  if (strFile.size() == 0) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    CStdString file = url.GetFileName();
    return file.IsEmpty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(CStdString& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    CStdString file = url.GetFileName();
    if (!file.IsEmpty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().IsEmpty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.Delete(strFolder.size() - 1);
}

bool URIUtils::CompareWithoutSlashAtEnd(const CStdString& strPath1, const CStdString& strPath2)
{
  CStdString strc1 = strPath1, strc2 = strPath2;
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return strc1.Equals(strc2);
}

CStdString URIUtils::AddFileToFolder(const CStdString& strFolder, 
                                const CStdString& strFile)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    if (url.GetFileName() != strFolder)
    {
      url.SetFileName(AddFileToFolder(url.GetFileName(), strFile));
      return url.Get();
    }
  }

  CStdString strResult = strFolder;
  if (!strResult.IsEmpty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.Mid(1);
  else
    strResult += strFile;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    strResult.Replace('\\', '/');
  else
    strResult.Replace('/', '\\');

  return strResult;
}

CStdString URIUtils::GetDirectory(const CStdString &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == string::npos)
    return strFilePath.Left(iPosSlash + 1); // Only path

  return strFilePath.Left(iPosSlash + 1) + strFilePath.Mid(iPosBar); // Path + options
}

void URIUtils::CreateArchivePath(CStdString& strUrlPath,
                                 const CStdString& strType,
                                 const CStdString& strArchivePath,
                                 const CStdString& strFilePathInArchive,
                                 const CStdString& strPwd)
{
  CStdString strBuffer;

  strUrlPath = strType+"://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CURL::Encode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }

  strBuffer = strArchivePath;
  CURL::Encode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInArchive;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  CURL::Encode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}

string URIUtils::GetRealPath(const string &path)
{
  if (path.empty())
    return path;

  CURL url(path);
  url.SetHostName(GetRealPath(url.GetHostName()));
  url.SetFileName(resolvePath(url.GetFileName()));
  
  return url.Get();
}

std::string URIUtils::resolvePath(const std::string &path)
{
  if (path.empty())
    return path;

  size_t posSlash = path.find('/');
  size_t posBackslash = path.find('\\');
  string delim = posSlash < posBackslash ? "/" : "\\";
  vector<string> parts = StringUtils::Split(path, delim);
  vector<string> realParts;

  for (vector<string>::const_iterator part = parts.begin(); part != parts.end(); part++)
  {
    if (part->empty() || part->compare(".") == 0)
      continue;

    // go one level back up
    if (part->compare("..") == 0)
    {
      if (!realParts.empty())
        realParts.pop_back();
      continue;
    }

    realParts.push_back(*part);
  }

  CStdString realPath;
  int i = 0;
  // re-add any / or \ at the beginning
  while (path.at(i) == delim.at(0))
  {
    realPath += delim;
    i++;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) && realPath.at(realPath.size() - 1) != delim.at(0))
    realPath += delim;

  return realPath;
}

bool URIUtils::UpdateUrlEncoding(std::string &strFilename)
{
  if (strFilename.empty())
    return false;
  
  CURL url(strFilename);
  // if this is a stack:// URL we need to work with its filename
  if (URIUtils::IsStack(strFilename))
  {
    vector<CStdString> files;
    if (!CStackDirectory::GetPaths(strFilename, files))
      return false;

    for (vector<CStdString>::iterator file = files.begin(); file != files.end(); file++)
    {
      std::string filePath = *file;
      UpdateUrlEncoding(filePath);
      *file = filePath;
    }

    CStdString stackPath;
    if (!CStackDirectory::ConstructStackPath(files, stackPath))
      return false;

    url.Parse(stackPath);
  }
  // if the protocol has an encoded hostname we need to work with its hostname
  else if (URIUtils::ProtocolHasEncodedHostname(url.GetProtocol()))
  {
    std::string hostname = url.GetHostName();
    UpdateUrlEncoding(hostname);
    url.SetHostName(hostname);
  }
  else
    return false;

  std::string newFilename = url.Get();
  if (newFilename == strFilename)
    return false;
  
  strFilename = newFilename;
  return true;
}
