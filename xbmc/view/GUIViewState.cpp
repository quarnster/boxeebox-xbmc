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

#include "view/GUIViewState.h"
#include "pvr/windows/GUIViewStatePVR.h"
#include "addons/GUIViewStateAddonBrowser.h"
#include "music/GUIViewStateMusic.h"
#include "video/GUIViewStateVideo.h"
#include "pictures/GUIViewStatePictures.h"
#include "profiles/ProfilesManager.h"
#include "programs/GUIViewStatePrograms.h"
#include "PlayListPlayer.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "GUIPassword.h"
#include "ViewDatabase.h"
#include "AutoSwitch.h"
#include "guilib/GUIWindowManager.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "view/ViewState.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "filesystem/AddonsDirectory.h"
#include "guilib/TextureManager.h"

#if defined(TARGET_ANDROID)
#include "filesystem/AndroidAppDirectory.h"
#endif

#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"

using namespace std;
using namespace ADDON;
using namespace PVR;

CStdString CGUIViewState::m_strPlaylistDirectory;
VECSOURCES CGUIViewState::m_sources;

CGUIViewState* CGUIViewState::GetViewState(int windowId, const CFileItemList& items)
{
  // don't expect derived classes to clear the sources
  m_sources.clear();

  if (windowId == 0)
    return GetViewState(g_windowManager.GetActiveWindow(),items);

  const CURL url=items.GetAsUrl();

  if (items.IsAddonsPath())
    return new CGUIViewStateAddonBrowser(items);

  if (items.HasSortDetails())
    return new CGUIViewStateFromItems(items);

  if (url.GetProtocol()=="musicdb")
    return new CGUIViewStateMusicDatabase(items);

  if (url.GetProtocol()=="musicsearch")
    return new CGUIViewStateMusicSearch(items);

  if (items.IsSmartPlayList() || url.GetProtocol() == "upnp" ||
      items.IsLibraryFolder())
  {
    if (items.GetContent() == "songs" ||
        items.GetContent() == "albums" ||
        items.GetContent() == "mixed")
      return new CGUIViewStateMusicSmartPlaylist(items);
    else if (items.GetContent() == "musicvideos")
      return new CGUIViewStateVideoMusicVideos(items);
    else if (items.GetContent() == "tvshows")
      return new CGUIViewStateVideoTVShows(items);
    else if (items.GetContent() == "episodes")
      return new CGUIViewStateVideoEpisodes(items);
    else if (items.GetContent() == "movies")
      return new CGUIViewStateVideoMovies(items);
  }

  if (url.GetProtocol() == "library")
    return new CGUIViewStateLibrary(items);

  if (items.IsPlayList())
    return new CGUIViewStateMusicPlaylist(items);

  if (items.GetPath() == "special://musicplaylists/")
    return new CGUIViewStateWindowMusicSongs(items);

  if (url.GetProtocol() == "androidapp")
    return new CGUIViewStateWindowPrograms(items);

  if (windowId==WINDOW_MUSIC_NAV)
    return new CGUIViewStateWindowMusicNav(items);

  if (windowId==WINDOW_MUSIC_FILES)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST)
    return new CGUIViewStateWindowMusicPlaylist(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST_EDITOR)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_VIDEO_FILES)
    return new CGUIViewStateWindowVideoFiles(items);

  if (windowId==WINDOW_VIDEO_NAV)
    return new CGUIViewStateWindowVideoNav(items);

  if (windowId==WINDOW_VIDEO_PLAYLIST)
    return new CGUIViewStateWindowVideoPlaylist(items);

  if (windowId==WINDOW_PVR)
    return new CGUIViewStatePVR(items);

  if (windowId==WINDOW_PICTURES)
    return new CGUIViewStateWindowPictures(items);

  if (windowId==WINDOW_PROGRAMS)
    return new CGUIViewStateWindowPrograms(items);
  
  if (windowId==WINDOW_ADDON_BROWSER)
    return new CGUIViewStateAddonBrowser(items);

  //  Use as fallback/default
  return new CGUIViewStateGeneral(items);
}

CGUIViewState::CGUIViewState(const CFileItemList& items) : m_items(items)
{
  m_currentViewAsControl=0;
  m_currentSortMethod=0;
  m_playlist = PLAYLIST_NONE;
  m_sortOrder = SortOrderAscending;
}

