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

#include "GUIUserMessages.h"
#include "GUIWindowVideoNav.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "utils/FileUtils.h"
#include "Util.h"
#include "utils/RegExp.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "playlists/PlayListFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "addons/AddonManager.h"
#include "PartyModeManager.h"
#include "music/MusicDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "TextureCache.h"
#include "guilib/GUIKeyboardFactory.h"
#include "video/VideoInfoScanner.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "pvr/recordings/PVRRecording.h"

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;
using namespace std;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_BTNSEARCH          8
#define CONTROL_LABELFILES        12

#define CONTROL_BTN_FILTER        19
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14
#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_LABELEMPTY        18

#define CONTROL_UPDATE_LIBRARY    20

CGUIWindowVideoNav::CGUIWindowVideoNav(void)
    : CGUIWindowVideoBase(WINDOW_VIDEO_NAV, "MyVideoNav.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoNav::~CGUIWindowVideoNav(void)
{
}

bool CGUIWindowVideoNav::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->IsParentFolder())
      return false;
    if (pItem && pItem->GetVideoInfoTag()->m_playCount == 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
    if (pItem && pItem->GetVideoInfoTag()->m_playCount > 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->SetPath("");
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      /* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);

      SetProperty("flattened", CSettings::Get().GetBool("myvideos.flatten"));
      if (message.GetNumStringParams() && message.GetStringParam(0).Equals("Files") &&
          CMediaSourceSettings::Get().GetSources("video")->empty())
      {
        message.SetStringParam("");
      }
      
      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNPARTYMODE)
      {
        if (g_partyModeManager.IsEnabled())
          g_partyModeManager.Disable();
        else
        {
          if (!g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO))
          {
            SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE,false);
            return false;
          }

          // Playlist directory is the root of the playlist window
          if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistvideo://");

          return true;
        }
        UpdateButtons();
      }

      if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        CMediaSettings::Get().CycleWatchedMode(m_vecItems->GetContent());
        CSettings::Get().Save();
        OnFilterItems(GetProperty("filter").asString());
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (CMediaSettings::Get().GetWatchedMode(m_vecItems->GetContent()) == WatchedModeAll)
          CMediaSettings::Get().SetWatchedMode(m_vecItems->GetContent(), WatchedModeUnwatched);
        else
          CMediaSettings::Get().SetWatchedMode(m_vecItems->GetContent(), WatchedModeAll);
        CSettings::Get().Save();
        OnFilterItems(GetProperty("filter").asString());
        return true;
      }
      else if (iControl == CONTROL_UPDATE_LIBRARY)
      {
        if (!g_application.IsVideoScanning())
          OnScan("");
        else
          g_application.StopVideoScan();
        return true;
      }
    }
    break;
    // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS:
      Refresh();
      break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

CStdString CGUIWindowVideoNav::GetQuickpathName(const CStdString& strPath) const
{
  CStdString path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  if (path.Equals("videodb://movies/genres/"))
    return "MovieGenres";
  else if (path.Equals("videodb://movies/titles/"))
    return "MovieTitles";
  else if (path.Equals("videodb://movies/years/"))
    return "MovieYears";
  else if (path.Equals("videodb://movies/actors/"))
    return "MovieActors";
  else if (path.Equals("videodb://movies/directors/"))
    return "MovieDirectors";
  else if (path.Equals("videodb://movies/studios/"))
    return "MovieStudios";
  else if (path.Equals("videodb://movies/sets/"))
    return "MovieSets";
  else if (path.Equals("videodb://movies/countries/"))
    return "MovieCountries";
  else if (path.Equals("videodb://movies/tags/"))
    return "MovieTags";
  else if (path.Equals("videodb://movies/"))
    return "Movies";
  else if (path.Equals("videodb://tvshows/genres/"))
    return "TvShowGenres";
  else if (path.Equals("videodb://tvshows/titles/"))
    return "TvShowTitles";
  else if (path.Equals("videodb://tvshows/years/"))
    return "TvShowYears";
  else if (path.Equals("videodb://tvshows/actors/"))
    return "TvShowActors";
  else if (path.Equals("videodb://tvshows/studios/"))
    return "TvShowStudios";
  else if (path.Equals("videodb://tvshows/tags/"))
    return "TvShowTags";
  else if (path.Equals("videodb://tvshows/"))
    return "TvShows";
  else if (path.Equals("videodb://musicvideos/genres/"))
    return "MusicVideoGenres";
  else if (path.Equals("videodb://musicvideos/titles/"))
    return "MusicVideoTitles";
  else if (path.Equals("videodb://musicvideos/years/"))
    return "MusicVideoYears";
  else if (path.Equals("videodb://musicvideos/artists/"))
    return "MusicVideoArtists";
  else if (path.Equals("videodb://musicvideos/albums/"))
    return "MusicVideoDirectors";
  else if (path.Equals("videodb://musicvideos/tags/"))
    return "MusicVideoTags";
  else if (path.Equals("videodb://musicvideos/"))
    return "MusicVideos";
  else if (path.Equals("videodb://recentlyaddedmovies/"))
    return "RecentlyAddedMovies";
  else if (path.Equals("videodb://recentlyaddedepisodes/"))
    return "RecentlyAddedEpisodes";
  else if (path.Equals("videodb://recentlyaddedmusicvideos/"))
    return "RecentlyAddedMusicVideos";
  else if (path.Equals("special://videoplaylists/"))
    return "Playlists";
  else if (path.Equals("sources://video/"))
    return "Files";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowVideoNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowVideoNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  items.ClearProperties();

  bool bResult = CGUIWindowVideoBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsVideoDb())
    {
      XFILE::CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(items.GetPath(),params);
      VIDEODATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

      items.SetArt("thumb", "");
      if (node == VIDEODATABASEDIRECTORY::NODE_TYPE_EPISODES ||
          node == NODE_TYPE_SEASONS                          ||
          node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
      {
        CLog::Log(LOGDEBUG, "WindowVideoNav::GetDirectory");
        // grab the show thumb
        CVideoInfoTag details;
        m_database.GetTvShowInfo("", details, params.GetTvShowId());
        map<string, string> art;
        if (m_database.GetArtForItem(details.m_iDbId, details.m_type, art))
        {
          items.AppendArt(art, "tvshow");
          items.SetArtFallback("fanart", "tvshow.fanart");
          if (node == NODE_TYPE_SEASONS)
          { // set an art fallback for "thumb"
            if (items.HasArt("tvshow.poster"))
              items.SetArtFallback("thumb", "tvshow.poster");
            else if (items.HasArt("tvshow.banner"))
              items.SetArtFallback("thumb", "tvshow.banner");
          }
        }

        // Grab fanart data
        items.SetProperty("fanart_color1", details.m_fanart.GetColor(0));
        items.SetProperty("fanart_color2", details.m_fanart.GetColor(1));
        items.SetProperty("fanart_color3", details.m_fanart.GetColor(2));

        // save the show description (showplot)
        items.SetProperty("showplot", details.m_strPlot);

        // the container folder thumb is the parent (i.e. season or show)
        if (node == NODE_TYPE_EPISODES || node == NODE_TYPE_RECENTLY_ADDED_EPISODES)
        {
          items.SetContent("episodes");
          // grab the season thumb as the folder thumb
          int seasonID = m_database.GetSeasonId(details.m_iDbId, params.GetSeason());
          CGUIListItem::ArtMap seasonArt;
          if (m_database.GetArtForItem(seasonID, "season", seasonArt))
          {
            items.AppendArt(art, "season");
            // set an art fallback for "thumb"
            if (items.HasArt("season.poster"))
              items.SetArtFallback("thumb", "season.poster");
            else if (items.HasArt("season.banner"))
              items.SetArtFallback("thumb", "season.banner");
          }
        }
        else
          items.SetContent("seasons");
      }
      else if (node == NODE_TYPE_TITLE_MOVIES ||
               node == NODE_TYPE_RECENTLY_ADDED_MOVIES)
        items.SetContent("movies");
      else if (node == NODE_TYPE_TITLE_TVSHOWS)
        items.SetContent("tvshows");
      else if (node == NODE_TYPE_TITLE_MUSICVIDEOS ||
               node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
        items.SetContent("musicvideos");
      else if (node == NODE_TYPE_GENRE)
        items.SetContent("genres");
      else if (node == NODE_TYPE_COUNTRY)
        items.SetContent("countries");
      else if (node == NODE_TYPE_ACTOR)
      {
        if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
          items.SetContent("artists");
        else
          items.SetContent("actors");
      }
      else if (node == NODE_TYPE_DIRECTOR)
        items.SetContent("directors");
      else if (node == NODE_TYPE_STUDIO)
        items.SetContent("studios");
      else if (node == NODE_TYPE_YEAR)
        items.SetContent("years");
      else if (node == NODE_TYPE_MUSICVIDEOS_ALBUM)
        items.SetContent("albums");
      else if (node == NODE_TYPE_SETS)
        items.SetContent("sets");
      else if (node == NODE_TYPE_TAGS)
        items.SetContent("tags");
      else
        items.SetContent("");
    }
    else if (!items.IsVirtualDirectoryRoot())
    { // load info from the database
      CStdString label;
      if (items.GetLabel().IsEmpty() && m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::Get().GetSources("video"), &label)) 
        items.SetLabel(label);
      if (!items.IsSourcesPath())
        LoadVideoInfo(items);
    }

    CVideoDbUrl videoUrl;
    if (videoUrl.FromString(items.GetPath()) && items.GetContent() == "tags" &&
       !items.Contains("newtag://" + videoUrl.GetType()))
    {
      CFileItemPtr newTag(new CFileItem("newtag://" + videoUrl.GetType(), false));
      newTag->SetLabel(g_localizeStrings.Get(20462));
      newTag->SetLabelPreformated(true);
      newTag->SetSpecialSort(SortSpecialOnTop);
      items.Add(newTag);
    }
  }
  return bResult;
}

void CGUIWindowVideoNav::LoadVideoInfo(CFileItemList &items)
{
  LoadVideoInfo(items, m_database);
}

void CGUIWindowVideoNav::LoadVideoInfo(CFileItemList &items, CVideoDatabase &database, bool allowReplaceLabels)
{
  // TODO: this could possibly be threaded as per the music info loading,
  //       we could also cache the info
  if (!items.GetContent().IsEmpty() && !items.IsPlugin())
    return; // don't load for listings that have content set and weren't created from plugins

  CStdString content = items.GetContent();
  // determine content only if it isn't set
  if (content.IsEmpty())
  {
    content = database.GetContentForPath(items.GetPath());
    items.SetContent(content.IsEmpty() ? "files" : content);
  }

  /*
    If we have a matching item in the library, so we can assign the metadata to it. In addition, we can choose
    * whether the item is stacked down (eg in the case of folders representing a single item)
    * whether or not we assign the library's labels to the item, or leave the item as is.

    As certain users (read: certain developers) don't want either of these to occur, we compromise by stacking
    items down only if stacking is available and enabled.

    Similarly, we assign the "clean" library labels to the item only if the "Replace filenames with library titles"
    setting is enabled.
    */
  const bool stackItems    = items.GetProperty("isstacked").asBoolean() || (StackingAvailable(items) && CSettings::Get().GetBool("myvideos.stackvideos"));
  const bool replaceLabels = allowReplaceLabels && CSettings::Get().GetBool("myvideos.replacelabels");

  CFileItemList dbItems;
  /* NOTE: In the future when GetItemsForPath returns all items regardless of whether they're "in the library"
           we won't need the fetchedPlayCounts code, and can "simply" do this directly on absense of content. */
  bool fetchedPlayCounts = false;
  if (!content.IsEmpty())
  {
    database.GetItemsForPath(content, items.GetPath(), dbItems);
    dbItems.SetFastLookup(true);
  }

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    CFileItemPtr match;
    if (!content.IsEmpty()) /* optical media will be stacked down, so it's path won't match the base path */
      match = dbItems.Get(pItem->IsOpticalMediaFile() ? pItem->GetLocalMetadataPath() : pItem->GetPath());
    if (match)
    {
      pItem->UpdateInfo(*match, replaceLabels);

      if (stackItems)
      {
        if (match->m_bIsFolder)
          pItem->SetPath(match->GetVideoInfoTag()->m_strPath);
        else
          pItem->SetPath(match->GetVideoInfoTag()->m_strFileNameAndPath);
        // if we switch from a file to a folder item it means we really shouldn't be sorting files and
        // folders separately
        if (pItem->m_bIsFolder != match->m_bIsFolder)
        {
          items.SetSortIgnoreFolders(true);
          pItem->m_bIsFolder = match->m_bIsFolder;
        }
      }
    }
    else
    {
      /* NOTE: Currently we GetPlayCounts on our items regardless of whether content is set
                as if content is set, GetItemsForPaths doesn't return anything not in the content tables.
                This code can be removed once the content tables are always filled */
      if (!pItem->m_bIsFolder && !fetchedPlayCounts)
      {
        database.GetPlayCounts(items.GetPath(), items);
        fetchedPlayCounts = true;
      }
      
      // preferably use some information from PVR info tag if available
      if (pItem->HasPVRRecordingInfoTag())
        pItem->GetPVRRecordingInfoTag()->CopyClientInfo(pItem->GetVideoInfoTag());

      // set the watched overlay
      if (pItem->IsVideo())
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_playCount > 0);
    }
  }
}

