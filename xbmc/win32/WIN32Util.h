#pragma once

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

#include "URL.h"
#include "Cfgmgr32.h"
#include "MediaSource.h"
#include "guilib/Geometry.h"
#include "powermanagement/PowerManager.h"
#include "utils/Stopwatch.h"

enum Drive_Types
{
  ALL_DRIVES = 0,
  LOCAL_DRIVES,
  REMOVABLE_DRIVES,
  DVD_DRIVES
};

#define BONJOUR_EVENT             ( WM_USER + 0x100 )	// Message sent to the Window when a Bonjour event occurs.
#define BONJOUR_BROWSER_EVENT     ( WM_USER + 0x110 )

class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static CStdString URLEncode(const CURL &url);
  static CStdString GetLocalPath(const CStdString &strPath);
  static char FirstDriveFromMask (ULONG unitmask);
  static int GetDriveStatus(const CStdString &strPath, bool bStatusEx=false);
  static bool PowerManagement(PowerState State);
  static int BatteryLevel();
  static bool XBMCShellExecute(const CStdString &strPath, bool bWaitForScriptExit=false);
  static std::vector<CStdString> GetDiskUsage();
  static CStdString GetResInfoString();
  static int GetDesktopColorDepth();
  static CStdString GetSpecialFolder(int csidl);
  static CStdString GetSystemPath();
  static CStdString GetProfilePath();
  static CStdString UncToSmb(const CStdString &strPath);
  static CStdString SmbToUnc(const CStdString &strPath);
  static bool AddExtraLongPathPrefix(std::wstring& path);
  static bool RemoveExtraLongPathPrefix(std::wstring& path);
  static void ExtendDllPath();
  static HRESULT ToggleTray(const char cDriveLetter='\0');
  static HRESULT EjectTray(const char cDriveLetter='\0');
  static HRESULT CloseTray(const char cDriveLetter='\0');
  static bool EjectDrive(const char cDriveLetter='\0');
#ifdef HAS_GL
  static void CheckGLVersion();
  static bool HasGLDefaultDrivers();
  static bool HasReqGLVersion();
#endif
  static BOOL IsCurrentUserLocalAdministrator();
  static void GetDrivesByType(VECSOURCES &localDrives, Drive_Types eDriveType=ALL_DRIVES, bool bonlywithmedia=false);
  static std::string GetFirstOpticalDrive();
  static bool IsAudioCD(const CStdString& strPath);
  static CStdString GetDiskLabel(const CStdString& strPath);

  static bool Is64Bit();
  static LONG UtilRegGetValue( const HKEY hKey, const char *const pcKey, DWORD *const pdwType, char **const ppcBuffer, DWORD *const pdwSizeBuff, const DWORD dwSizeAdd );
  static bool UtilRegOpenKeyEx( const HKEY hKeyParent, const char *const pcKey, const REGSAM rsAccessRights, HKEY *hKey, const bool bReadX64= false );

  static bool GetCrystalHDLibraryPath(CStdString &strPath);

  static bool GetFocussedProcess(CStdString &strProcessFile);
  static void CropSource(CRect& src, CRect& dst, CRect target);

  static bool IsUsbDevice(const CStdStringW &strWdrive);

private:
  static DEVINST GetDrivesDevInstByDiskNumber(long DiskNumber);
};


class CWinIdleTimer : public CStopWatch
{
public:
  void StartZero();
};
