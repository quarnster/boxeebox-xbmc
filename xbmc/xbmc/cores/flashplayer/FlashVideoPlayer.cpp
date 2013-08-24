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


#include "FlashVideoPlayer.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "Settings.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/RenderFlags.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoTag.h"
#include "utils/SystemInfo.h"
#include "AppManager.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "utils/TimeUtils.h"
#include "../../lib/libBoxee/bxutils.h"
#include "../../lib/libBoxee/bxcurl.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"
#include "utils/SingleLock.h"
#include "GUIDialogProgress.h"
#ifdef CANMORE
#include "IntelSMDGlobals.h"
#include "../../lib/libBoxee/bxoemconfiguration.h"
#endif
#ifdef HAS_EMBEDDED
#include "../../BoxeeHalServices.h"
#endif
#include "GUIDialogYesNo2.h"
#include "GUIAudioManager.h"
#include "utils/Builtins.h"
#include "utils/GUIInfoManager.h"
#include "utils/CharsetConverter.h"
#include "GUIDialogBoxeeBrowserCtx.h"
#include "GUIDialogBoxeeDropdown.h"
#include "GUIDialogBoxeeSeekBar.h"
#include "FileSystem/FileCurl.h"
#include "FileSystem/DllLibCurl.h"

#include "xbmc/VideoReferenceClock.h"
#include "xbmc/utils/CPUInfo.h"
#include "lib/libBoxee/boxee.h"

#include "BoxeeUtils.h"

#ifdef _LINUX
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef WIN32
//#include "ffmpeg/libavutil/libm.h"
#include <math.h>
static inline const double round(double x)
{
  return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}
#endif

#include <vector>
#include <set>

#define HEIGHT 480
#define WIDTH 640

#include "MouseStat.h"
extern CMouseStat g_Mouse;

#ifdef HAS_INTEL_SMD
static bool bRenderLowRes;
#endif

#define MOVE_TIME_OUT 200L
#define MENU_CMD_WAIT_PERIOD 10000L

CFlashVideoPlayer::CFlashVideoPlayer(IPlayerCallback& callback)
: IPlayer(callback),
CThread()
{
  m_paused = false;
  m_playing = false;
  m_clock = 0;
  m_clockSet = false;
  m_lastTime = 0;
  m_speed = 1;
  m_handle = NULL;
  m_height = HEIGHT;
  m_width = WIDTH;
  m_cropTop = 0;
  m_cropBottom = 0;
  m_cropLeft = 0;
  m_cropRight = 0;
  m_isFullscreen = true;
  m_totalTime = 0;
  m_pDlgCache = NULL;

  m_canPause = false;
  m_canSkip = false;
  m_canSkipTo = false;
  m_canSetVolume = false;

  m_userStopped = true;
  m_bUserRequestedToStop = false;
  m_bFlashPlaybackEnded = false;

  m_bDrawMouse = true;
  m_bMouseActiveTimeout = false;
  m_bCloseKeyboard = false;

  m_bConfigChanged = true;
  m_bImageLocked = false;
  m_bDirty = false;

  m_bLockedPlayer = false;
  
  m_loadingPct = 0;
  m_pct = 0;
#ifdef HAS_XCOMPOSITE
  setenv("BOXEE_USE_XCOMPOSITE", "1", 1);
#endif
  m_lastMoveTime = 0;
  m_fSpeed = 5.0;
  m_fMaxSpeed = 60.0;
  m_fAcceleration = 1.5f;
  m_bMouseMoved = false;
  m_lastMoveUpdateTime = 0;
  m_bFullScreenPlayback = false;

  m_mediaPlayerInfo.pPlayer = NULL;
  m_mediaPlayerInfo.clear();

  m_mouseState = MOUSE_STATE_BROWSER_NORMAL;
#ifdef CANMORE
  m_sQtPluginPath = getenv("QT_PLUGIN_PATH");
  char qtPluginPath[128];
  strcpy(qtPluginPath, "/opt/local/qt-4.7/plugins");
  CLog::Log(LOGINFO, "current QT_PLUGIN_PATH is %s, changing to %s", m_sQtPluginPath.c_str(), qtPluginPath);
  setenv("QT_PLUGIN_PATH", qtPluginPath, 1);
#elif defined(_LINUX) && !defined (__APPLE__)
  CStdString path1 = _P("special://xbmc/system/qt/plugins");
  CStdString path2 = _P("special://xbmc/system/qt/linux/plugins");
  char qtPluginPath[1024];
  sprintf(qtPluginPath, "%s:%s", path1.c_str(), path2.c_str());
  CLog::Log(LOGINFO, "Changing QT_PLUGIN_PATH to: %s", qtPluginPath);
  setenv("QT_PLUGIN_PATH", qtPluginPath, 1);
#elif defined(__APPLE__)
  CStdString path1 = _P("special://xbmc/system/qt/plugins");
  CStdString path2 = _P("special://xbmc/system/qt/osx/plugins");
  char qtPluginPath[1024];
  sprintf(qtPluginPath, "%s:%s", path1.c_str(), path2.c_str());
  CLog::Log(LOGINFO, "Changing QT_PLUGIN_PATH to: %s", qtPluginPath);
  setenv("QT_PLUGIN_PATH", qtPluginPath, 1);
#elif defined (WIN32)
  CStdString path1 = _P("special://xbmc/system/qt/plugins");
  CStdString path2 = _P("special://xbmc/system/qt/win32/plugins");
  char qtPluginPath[1024];
  sprintf(qtPluginPath, "%s;%s", path1.c_str(), path2.c_str());
  CLog::Log(LOGINFO, "Changing QT_PLUGIN_PATH to: %s", qtPluginPath);
  SetEnvironmentVariable("QT_PLUGIN_PATH", qtPluginPath);
#endif

  m_seekTimeRequest = m_lastSeekTimeUpdateTime = 0;
  m_bUnAckedMenuCommand = true;
  m_menuCommandSentTime = CTimeUtils::GetFrameTime();
  m_bWebControl = false;
}

CFlashVideoPlayer::~CFlashVideoPlayer()
{

  if (!m_bWebControl && !m_bUserRequestedToStop)
    g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_EXCLAMATION,"", g_localizeStrings.Get(55413), TOAST_DISPLAY_TIME, KAI_RED_COLOR, KAI_RED_COLOR);

  CloseFile();

#ifdef CANMORE
  CLog::Log(LOGINFO, "setting QT_PLUGIN_PATH to %s", m_sQtPluginPath.c_str());
  setenv("QT_PLUGIN_PATH", m_sQtPluginPath.c_str(), 1);
#endif
}

void CFlashVideoPlayer::OSDExtensionClicked(int nId)
{
  if (m_handle)
    m_dll.FlashActivateExt(m_handle, nId);
}