void CGUIWindowVideoNav::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();

  // Update object count
  int iItems = m_vecItems->Size();
  if (iItems)
  {
    // check for parent dir and "all" items
    // should always be the first two items
    for (int i = 0; i <= (iItems>=2 ? 1 : 0); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->IsParentFolder()) iItems--;
      if (pItem->GetPath().Left(4).Equals("/-1/")) iItems--;
    }
    // or the last item
    if (m_vecItems->Size() > 2 &&
      m_vecItems->Get(m_vecItems->Size()-1)->GetPath().Left(4).Equals("/-1/"))
      iItems--;
  }
  CStdString items;
  items.Format("%i %s", iItems, g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  // set the filter label
  CStdString strLabel;

  // "Playlists"
  if (m_vecItems->GetPath().Equals("special://videoplaylists/"))
    strLabel = g_localizeStrings.Get(136);
  // "{Playlist Name}"
  else if (m_vecItems->IsPlayList())
  {
    // get playlist name from path
    CStdString strDummy;
    URIUtils::Split(m_vecItems->GetPath(), strDummy, strLabel);
  }
  else if (m_vecItems->GetPath().Equals("sources://video/"))
    strLabel = g_localizeStrings.Get(744);
  // everything else is from a videodb:// path
  else if (m_vecItems->IsVideoDb())
  {
    CVideoDatabaseDirectory dir;
    dir.GetLabel(m_vecItems->GetPath(), strLabel);
  }
  else
    strLabel = URIUtils::GetFileName(m_vecItems->GetPath());

  SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

  int watchMode = CMediaSettings::Get().GetWatchedMode(m_vecItems->GetContent());
  SET_CONTROL_LABEL(CONTROL_BTNSHOWMODE, g_localizeStrings.Get(16100 + watchMode));

  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNSHOWALL, watchMode != WatchedModeAll);

  SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE, g_partyModeManager.IsEnabled());

  CONTROL_ENABLE_ON_CONDITION(CONTROL_UPDATE_LIBRARY, !m_vecItems->IsAddonsPath() && !m_vecItems->IsPlugin() && !m_vecItems->IsScript());
}