CGUIViewState::~CGUIViewState()
{
}

SortOrder CGUIViewState::GetDisplaySortOrder() const
{
  // we actually treat some sort orders in reverse, so that we can have
  // the one sort order variable to save but it can be ascending usually,
  // and descending for the views which should be usually descending.
  // default sort order for date, size, program count + rating is reversed
  SortDescription sorting = GetSortMethod();
  if (sorting.sortBy == SortByDate || sorting.sortBy == SortBySize || sorting.sortBy == SortByPlaycount ||
      sorting.sortBy == SortByRating || sorting.sortBy == SortByProgramCount ||
      sorting.sortBy == SortByBitrate || sorting.sortBy == SortByListeners)
  {
    if (m_sortOrder == SortOrderAscending)
      return SortOrderDescending;
    if (m_sortOrder == SortOrderDescending)
      return SortOrderAscending;
  }

  return m_sortOrder;
}

SortOrder CGUIViewState::SetNextSortOrder()
{
  if (m_sortOrder == SortOrderAscending)
    SetSortOrder(SortOrderDescending);
  else
    SetSortOrder(SortOrderAscending);

  SaveViewState();

  return m_sortOrder;
}

int CGUIViewState::GetViewAsControl() const
{
  return m_currentViewAsControl;
}

void CGUIViewState::SetViewAsControl(int viewAsControl)
{
  if (viewAsControl == DEFAULT_VIEW_AUTO)
    m_currentViewAsControl = CAutoSwitch::GetView(m_items);
  else
    m_currentViewAsControl = viewAsControl;
}

void CGUIViewState::SaveViewAsControl(int viewAsControl)
{
  SetViewAsControl(viewAsControl);
  SaveViewState();
}

SortDescription CGUIViewState::GetSortMethod() const
{
  SortDescription sorting;
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    sorting = m_sortMethods[m_currentSortMethod].m_sortDescription;
  sorting.sortOrder = m_sortOrder;

  return sorting;
}

bool CGUIViewState::HasMultipleSortMethods() const
{
  return m_sortMethods.size() > 1;
}

int CGUIViewState::GetSortMethodLabel() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_buttonLabel;

  return 103; // Sort By: Name
}

void CGUIViewState::GetSortMethodLabelMasks(LABEL_MASKS& masks) const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
  {
    masks=m_sortMethods[m_currentSortMethod].m_labelMasks;
    return;
  }

  masks.m_strLabelFile.Empty();
  masks.m_strLabel2File.Empty();
  masks.m_strLabelFolder.Empty();
  masks.m_strLabel2Folder.Empty();
  return;
}

void CGUIViewState::AddSortMethod(SortBy sortBy, int buttonLabel, const LABEL_MASKS &labelMasks, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  AddSortMethod(sortBy, sortAttributes, buttonLabel, labelMasks);
}

void CGUIViewState::AddSortMethod(SortBy sortBy, SortAttribute sortAttributes, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  for (size_t i = 0; i < m_sortMethods.size(); ++i)
    if (m_sortMethods[i].m_sortDescription.sortBy == sortBy)
      return;

  SORT_METHOD_DETAILS sort;
  sort.m_sortDescription.sortBy = sortBy;
  sort.m_sortDescription.sortAttributes = sortAttributes;
  sort.m_buttonLabel = buttonLabel;
  sort.m_labelMasks = labelMasks;
  m_sortMethods.push_back(sort);
}

void CGUIViewState::AddSortMethod(SortDescription sortDescription, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  AddSortMethod(sortDescription.sortBy, sortDescription.sortAttributes, buttonLabel, labelMasks);
}

void CGUIViewState::SetCurrentSortMethod(int method)
{
  SortBy sortBy = (SortBy)method;
  SortAttribute sortAttributes = SortAttributeNone;
  if (CSettings::Get().GetBool("filelists.ignorethewhensorting"))
    sortAttributes = SortAttributeIgnoreArticle;

  if (sortBy < SortByNone || sortBy > SortByRandom)
    return; // invalid

  SetSortMethod(sortBy, sortAttributes);
  SaveViewState();
}