bool CFlashVideoPlayer::OnAction(const CAction &action)
{
  // make sure keyboard closes
  if (m_bCloseKeyboard)
  {
    CGUIDialogKeyboard *dlg = (CGUIDialogKeyboard *)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    if (dlg) dlg->Close();
    m_bCloseKeyboard = false;
  }
  
  // handle menu/back
  // - if we are in "player mode", this presents the user with an option to go back to browser or leave (back to boxee)
  // - if we are in another mode - we show the browser's menu
  if (action.id == ACTION_BACKSPACE || action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR || action.id == ACTION_SHOW_GUI)
  {
    if (m_modesStack.size())
    {
      // return to previous mode
      CStdString mode = m_modesStack.back();
      m_modesStack.pop_back();

      Json::Value command;
      command["command"]= "SETMODE";
      command["parameters"]["mode"] = mode;
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }
    else if ((m_bDrawMouse && !m_bFullScreenPlayback) || m_mode == "keyboard")
    {
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"MENU\"}\n");
      m_bUnAckedMenuCommand = true;
      m_menuCommandSentTime = CTimeUtils::GetFrameTime();
    }
    else
    {
      return false;
    }
    return true;
  }

  bool bMouseClicked = (action.id == ACTION_MOUSE && g_Mouse.bClick[MOUSE_LEFT_BUTTON]);
  //printf("action: %d. clicked: %d. fullscreen: %d. mode: %s. path: %s\n", action.id, bMouseClicked, m_bFullScreenPlayback, m_mode.c_str(), m_path.c_str());
  // handle "select/enter"
  // even though - this should end up presenting the OSD when in "player mode" - we still pass it through to the browser
  // and wait for the browser to call us with "show OSD". this is done to allow the browser to set the playerState correctly
  if (action.id == ACTION_SELECT_ITEM || action.id == ACTION_SHOW_OSD ||  action.id == ACTION_SHOW_INFO || bMouseClicked)
  {
    if (m_mode != "player" && action.id != ACTION_SHOW_OSD)
    {
      // CLICK
      float x, y;
      if (TranslateMouseCoordinates(x, y))
        m_dll.FlashSendMouseClick(m_handle, (int)x, (int)y);
      else
        m_dll.FlashSendMouseClick(m_handle, 0, 0);
    }
    else
    {
      // OSD
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"MENU\"}\n");
      m_bUnAckedMenuCommand = true;
      m_menuCommandSentTime = CTimeUtils::GetFrameTime();
    }
    return true;
  }
  
  if (action.id == ACTION_BROWSER_FULL_SCREEN_ON)
  {
    m_dll.FlashJsonCommand(m_handle, "{\"command\":\"TOGGLE_FULLSCREEN\",\"parameters\":{\"full\":true}}\n");
    CloseOSD();
    return true;
  }
  else if (action.id == ACTION_BROWSER_FULL_SCREEN_OFF)
  {
    m_dll.FlashJsonCommand(m_handle, "{\"command\":\"TOGGLE_FULLSCREEN\",\"parameters\":{\"full\":false}}\n");
    CloseOSD();
    return true;
  }
  else if (action.id == ACTION_BROWSER_BACK)
  {
    m_dll.FlashJsonCommand(m_handle, "{\"command\":\"BROWSER_BACK\"}\n");
    return true;
  }
  else if (action.id == ACTION_BROWSER_FORWARD)
  {
    m_dll.FlashJsonCommand(m_handle, "{\"command\":\"BROWSER_FORWARD\"}\n");
    return true;
  }
  else if (action.id == ACTION_BROWSER_PLAYBACK_SKIP)
  {
    bool bFwd = action.amount1 > 0.0;
    bool bBig = fabs(action.amount1) > 1.0;

    Json::Value command;
    command["command"]= "BROWSER_SEEK";
    command["parameters"]["fwd"] = bFwd;
    command["parameters"]["big"] = bBig;

    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_BROWSER_EXT)
  {
    int nExt = (int)action.amount1;
    Json::Value command;
    command["command"]= "ACTIVATE_EXT";
    command["parameters"]["ext"] = nExt;

    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_STEP_BACK)
  { 
    if (m_bDrawMouse)
    {
      UpdateMouseSpeed(DIRECTION_LEFT);
      MoveMouse((int)-m_fSpeed, 0);
    }
    else if (m_mode == "keyboard")
    {
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"LEFTKEY\"}\n");
      return true;
    }
    else if (m_totalTime != 0)
      return false;
    else
    {
      Json::Value command;
      command["command"]= "REFRESH";
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }
    return true;
  }
  else if (action.id == ACTION_STEP_FORWARD)
  { 
    if (m_bDrawMouse)
    {
      UpdateMouseSpeed(DIRECTION_RIGHT);
      MoveMouse((int)m_fSpeed, 0);
    }
    else if (m_mode == "keyboard")
    {
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"RIGHTKEY\"}\n");
      return true;
    }
    else if (m_totalTime != 0)
    {
      return false;
    }
    else
    {
      Json::Value command;
      command["command"]= "REFRESH";
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }

    return true;
  }
  else if (action.id == ACTION_PAUSE)
  {
    Pause();
    return true;
  }
  else if (action.id == ACTION_MOUSE)
  {
    g_Mouse.SetState(m_mouseState);
    m_bMouseMoved = true;
    return true;
  }
  else if (action.id == ACTION_MOVE_UP || (action.id == ACTION_VOLUME_UP && action.amount2))
  { 
    if (m_bDrawMouse)
    {
      UpdateMouseSpeed(DIRECTION_UP);
      MoveMouse(0, (int)-m_fSpeed);
    }
    else if (m_mode == "keyboard")
    {
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"UPKEY\"}\n");
    }
    else
      return false;

    return true;
  }
  else if (action.id == ACTION_PAGE_UP)
  {
    m_dll.FlashScroll(m_handle, -100, 0);
    return true;
  }
  else if (action.id == ACTION_PAGE_DOWN)
  {
    m_dll.FlashScroll(m_handle, 100, 0);
    return true;
  }
  else if (action.id == ACTION_MUTE)
  {
    return false;
  }
  else if (action.id == ACTION_MOVE_DOWN || (action.id == ACTION_VOLUME_DOWN && action.amount2))
  { 
    if (m_bDrawMouse)
    {
      UpdateMouseSpeed(DIRECTION_DOWN);
      MoveMouse(0, (int)m_fSpeed);
    }
    else if (m_mode == "keyboard")
    {
      m_dll.FlashJsonCommand(m_handle, "{\"command\":\"DOWNKEY\"}\n");
    }
    else
      return false;
    return true;
  }
  else if (action.id == ACTION_BROWSER_SET_MODE)
  { 
    CloseOSD();
    
    Json::Value command;
    command["command"]= "SETMODE";
    command["parameters"]["mode"] = action.strAction.c_str();    
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());

    m_modesStack.push_back(m_mode);
    return true;
  }
  else if (action.id == ACTION_BROWSER_NAVIGATE)
  { 
    Json::Value command;
    command["command"]= "NAVIGATE";
    command["parameters"]["text"] = action.strAction.c_str(); // can be search term or url
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_BROWSER_TOGGLE_MODE)
  {
    Json::Value command;
    command["command"]= "TOGGLEMODE";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_BROWSER_THUMBNAIL_GENERATE)
  {
    Json::Value command;
    command["command"]= "THUMBNAIL_GENERATE";
    command["parameters"]["path"] = action.strAction.c_str(); // path to save thumbnail
    command["parameters"]["width"] = 320; //should be action.amount1
    command["parameters"]["height"] = 180; //should be action.amount2
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_BROWSER_RELOAD)
  {
    Json::Value command;
    command["command"]= "RELOAD";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_BROWSER_STOP_LOADING)
  {
    Json::Value command;
    command["command"]= "ABORT";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_SHOW_INFO)
  {
    if (m_bDrawMouse)
    {
      // CLICK
      float x, y;
      if (TranslateMouseCoordinates(x, y))
        m_dll.FlashSendMouseClick(m_handle, (int)x, (int)y);
      else
        m_dll.FlashSendMouseClick(m_handle, 0, 0);

      return true;
    }
    else
    {
      return false;
    }
  }
  else if (action.id == ACTION_BROWSER_TOGGLE_QUALITY)
  {
    Json::Value command;
    command["command"]= "TOGGLE_QUALITY";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    return true;
  }
  else if (action.id == ACTION_VOLUME_UP || action.id == ACTION_VOLUME_DOWN)
  {
    return false;
  }
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return false;
  }
  else
  {
    if ((action.id & 0xFF00) == KEY_VKEY)
    {
      uint8_t b = action.id & 0xFF;
      if (b == 0x08)
      {
        m_dll.FlashSendKeyStroke(m_handle, action.id ^ KEY_VKEY);
      }
    }
    else if ((action.id & 0xFF00) == KEY_ASCII)
    {
      m_dll.FlashSendKeyStroke(m_handle, action.id ^ KEY_ASCII);
    }

    if(m_mediaPlayerInfo.pPlayer)
      m_mediaPlayerInfo.pPlayer->OnAction(action);

    return true;
  }

  return false;
}

void CFlashVideoPlayer::ParseFlashURL(const CStdString &strUrl, std::vector<CStdString> &vars, std::vector<CStdString> &values)
{
  CURI url(strUrl);
  CLog::Log(LOGDEBUG,"%s - parsing <%s>", __FUNCTION__, strUrl.c_str());

  std::set<CStdString> keys;
  if (url.GetProtocol().Equals("flash", false))
  {
    CStdString strFileName = url.GetFileName();
    if (strFileName.size() && strFileName[0] == '?')
      strFileName = strFileName.Mid(1);

    std::vector<CStdString> params;
    CUtil::Tokenize(strFileName, params, "&");

    for (size_t nParam = 0; nParam < params.size(); nParam++)
    {
      CStdString strParam = params[nParam];
      CStdString strKey,strVal;
      int nPos = strParam.Find("=");
      if (nPos > 0)
      {
        strKey = strParam.Mid(0, nPos);
        strVal = strParam.Mid(nPos+1);
      }
      else 
      {
        strKey = "dummy";
        strVal = strParam;
      }

      if (strKey == "src")
      {
        vars.push_back(strKey);
        CStdString strToPush = strVal; 
        values.push_back(strToPush);       
      }
      else
      {
        if (strKey == "bx-croptop")
          m_cropTop = atoi(strVal.c_str());
        if (strKey == "bx-cropbottom")
          m_cropBottom = atoi(strVal.c_str());
        if (strKey == "bx-cropleft")
          m_cropLeft = atoi(strVal.c_str());
        if (strKey == "bx-cropright")
          m_cropRight = atoi(strVal.c_str());
        vars.push_back(strKey);
        values.push_back(strVal);        
      }

      keys.insert(strKey);
    }
  }

  if (keys.find("src") == keys.end() && keys.find("source") == keys.end())
  {
    vars.push_back("src");
    values.push_back(strUrl.c_str());    
  }
}

bool CFlashVideoPlayer::Launch(const CFileItem& file, const CPlayerOptions &options)
{
  if (!m_dll.Load())
  {
    CLog::Log(LOGERROR,"failed to load flashlib");
    return false;
  }

  CGUIDialogProgress *dlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dlg && dlg->IsCanceled())
  {
    CloseFile();
    return false;
  }

  if (!m_bWebControl)
    m_pDlgCache = new CDlgCache(500, g_localizeStrings.Get(10214), file.GetLabel());
  
  std::vector<CStdString> params;
  std::vector<CStdString> values;

  m_path = file.m_strPath;
  
  //
  // if we are getting an app:// link - we let the app manager launch it.
  // at this point - the player's state is "loading" with progress indication so whatever the app manager does it needs
  // to update the player (probably via python callback)
  //
  if (!m_bWebControl)
  {
    if (file.IsApp() && !file.HasProperty("ActualPlaybackPath"))
    {
      CLog::Log(LOGDEBUG,"%s - launching app (path= <%s>)", __FUNCTION__, file.m_strPath.c_str());
      g_application.CurrentFileItem().SetProperty("WaitPlaybackPath", true);
      ThreadMessage tMsg (TMSG_LAUNCH_APP);
      tMsg.strParam = file.m_strPath;
      g_application.getApplicationMessenger().SendMessage(tMsg, false);
      return true;
    }

    g_application.CurrentFileItem().SetProperty("WaitPlaybackPath", false);

    if (file.IsApp() && file.GetPropertyBOOL("AddToHistory"))
    {
      CFileItem copy = file;
      copy.ClearProperty("actualplaybackpath"); // this is a temp path and should not be set in order for history playback to work correctly (go through the whole playback procedure)
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(copy);
    }
  }

  CStdString strActualPath = file.m_strPath;
  if (file.HasProperty("ActualPlaybackPath"))
    strActualPath = file.GetProperty("ActualPlaybackPath");

  m_urlSource = "other";
  if (file.HasProperty("url_source"))
    m_urlSource = file.GetProperty("url_source");

  //strActualPath = "http://www.youtube.com/watch?v=Wo85qZm-Zuk&fmt=18";
  //strActualPath = "http://www.hulu.com/watch/182712/saturday-night-live-the-miley-cyrus-show";
  CLog::Log(LOGINFO,"about to play url %s", strActualPath.c_str());

  ParseFlashURL(strActualPath, params, values);
  params.push_back("bx-title");
  values.push_back(file.GetLabel().c_str());
  params.push_back("bx-lang");
  values.push_back(g_guiSettings.GetString("locale.language"));
  if (file.HasVideoInfoTag())
  {
    CStdString strShow = file.GetVideoInfoTag()->m_strShowTitle;
    if (strShow.IsEmpty())
      strShow = file.GetVideoInfoTag()->m_strTitle;
    int nSeason = file.GetVideoInfoTag()->m_iSeason;
    int nEpisode = file.GetVideoInfoTag()->m_iEpisode;
    CStdString strSeason, strEpisode;
    strSeason.Format("%d",nSeason);
    strEpisode.Format("%d",nEpisode);

    params.push_back("bx-show");
    values.push_back(strShow);
    params.push_back("bx-season");
    values.push_back(strSeason);
    params.push_back("bx-episode");
    values.push_back(strEpisode);
    params.push_back("bx-plot");
    values.push_back(file.GetVideoInfoTag()->m_strPlot);
    params.push_back("bx-plot2");
    values.push_back(file.GetVideoInfoTag()->m_strPlotOutline);
    params.push_back("bx-studio");
    values.push_back(file.GetVideoInfoTag()->m_strStudio);
  }
  
  params.push_back("bx-cookiejar");
  values.push_back(BOXEE::BXCurl::GetCookieJar());
  params.push_back("bx-thumb");
  values.push_back(_P(file.GetThumbnailImage()));
  params.push_back("url_source");
  values.push_back(m_urlSource);
  // control java script with oem.config
  params.push_back("bx-loadjs");
  if (m_urlSource == "browser-app")
  {
    values.push_back("false");
  }
  else
  {
    values.push_back("true");
  }