bool CGUIWindowVideoNav::GetFilteredItems(const CStdString &filter, CFileItemList &items)
{
  bool listchanged = CGUIMediaWindow::GetFilteredItems(filter, items);
  listchanged |= ApplyWatchedFilter(items);

  return listchanged;
}

/// \brief Search for names, genres, artists, directors, and plots with search string \e strSearch in the
/// \brief video databases and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  CFileItemList tempItems;
  CStdString strGenre = g_localizeStrings.Get(515); // Genre
  CStdString strActor = g_localizeStrings.Get(20337); // Actor
  CStdString strDirector = g_localizeStrings.Get(20339); // Director
  CStdString strMovie = g_localizeStrings.Get(20338); // Movie

  //get matching names
  m_database.GetMoviesByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20338) + "] ", items);

  m_database.GetEpisodesByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20359) + "] ", items);

  m_database.GetTvShowsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20364) + "] ", items);

  m_database.GetMusicVideosByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20391) + "] ", items);

  m_database.GetMusicVideosByAlbum(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(558) + "] ", items);
  
  // get matching genres
  m_database.GetMovieGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoGenresByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strGenre + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //get actors/artists
  m_database.GetMovieActorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowsActorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoArtistsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strActor + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //directors
  m_database.GetMovieDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20342) + "] ", items);

  m_database.GetTvShowsDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20343) + "] ", items);

  m_database.GetMusicVideoDirectorsByName(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strDirector + " - " + g_localizeStrings.Get(20389) + "] ", items);

  //plot
  m_database.GetEpisodesByPlot(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + g_localizeStrings.Get(20365) + "] ", items);

  m_database.GetMoviesByPlot(strSearch, tempItems);
  AppendAndClearSearchItems(tempItems, "[" + strMovie + " " + g_localizeStrings.Get(207) + "] ", items);
}

void CGUIWindowVideoNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems->IsVirtualDirectoryRoot())
    return;

  CGUIWindowVideoBase::PlayItem(iItem);
}

void CGUIWindowVideoNav::OnInfo(CFileItem* pItem, ADDON::ScraperPtr& scraper)
{
  m_database.Open(); // since we can be called from the music library without being inited
  if (pItem->IsVideoDb())
    scraper = m_database.GetScraperForPath(pItem->GetVideoInfoTag()->m_strPath);
  else
  {
    CStdString strPath,strFile;
    URIUtils::Split(pItem->GetPath(),strPath,strFile);
    scraper = m_database.GetScraperForPath(strPath);
  }
  m_database.Close();
  CGUIWindowVideoBase::OnInfo(pItem,scraper);
}

bool CGUIWindowVideoNav::CanDelete(const CStdString& strPath)
{
  CQueryParams params;
  CVideoDatabaseDirectory::GetQueryParams(strPath,params);

  if (params.GetMovieId()   != -1 ||
      params.GetEpisodeId() != -1 ||
      params.GetMVideoId()  != -1 ||
      (params.GetTvShowId() != -1 && params.GetSeason() <= -1
              && !CVideoDatabaseDirectory::IsAllItem(strPath)))
    return true;

  return false;
}