void CGUIViewState::SetSortMethod(SortBy sortBy, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  for (int i = 0; i < (int)m_sortMethods.size(); ++i)
  {
    if (m_sortMethods[i].m_sortDescription.sortBy == sortBy &&
       // don't care about SortAttributeIgnoreFolders as it wasn't part of the old sorting comparison
        m_sortMethods[i].m_sortDescription.sortAttributes == (sortAttributes & ~SortAttributeIgnoreFolders))
    {
      m_currentSortMethod = i;
      break;
    }
  }
  SetSortOrder(m_sortOrder);
}

void CGUIViewState::SetSortMethod(SortDescription sortDescription)
{
  return SetSortMethod(sortDescription.sortBy, sortDescription.sortAttributes);
}

SortDescription CGUIViewState::SetNextSortMethod(int direction /* = 1 */)
{
  m_currentSortMethod += direction;

  if (m_currentSortMethod >= (int)m_sortMethods.size())
    m_currentSortMethod = 0;
  if (m_currentSortMethod < 0)
    m_currentSortMethod = m_sortMethods.size() ? (int)m_sortMethods.size() - 1 : 0;
  SetSortOrder(m_sortOrder);

  SaveViewState();

  return GetSortMethod();
}

bool CGUIViewState::HideExtensions()
{
  return !CSettings::Get().GetBool("filelists.showextensions");
}

bool CGUIViewState::HideParentDirItems()
{
  return !CSettings::Get().GetBool("filelists.showparentdiritems");
}

bool CGUIViewState::DisableAddSourceButtons()
{
  if (CProfilesManager::Get().GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser)
    return !CSettings::Get().GetBool("filelists.showaddsourcebuttons");

  return true;
}

int CGUIViewState::GetPlaylist()
{
  return m_playlist;
}

const CStdString& CGUIViewState::GetPlaylistDirectory()
{
  return m_strPlaylistDirectory;
}

void CGUIViewState::SetPlaylistDirectory(const CStdString& strDirectory)
{
  m_strPlaylistDirectory=strDirectory;
  URIUtils::RemoveSlashAtEnd(m_strPlaylistDirectory);
}

bool CGUIViewState::IsCurrentPlaylistDirectory(const CStdString& strDirectory)
{
  if (g_playlistPlayer.GetCurrentPlaylist()!=GetPlaylist())
    return false;

  CStdString strDir=strDirectory;
  URIUtils::RemoveSlashAtEnd(strDir);

  return (m_strPlaylistDirectory==strDir);
}

bool CGUIViewState::AutoPlayNextItem()
{
  return false;
}

CStdString CGUIViewState::GetLockType()
{
  return "";
}

CStdString CGUIViewState::GetExtensions()
{
  return "";
}

VECSOURCES& CGUIViewState::GetSources()
{
  return m_sources;
}

void CGUIViewState::AddAddonsSource(const CStdString &content, const CStdString &label, const CStdString &thumb)
{
  if (!g_advancedSettings.m_bVirtualShares)
    return;

  CFileItemList items;
  if (XFILE::CAddonsDirectory::GetScriptsAndPlugins(content, items))
  { // add the plugin source
    CMediaSource source;
    source.strPath = "addons://sources/" + content + "/";    
    source.strName = label;
    if (!thumb.IsEmpty() && g_TextureManager.HasTexture(thumb))
      source.m_strThumbnailImage = thumb;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_ignore = true;
    m_sources.push_back(source);
  }
}

#if defined(TARGET_ANDROID)
void CGUIViewState::AddAndroidSource(const CStdString &content, const CStdString &label, const CStdString &thumb)
{
  CFileItemList items;
  XFILE::CAndroidAppDirectory apps;
  if (apps.GetDirectory(content, items))
  {
    CMediaSource source;
    source.strPath = "androidapp://sources/" + content + "/";
    source.strName = label;
    if (!thumb.IsEmpty() && g_TextureManager.HasTexture(thumb))
      source.m_strThumbnailImage = thumb;
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_ignore = true;
    m_sources.push_back(source);
  }
}
#endif

void CGUIViewState::AddLiveTVSources()
{
  VECSOURCES *sources = CMediaSourceSettings::Get().GetSources("video");
  for (IVECSOURCES it = sources->begin(); it != sources->end(); it++)
  {
    if (URIUtils::IsLiveTV((*it).strPath))
    {
      CMediaSource source;
      source.strPath = (*it).strPath;
      source.strName = (*it).strName;
      source.vecPaths = (*it).vecPaths;
      source.m_strThumbnailImage = "";
      source.FromNameAndPaths("video", source.strName, source.vecPaths);
      m_sources.push_back(source);
    }
  }
}