#ifdef HAS_BOXEE_HAL

  /********************************************
  *                                           *
  *  params for SystemInfo Browser Extension  *
  *                                           *
  ********************************************/

  //mandatory

  CHalHardwareInfo hwInfo;
  if (CHalServicesFactory::GetInstance().GetHardwareInfo(hwInfo))
  {
    /**
     * serialnumber
     * String
     * The device's unique serial number or any other unique ID string
     */
    params.push_back("bx-serialnumber");
    values.push_back(hwInfo.serialNumber);

    /**
     * vendor
     * String
     * Name of the manufacturer
     */
    params.push_back("bx-vendor");
    values.push_back(hwInfo.vendor);
  }
  else
  {
    params.push_back("bx-serialnumber");
    values.push_back("error retrieving serialnumber");
    params.push_back("bx-vendor");
    values.push_back("error retrieving vendor");
  }

  CHalSoftwareInfo swInfo;
  if (CHalServicesFactory::GetInstance().GetSoftwareInfo(swInfo))
  {
    /**
     * firmwareversion
     * String
     * Version number of installed firmware image
     */
    params.push_back("bx-firmwareversion");
    values.push_back(swInfo.version);
  }
  else
  {
    params.push_back("bx-firmwareversion");
    values.push_back("error retrieving firmwareversion");
  }

  /**
   * productname
   * String
   * The device's model designation
   */
  params.push_back("bx-productname");
  values.push_back(BoxeeUtils::GetPlatformStr());

  /**
   * resolution
   * Number
   * Approximate native resolution available for the web browser.
   * May be different from the device's output resolution
   */
  params.push_back("bx-resolution");
  values.push_back("2"); //720p/i-approx. 1280x720 px

  CHalWirelessConfig wirelessConfig;
  if (CHalServicesFactory::GetInstance().GetWirelessConfig(0, wirelessConfig))
  {
    /**
     * network
     * Number
     * Indicating the active networking mode
     */
    params.push_back("bx-network");
    if (wirelessConfig.addr_type != ADDR_NONE)
    {
      values.push_back("1"); //WiFi
    }
    else
    {
      values.push_back("0"); //LAN
    }
  }
  else
  {
    params.push_back("bx-network");
    values.push_back("error retrieving network"); //error
  }

  //optional members

  /**
   * techplatform
   * String
   * Indicating the active networking mode
   */
  params.push_back("bx-techplatform");
  values.push_back("STB"); //Set Top Box

  /**
   * secureSerialNumber
   * String
   * Indicating the active networking mode
   */
  params.push_back("bx-secureSerialNumber");
  values.push_back("not implemented yet");

  /**
   * debug_free
   * String
   * Indicating the active networking mode
   */
  params.push_back("bx-debug_free");
  values.push_back("not implemented yet");
#endif

  char **argn = new char* [params.size()];
  char **argv = new char* [params.size()];

  for (size_t i=0; i<params.size(); i++)
  {
    argn[i] = strdup(params[i].c_str());

    if (values[i].size() > 4096)
      values[i] = values[i].substr(0,4096);

    argv[i] = strdup(values[i].c_str());    

    if (strcasecmp(argn[i], "height") == 0)
    {
      m_height = atoi(argv[i]);
    }

    if (strcasecmp(argn[i], "width") == 0)
    {
      m_width = atoi(argv[i]);
    }
  }

  m_isFullscreen = options.fullscreen;

  m_bConfigChanged = false;

  // setup cache folder for persistent browser info
#ifdef _LINUX
#ifdef CANMORE
  // don't want to save cache on the nand
  setenv("BOXEE_CACHE_FOLDER","/tmp/browser_cache",1);
#else
  setenv("BOXEE_CACHE_FOLDER",_P("special://profile/browser").c_str(),1);
#endif
  setenv("BOXEE_COOKIE_JAR",_P("special://profile/browser/cookies.dat").c_str(),1);
  setenv("BOXEE_BROWSER_STORAGE_FOLDER",_P("special://profile/browser/localstorage/").c_str(),1);
#else
  SetEnvironmentVariable("BOXEE_CACHE_FOLDER",_P("special://profile/browser").c_str());
  SetEnvironmentVariable("BOXEE_COOKIE_JAR",_P("special://profile/browser/cookies.dat").c_str());
  SetEnvironmentVariable("BOXEE_BROWSER_STORAGE_FOLDER",_P("special://profile/browser/localstorage/").c_str());
#endif

  g_infoManager.SetPageLoadProgress(0);
  m_loadingPct = 0;
  m_pct = 0;

 RESOLUTION iRes = g_graphicsContext.GetVideoResolution();
 // x and y not smaller than 0 for now
 if (m_bWebControl)
 {
   m_screenX1 = (int)m_rect.x1;
   m_screenY1 = (int)m_rect.y1;
   m_screenX2 = (int)m_rect.x2;
   m_screenY2 = (int)m_rect.y2;
   m_screenWidth = m_screenX2 - m_screenX1;
   m_screenHeight = m_screenY2 - m_screenY1;
 }
 else
 {
   m_screenX1 = std::max(g_settings.m_ResInfo[iRes].Overscan.left, 0);
   m_screenY1 = std::max(g_settings.m_ResInfo[iRes].Overscan.top, 0);
   m_screenX2 = g_settings.m_ResInfo[iRes].Overscan.right;
   m_screenY2 = g_settings.m_ResInfo[iRes].Overscan.bottom;
   // width and height not bigger than screen width and height
   m_screenWidth = std::min(m_screenX2 - m_screenX1, g_settings.m_ResInfo[iRes].iWidth);
   m_screenHeight = std::min(m_screenY2 - m_screenY1, g_settings.m_ResInfo[iRes].iHeight);
 }

  m_handle = m_dll.FlashCreate(m_width*4);
  if (m_handle)
    m_dll.FlashSetDestRect(m_handle, m_screenX1, m_screenY1, m_screenWidth, m_screenHeight);

  m_dll.FlashSetWorkingPath(m_handle, PTH_IC("special://xbmc/system/players/flashplayer"));
  m_dll.FlashSetCrop(m_handle, m_cropTop, m_cropBottom, m_cropLeft, m_cropRight);
  m_dll.FlashSetCallback(m_handle, this);
  if (!m_dll.FlashOpen(m_handle, params.size(), (const char **)argn, (const char **)argv))
  {
    m_dll.FlashClose(m_handle);
    m_handle = NULL;
  }

  //set mouse location to center of screen

  XBMC_Event newEvent;                            
  newEvent.type = XBMC_MOUSEMOTION;               
                                                  
  newEvent.motion.y = (m_screenY2 - m_screenY1)/2;
  newEvent.motion.x = (m_screenX2 - m_screenX1)/2;
                                                  
  g_Mouse.HandleEvent(newEvent);                  
  g_Mouse.SetState(m_mouseState);                 
  m_bMouseMoved = true;
                                                  

  for (size_t p=0;p<params.size();p++)
  {
    free(argn[p]);    
    free(argv[p]);    
  }

  delete [] argn;
  delete [] argv;

  if (!m_handle)
  {
    if (m_pDlgCache)
      m_pDlgCache->Close();
    m_pDlgCache = NULL;
    return false;
  }

  if (file.HasVideoInfoTag())
  {
    std::vector<CStdString> tokens;
    CUtil::Tokenize(file.GetVideoInfoTag()->m_strRuntime, tokens, ":");
    int nHours = 0;
    int nMinutes = 0;
    int nSeconds = 0;

    if (tokens.size() == 2)
    {
      nMinutes = atoi(tokens[0].c_str());
      nSeconds = atoi(tokens[1].c_str());
    }
    else if (tokens.size() == 3)
    {
      nHours = atoi(tokens[0].c_str());
      nMinutes = atoi(tokens[1].c_str());
      nSeconds = atoi(tokens[2].c_str());
    }
    m_totalTime = (nHours * 3600) + (nMinutes * 60) + nSeconds;
  }

  Create();
  if( options.starttime > 0 )
    SeekTime( (__int64)(options.starttime * 1000) );

#ifdef HAS_INTEL_SMD
  bRenderLowRes = g_graphicsContext.GetRenderLowresGraphics();
  g_graphicsContext.SetRenderLowresGraphics(false);
#endif

  return true;
}

bool CFlashVideoPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  return Launch(file, options);
}

bool CFlashVideoPlayer::LoadURL(const CStdString& url, const CRect &rect)
{
  m_bWebControl = true;
  CPlayerOptions options;
  options.fullscreen = true;
  options.starttime = 0;
  CFileItem file;
  file.m_strPath= url;
  m_rect = rect;
  return Launch(file, options);

}

bool CFlashVideoPlayer::CloseFile()
{
  if (!m_bFlashPlaybackEnded)
    m_bUserRequestedToStop = true;

  int locks = ExitCriticalSection(g_graphicsContext);

  m_bStop = true;
  if (m_bCloseKeyboard)
  {
    CGUIDialogKeyboard *dlg = (CGUIDialogKeyboard *)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    if (dlg) dlg->Close();
    m_bCloseKeyboard = false;
  }

  CSingleLock lock(m_lock);  
  if (m_bImageLocked)
  {
    m_dll.FlashUnlockImage(m_handle);
    m_bImageLocked = false;
  }  
  lock.Leave();

  StopThread();
  if (locks > 0)
    RestoreCriticalSection(g_graphicsContext, locks);

  if (m_handle)
    m_dll.FlashClose(m_handle);
  m_handle = NULL;

  g_renderManager.UnInitBrowser();

  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL;

  m_mediaPlayerInfo.clear();
  
#ifdef HAS_INTEL_SMD
  g_graphicsContext.SetRenderLowresGraphics(bRenderLowRes);
#endif

  return true;
}

void CFlashVideoPlayer::CheckConfig()
{
  CSingleLock lock(m_lock);  
  if (m_bConfigChanged)
  {
    if (m_pDlgCache)
      m_pDlgCache->Close();
    m_pDlgCache = NULL;

    if (!m_playing)
    {      
      g_renderManager.PreInitBrowser();
      m_playing = true;
    }

    unsigned flags = 0;

    if(m_mediaPlayerInfo.pPlayer == NULL)
      flags = CONF_FLAGS_EXTERN_IMAGE;

    if (m_isFullscreen)
    {
      flags |= CONF_FLAGS_FULLSCREEN;
    }

    CRect destRect(0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight());
    g_renderManager.Configure(m_width, m_height, m_width, m_height, 30.0f, flags, destRect);

    m_bConfigChanged = false;

  }
}