void CGUIWindowVideoNav::OnDeleteItem(CFileItemPtr pItem)
{
  if (m_vecItems->IsParentFolder())
    return;

  if (!m_vecItems->IsVideoDb() && !pItem->IsVideoDb())
  {
    if (!pItem->GetPath().Equals("newsmartplaylist://video") &&
        !pItem->GetPath().Equals("special://videoplaylists/") &&
        !pItem->GetPath().Equals("sources://video/") &&
        !pItem->GetPath().Left(9).Equals("newtag://"))
      CGUIWindowVideoBase::OnDeleteItem(pItem);
  }
  else if (StringUtils::StartsWithNoCase(pItem->GetPath(), "videodb://movies/sets/") &&
           pItem->GetPath().size() > 22 && pItem->m_bIsFolder)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    pDialog->SetHeading(432);
    CStdString strLabel;
    strLabel.Format(g_localizeStrings.Get(433),pItem->GetLabel());
    pDialog->SetLine(1, strLabel);
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
    if (pDialog->IsConfirmed())
    {
      CFileItemList items;
      CDirectory::GetDirectory(pItem->GetPath(),items,"",DIR_FLAG_NO_FILE_DIRS);
      for (int i=0;i<items.Size();++i)
        OnDeleteItem(items[i]);

      CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(pItem->GetPath(),params);
      m_database.DeleteSet(params.GetSetId());
    }
  }
  else if (m_vecItems->GetContent() == "tags" && pItem->m_bIsFolder)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    pDialog->SetHeading(432);
    CStdString strLabel;
    strLabel.Format(g_localizeStrings.Get(433), pItem->GetLabel());
    pDialog->SetLine(1, strLabel);
    pDialog->SetLine(2, "");
    pDialog->DoModal();
    if (pDialog->IsConfirmed())
    {
      CVideoDatabaseDirectory dir;
      CQueryParams params;
      dir.GetQueryParams(pItem->GetPath(), params);
      m_database.DeleteTag(params.GetTagId(), (VIDEODB_CONTENT_TYPE)params.GetContentType());
    }
  }
  else if (m_vecItems->GetPath().Equals(CUtil::VideoPlaylistsLocation()) ||
           m_vecItems->GetPath().Equals("special://videoplaylists/"))
  {
    pItem->m_bIsFolder = false;
    CFileUtils::DeleteItem(pItem);
  }
  else
  {
    if (!DeleteItem(pItem.get()))
      return;

    CStdString strDeletePath;
    if (pItem->m_bIsFolder)
      strDeletePath=pItem->GetVideoInfoTag()->m_strPath;
    else
      strDeletePath=pItem->GetVideoInfoTag()->m_strFileNameAndPath;

    if (URIUtils::GetFileName(strDeletePath).Equals("VIDEO_TS.IFO"))
    {
      strDeletePath = URIUtils::GetDirectory(strDeletePath);
      if (strDeletePath.Right(9).Equals("VIDEO_TS/"))
      {
        URIUtils::RemoveSlashAtEnd(strDeletePath);
        strDeletePath = URIUtils::GetDirectory(strDeletePath);
      }
    }
    if (URIUtils::HasSlashAtEnd(strDeletePath))
      pItem->m_bIsFolder=true;

    if (CSettings::Get().GetBool("filelists.allowfiledeletion") &&
        CUtil::SupportsWriteFileOperations(strDeletePath))
    {
      pItem->SetPath(strDeletePath);
      CGUIWindowVideoBase::OnDeleteItem(pItem);
    }
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();
}

bool CGUIWindowVideoNav::DeleteItem(CFileItem* pItem, bool bUnavailable /* = false */)
{
  if (!pItem->HasVideoInfoTag() || !CanDelete(pItem->GetPath()))
    return false;

  VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty())
    iType = VIDEODB_CONTENT_TVSHOWS;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder)
    iType = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_artist.empty())
    iType = VIDEODB_CONTENT_MUSICVIDEOS;

  // dont allow update while scanning
  if (g_application.IsVideoScanning())
  {
    CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
    return false;
  }


  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    pDialog->SetHeading(432);
  if (iType == VIDEODB_CONTENT_EPISODES)
    pDialog->SetHeading(20362);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    pDialog->SetHeading(20363);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    pDialog->SetHeading(20392);

  if(bUnavailable)
  {
    pDialog->SetLine(0, g_localizeStrings.Get(662));
    pDialog->SetLine(1, g_localizeStrings.Get(663));
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }
  else
  {
    CStdString strLine;
    strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
    pDialog->SetLine(0, strLine);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, "");;
    pDialog->DoModal();
  }

  if (!pDialog->IsConfirmed())
    return false;

  CStdString path;
  CVideoDatabase database;
  database.Open();

  database.GetFilePathById(pItem->GetVideoInfoTag()->m_iDbId, path, iType);
  if (path.IsEmpty())
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    database.DeleteMovie(path);
  if (iType == VIDEODB_CONTENT_EPISODES)
    database.DeleteEpisode(path, pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.DeleteTvShow(path);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    database.DeleteMusicVideo(path);

  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.SetPathHash(path,"");
  else
    database.SetPathHash(URIUtils::GetDirectory(path), "");

  return true;
}

void CGUIWindowVideoNav::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);
}

void CGUIWindowVideoNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);

  if (item && item->GetProperty("pluginreplacecontextitems").asBoolean())
    return;

  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(m_vecItems->GetPath());

  if (!item)
  {
    // nothing to do here
  }
  else if (m_vecItems->GetPath().Equals("sources://video/"))
  {
    // get the usual shares
    CGUIDialogContextMenu::GetContextButtons("video", item, buttons);
    // add scan button somewhere here
    if (g_application.IsVideoScanning())
      buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);  // Stop Scanning
    if (!item->IsDVD() && item->GetPath() != "add" && !item->IsParentFolder() &&
        (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CVideoDatabase database;
      database.Open();
      ADDON::ScraperPtr info = database.GetScraperForPath(item->GetPath());

      if (!g_application.IsVideoScanning())
      {
        if (!item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath() && !URIUtils::IsUPnP(item->GetPath()))
        {
          if (info && info->Content() != CONTENT_NONE)
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          else
            buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
        }
      }

      if (info && !g_application.IsVideoScanning())
        buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
    }
  }
  else
  {
    // are we in the playlists location?
    bool inPlaylists = m_vecItems->GetPath().Equals(CUtil::VideoPlaylistsLocation()) ||
                       m_vecItems->GetPath().Equals("special://videoplaylists/");

    if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_artist.empty())
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetArtistByName(StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator)) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ARTIST, 20396);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetAlbumByName(item->GetVideoInfoTag()->m_strAlbum) > -1)
        buttons.Add(CONTEXT_BUTTON_GO_TO_ALBUM, 20397);
    }
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_strAlbum.size() > 0 &&
        item->GetVideoInfoTag()->m_artist.size() > 0                              &&
        item->GetVideoInfoTag()->m_strTitle.size() > 0)
    {
      CMusicDatabase database;
      database.Open();
      if (database.GetSongByArtistAndAlbumAndTitle(StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator),
                                                   item->GetVideoInfoTag()->m_strAlbum,
                                                   item->GetVideoInfoTag()->m_strTitle) > -1)
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 20398);
      }
    }
    if (!item->IsParentFolder())
    {
      ADDON::ScraperPtr info;
      VIDEO::SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);

      if (info && info->Content() == CONTENT_TVSHOWS)
        buttons.Add(CONTEXT_BUTTON_INFO, item->m_bIsFolder ? 20351 : 20352);
      else if (info && info->Content() == CONTENT_MUSICVIDEOS)
        buttons.Add(CONTEXT_BUTTON_INFO,20393);
      else if (info && info->Content() == CONTENT_MOVIES)
        buttons.Add(CONTEXT_BUTTON_INFO, 13346);

      // can we update the database?
      if (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if (node == NODE_TYPE_TITLE_TVSHOWS)
        {
          if (g_application.IsVideoScanning())
            buttons.Add(CONTEXT_BUTTON_STOP_SCANNING, 13353);
          else
            buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }
        if (!item->IsPlugin() && !item->IsScript() && !item->IsLiveTV() && !item->IsAddonsPath() &&
            item->GetPath() != "sources://video/" && item->GetPath() != "special://videoplaylists/" &&
            item->GetPath().Left(19) != "newsmartplaylist://" && item->GetPath().Left(14) != "newplaylist://" &&
            item->GetPath().Left(9) != "newtag://")
        {
          if (item->m_bIsFolder)
          {
            // Have both options for folders since we don't know whether all childs are watched/unwatched
            buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
            buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
          }
          else
          {
            if (item->GetOverlayImage().Equals("OverlayWatched.png"))
              buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); //Mark as UnWatched
            else
              buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   //Mark as Watched
          }
        }
        if (!g_application.IsVideoScanning() && item->IsVideoDb() && item->HasVideoInfoTag() &&
           (!item->m_bIsFolder || m_vecItems->GetContent().Equals("movies") || m_vecItems->GetContent().Equals("tvshows")))
        {
          buttons.Add(CONTEXT_BUTTON_EDIT, 16106);
        }

        if (node == NODE_TYPE_SEASONS && item->m_bIsFolder)
          buttons.Add(CONTEXT_BUTTON_SET_SEASON_ART, 13511);

        if (StringUtils::StartsWithNoCase(item->GetPath(), "videodb://movies/sets/") && item->GetPath().size() > 22 && item->m_bIsFolder) // sets
        {
          buttons.Add(CONTEXT_BUTTON_SET_MOVIESET_ART, 13511);
          buttons.Add(CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS, 20465);
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);
        }

        if (m_vecItems->GetContent() == "tags" && item->m_bIsFolder) // tags
        {
          CVideoDbUrl videoUrl;
          if (videoUrl.FromString(item->GetPath()))
          {
            std::string mediaType = videoUrl.GetItemType();

            CStdString strLabelAdd; strLabelAdd.Format(g_localizeStrings.Get(20460), GetLocalizedType(videoUrl.GetItemType()).c_str());
            CStdString strLabelRemove; strLabelRemove.Format(g_localizeStrings.Get(20461), GetLocalizedType(videoUrl.GetItemType()).c_str());
            buttons.Add(CONTEXT_BUTTON_TAGS_ADD_ITEMS, strLabelAdd);
            buttons.Add(CONTEXT_BUTTON_TAGS_REMOVE_ITEMS, strLabelRemove);
            buttons.Add(CONTEXT_BUTTON_DELETE, 646);
          }
        }

        if (node == NODE_TYPE_ACTOR && !dir.IsAllItem(item->GetPath()) && item->m_bIsFolder)
        {
          if (StringUtils::StartsWithNoCase(m_vecItems->GetPath(), "videodb://musicvideos")) // mvids
            buttons.Add(CONTEXT_BUTTON_SET_ARTIST_THUMB, 13359);
          else
            buttons.Add(CONTEXT_BUTTON_SET_ACTOR_THUMB, 20403);
        }
        if (item->IsVideoDb() && item->HasVideoInfoTag() &&
          (!item->m_bIsFolder || node == NODE_TYPE_TITLE_TVSHOWS))
          buttons.Add(CONTEXT_BUTTON_DELETE, 646);
      }

      if (!m_vecItems->IsVideoDb() && !m_vecItems->IsVirtualDirectoryRoot())
      { // non-video db items, file operations are allowed
        if ((CSettings::Get().GetBool("filelists.allowfiledeletion") &&
            CUtil::SupportsWriteFileOperations(item->GetPath())) ||
            (inPlaylists && !URIUtils::GetFileName(item->GetPath()).Equals("PartyMode-Video.xsp")
                         && (item->IsPlayList() || item->IsSmartPlayList())))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
        // add "Set/Change content" to folders
        if (item->m_bIsFolder && !item->IsVideoDb() && !item->IsPlayList() && !item->IsSmartPlayList() && !item->IsLibraryFolder() && !item->IsLiveTV() && !item->IsPlugin() && !item->IsAddonsPath() && !URIUtils::IsUPnP(item->GetPath()))
        {
          if (!g_application.IsVideoScanning())
          {
            if (info && info->Content() != CONTENT_NONE)
            {
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
              buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
            }
            else
              buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
          }
        }
      }
      if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
        buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
    }
  }
}

// predicate used by sorting and set_difference
bool compFileItemsByDbId(const CFileItemPtr& lhs, const CFileItemPtr& rhs) 
{
  return lhs->HasVideoInfoTag() && rhs->HasVideoInfoTag() && lhs->GetVideoInfoTag()->m_iDbId < rhs->GetVideoInfoTag()->m_iDbId;
}