CGUIViewStateGeneral::CGUIViewStateGeneral(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, size | Foldername, empty
  SetSortMethod(SortByLabel);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderAscending);
}

void CGUIViewState::SetSortOrder(SortOrder sortOrder)
{
  if (GetSortMethod().sortBy == SortByNone)
    m_sortOrder = SortOrderNone;
  else if (sortOrder == SortOrderNone)
    m_sortOrder = SortOrderAscending;
  else
    m_sortOrder = sortOrder;
}

void CGUIViewState::LoadViewState(const CStdString &path, int windowID)
{ // get our view state from the db
  CViewDatabase db;
  if (db.Open())
  {
    CViewState state;
    if (db.GetViewState(path, windowID, state, CSettings::Get().GetString("lookandfeel.skin")) ||
        db.GetViewState(path, windowID, state, ""))
    {
      SetViewAsControl(state.m_viewMode);
      SetSortMethod(state.m_sortDescription);
      SetSortOrder(state.m_sortDescription.sortOrder);
    }
    db.Close();
  }
}

void CGUIViewState::SaveViewToDb(const CStdString &path, int windowID, CViewState *viewState)
{
  CViewDatabase db;
  if (db.Open())
  {
    SortDescription sorting = GetSortMethod();
    CViewState state(m_currentViewAsControl, sorting.sortBy, m_sortOrder, sorting.sortAttributes);
    if (viewState)
      *viewState = state;
    db.SetViewState(path, windowID, state, CSettings::Get().GetString("lookandfeel.skin"));
    db.Close();
    if (viewState)
      CSettings::Get().Save();
  }
}

void CGUIViewState::AddPlaylistOrder(const CFileItemList &items, LABEL_MASKS label_masks)
{
  SortBy sortBy = SortByPlaylistOrder;
  int         sortLabel = 559;
  SortOrder   sortOrder = SortOrderAscending;
  if (items.HasProperty(PROPERTY_SORT_ORDER))
  {
    sortBy = (SortBy)items.GetProperty(PROPERTY_SORT_ORDER).asInteger();
    if (sortBy != SortByNone)
    {
      sortLabel = SortUtils::GetSortLabel(sortBy);
      sortOrder = items.GetProperty(PROPERTY_SORT_ASCENDING).asBoolean() ? SortOrderAscending : SortOrderDescending;
    }
  }
  AddSortMethod(sortBy, sortLabel, label_masks);
  SetSortMethod(sortBy);
  SetSortOrder(sortOrder);
}


CGUIViewStateFromItems::CGUIViewStateFromItems(const CFileItemList &items) : CGUIViewState(items)
{
  const vector<SORT_METHOD_DETAILS> &details = items.GetSortDetails();
  for (unsigned int i = 0; i < details.size(); i++)
  {
    const SORT_METHOD_DETAILS sort = details[i];
    AddSortMethod(sort.m_sortDescription, sort.m_buttonLabel, sort.m_labelMasks);
  }
  // TODO: Should default sort/view mode be specified?
  m_currentSortMethod = 0;

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SortOrderAscending);
  if (items.IsPlugin())
  {
    CURL url(items.GetPath());
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(url.GetHostName(),addon) && addon)
    {
      PluginPtr plugin = boost::static_pointer_cast<CPluginSource>(addon);
      if (plugin->Provides(CPluginSource::AUDIO))
        m_playlist = PLAYLIST_MUSIC;
      if (plugin->Provides(CPluginSource::VIDEO))
        m_playlist = PLAYLIST_VIDEO;
    }
  }
  LoadViewState(items.GetPath(), g_windowManager.GetActiveWindow());
}

void CGUIViewStateFromItems::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), g_windowManager.GetActiveWindow());
}

CGUIViewStateLibrary::CGUIViewStateLibrary(const CFileItemList &items) : CGUIViewState(items)
{
  AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  SetSortMethod(SortByNone);
  SetSortOrder(SortOrderNone);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  LoadViewState(items.GetPath(), g_windowManager.GetActiveWindow());
}

void CGUIViewStateLibrary::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), g_windowManager.GetActiveWindow());
}