bool CFlashVideoPlayer::PreRender()
{
  CheckConfig();

  if (m_playing && m_handle)
  {
    m_dll.FlashSetDestRect(m_handle, m_screenX1, m_screenY1, m_screenWidth, m_screenHeight);
  }

#ifndef HAS_INTELCE
  if (m_playing)
  {  
    if (!m_bImageLocked)
    {
      m_dll.FlashLockImage(m_handle);
      m_bImageLocked = true;
    }

    RESOLUTION iRes = g_graphicsContext.GetVideoResolution();

    int offsetX = g_settings.m_ResInfo[iRes].Overscan.left;
    int offsetY = g_settings.m_ResInfo[iRes].Overscan.top;
    int screenWidth = g_settings.m_ResInfo[iRes].Overscan.right - g_settings.m_ResInfo[iRes].Overscan.left ;
    int screenHeight = g_settings.m_ResInfo[iRes].Overscan.bottom - g_settings.m_ResInfo[iRes].Overscan.top ;

    if (m_handle)
    {
      // must release the lock because set dest rect locks
      if (m_bImageLocked)
      {
        m_bImageLocked = false;
        m_dll.FlashUnlockImage(m_handle);
      }
      
      m_dll.FlashSetDestRect(m_handle, offsetX, offsetY, screenWidth, screenHeight);
      
      if (!m_bImageLocked)
      {
        m_dll.FlashLockImage(m_handle);
        m_bImageLocked = true;
      }
      
    }
    
    int nHeight = m_dll.FlashGetHeight(m_handle)  - m_cropTop - m_cropBottom;
    int nWidth = m_dll.FlashGetWidth(m_handle)  - m_cropLeft - m_cropRight;

    CSingleLock lock(m_lock);
    char *buf = NULL;
    if (m_bDirty)
    {
      buf = (char*)m_dll.FlashGetImage(m_handle);
	  g_renderManager.SetRGB32Image(buf, nHeight, nWidth, m_dll.FlashGetWidth(m_handle) * 4);
    }

    m_bDirty = false;
  }

  return m_playing;
#else
  return true;
#endif
}

void CFlashVideoPlayer::PostRender()
{
#ifndef HAS_INTELCE
  CSingleLock lock(m_lock);
  if (m_bImageLocked)
  {
    m_bImageLocked = false;
    m_dll.FlashUnlockImage(m_handle);
  }  
#endif
}

void CFlashVideoPlayer::Ping()
{
  if (m_pDlgCache && m_pDlgCache->IsCanceled())
  {
    CloseFile();
    return;    
  }

  CheckConfig();
}

bool CFlashVideoPlayer::IsPlaying() const
{
  return m_playing;
}

bool CFlashVideoPlayer::PrintPlayerInfo()
{
  CStdString audioStr, videoStr, strGeneral;
  m_mediaPlayerInfo.pPlayer->GetAudioInfo(audioStr);
  {
    CLog::Log(LOGINFO, "audio info %s\n", audioStr.c_str());
  }

  // show video codec info
  m_mediaPlayerInfo.pPlayer->GetVideoInfo(videoStr);
  {
    CLog::Log(LOGINFO, "video info %s\n", videoStr.c_str());
  }
  // show general info
  m_mediaPlayerInfo.pPlayer->GetGeneralInfo(strGeneral);
  {
    CStdString strGeneralFPS;
#ifdef __APPLE__
    // We show CPU usage for the entire process, as it's arguably more useful.
    double dCPU = m_resourceCounter.GetCPUUsage();
    CStdString strCores;
    strCores.Format("cpu: %.0f%%", dCPU);
#else
    CStdString strCores = g_cpuInfo.GetCoresUsageString();
#endif
    //int missedvblanks;
    //int refreshrate;
    //double clockspeed;
    CStdString strClock;

//    if (g_VideoReferenceClock.GetClockInfo(missedvblanks, clockspeed, refreshrate))
//      strClock.Format("S( refresh:%i missed:%i speed:%+.3f%% %s )"
//                     , refreshrate
//                     , missedvblanks
//                     , clockspeed - 100.0
//                     , g_renderManager.GetVSyncState().c_str());

    strGeneralFPS.Format("%s\nW( fps:%02.2f %s ) %s"
                       , strGeneral.c_str()
                       , g_infoManager.GetFPS()
                       , strCores.c_str(), strClock.c_str() );
#ifdef HAS_INTEL_SMD
    strGeneralFPS.Format("%s\nW( DVDPlayer: %.2f SMD Audio: %.2f SMD Video %.2f %s ) %s"
                             , strGeneral.c_str()
                             , (float)(g_application.m_pPlayer->GetTime() / 1000.0)
                             ,  g_IntelSMDGlobals.IsmdToDvdPts(g_IntelSMDGlobals.GetAudioCurrentTime())/1000000
                             ,  g_IntelSMDGlobals.IsmdToDvdPts(g_IntelSMDGlobals.GetVideoCurrentTime())/1000000
                             , strCores.c_str()
                             , strClock.c_str() );
#endif
    CLog::Log(LOGINFO,"%s\n", strGeneralFPS.c_str());
  }

  return true;
}

void CFlashVideoPlayer::Process()
{
  m_clock = 0;
  m_clockSet = false;
  m_lastTime = CTimeUtils::GetTimeMS();
  bool bUpdatePlayer = false;

  g_Mouse.SetEnabled(true);

  while (!m_bStop)
  {
    if (m_playing && m_clockSet)
    {      
      if (!m_paused)
        m_clock += (CTimeUtils::GetTimeMS() - m_lastTime)*m_speed;
      m_lastTime = CTimeUtils::GetTimeMS();      
    }

    if (m_bDrawMouse && m_bMouseMoved &&
        CTimeUtils::GetFrameTime() - m_lastMoveUpdateTime > MOVE_TIME_OUT)
    {
      float x, y;
      if (TranslateMouseCoordinates(x, y))
        m_dll.FlashSendMouseMove(m_handle, (int)x, (int)y);
      else
        m_dll.FlashSendMouseMove(m_handle, 0, 0);

      m_bMouseMoved = false;
      m_lastMoveUpdateTime = CTimeUtils::GetFrameTime();
    }

    int diff = CTimeUtils::GetFrameTime() - m_lastSeekTimeUpdateTime;
    if (m_seekTimeRequest && diff > 400)
    {
      Json::Value command;
      command["command"]= "BROWSER_SEEK_TO";
      command["parameters"]["time"] = (unsigned int)m_seekTimeRequest;
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
      m_seekTimeRequest = 0;
      bUpdatePlayer = true;
    }
    else if (!m_seekTimeRequest  && bUpdatePlayer && diff > 4000)
    {
      bUpdatePlayer = false;
      Json::Value command;
      command["command"]= "REFRESH";
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
      if(!IsPaused())
        g_infoManager.m_performingSeek = false;
    }


    if (m_bUnAckedMenuCommand &&
        CTimeUtils::GetFrameTime() - m_menuCommandSentTime > MENU_CMD_WAIT_PERIOD)
    {
      std::vector<CStdString> vecParams;
      vecParams.push_back("OSD");

      ThreadMessage tMsg ( TMSG_GUI_INVOKE_FROM_BROWSER, 0,  0, "", vecParams, "", NULL, this );
      g_application.getApplicationMessenger().SendMessage(tMsg, false);
      m_bUnAckedMenuCommand = false;
    }
	
    if (m_handle) 
#ifndef CANMORE
      m_dll.FlashUpdate(m_handle, 30000);
#else
      // On the canmore, since we do not copy image over shared memory, we don't get enough
      // image updates, which causes the FPS to be really low. We need to sleep less and do more.
      // This brings fps in full screen to around 30fps.
      m_dll.FlashUpdate(m_handle, 30000);
#endif

    if (m_mediaPlayerInfo.pPlayer && m_mediaPlayerInfo.pPlayer->IsCaching() &&
             CTimeUtils::GetFrameTime() - m_lastMoveUpdateTime > MOVE_TIME_OUT * 5)
    {
      m_lastMoveUpdateTime = CTimeUtils::GetFrameTime();

      Json::Value command;
      Json::FastWriter writer;

      command["command"]= "MEDIAPLAYER.BufferingResp";
      command["parameters"]["result"] = m_mediaPlayerInfo.pPlayer->IsCaching();
      command["parameters"]["level"] = m_mediaPlayerInfo.pPlayer->GetCacheLevel();
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }
  } //while !m_bStop

  g_Mouse.SetEnabled(g_Mouse.IsEnabledInSettings());

  m_playing = false;

  if (m_bStop)
  {
    if (m_userStopped)
    {
      if (g_application.m_pPlayer == this)
      {
        m_callback.OnPlayBackStopped();
	  }
    }
    else
    {
      m_callback.OnPlayBackEnded();
    }
  }      
}

void CFlashVideoPlayer::Pause()
{
  if(CanPause())
  {
    if (m_paused)
    {
      m_dll.FlashPlay(m_handle);
    }
    else
    {
      m_dll.FlashPause(m_handle);
    }

    m_paused = !m_paused;
  }
}

bool CFlashVideoPlayer::IsPaused() const
{
  return m_paused && m_bFullScreenPlayback;
}

bool CFlashVideoPlayer::HasVideo() const
{
  return true;
}

bool CFlashVideoPlayer::HasAudio() const
{
  return true;
}

void CFlashVideoPlayer::SwitchToNextLanguage()
{
}

void CFlashVideoPlayer::ToggleSubtitles()
{
}

bool CFlashVideoPlayer::CanSeek()
{
  return CanSkip();
}

bool CFlashVideoPlayer::CanSeekToTime()
{
  return (CanSeek() && m_canSkipTo);
}

void CFlashVideoPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if(CanSeek())
  {
    Json::Value command;
    command["command"]= "BROWSER_SEEK";
    command["parameters"]["fwd"] = bPlus;
    command["parameters"]["big"] = bLargeStep;
    
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
}

void CFlashVideoPlayer::ToggleFrameDrop()
{
}

void CFlashVideoPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  if( m_mediaPlayerInfo.pPlayer )
  {
    CStdString nfo;
    m_mediaPlayerInfo.pPlayer->GetAudioInfo(nfo);
    strAudioInfo.Format( "FLASH: %s", nfo.c_str() );
  }
  else
    strAudioInfo = "CFlashVideoPlayer - nothing to see here";
}

void CFlashVideoPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  if( m_mediaPlayerInfo.pPlayer )
  {
    CStdString nfo;
    m_mediaPlayerInfo.pPlayer->GetVideoInfo(nfo);
    strVideoInfo.Format( "FLASH: %dx%d %s", m_width, m_height, nfo.c_str() );
  }
  else
    strVideoInfo.Format("Width: %d, height: %d", m_width, m_height);
}

void CFlashVideoPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "CFlashVideoPlayer ("+m_path+")";
}

void CFlashVideoPlayer::SwitchToNextAudioLanguage()
{
}

void CFlashVideoPlayer::SetVolume(long nVolume) 
{
#ifdef CANMORE
  g_IntelSMDGlobals.SetMasterVolume(nVolume);
  return;
#endif

  if (g_guiSettings.GetBool("audiooutput.controlmastervolume") == true)
  {
    return;
  }

  if(!CanSetVolume())
  {
    g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_EXCLAMATION,"",g_localizeStrings.Get(51673),2000, KAI_ORANGE_COLOR, KAI_ORANGE_COLOR);
  }
  else
  {
    int nPct = (int)fabs(((float)(nVolume - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)) * 100.0);
    m_dll.FlashSetVolume(m_handle, nPct);    
  }
}

void CFlashVideoPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalMsec = GetTotalTime() * 1000;
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);
  SeekTime(iTime);
}

float CFlashVideoPlayer::GetPercentage()
{
  return (float)m_pct ; 
}

//This is how much audio is delayed to video, we count the oposite in the dvdplayer
void CFlashVideoPlayer::SetAVDelay(float fValue)
{
}

float CFlashVideoPlayer::GetAVDelay()
{
  return 0.0f;
}