bool CGUIWindowVideoNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("video", item, button))
  {
    //TODO should we search DB for entries from plugins?
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin()
        && !item->IsLiveTV() &&!item->IsRSS() && !URIUtils::IsUPnP(item->GetPath()))
    {
      OnUnAssignContent(item->GetPath(),20375,20340,20341);
    }
    Refresh();
    return true;
  }
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT:
    {
      CONTEXT_BUTTON ret = (CONTEXT_BUTTON)CGUIDialogVideoInfo::ManageVideoItem(item);
      if (ret >= 0)
      {
        if (ret == CONTEXT_BUTTON_MARK_WATCHED)
          m_viewControl.SetSelectedItem(itemNumber + 1);

        Refresh(true);
      }
      return true;
    }

  case CONTEXT_BUTTON_SET_SEASON_ART:
  case CONTEXT_BUTTON_SET_ACTOR_THUMB:
  case CONTEXT_BUTTON_SET_ARTIST_THUMB:
  case CONTEXT_BUTTON_SET_MOVIESET_ART:
    {
      // Grab the thumbnails from the web
      CFileItemList items;
      CFileItemPtr noneitem(new CFileItem("thumb://None", false));
      CStdString currentThumb;
      int idArtist = -1;
      CStdString artistPath;
      string artType = "thumb";
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CMusicDatabase database;
        database.Open();
        idArtist = database.GetArtistByName(m_vecItems->Get(itemNumber)->GetLabel());
        database.GetArtistPath(idArtist, artistPath);
        currentThumb = database.GetArtForItem(idArtist, "artist", "thumb");
        if (currentThumb.empty())
          currentThumb = m_database.GetArtForItem(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iDbId, m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_type, artType);
      }
      else if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
        currentThumb = m_database.GetArtForItem(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iDbId, m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_type, artType);
      else
      { // SEASON, SET
        map<string, string> currentArt;
        artType = CGUIDialogVideoInfo::ChooseArtType(*m_vecItems->Get(itemNumber), currentArt);
        if (artType.empty())
          return false;

        if (artType == "fanart")
        {
          OnChooseFanart(*m_vecItems->Get(itemNumber));
          return true;
        }

        if (currentArt.find(artType) != currentArt.end())
          currentThumb = currentArt[artType];
        else if ((artType == "poster" || artType == "banner") && currentArt.find("thumb") != currentArt.end())
          currentThumb = currentArt["thumb"];
      }
      if (!currentThumb.IsEmpty())
      {
        CFileItemPtr item(new CFileItem("thumb://Current", false));
        item->SetArt("thumb", currentThumb);
        item->SetLabel(g_localizeStrings.Get(13512));
        items.Add(item);
      }
      noneitem->SetIconImage("DefaultFolder.png");
      noneitem->SetLabel(g_localizeStrings.Get(13515));

      vector<CStdString> thumbs;
      if (button != CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CVideoInfoTag tag;
        if (button == CONTEXT_BUTTON_SET_SEASON_ART)
          m_database.GetTvShowInfo("",tag,m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iIdShow);
        else
          tag = *m_vecItems->Get(itemNumber)->GetVideoInfoTag();
        if (button == CONTEXT_BUTTON_SET_SEASON_ART)
          tag.m_strPictureURL.GetThumbURLs(thumbs, artType, m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iSeason);
        else
          tag.m_strPictureURL.GetThumbURLs(thumbs, artType);

        for (unsigned int i = 0; i < thumbs.size(); i++)
        {
          CStdString strItemPath;
          strItemPath.Format("thumb://Remote%i",i);
          CFileItemPtr item(new CFileItem(strItemPath, false));
          item->SetArt("thumb", thumbs[i]);
          item->SetIconImage("DefaultPicture.png");
          item->SetLabel(g_localizeStrings.Get(13513));
          items.Add(item);

          // TODO: Do we need to clear the cached image?
          //    CTextureCache::Get().ClearCachedImage(thumbs[i]);
        }
      }

      bool local=false;
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CStdString strThumb = URIUtils::AddFileToFolder(artistPath, "folder.jpg");
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(13514));
          pItem->SetArt("thumb", strThumb);
          items.Add(pItem);
          local = true;
        }
        else
          noneitem->SetIconImage("DefaultArtist.png");
      }

      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
      {
        CStdString picturePath;
        CStdString strThumb = URIUtils::AddFileToFolder(picturePath, "folder.jpg");
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(13514));
          pItem->SetArt("thumb", strThumb);
          items.Add(pItem);
          local = true;
        }
        else
          noneitem->SetIconImage("DefaultActor.png");
      }

      if (button == CONTEXT_BUTTON_SET_MOVIESET_ART)
        noneitem->SetIconImage("DefaultVideo.png");

      if (!local)
        items.Add(noneitem);

      VECSOURCES sources=*CMediaSourceSettings::Get().GetSources("video");
      g_mediaManager.GetLocalDrives(sources);
      CStdString result;
      CGUIDialogVideoInfo::AddItemPathToFileBrowserSources(sources, *item);
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources,
                                                  g_localizeStrings.Get(13511), result))
      {
        return false;   // user cancelled
      }

      if (result == "thumb://Current")
        result = currentThumb;   // user chose the one they have

      // delete the thumbnail if that's what the user wants, else overwrite with the
      // new thumbnail
      if (result.Left(14) == "thumb://Remote")
      {
        int number = atoi(result.Mid(14));
        result = thumbs[number];
      }
      else if (result == "thumb://None")
        result.clear();
      if (button == CONTEXT_BUTTON_SET_MOVIESET_ART ||
          button == CONTEXT_BUTTON_SET_ACTOR_THUMB ||
          button == CONTEXT_BUTTON_SET_SEASON_ART ||
         (button == CONTEXT_BUTTON_SET_ARTIST_THUMB && idArtist < 0))
        m_database.SetArtForItem(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iDbId, m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_type, artType, result);
      else
      {
        CMusicDatabase db;
        if (db.Open())
          db.SetArtForItem(idArtist, "artist", artType, result);
      }

      CUtil::DeleteVideoDatabaseDirectoryCache();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
      g_windowManager.SendMessage(msg);
      Refresh();

      return true;
    }
  case CONTEXT_BUTTON_TAGS_ADD_ITEMS:
    {
      CVideoDbUrl videoUrl;
      if (!videoUrl.FromString(item->GetPath()))
        return false;
      
      std::string mediaType = videoUrl.GetItemType();
      mediaType = mediaType.substr(0, mediaType.length() - 1);

      CFileItemList items;
      CStdString localizedType = GetLocalizedType(mediaType);
      CStdString strLabel; strLabel.Format(g_localizeStrings.Get(20464), localizedType.c_str());
      if (!GetItemsForTag(strLabel, mediaType, items, item->GetVideoInfoTag()->m_iDbId))
        return true;

      CVideoDatabase videodb;
      if (!videodb.Open())
        return true;

      for (int index = 0; index < items.Size(); index++)
      {
        if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
          continue;

        videodb.AddTagToItem(items[index]->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_iDbId, mediaType);
      }

      // we need to clear any cached version of this tag's listing
      items.SetPath(item->GetPath());
      items.RemoveDiscCache(GetID());
      return true;
    }
  case CONTEXT_BUTTON_TAGS_REMOVE_ITEMS:
    {
      CVideoDbUrl videoUrl;
      if (!videoUrl.FromString(item->GetPath()))
        return false;
      
      std::string mediaType = videoUrl.GetItemType();
      mediaType = mediaType.substr(0, mediaType.length() - 1);

      CFileItemList items;
      CStdString localizedType = GetLocalizedType(mediaType);
      CStdString strLabel; strLabel.Format(g_localizeStrings.Get(20464), localizedType.c_str());
      if (!GetItemsForTag(strLabel, mediaType, items, item->GetVideoInfoTag()->m_iDbId, false))
        return true;

      CVideoDatabase videodb;
      if (!videodb.Open())
        return true;

      for (int index = 0; index < items.Size(); index++)
      {
        if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
          continue;

        videodb.RemoveTagFromItem(items[index]->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_iDbId, mediaType);
      }

      // we need to clear any cached version of this tag's listing
      items.SetPath(item->GetPath());
      items.RemoveDiscCache(GetID());
      return true;
    }
  case CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS:
    {
      CFileItemList originalItems;
      CFileItemList selectedItems;

      if (!CGUIDialogVideoInfo::GetMoviesForSet(item.get(), originalItems, selectedItems) || selectedItems.Size() == 0) // need at least one item selected
        return true;
      VECFILEITEMS original = originalItems.GetList();
      std::sort(original.begin(), original.end(), compFileItemsByDbId);
      VECFILEITEMS selected = selectedItems.GetList();
      std::sort(selected.begin(), selected.end(), compFileItemsByDbId);

      bool refreshNeeded = false;
      // update the "added" items
      VECFILEITEMS addedItems;
      set_difference(selected.begin(),selected.end(), original.begin(),original.end(), std::back_inserter(addedItems), compFileItemsByDbId);
      for (VECFILEITEMS::iterator it = addedItems.begin();  it != addedItems.end(); ++it)
      {
        if (CGUIDialogVideoInfo::SetMovieSet(it->get(), item.get()))
          refreshNeeded = true;
      }
      // update the "deleted" items
      CFileItemPtr clearItem(new CFileItem());
      clearItem->GetVideoInfoTag()->m_iDbId = -1; // -1 will be used to clear set
      VECFILEITEMS deletedItems;
      set_difference(original.begin(),original.end(), selected.begin(),selected.end(), std::back_inserter(deletedItems), compFileItemsByDbId);
      for (VECFILEITEMS::iterator it = deletedItems.begin();  it != deletedItems.end(); ++it)
      {
        if (CGUIDialogVideoInfo::SetMovieSet(it->get(), clearItem.get()))
          refreshNeeded = true;
      }

      // we need to clear any cached version of this tag's listing
      if (refreshNeeded) 
        Refresh();
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://artists/%ld/",database.GetArtistByName(StringUtils::Join(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator)));
      g_windowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ALBUM:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://albums/%ld/",database.GetAlbumByName(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum));
      g_windowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CMusicDatabase database;
      database.Open();
      CSong song;
      if (database.GetSong(database.GetSongByArtistAndAlbumAndTitle(StringUtils::Join(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator),m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum,
                                                                        m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strTitle),
                                                                        song))
      {
        CApplicationMessenger::Get().PlayFile(song);
      }
      return true;
    }

  default:
    break;

  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoNav::OnChooseFanart(const CFileItem &videoItem)
{
  if (!videoItem.HasVideoInfoTag())
    return;

  CFileItem item(videoItem);

  CFileItemList items;

  CVideoThumbLoader loader;
  loader.LoadItem(&item);

  if (item.HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetArt("thumb", item.GetArt("fanart"));
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // add the none option
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultVideo.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  CStdString result;
  VECSOURCES sources(*CMediaSourceSettings::Get().GetSources("video"));
  g_mediaManager.GetLocalDrives(sources);
  CGUIDialogVideoInfo::AddItemPathToFileBrowserSources(sources, item);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445) || result.Equals("fanart://Current"))
    return;

  if (result.Equals("fanart://None") || !CFile::Exists(result))
    result.clear();
  if (!result.IsEmpty() && flip)
    result = CTextureCache::GetWrappedImageURL(result, "", "flipped");

  // update the db
  CVideoDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(item.GetVideoInfoTag()->m_iDbId, item.GetVideoInfoTag()->m_type, "fanart", result);
    db.Close();
  }

  // clear view cache and reload images
  CUtil::DeleteVideoDatabaseDirectoryCache();

  Refresh();
}