void CFlashVideoPlayer::SetSubTitleDelay(float fValue)
{
}

float CFlashVideoPlayer::GetSubTitleDelay()
{
  return 0.0;
}

void CFlashVideoPlayer::SeekTime(__int64 iTime)
{
  if (!m_canSkipTo)
  {
    return;
  }

  m_seekTimeRequest = iTime;
  m_clock = m_seekTimeRequest;
  m_lastSeekTimeUpdateTime  = CTimeUtils::GetFrameTime();

  return;
}

// return the time in milliseconds
__int64 CFlashVideoPlayer::GetTime()
{
  return m_clock;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CFlashVideoPlayer::GetTotalTime()
{
  return m_totalTime;
}

void CFlashVideoPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

void CFlashVideoPlayer::ShowOSD(bool bOnoff)
{
}

CStdString CFlashVideoPlayer::GetPlayerState()
{
  return "";
}

bool CFlashVideoPlayer::SetPlayerState(CStdString state)
{
  return true;
}

void CFlashVideoPlayer::Render()
{
}

bool CFlashVideoPlayer::CanPause() const
{
  return m_canPause;
}

bool CFlashVideoPlayer::CanSkip() const
{
  return m_canSkip;
}

bool CFlashVideoPlayer::CanSetVolume() const
{
#ifdef CANMORE
  return true;
#endif
  if (g_guiSettings.GetBool("audiooutput.controlmastervolume") == false)
    return m_canSetVolume;

  return false;
}

//static bool rectSet = false;

void CFlashVideoPlayer::FlashProcessCommand(const char *command)
{
  std::string strCmd(command);
  Json::Value cmd;
  Json::Reader reader;
  //printf("BROWSER COMMAND\n***\n%s\n***\n", command); fflush(0);
  bool bOk = reader.parse( strCmd, cmd );

  if (!bOk)
  {
    CLog::Log(LOGERROR, "can not parse command <%s>\n", command);
    return;
  }

  std::string cmdType = cmd.get("command","NONE").asString();

  if (cmdType.compare("END") == 0)
  {
    m_bUserRequestedToStop = true;
    m_dll.FlashUpdatePlayerCount(m_handle, false);
    FlashPlaybackEnded();
  }

  else if (cmdType.compare("MEDIAPLAYER.GetStatusUpdate") == 0)
  {
    /**
     * get cummulative status update from the player:
     * currentTime
     * buffered
     * isBuffering
     */

    if(m_mediaPlayerInfo.pPlayer)
    {
        //currentTime
        m_mediaPlayerInfo.currentTime = m_mediaPlayerInfo.pPlayer->GetTime()/1000;

        //buffered
        int newBuffered = m_mediaPlayerInfo.pPlayer->GetPositionWithCache();
        if(newBuffered > m_mediaPlayerInfo.buffered)
        {
            m_mediaPlayerInfo.buffered = newBuffered;
        }

        //isBuffering
        m_mediaPlayerInfo.isBuffering = m_mediaPlayerInfo.pPlayer->IsCaching();

        Json::FastWriter writer;
        Json::Value command;
        Json::Value parameters = cmd["parameters"];
        command["command"]= "MEDIAPLAYER.GetStatusUpdateResp";
        command["parameters"]["playerID"] = (Json::Int)m_mediaPlayerInfo.playerID;
        command["parameters"]["buffered"] = (Json::Int)m_mediaPlayerInfo.buffered;
        command["parameters"]["currentTime"] = (Json::Int)m_mediaPlayerInfo.currentTime;
        command["parameters"]["isBuffering"] = m_mediaPlayerInfo.isBuffering;
        command["parameters"]["isPaused"] = m_mediaPlayerInfo.paused;
        m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }
  }
  else if (cmdType.compare("START") == 0 )
  {
    m_dll.FlashUpdatePlayerCount(m_handle, true);
    m_callback.OnPlayBackStarted();
    m_bDirty = true;
  }

  else if (cmdType.compare("PAUSED") ==0 )
  {
    m_paused = true;
  }
  else if (cmdType.compare("RESUMED") == 0)
  {
    m_paused = false;
  }
  else if (cmdType.compare("PROGRESS") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    int nPct = paremeters.get("percent",0).asInt();
    m_pct = nPct;
  }
  else if (cmdType.compare("LOADPROGRESS") == 0 )
  {
    m_paused = false;
    Json::Value paremeters = cmd["parameters"];
    int nPct = paremeters.get("percent",0).asInt();
    g_infoManager.SetPageLoadProgress(nPct);

    if (m_loadingPct == 100 && nPct < 100)
    {
      // start load. not first page
      g_application.CurrentFileItem().Reset();
      g_application.CurrentFileItem().SetProperty("isbrowser",!m_bLockedPlayer);
    }
    else if (m_loadingPct < 100 && nPct == 100)
    {
      // check if we need to reproduce the thumb
      CStdString location = paremeters.get("location","").asString();
      CLog::Log(LOGDEBUG, "loaded %s", location.c_str());
      if (!m_bWebControl && BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInWebFavorites(location))
      {
        CLog::Log(LOGDEBUG, "site %s is in favorites", location.c_str());
        CFileItem tmpItem;
        if (location != "" && location != "about:blank")
          tmpItem.m_strPath = location;
        CLog::Log(LOGDEBUG, "thumb for %s is %s", location.c_str(), _P(tmpItem.GetCachedVideoThumb()).c_str());
        // if the thumb doesn't exist do
        if (!XFILE::CFile::Exists(_P(tmpItem.GetCachedVideoThumb()).c_str()))
        {
          CLog::Log(LOGDEBUG, "can't find thumb for %s", location.c_str());

          Json::Value command;
          command["command"]= "THUMBNAIL_GENERATE";
          command["parameters"]["path"] =  _P(tmpItem.GetCachedVideoThumb());
          command["parameters"]["width"] = 320; //should be action.amount1
          command["parameters"]["height"] = 180; //should be action.amount2
          Json::FastWriter writer;
          m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
        }
      }
      else
      {
        CLog::Log(LOGDEBUG, "site %s is not a favorite", location.c_str());
      }
    }
    m_loadingPct = nPct;
  }
  else if (cmdType.compare("TIME") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    int nTime = paremeters.get("time",0).asInt();
    m_clock = nTime * 1000;
    m_clockSet = true;
  }

  else if (cmdType.compare("DURATION") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    int nTime = paremeters.get("duration",0).asInt();
    m_totalTime = nTime;
  }

  else if (cmdType.compare("CONFIG") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    int nWidth = paremeters.get("width",0).asInt();
    int nHeight = paremeters.get("height",0).asInt();

    m_dll.FlashSetHeight(m_handle, nHeight);
    m_dll.FlashSetWidth(m_handle, nWidth);

    CSingleLock lock(m_lock);

    int h = nHeight  - m_cropTop - m_cropBottom;
    int w = nWidth  - m_cropLeft - m_cropRight;

    m_width = w;
    m_height = h;

    m_bConfigChanged = true;
    m_bDirty = true;
  }

  else if (cmdType.compare("EXTON") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    int nID = paremeters.get("ID",0).asInt();
    std::string text = paremeters.get("text",0).asString();
    std::string thumb = paremeters.get("thumb",0).asString();

    CFileItem& f = g_application.CurrentFileItem();
    CStdString extPref;
    extPref.Format( "osd-ext-%d-", nID );
    f.SetProperty(extPref+"text", BOXEE::BXUtils::URLDecode(text).c_str());
    f.SetProperty(extPref+"thumb", BOXEE::BXUtils::URLDecode(thumb).c_str());
    f.SetProperty(extPref+"on", true);
  }
  else if (cmdType.compare("EXTOFF") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    int nID = paremeters.get("ID",0).asInt();

    CFileItem& f = g_application.CurrentFileItem();
    CStdString extPref;
    extPref.Format( "osd-ext-%d-", nID );
    f.SetProperty(extPref+"on", false);

  }
  else if (cmdType.compare("NOTIFY") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    std::string text = paremeters.get("text","").asString();
    std::string thumb = paremeters.get("thumb","").asString();
    int nTimeout = paremeters.get("timeout",0).asInt();

    if (text.empty() || thumb.empty())
      return;

    CStdString strDecoded = BOXEE::BXUtils::URLDecode(text);

    CLog::Log(LOGDEBUG,"notification from flash: <%s> (%s)", text.c_str(), strDecoded.c_str());

    g_application.m_guiDialogKaiToast.QueueNotification(BOXEE::BXUtils::URLDecode(thumb), "", strDecoded, nTimeout*1000);

  }
  else if (cmdType.compare("NOTIFYERROR") == 0)
  {
    Json::Value paremeters = cmd["parameters"];

    std::string reason = paremeters.get("reason",Json::Value("ssl")).asString();
    std::string location = paremeters.get("location",Json::Value("")).asString();
    CLog::Log(LOGINFO,"notify error from flash for site %s, type %s", reason.c_str(), location.c_str());

    if (reason.compare("ssl") == 0)
    {
      if (!m_bWebControl)
      {
        g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(57101));
      }
    }
  }
  else if (cmdType.compare("GETTEXT") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    std::string title = paremeters.get("title",0).asString();
    std::string callback = paremeters.get("callback",0).asString();
    std::string hidden = paremeters.get("hidden",0).asString();
    std::string initialValue = paremeters.get("initialValue",0).asString();

    std::vector<CStdString> vecParams;
    vecParams.push_back("Keyboard");
    vecParams.push_back(title);
    vecParams.push_back(callback);
    vecParams.push_back(hidden);
    vecParams.push_back(initialValue);

    ThreadMessage tMsg ( TMSG_GUI_INVOKE_FROM_BROWSER, 0,  0, "", vecParams, "", NULL, this );
    g_application.getApplicationMessenger().SendMessage(tMsg, false);

  }
  else if (cmdType.compare("OPENDIALOG") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    std::string type  = paremeters.get("type",0).asString();
    std::string header = paremeters.get("header",0).asString();
    std::string line = paremeters.get("line",0).asString();
    std::string callback = paremeters.get("callback",0).asString();

    std::vector<CStdString> vecParams;
    vecParams.push_back(type);
    vecParams.push_back(header);
    vecParams.push_back(line);
    vecParams.push_back(callback);

    ThreadMessage tMsg ( TMSG_GUI_INVOKE_FROM_BROWSER, 0,  0, "", vecParams, "", NULL, this );
    g_application.getApplicationMessenger().SendMessage(tMsg, false);

  }

  else if (cmdType.compare("CANPAUSE") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    bool canPause = paremeters.get("canPause",0).asBool();
    m_canPause = canPause;
  }

  else if (cmdType.compare("CANSKIP") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    bool canSkip = paremeters.get("canSkip",0).asBool();
    m_canSkip = canSkip;
  }
  else if (cmdType.compare("CANSETVOLUME") == 0)
  {
    Json::Value paremeters = cmd["parameters"];
    bool canSetVolume = paremeters.get("canSetVolume",0).asBool();
    m_canSetVolume = canSetVolume;
  }
  else if (cmdType.compare("MODE") == 0 )
  {
    bool bCurrDrawMouse = m_bDrawMouse;
    bool bCurrMouseActiveTimeout = m_bMouseActiveTimeout;
    Json::Value paremeters = cmd["parameters"];
    m_bDrawMouse = paremeters.get("drawMouse", bCurrDrawMouse).asBool();
    m_bMouseActiveTimeout = paremeters.get("activeTimeout", bCurrMouseActiveTimeout).asBool();
    m_mode = paremeters.get("modeName", "cursor").asString();

    m_bLockedPlayer = paremeters.get("lockPlayer", false).asBool();

    if (paremeters.get("clearstack", false).asBool())
      m_modesStack.clear();

    if (m_bLockedPlayer)
    {
      CFileItem &item = g_application.CurrentFileItem();
      item.SetProperty("isbrowser", false);
    }
  }
  else if (cmdType.compare("TITLE_CHANGED") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    std::string title = paremeters.get("title","").asString();
    std::string location = paremeters.get("location","").asString();
    if (!m_bWebControl)
    {
      g_application.CurrentFileItem().SetProperty("page-title", title);
      g_application.GetBoxeeBrowseHistoryList().AddLinkToHistory(location, title);
    }
  }
  else if (cmdType.compare("STATUS_CHANGED") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    std::string status = paremeters.get("status","").asString();
    g_application.CurrentFileItem().SetProperty("page-status", status);
  }
  else if (cmdType.compare("BUILTIN") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    std::string builtin = paremeters.get("cmd","").asString();
    CBuiltins::Execute(builtin);
  }
  else if (cmdType.compare("MOUSE") == 0 )
  {
    Json::Value paremeters = cmd["parameters"];
    int x = paremeters.get("xcoord","0").asUInt();
    int y = paremeters.get("ycoord","0").asUInt();

    XBMC_Event newEvent;
    newEvent.type = XBMC_MOUSEMOTION;

                                /* not below zero, not greater than size */
    newEvent.motion.y = std::max(m_screenY1, std::min(m_screenY2 - 10, y));
    newEvent.motion.x = std::max(m_screenX1, std::min(m_screenX2 - 10, x));

    g_Mouse.HandleEvent(newEvent);
    g_Mouse.SetState(m_mouseState);
    m_bMouseMoved = true;
  }
  else if (cmdType.compare("REFRESH_STATE") == 0 || cmdType.compare("UPDATE_STATE") == 0)
  {
    m_bUnAckedMenuCommand = false;
    Json::Value paremeters = cmd["parameters"];

    m_bFullScreenPlayback = paremeters.get("fullscreen",m_bLockedPlayer).asBool();
    
    bool bCanSeek= paremeters.get("canseek",false).asBool();
    bool bCanPause= paremeters.get("canpause",false).asBool();
    bool bCanSetFullScreen= !m_bLockedPlayer;
    bool bShowOSD = paremeters.get("showosd",true).asUInt();
    bool bHideSeek = paremeters.get("hideseekbar",false).asBool();
    bool bCanSeekTo = paremeters.get("canseekto",false).asBool();
    
    std::string strPrevUrl = paremeters.get("prevurl","").asString();
    std::string strNextUrl = paremeters.get("nexturl","").asString();
    std::string strStatusLine = paremeters.get("statusline","").asString();
    std::string strTitle = paremeters.get("title","").asString();
    std::string strUrl = paremeters.get("url","").asString();

    // extensions
    int bCanToggleQuality = paremeters.get("cantogglequality",false).asBool();
    int bShowingHD = paremeters.get("currentqualityishd",false).asBool();
    if (bCanToggleQuality)
    {
      CFileItem& f = g_application.CurrentFileItem();
      f.SetProperty("browsercantogglequality", bCanToggleQuality);
      f.SetProperty("browsercurrentqualityishd", bShowingHD);
    }
    
    int nProgress = paremeters.get("progress","").asInt();
    int nDuration = paremeters.get("duration","").asInt();
    int nTime     = paremeters.get("time","").asInt();

    m_paused = paremeters.get("paused",m_paused).asBool();
    
    if (nProgress > 0)
    {
      m_pct = nProgress;
    }
    
    if (nTime >= 0)
    {
      m_clock = nTime * 1000;
      m_clockSet = true;
    }
  
    if (nDuration > 0)
    {
      m_totalTime = nDuration;
    }

    CFileItem &item = g_application.CurrentFileItem();
    item.SetProperty("browserfullscreen", m_bFullScreenPlayback);
    item.SetProperty("browsercanseek",bCanSeek);
    item.SetProperty("browsercanpause",bCanPause);
    item.SetProperty("browsercansetfullscreen",bCanSetFullScreen);
    item.SetProperty("browserprevurl",strPrevUrl);
    item.SetProperty("browsernexturl",strNextUrl);
    item.SetProperty("browserhasprevurl", !strPrevUrl.empty());
    item.SetProperty("browserhasnexturl", !strNextUrl.empty());
    item.SetProperty("isbrowser",!m_bLockedPlayer);
    item.SetProperty("isloaded",true);
    item.SetProperty("IsInternetStream",true);
    item.SetProperty("browsercursormode", m_bDrawMouse);
    item.SetProperty("hideseekbar",bHideSeek);
    item.SetProperty("load-failed", strUrl == "about:blank");
    if (strUrl == "about:blank" && !m_bWebControl)
      bShowOSD = true;

    // hack to keep initial title
    if (!item.HasVideoInfoTag())
      item.SetLabel(strTitle);

    if (strUrl != "about:blank" && strUrl != "")
      item.m_strPath = strUrl;
    item.SetContentType("text/html");

    m_canPause = bCanPause;
    m_canSkip = bCanSeek;
    m_canSkipTo = bCanSeekTo;

    if (m_bWebControl)
      return;

    CGUIDialogBoxeeCtx *pBrowserOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSER_CTX);
    CGUIDialogBoxeeCtx *pPlayerOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_CTX);
    CGUIDialogBoxeeCtx *pOSD = pBrowserOSD;

    if (cmdType.compare("UPDATE_STATE") == 0)
    {
      if (bShowOSD)
      {
        // choose correct browser and make sure the other one is closed
        if (m_bFullScreenPlayback || m_bLockedPlayer)
        {
          pOSD = pPlayerOSD;
          if (pBrowserOSD && pBrowserOSD->IsDialogRunning())
            pBrowserOSD->Close();
        }
        else
        {
          if (pPlayerOSD && pPlayerOSD->IsDialogRunning())
            pPlayerOSD->Close();
        }

        if (pOSD)
          pOSD->SetItem(item);

        // only relaunch if not yet open
        CGUIDialogBoxeeSeekBar* pSeekBar = (CGUIDialogBoxeeSeekBar*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SEEK_BAR);
        if (pOSD && !pOSD->IsDialogRunning() && pSeekBar && !pSeekBar->IsDialogRunning())
        {
          pOSD->SetAutoClose(3000);

          ThreadMessage tMsg (TMSG_GUI_DO_MODAL);
          tMsg.lpVoid = pOSD;
          tMsg.dwParam1 = (DWORD)pOSD->GetID();

          g_application.getApplicationMessenger().SendMessage(tMsg, false);
        }
      }
      else
        CloseOSD();
    }
    else //refresh state
    {
      if (m_bFullScreenPlayback || m_bLockedPlayer)
      {
        pOSD = pPlayerOSD;
      }
      if (pOSD)
      {
        pOSD->SetItem(item);
      }
    }
  }
  else if ( cmdType.compare("MEDIAPLAYER.Load") == 0 )
  {
    Json::Value parameters = cmd["parameters"];
    int playerID = parameters.get("playerID", "").asInt();
    CStdString strPath = parameters.get("url","").asString();
    CStdString cookies = parameters.get("cookies", "").asString();
    CStdString userAgent = parameters.get("user-agent", "").asString();

    bool isVideo = parameters.get("IsVideo", Json::Value(true)).asBool();

    CLog::Log(LOGINFO ,"%s url %s, is video %d\n", cmdType.c_str(), strPath.c_str(), isVideo);

    // video tag supports the following also:
    //  autoplay: starts playing at startup; should be handled by webkit
    //  controls: osd setup; should be handled by webkit
    //  loop: loop the video when done; should be handled by webkit
    //  preload: loaded on page load; should be handled by webkit

    // We don't handle multiple videos at the moment; this will close up the previous stream
    m_mediaPlayerInfo.clear();

    //set new playerID
    m_mediaPlayerInfo.playerID = playerID;

    // Set any domain specific cookies for this content
    CURL_HANDLE* handle = g_curlInterface.easy_init();
    CStdString cookiejar = BOXEE::BXCurl::GetCookieJar();
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIEFILE, cookiejar.c_str());
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIEJAR, cookiejar.c_str());
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIELIST, "FLUSH");

    std::vector<CStdString> tokens;
    CUtil::Tokenize( cookies, tokens, "\n");
    std::vector<CStdString>::const_iterator it = tokens.begin();
    while( it != tokens.end() )
    {
      g_curlInterface.easy_setopt(handle, CURLOPT_COOKIELIST , (*it).c_str());
      it++;
    }

    // Flush cookies and clean our handle. it's not the last curl reference here,
    // and our cookies will persist
    (void)g_curlInterface.easy_setopt(handle,CURLOPT_COOKIELIST, "FLUSH");
    g_curlInterface.easy_cleanup(handle);

    // Now create a DVDPlayer
    
    if (isVideo)
      m_mediaPlayerInfo.pPlayer = new CDVDPlayer(*this);
    else
      m_mediaPlayerInfo.pPlayer = new CDVDPlayer/*PAPlayer*/(*this);

    if(m_mediaPlayerInfo.pPlayer)
    {
      m_mediaPlayerInfo.opening = true;

      // set the current item
      userAgent = "User-Agent: " + userAgent;

      CURI url(strPath);
      CStdString fileName = url.GetFileName();

      if (CUtil::GetExtension(fileName) == ".m3u8")
      {
        CUtil::URLEncode(strPath);
        strPath = CStdString("playlist://") + strPath;
        m_mediaPlayerInfo.item.m_strPath = strPath;
        m_mediaPlayerInfo.item.SetContentType("application/octet-stream");
      }
      else
      {
        m_mediaPlayerInfo.item.m_strPath = strPath;
      }

      m_mediaPlayerInfo.item.SetExtraInfo(userAgent);
      m_mediaPlayerInfo.item.SetProperty("DisableBoxeeUI", true);
      m_mediaPlayerInfo.item.SetProperty("HTML5Media", true);

      // open the file and start buffering
      CPlayerOptions opts;
      if( !m_mediaPlayerInfo.pPlayer->OpenFile(m_mediaPlayerInfo.item, opts) )
      {
        m_mediaPlayerInfo.opening = false; //clear struct later
      }
    }
    else
    {
      m_mediaPlayerInfo.opening = false;
    }

    // now send up all of the status on the file
    Json::Value command;
    Json::FastWriter writer;
    
    if(m_mediaPlayerInfo.opening)
    {
      //opening was successful
      m_mediaPlayerInfo.loaded = true;

      command["command"]= "MEDIAPLAYER.LoadFinished";
      command["parameters"]["hasaudio"] = m_mediaPlayerInfo.pPlayer->HasAudio();
      command["parameters"]["hasvideo"] = m_mediaPlayerInfo.pPlayer->HasVideo();
      command["parameters"]["subcount"] = m_mediaPlayerInfo.pPlayer->GetSubtitleCount() > 0;
      command["parameters"]["duration"] = m_mediaPlayerInfo.pPlayer->GetTotalTime();  // in seconds
      CLog::Log(LOGINFO ,"new stream duration is %d", m_mediaPlayerInfo.pPlayer->GetTotalTime());
      command["parameters"]["starttime"] = m_mediaPlayerInfo.pPlayer->GetStartTime();
      command["parameters"]["cachelevel"] = m_mediaPlayerInfo.pPlayer->GetCacheLevel();
      command["parameters"]["videowidth"] = m_mediaPlayerInfo.pPlayer->GetPictureWidth();
      command["parameters"]["videoheight"] = m_mediaPlayerInfo.pPlayer->GetPictureHeight();
      command["parameters"]["playerID"] = (Json::Int)m_mediaPlayerInfo.playerID;
    }
    else
    {
      //openning failed
      m_mediaPlayerInfo.clear();
      command["command"]= "MEDIAPLAYER.LoadFailure";
    }

    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.CancelLoad") == 0 )
  {
    m_mediaPlayerInfo.clear();
  }
  else if ( cmdType.compare("MEDIAPLAYER.Play") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());

    if(m_mediaPlayerInfo.pPlayer && m_mediaPlayerInfo.paused)
    {
      m_mediaPlayerInfo.pPlayer->Pause();
    }
//  else
//  {
//    printf("\n***** %s::%s(%d)\tSKIPPING m_mediaPlayerInfo.pPlayer->Pause()\n", __FILE__, __FUNCTION__, __LINE__);
//  }
  }
  else if ( cmdType.compare("MEDIAPLAYER.Pause") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    if( m_mediaPlayerInfo.pPlayer && !m_mediaPlayerInfo.paused )
    {
      m_mediaPlayerInfo.pPlayer->Pause();
    }
//  else
//  {
//    printf("\n***** %s::%s(%d)\tSKIPPING m_mediaPlayerInfo.pPlayer->Pause()\n", __FILE__, __FUNCTION__, __LINE__);
//  }
  }
  else if ( cmdType.compare("MEDIAPLAYER.Close") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    m_mediaPlayerInfo.clear();
  }
  else if ( cmdType.compare("MEDIAPLAYER.NaturalSize") == 0 )
  {
    Json::Value parameters = cmd["parameters"];

    // no idea.
    Json::Value command;
    command["command"]= "MEDIAPLAYER.NaturalSizeResp";
    command["parameters"]["result"] = 55555555;
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    
  }
  else if ( cmdType.compare("MEDIAPLAYER.Seek") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    if( m_mediaPlayerInfo.pPlayer )
    {
      float t = cmd["parameters"]["position"].asDouble();
      CLog::Log(LOGINFO ,"%s %f\n", cmdType.c_str(), t);

      if (t != 0.0)
      {
        m_mediaPlayerInfo.pPlayer->SeekPercentage(t);
      }
//      else
//      {
//        printf("\n***** %s::%s(%d)\tMEDIAPLAYER.Seek position %f - ignored\n", __FILE__, __FUNCTION__, __LINE__, t);
//      }
    }
//    else
//    {
//      printf("\n***** %s::%s(%d)\tMEDIAPLAYER.Seek ignored\n", __FILE__, __FUNCTION__, __LINE__);
//    }
  }
  else if ( cmdType.compare("MEDIAPLAYER.SetVolume") == 0 )
  {
    // hard to know without scale. assume pct
    if( m_mediaPlayerInfo.pPlayer )
    {
      std::string vol = cmd["parameters"]["volume"].asString();
      float val = VOLUME_MAXIMUM;
      sscanf( vol.c_str(), "%f", &val );
      
      int setting = (long)(val * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);
     
      m_mediaPlayerInfo.pPlayer->SetVolume(setting);
      m_mediaPlayerInfo.preMuteVolume = setting;
    }
  }
  else if ( cmdType.compare("MEDIAPLAYER.SetMuted") == 0 )
  {
    if( m_mediaPlayerInfo.pPlayer )
    {
      if( m_mediaPlayerInfo.muted )
      {
        m_mediaPlayerInfo.pPlayer->SetVolume(m_mediaPlayerInfo.preMuteVolume);
        m_mediaPlayerInfo.muted = false;
      }
      else
      {
        m_mediaPlayerInfo.pPlayer->SetVolume(VOLUME_MINIMUM);
        m_mediaPlayerInfo.muted = true;
      }
    }
  }
  else if ( cmdType.compare("MEDIAPLAYER.NetworkState") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    Json::Value parameters = cmd["parameters"];

    bool res = false; // TODO fixthis
    Json::Value command;
    command["command"]= "MEDIAPLAYER.NetworkStateResp";
    command["parameters"]["result"] = res;
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.ReadyState") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    Json::Value parameters = cmd["parameters"];

    bool res = false; // TODO fixthis
    Json::Value command;
    command["command"]= "MEDIAPLAYER.ReadyStateResp";
    command["parameters"]["result"] = res;
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.MaxTimeSeekable") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    Json::Value parameters = cmd["parameters"];

    CStdString t;
    t.Format("%d", m_mediaPlayerInfo.pPlayer ? m_mediaPlayerInfo.pPlayer->GetTotalTime() : 0);

    Json::Value command;
    command["command"]= "MEDIAPLAYER.MaxTimeSeekableResp";
    command["parameters"]["result"] = t;
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.BytesLoaded") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());
    Json::Value parameters = cmd["parameters"];

    Json::Value command;
    command["command"]= "MEDIAPLAYER.BytesLoadedResp";
    command["parameters"]["result"] = 0;
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.Paint") == 0 )
  {
    // this gives a GraphicsContext and an IntRect so we should set our view rect here?
    CLog::Log(LOGDEBUG ,"%s \n", cmdType.c_str());
    Json::Value parameters = cmd["parameters"];
    
    int width = (int)(m_scaleX * (double)parameters.get("width", g_graphicsContext.GetWidth()).asInt());
    int height = (int)(m_scaleY * parameters.get("height", g_graphicsContext.GetHeight()).asInt());
    int xpos = parameters.get("x", (int)g_graphicsContext.GetViewPort().x1).asInt();
    int ypos = parameters.get("y", (int)g_graphicsContext.GetViewPort().y1).asInt();

    if (m_scaleY > 1.0)
    {
      ypos -= m_screenY1;
      ypos  = (int)round((double)ypos * m_scaleY) ;
      ypos += m_screenY1;
    }

    if (m_scaleX > 1.0)
    {
      xpos -= m_screenX1;
      xpos  = (int)round((double)xpos * m_scaleX) ;
      xpos += m_screenX1;
    }

    // Set up the viewing window
    g_graphicsContext.PushTransform(TransformMatrix(), true);
    g_graphicsContext.SetViewWindow((float)xpos, (float)ypos, (float)(xpos+width), (float)(ypos+height));
    g_graphicsContext.PopTransform();
  }
  else if ( cmdType.compare("MEDIAPLAYER.HasAvailableVideoFrame") == 0 )
  {
    CLog::Log(LOGINFO ,"%s \n", cmdType.c_str());

    Json::Value parameters = cmd["parameters"];

    Json::Value command;
    command["command"]= "MEDIAPLAYER.HasAvailableVideoFrameResp";
    command["parameters"]["result"] = "true";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if ( cmdType.compare("MEDIAPLAYER.SupportsAcceleratedRendering") == 0 )
  {
    // TODO format specific; need to filter ogg
    Json::Value parameters = cmd["parameters"];

    Json::Value command;
    command["command"]= "MEDIAPLAYER.SupportsAcceleratedRenderingResp";
    command["parameters"]["result"] = "true";
    Json::FastWriter writer;
    m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
  }
  else if (cmdType.compare("LINK_HOVERED") == 0)
  {
    Json::Value parameters = cmd["parameters"];
    bool linkHovered = parameters.get("linkhovered",0).asBool();
    m_mouseState = (linkHovered ? MOUSE_STATE_BROWSER_HAND : MOUSE_STATE_BROWSER_NORMAL);
    g_Mouse.SetState(m_mouseState);
  }
  else if (cmdType.compare("BROWSERSIZE") == 0)
  {
    Json::Value parameters = cmd["parameters"];
    unsigned int browserWidth = parameters.get("width",1280).asUInt();
    unsigned int browserHeight = parameters.get("height",720).asUInt();
    m_scaleX = (double)m_screenWidth / (double)browserWidth;
    m_scaleY = (double)m_screenHeight / (double)browserHeight;
  }
  else if (cmdType.compare("DROPDOWN.Choose") == 0)
  {
    std::vector<CStdString> vecParams;
    vecParams.push_back("Dropdown");
    ThreadMessage tMsg ( TMSG_GUI_INVOKE_FROM_BROWSER, 0,  0, "", vecParams, cmd["parameters"].toStyledString(), NULL, this );
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }
  else if (cmdType.compare("REQUEST_SHUTDOWN") == 0)
  {
    Json::Value parameters = cmd["parameters"];
    CStdString cookies = parameters.get("cookies", "").asString();

    CURL_HANDLE* handle = g_curlInterface.easy_init();
    CStdString cookiejar = BOXEE::BXCurl::GetCookieJar();
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIEFILE, cookiejar.c_str());
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIEJAR, cookiejar.c_str());
    g_curlInterface.easy_setopt(handle, CURLOPT_COOKIELIST, "FLUSH");

    std::vector<CStdString> tokens;
    CUtil::Tokenize( cookies, tokens, "\n");
    std::vector<CStdString>::const_iterator it = tokens.begin();
    while( it != tokens.end() )
    {
      g_curlInterface.easy_setopt(handle, CURLOPT_COOKIELIST , (*it).c_str());
      it++;
    }
    (void)g_curlInterface.easy_setopt(handle,CURLOPT_COOKIELIST, "FLUSH");
    g_curlInterface.easy_cleanup(handle);


    m_callback.OnPlayBackEnded();
  }
  else if (cmdType.compare("WINDOW_CREATE") == 0)
  {
    Json::Value parameters = cmd["parameters"];
    CStdString url = parameters.get("url", "").asString();

    std::vector<CStdString> vecParams;
    vecParams.push_back("Popup");
    vecParams.push_back(url);

    ThreadMessage tMsg ( TMSG_GUI_INVOKE_FROM_BROWSER, 0,  0, "", vecParams, "", NULL, this );
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }
}