bool CGUIWindowVideoNav::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item->m_bIsFolder && item->IsVideoDb() && !item->Exists())
  {
    CLog::Log(LOGDEBUG, "%s called on '%s' but file doesn't exist", __FUNCTION__, item->GetPath().c_str());
    if (!DeleteItem(item.get(), true))
      return true;

    // update list
    Refresh(true);
    m_viewControl.SetSelectedItem(iItem);
    return true;
  }
  else if (item->GetPath().Left(9).Equals("newtag://"))
  {
    // dont allow update while scanning
    if (g_application.IsVideoScanning())
    {
      CGUIDialogOK::ShowAndGetInput(257, 0, 14057, 0);
      return true;
    }

    //Get the new title
    CStdString strTag;
    if (!CGUIKeyboardFactory::ShowAndGetInput(strTag, g_localizeStrings.Get(20462), false))
      return true;

    CVideoDatabase videodb;
    if (!videodb.Open())
      return true;

    // get the media type and convert from plural to singular (by removing the trailing "s")
    CStdString mediaType = item->GetPath().Mid(9);
    mediaType = mediaType.Left(mediaType.size() - 1);
    CStdString localizedType = GetLocalizedType(mediaType);
    if (localizedType.empty())
      return true;

    if (!videodb.GetSingleValue("tag", "tag.idTag", videodb.PrepareSQL("tag.strTag = '%s' AND tag.idTag IN (SELECT taglinks.idTag FROM taglinks WHERE taglinks.media_type = '%s')", strTag.c_str(), mediaType.c_str())).empty())
    {
      CStdString strError; strError.Format(g_localizeStrings.Get(20463), strTag.c_str());
      CGUIDialogOK::ShowAndGetInput(20462, "", strError, "");
      return true;
    }

    int idTag = videodb.AddTag(strTag);
    CFileItemList items;
    CStdString strLabel; strLabel.Format(g_localizeStrings.Get(20464), localizedType.c_str());
    if (GetItemsForTag(strLabel, mediaType, items, idTag))
    {
      for (int index = 0; index < items.Size(); index++)
      {
        if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
          continue;

        videodb.AddTagToItem(items[index]->GetVideoInfoTag()->m_iDbId, idTag, mediaType);
      }
    }

    Refresh(true);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem);
}