void CFlashVideoPlayer::FlashNewFrame() 
{
  CSingleLock lock(m_lock);
  m_bDirty = true;
  g_application.NewFrame();
}

void CFlashVideoPlayer::FlashPlaybackEnded()
{
  m_bFlashPlaybackEnded = true;
  m_userStopped = false;
  ThreadMessage tMsg (TMSG_MEDIA_STOP);

  // We need to set this parameter to indicate that we should not switch to previous window after stop
  tMsg.dwParam1 = 1;
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void CFlashVideoPlayer::ProcessExternalMessage(/*ThreadMessage */  void *pMsg)
{
    ThreadMessage  *p_tMsg = (ThreadMessage *)pMsg;

    std::vector<CStdString> vecParams = p_tMsg->params;
    CStdString strCmdType = vecParams[0];

    if (strCmdType == "Keyboard")
    {
      CStdString strTitle = vecParams[1];
      CStdString strCallback = vecParams[2];
      CStdString strText = vecParams[4];

      m_bCloseKeyboard = true;

      bool bMouseEnabled = g_Mouse.IsEnabled();
      g_Mouse.SetEnabled(false);
      bool bConfirmed = CGUIDialogKeyboard::ShowAndGetInput(strText, strTitle, true, (vecParams[3] == "true"));
      g_Mouse.SetEnabled(bMouseEnabled);

      m_bCloseKeyboard = false;
      m_dll.FlashUserText(m_handle, BOXEE::BXUtils::URLEncode(strText.c_str()).c_str(), strCallback.c_str(), bConfirmed);
    }
    else if (strCmdType == "YesNo")
    {
      CStdString strHeader = vecParams[1];
      CStdString strLine = vecParams[2];
      CStdString strCallback = vecParams[3];

      bool bMouseEnabled = g_Mouse.IsEnabled();
      g_Mouse.SetEnabled(false);

      bool bConfirmed = CGUIDialogYesNo2::ShowAndGetInput(strHeader, strLine);

      g_Mouse.SetEnabled(bMouseEnabled);
      m_dll.FlashOpenDialog(m_handle,strCallback.c_str(), bConfirmed);
    }
    else if (strCmdType == "Dropdown")
    {
      bool bMouseEnabled = g_Mouse.IsEnabled();
      g_Mouse.SetEnabled(false);

      Json::Value parameters;
      Json::Reader reader;
      bool bOk = reader.parse(p_tMsg->strParam2, parameters);
      if (!bOk)
      {

      }

      CStdString selectedItem;
      CFileItemList itemsList;

      for (int i = 0; i < (int) parameters.size(); i++)
      {
        if (parameters[i]["type"].asString() == "option")
        {
          CFileItemPtr item(new CFileItem(parameters[i]["value"].asString()));
          item->SetProperty("value", parameters[i]["value"].asString());
          if (parameters[i]["selected"].asBool() == true)
            selectedItem = parameters[i]["value"].asString();
          itemsList.Add(item);
        }
        else if (parameters[i]["type"].asString() == "group")
        {
          CFileItemPtr item(new CFileItem(parameters[i]["value"].asString()));
          item->SetProperty("value", parameters[i]["value"].asString());
          item->SetProperty("isseparator", true);
          itemsList.Add(item);
        }
      }
      CGUIDialogBoxeeBrowserDropdown *dialog = (CGUIDialogBoxeeBrowserDropdown*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSER_DROPDOWN);
      if (dialog)
      {
        dialog->Show(itemsList, "drop down", selectedItem,0.0,15.0,2,NULL);
      }

      g_Mouse.SetEnabled(bMouseEnabled);

      Json::Value command, commandResults;
      command["command"]= "DROPDOWN.Reply";

      command["result"][0u] = selectedItem;
      Json::FastWriter writer;
      m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
    }
    else if (strCmdType == "ExitApp")
    {

      bool bMouseEnabled = g_Mouse.IsEnabled();
      g_Mouse.SetEnabled(false);

      bool response, rc;
      if (m_urlSource == "browser-app")
        rc = CGUIDialogYesNo2::ShowAndGetInput(0, 55404, response);
      else
        rc = CGUIDialogYesNo2::ShowAndGetInput(0, 55412, response);

      g_Mouse.SetEnabled(bMouseEnabled);
      m_bUserRequestedToStop = true;
      if (rc)
      {
        m_dll.FlashClose(m_handle);
      }
    }
    else if (strCmdType == "Popup")
    {
      bool bMouseEnabled = g_Mouse.IsEnabled();
      g_Mouse.SetEnabled(false);

      CStdString url;
      CUtil::MakeShortenPath(vecParams[1], url, 100);
      bool response;
      CStdString header;
      header.Format(g_localizeStrings.Get(55417).c_str(), url.c_str());

      bool rc = CGUIDialogYesNo2::ShowAndGetInput(g_localizeStrings.Get(55416), header, g_localizeStrings.Get(55419), g_localizeStrings.Get(55418), response);
      if (rc)
      {
        Json::Value command;
        command["command"]= "NAVIGATE";
        command["parameters"]["text"] = vecParams[1];
        Json::FastWriter writer;
        m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
      }
      g_Mouse.SetEnabled(bMouseEnabled);
    }
}

//Returns true if not playback (paused or stopped beeing filled)
bool CFlashVideoPlayer::IsCaching() const
{

  if( m_loadingPct < 100 )
    return true;
  else
    return false;
}

//Cache filled in Percent
int CFlashVideoPlayer::GetCacheLevel() const
{
  if( m_loadingPct < 100 )
    return m_loadingPct;
  else if( m_mediaPlayerInfo.pPlayer && m_mediaPlayerInfo.pPlayer->IsCaching() )
    return m_mediaPlayerInfo.pPlayer->GetCacheLevel();
  else if( m_mediaPlayerInfo.pPlayer && m_mediaPlayerInfo.opening )
    return 5; // 5pct loaded until we finish open
  else
    return 100;
}

void CFlashVideoPlayer::UpdateMouseSpeed(int nDirection)
{
  if (nDirection == m_nDirection && CTimeUtils::GetFrameTime() - m_lastMoveTime <= MOVE_TIME_OUT)
  { // accelerate
    m_fSpeed += m_fAcceleration;
    if (m_fSpeed > m_fMaxSpeed) m_fSpeed = m_fMaxSpeed;
  }
  else
  { // reset direction and speed
    m_fSpeed = 4;
    m_nDirection = nDirection;
  }
  m_lastMoveTime = CTimeUtils::GetFrameTime();
}

void CFlashVideoPlayer::MoveMouse(int iX, int iY)
{
  CPoint p = g_Mouse.GetLocation();
  XBMC_Event newEvent;
  newEvent.type = XBMC_MOUSEMOTION;

                              /* not below zero, not greater than size */
  newEvent.motion.y = std::max(m_screenY1, std::min(m_screenY2 - 10, (int)(p.y + iY)));
  newEvent.motion.x = std::max(m_screenX1, std::min(m_screenX2 - 10, (int)(p.x + iX)));

  if (iX != 0 && newEvent.motion.x == (uint16_t)p.x)
  {

    if (iX < 0)
      m_dll.FlashScroll(m_handle, -50, 0);
    else
      m_dll.FlashScroll(m_handle, 50, 0);
  }

  if (iY != 0 && newEvent.motion.y == (uint16_t)p.y)
  {
    if (iY < 0)
      m_dll.FlashScroll(m_handle, 0, -50);
    else
      m_dll.FlashScroll(m_handle, 0, 50);
  }

  g_Mouse.HandleEvent(newEvent);
  g_Mouse.SetEnabled(true);
  g_Mouse.SetState(m_mouseState);
  m_bMouseMoved = true;
}

bool CFlashVideoPlayer::TranslateMouseCoordinates(float &x, float &y)
{
  CRect source, dest;
  CPoint p = g_Mouse.GetLocation();

  x = p.x - m_screenX1;
  y = p.y - m_screenY1;

  return true;
}

bool CFlashVideoPlayer::MouseRenderingEnabled()
{
  CGUIDialogBoxeeCtx *pBrowserOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSER_CTX);
  if (pBrowserOSD && pBrowserOSD->IsDialogRunning())
    return false;

  CGUIDialogBoxeeCtx *pPlayerOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_CTX);
  if (pPlayerOSD && pPlayerOSD->IsDialogRunning())
    return false;

  CGUIDialogBoxeeDropdown *pDropdownDialog = (CGUIDialogBoxeeDropdown*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_DROPDOWN);
  if (pDropdownDialog && pDropdownDialog->IsDialogRunning())
    return false;

  if (m_bDrawMouse)
    return true;
  else
    return false;
}

bool CFlashVideoPlayer::ForceMouseRendering()
{
  CGUIDialogBoxeeCtx *pBrowserOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSER_CTX);
  if (pBrowserOSD && pBrowserOSD->IsDialogRunning())
    return false;

  CGUIDialogBoxeeDropdown *pDropdownDialog = (CGUIDialogBoxeeDropdown*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_DROPDOWN);
  if (pDropdownDialog && pDropdownDialog->IsDialogRunning())
    return false;

  if (m_bDrawMouse && !m_bMouseActiveTimeout)
    return true;
  else
    return false;
}

bool CFlashVideoPlayer::CanReportGetTime()
{
  return m_clockSet;
}

void CFlashVideoPlayer::CloseOSD()
{
  CGUIDialogBoxeeCtx *pBrowserOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSER_CTX);
  CGUIDialogBoxeeCtx *pPlayerOSD = (CGUIDialogBoxeeCtx *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_VIDEO_CTX);
  if (pBrowserOSD && pBrowserOSD->IsDialogRunning())
    pBrowserOSD->Close();
  if (pPlayerOSD && pPlayerOSD->IsDialogRunning())
    pPlayerOSD->Close();
}

void CFlashVideoPlayer::OnPlayBackEnded(bool bError, const CStdString& error)
{
  CLog::Log(LOGINFO, "ONPLAYBACKENDED %s",bError? CStdString("with an error ").append(error).c_str() : "without an error");
  Json::Value command;
  command["command"]= "MEDIAPLAYER.PlaybackEnded";
  Json::FastWriter writer;
  m_dll.FlashJsonCommand(m_handle, writer.write( command ).c_str());
}

void CFlashVideoPlayer::OnPlayBackStarted()
{
  CLog::Log(LOGINFO,"On Playback Started");
//printf("\n%s::%s(%d):\tcalled\n", __FILE__, __FUNCTION__, __LINE__);
}

void CFlashVideoPlayer::OnPlayBackResumed()
{
  CLog::Log(LOGINFO,"On Playback Resumed");
  m_mediaPlayerInfo.paused = false;
//printf("\n%s::%s(%d):\tcalled\n", __FILE__, __FUNCTION__, __LINE__);
}

void CFlashVideoPlayer::OnPlayBackPaused()
{
  CLog::Log(LOGINFO,"On Playback Paused");
  m_mediaPlayerInfo.paused = true;
//printf("\n%s::%s(%d):\tcalled\n", __FILE__, __FUNCTION__, __LINE__);
}

void CFlashVideoPlayer::OnPlayBackSeek(int iTime, int seekOffset)
{
  CLog::Log(LOGINFO,"On Playback Seek");
//printf("\n%s::%s(%d):\tcalled\n", __FILE__, __FUNCTION__, __LINE__);
}