CStdString CGUIWindowVideoNav::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("MovieGenres"))
    return "videodb://movies/genres/";
  else if (dir.Equals("MovieTitles"))
    return "videodb://movies/titles/";
  else if (dir.Equals("MovieYears"))
    return "videodb://movies/years/";
  else if (dir.Equals("MovieActors"))
    return "videodb://movies/actors/";
  else if (dir.Equals("MovieDirectors"))
    return "videodb://movies/directors/";
  else if (dir.Equals("MovieStudios"))
    return "videodb://movies/studios/";
  else if (dir.Equals("MovieSets"))
    return "videodb://movies/sets/";
  else if (dir.Equals("MovieCountries"))
    return "videodb://movies/countries/";
  else if (dir.Equals("MovieTags"))
    return "videodb://movies/tags/";
  else if (dir.Equals("Movies"))
    return "videodb://movies/";
  else if (dir.Equals("TvShowGenres"))
    return "videodb://tvshows/genres/";
  else if (dir.Equals("TvShowTitles"))
    return "videodb://tvshows/titles/";
  else if (dir.Equals("TvShowYears"))
    return "videodb://tvshows/years/";
  else if (dir.Equals("TvShowActors"))
    return "videodb://tvshows/actors/";
  else if (dir.Equals("TvShowStudios"))
    return "videodb://tvshows/studios/";
  else if (dir.Equals("TvShowTags"))
    return "videodb://tvshows/tags/";
  else if (dir.Equals("TvShows"))
    return "videodb://tvshows/";
  else if (dir.Equals("MusicVideoGenres"))
    return "videodb://musicvideos/genres/";
  else if (dir.Equals("MusicVideoTitles"))
    return "videodb://musicvideos/titles/";
  else if (dir.Equals("MusicVideoYears"))
    return "videodb://musicvideos/years/";
  else if (dir.Equals("MusicVideoArtists"))
    return "videodb://musicvideos/artist/";
  else if (dir.Equals("MusicVideoAlbums"))
    return "videodb://musicvideos/albums/";
  else if (dir.Equals("MusicVideoDirectors"))
    return "videodb://musicvideos/directors/";
  else if (dir.Equals("MusicVideoStudios"))
    return "videodb://musicvideos/studios/";
  else if (dir.Equals("MusicVideoTags"))
    return "videodb://musicvideos/tags/";
  else if (dir.Equals("MusicVideos"))
    return "videodb://musicvideos/";
  else if (dir.Equals("RecentlyAddedMovies"))
    return "videodb://recentlyaddedmovies/";
  else if (dir.Equals("RecentlyAddedEpisodes"))
    return "videodb://recentlyaddedepisodes/";
  else if (dir.Equals("RecentlyAddedMusicVideos"))
    return "videodb://recentlyaddedmusicvideos/";
  else if (dir.Equals("Files"))
    return "sources://video/";
  return CGUIWindowVideoBase::GetStartFolder(dir);
}

bool CGUIWindowVideoNav::ApplyWatchedFilter(CFileItemList &items)
{
  bool listchanged = false;
  CVideoDatabaseDirectory dir;
  NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());

  // now filter watched items as necessary
  bool filterWatched=false;
  if (node == NODE_TYPE_EPISODES
  ||  node == NODE_TYPE_SEASONS
  ||  node == NODE_TYPE_SETS
  ||  node == NODE_TYPE_TAGS
  ||  node == NODE_TYPE_TITLE_MOVIES
  ||  node == NODE_TYPE_TITLE_TVSHOWS
  ||  node == NODE_TYPE_TITLE_MUSICVIDEOS
  ||  node == NODE_TYPE_RECENTLY_ADDED_EPISODES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MOVIES
  ||  node == NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS)
    filterWatched = true;
  if (!items.IsVideoDb())
    filterWatched = true;
  if (items.GetContent() == "tvshows" &&
     (items.IsSmartPlayList() || items.IsLibraryFolder()))
    node = NODE_TYPE_TITLE_TVSHOWS; // so that the check below works

  int watchMode = CMediaSettings::Get().GetWatchedMode(m_vecItems->GetContent());

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);

    if(item->HasVideoInfoTag() && (node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS))
    {
      if (watchMode == WatchedModeUnwatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("unwatchedepisodes").asInteger();
      if (watchMode == WatchedModeWatched)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("watchedepisodes").asInteger();
      if (watchMode == WatchedModeAll)
        item->GetVideoInfoTag()->m_iEpisode = (int)item->GetProperty("totalepisodes").asInteger();
      item->SetProperty("numepisodes", item->GetVideoInfoTag()->m_iEpisode);
      listchanged = true;
    }

    if (filterWatched)
    {
      if((watchMode==WatchedModeWatched   && item->GetVideoInfoTag()->m_playCount== 0)
      || (watchMode==WatchedModeUnwatched && item->GetVideoInfoTag()->m_playCount > 0))
      {
        items.Remove(i);
        i--;
        listchanged = true;
      }
    }
  }

  if(node == NODE_TYPE_TITLE_TVSHOWS || node == NODE_TYPE_SEASONS)
  {
    // the watched filter may change the "numepisodes" property which is reflected in the TV_SHOWS and SEASONS nodes
    // therefore, the items labels have to be refreshed, and possibly the list needs resorting as well.
    items.ClearSortState(); // this is needed to force resorting even if sort method did not change
    FormatAndSort(items);
  }

  return listchanged;
}

bool CGUIWindowVideoNav::GetItemsForTag(const CStdString &strHeading, const std::string &type, CFileItemList &items, int idTag /* = -1 */, bool showAll /* = true */)
{
  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  MediaType mediaType = MediaTypeNone;
  std::string baseDir = "videodb://";
  std::string idColumn;
  if (type.compare("movie") == 0)
  {
    mediaType = MediaTypeMovie;
    baseDir += "movies";
    idColumn = "idMovie";
  }
  else if (type.compare("tvshow") == 0)
  {
    mediaType = MediaTypeTvShow;
    baseDir += "tvshows";
    idColumn = "idShow";
  }
  else if (type.compare("musicvideo") == 0)
  {
    mediaType = MediaTypeMusicVideo;
    baseDir += "musicvideos";
    idColumn = "idMVideo";
  }

  baseDir += "/titles/";
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(baseDir))
    return false;

  CVideoDatabase::Filter filter;
  if (idTag > 0)
  {
    if (!showAll)
      videoUrl.AddOption("tagid", idTag);
    else
      filter.where = videodb.PrepareSQL("%sview.%s NOT IN (SELECT taglinks.idMedia FROM taglinks WHERE taglinks.idTag = %d AND taglinks.media_type = '%s')", type.c_str(), idColumn.c_str(), idTag, type.c_str());
  }

  CFileItemList listItems;
  if (!videodb.GetSortedVideos(mediaType, videoUrl.ToString(), SortDescription(), listItems, filter) || listItems.Size() <= 0)
    return false;

  CGUIDialogSelect *dialog = (CGUIDialogSelect *)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (dialog == NULL)
    return false;

  listItems.Sort(SortByLabel, SortOrderAscending, SortAttributeIgnoreArticle);

  dialog->Reset();
  dialog->SetMultiSelection(true);
  dialog->SetHeading(strHeading);
  dialog->SetItems(&listItems);
  dialog->EnableButton(true, 186);
  dialog->DoModal();

  items.Copy(dialog->GetSelectedItems());
  return items.Size() > 0;
}

CStdString CGUIWindowVideoNav::GetLocalizedType(const std::string &strType)
{
  if (strType == "movie" || strType == "movies")
    return g_localizeStrings.Get(20342);
  else if (strType == "tvshow" || strType == "tvshows")
    return g_localizeStrings.Get(20343);
  else if (strType == "episode" || strType == "episodes")
    return g_localizeStrings.Get(20359);
  else if (strType == "musicvideo" || strType == "musicvideos")
    return g_localizeStrings.Get(20391);
  else
    return "";
}
