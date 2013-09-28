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

#include "PlayerOperations.h"
#include "Application.h"
#include "Util.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "GUIUserMessages.h"
#include "pictures/GUIWindowSlideShow.h"
#include "interfaces/Builtins.h"
#include "PartyModeManager.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "VideoLibrary.h"
#include "video/VideoDatabase.h"
#include "AudioLibrary.h"
#include "GUIInfoManager.h"
#include "filesystem/File.h"
#include "PartyModeManager.h"
#include "epg/EpgInfoTag.h"
#include "music/MusicDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "cores/IPlayer.h"
#include "settings/MediaSettings.h"

using namespace JSONRPC;
using namespace PLAYLIST;
using namespace PVR;

JSONRPC_STATUS CPlayerOperations::GetActivePlayers(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int activePlayers = GetActivePlayers();
  result = CVariant(CVariant::VariantTypeArray);

  if (activePlayers & Video)
  {
    CVariant video = CVariant(CVariant::VariantTypeObject);
    video["playerid"] = GetPlaylist(Video);
    video["type"] = "video";
    result.append(video);
  }
  if (activePlayers & Audio)
  {
    CVariant audio = CVariant(CVariant::VariantTypeObject);
    audio["playerid"] = GetPlaylist(Audio);
    audio["type"] = "audio";
    result.append(audio);
  }
  if (activePlayers & Picture)
  {
    CVariant picture = CVariant(CVariant::VariantTypeObject);
    picture["playerid"] = GetPlaylist(Picture);
    picture["type"] = "picture";
    result.append(picture);
  }

  return OK;
}

JSONRPC_STATUS CPlayerOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);

  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(player, propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CPlayerOperations::GetItem(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  CFileItemPtr fileItem;

  switch (player)
  {
    case Video:
    case Audio:
    {
      fileItem = CFileItemPtr(new CFileItem(g_application.CurrentFileItem()));
      if (IsPVRChannel())
      {
        CPVRChannelPtr currentChannel;
        if (g_PVRManager.GetCurrentChannel(currentChannel) && currentChannel.get() != NULL)
          fileItem = CFileItemPtr(new CFileItem(*currentChannel.get()));
      }
      else if (player == Video)
      {
        if (!CVideoLibrary::FillFileItem(g_application.CurrentFile(), fileItem, parameterObject))
        {
          const CVideoInfoTag *currentVideoTag = g_infoManager.GetCurrentMovieTag();
          if (currentVideoTag != NULL)
          {
            CStdString originalLabel = fileItem->GetLabel();
            fileItem->SetFromVideoInfoTag(*currentVideoTag);
            if (fileItem->GetLabel().empty())
              fileItem->SetLabel(originalLabel);
          }
          fileItem->SetPath(g_application.CurrentFileItem().GetPath());
        }
      }
      else
      {
        if (!CAudioLibrary::FillFileItem(g_application.CurrentFile(), fileItem, parameterObject))
        {
          const MUSIC_INFO::CMusicInfoTag *currentMusicTag = g_infoManager.GetCurrentSongTag();
          if (currentMusicTag != NULL)
          {
            CStdString originalLabel = fileItem->GetLabel();
            fileItem = CFileItemPtr(new CFileItem(*currentMusicTag));
            if (fileItem->GetLabel().empty())
              fileItem->SetLabel(originalLabel);
          }
          fileItem->SetPath(g_application.CurrentFileItem().GetPath());
        }
      }

      if (IsPVRChannel())
        break;

      if (player == Video)
      {
        bool additionalInfo = false;
        for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
        {
          CStdString fieldValue = itr->asString();
          if (fieldValue == "cast" || fieldValue == "set" || fieldValue == "setid" || fieldValue == "showlink" || fieldValue == "resume" ||
             (fieldValue == "streamdetails" && !fileItem->GetVideoInfoTag()->m_streamDetails.HasItems()))
            additionalInfo = true;
        }

        CVideoDatabase videodatabase;
        if ((additionalInfo) &&
            videodatabase.Open())
        {
          if (additionalInfo)
          {
            switch (fileItem->GetVideoContentType())
            {
              case VIDEODB_CONTENT_MOVIES:
                videodatabase.GetMovieInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_MUSICVIDEOS:
                videodatabase.GetMusicVideoInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_EPISODES:
                videodatabase.GetEpisodeInfo("", *(fileItem->GetVideoInfoTag()), fileItem->GetVideoInfoTag()->m_iDbId);
                break;

              case VIDEODB_CONTENT_TVSHOWS:
              case VIDEODB_CONTENT_MOVIE_SETS:
              default:
                break;
            }
          }

          videodatabase.Close();
        }
      }
      else if (player == Audio)
      {
        if (fileItem->IsMusicDb())
        {
          CMusicDatabase musicdb;
          CFileItemList items;
          items.Add(fileItem);
          CAudioLibrary::GetAdditionalSongDetails(parameterObject, items, musicdb);
        }
      }
      break;
    }

    case Picture:
    {
      CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (!slideshow)
        return FailedToExecute;

      CFileItemList slides;
      slideshow->GetSlideShowContents(slides);
      fileItem = slides[slideshow->CurrentSlide() - 1];
      break;
    }

    case None:
    default:
      return FailedToExecute;
  }

  HandleFileItem("id", !IsPVRChannel(), "item", fileItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CPlayerOperations::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CGUIWindowSlideShow *slideshow = NULL;
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (!g_application.m_pPlayer->CanPause())
        return FailedToExecute;
      
      if (parameterObject["play"].isString())
        CBuiltins::Execute("playercontrol(play)");
      else
      {
        if (parameterObject["play"].asBoolean())
        {
          if (g_application.m_pPlayer->IsPausedPlayback())
            CApplicationMessenger::Get().MediaPause();
          else if (g_application.m_pPlayer->GetPlaySpeed() != 1)
            g_application.m_pPlayer->SetPlaySpeed(1, g_application.IsMutedInternal());
        }
        else if (!g_application.m_pPlayer->IsPausedPlayback())
          CApplicationMessenger::Get().MediaPause();
      }
      result["speed"] = g_application.m_pPlayer->IsPausedPlayback() ? 0 : g_application.m_pPlayer->GetPlaySpeed();
      return OK;

    case Picture:
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (slideshow && slideshow->IsPlaying() &&
         (parameterObject["play"].isString() ||
         (parameterObject["play"].isBoolean() && parameterObject["play"].asBoolean() == slideshow->IsPaused())))
        SendSlideshowAction(ACTION_PAUSE);

      if (slideshow && slideshow->IsPlaying() && !slideshow->IsPaused())
        result["speed"] = slideshow->GetDirection();
      else
        result["speed"] = 0;
      return OK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      CApplicationMessenger::Get().MediaStop(true, (int)parameterObject["playerid"].asInteger());
      return ACK;

    case Picture:
      SendSlideshowAction(ACTION_STOP);
      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::SetSpeed(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (parameterObject["speed"].isInteger())
      {
        int speed = (int)parameterObject["speed"].asInteger();
        if (speed != 0)
        {
          // If the player is paused we first need to unpause
          if (g_application.m_pPlayer->IsPausedPlayback())
            g_application.m_pPlayer->Pause();
          g_application.m_pPlayer->SetPlaySpeed(speed, g_application.IsMutedInternal());
        }
        else
          g_application.m_pPlayer->Pause();
      }
      else if (parameterObject["speed"].isString())
      {
        if (parameterObject["speed"].asString().compare("increment") == 0)
          CBuiltins::Execute("playercontrol(forward)");
        else
          CBuiltins::Execute("playercontrol(rewind)");
      }
      else
        return InvalidParams;

      result["speed"] = g_application.m_pPlayer->IsPausedPlayback() ? 0 : g_application.m_pPlayer->GetPlaySpeed();
      return OK;

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Seek(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  switch (player)
  {
    case Video:
    case Audio:
      if (!g_application.m_pPlayer->CanSeek())
        return FailedToExecute;
      
      if (parameterObject["value"].isObject())
        g_application.SeekTime(ParseTimeInSeconds(parameterObject["value"]));
      else if (IsType(parameterObject["value"], NumberValue))
        g_application.SeekPercentage(parameterObject["value"].asFloat());
      else if (parameterObject["value"].isString())
      {
        CStdString step = parameterObject["value"].asString();
        if (step.Equals("smallforward"))
          CBuiltins::Execute("playercontrol(smallskipforward)");
        else if (step.Equals("smallbackward"))
          CBuiltins::Execute("playercontrol(smallskipbackward)");
        else if (step.Equals("bigforward"))
          CBuiltins::Execute("playercontrol(bigskipforward)");
        else if (step.Equals("bigbackward"))
          CBuiltins::Execute("playercontrol(bigskipbackward)");
        else
          return InvalidParams;
      }
      else
        return InvalidParams;

      GetPropertyValue(player, "percentage", result["percentage"]);
      GetPropertyValue(player, "time", result["time"]);
      GetPropertyValue(player, "totaltime", result["totaltime"]);
      return OK;

    case Picture:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Move(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string direction = parameterObject["direction"].asString();
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (direction == "left")
        SendSlideshowAction(ACTION_MOVE_LEFT);
      else if (direction == "right")
        SendSlideshowAction(ACTION_MOVE_RIGHT);
      else if (direction == "up")
        SendSlideshowAction(ACTION_MOVE_UP);
      else if (direction == "down")
        SendSlideshowAction(ACTION_MOVE_DOWN);
      else
        return InvalidParams;

      return ACK;

    case Video:
    case Audio:
      if (direction == "left" || direction == "up")
        CApplicationMessenger::Get().SendAction(CAction(ACTION_PREV_ITEM));
      else if (direction == "right" || direction == "down")
        CApplicationMessenger::Get().SendAction(CAction(ACTION_NEXT_ITEM));
      else
        return InvalidParams;

      return ACK;

    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Zoom(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant zoom = parameterObject["zoom"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (zoom.isInteger())
        SendSlideshowAction(ACTION_ZOOM_LEVEL_NORMAL + ((int)zoom.asInteger() - 1));
      else if (zoom.isString())
      {
        std::string strZoom = zoom.asString();
        if (strZoom == "in")
          SendSlideshowAction(ACTION_ZOOM_IN);
        else if (strZoom == "out")
          SendSlideshowAction(ACTION_ZOOM_OUT);
        else
          return InvalidParams;
      }
      else
        return InvalidParams;

      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Rotate(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Picture:
      if (parameterObject["value"].asString().compare("clockwise") == 0)
        SendSlideshowAction(ACTION_ROTATE_PICTURE_CW);
      else
        SendSlideshowAction(ACTION_ROTATE_PICTURE_CCW);
      return ACK;

    case Video:
    case Audio:
    case None:
    default:
      return FailedToExecute;
  }
}

JSONRPC_STATUS CPlayerOperations::Open(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant optionShuffled = parameterObject["options"]["shuffled"];
  CVariant optionRepeat = parameterObject["options"]["repeat"];
  CVariant optionResume = parameterObject["options"]["resume"];

  if (parameterObject["item"].isObject() && parameterObject["item"].isMember("playlistid"))
  {
    int playlistid = (int)parameterObject["item"]["playlistid"].asInteger();

    if (playlistid < PLAYLIST_PICTURE)
    {
      // Apply the "shuffled" option if available
      if (optionShuffled.isBoolean())
        g_playlistPlayer.SetShuffle(playlistid, optionShuffled.asBoolean(), false);
      // Apply the "repeat" option if available
      if (!optionRepeat.isNull())
        g_playlistPlayer.SetRepeat(playlistid, (REPEAT_STATE)ParseRepeatState(optionRepeat), false);
    }

    switch (playlistid)
    {
      case PLAYLIST_MUSIC:
      case PLAYLIST_VIDEO:
        CApplicationMessenger::Get().MediaPlay(playlistid, (int)parameterObject["item"]["position"].asInteger());
        OnPlaylistChanged();
        break;

      case PLAYLIST_PICTURE:
        return StartSlideshow("", false, optionShuffled.isBoolean() && optionShuffled.asBoolean());
        break;
    }

    return ACK;
  }
  else if (parameterObject["item"].isObject() && parameterObject["item"].isMember("path"))
  {
    bool random = (optionShuffled.isBoolean() && optionShuffled.asBoolean()) ||
                  (!optionShuffled.isBoolean() && parameterObject["item"]["random"].asBoolean());
    return StartSlideshow(parameterObject["item"]["path"].asString(), parameterObject["item"]["recursive"].asBoolean(), random);
  }
  else if (parameterObject["item"].isObject() && parameterObject["item"].isMember("partymode"))
  {
    if (g_partyModeManager.IsEnabled())
      g_partyModeManager.Disable();
    CApplicationMessenger::Get().ExecBuiltIn("playercontrol(partymode(" + parameterObject["item"]["partymode"].asString() + "))");
    return ACK;
  }
  else if (parameterObject["item"].isObject() && parameterObject["item"].isMember("channelid"))
  {
    if (!g_PVRManager.IsStarted())
      return FailedToExecute;

    CPVRChannelGroupsContainer *channelGroupContainer = g_PVRChannelGroups;
    if (channelGroupContainer == NULL)
      return FailedToExecute;

    CPVRChannelPtr channel = channelGroupContainer->GetChannelById((int)parameterObject["item"]["channelid"].asInteger());
    if (channel == NULL)
      return InvalidParams;

    CApplicationMessenger::Get().MediaPlay(CFileItem(*channel.get()));
    return ACK;
  }
  else
  {
    CFileItemList list;
    if (FillFileItemList(parameterObject["item"], list) && list.Size() > 0)
    {
      bool slideshow = true;
      for (int index = 0; index < list.Size(); index++)
      {
        if (!list[index]->IsPicture())
        {
          slideshow = false;
          break;
        }
      }

      if (slideshow)
      {
        CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (!slideshow)
          return FailedToExecute;

        SendSlideshowAction(ACTION_STOP);
        slideshow->Reset();
        for (int index = 0; index < list.Size(); index++)
          slideshow->Add(list[index].get());

        return StartSlideshow("", false, optionShuffled.isBoolean() && optionShuffled.asBoolean());
      }
      else
      {
        // Handle "shuffled" option
        if (optionShuffled.isBoolean())
          list.SetProperty("shuffled", optionShuffled);
        // Handle "repeat" option
        if (!optionRepeat.isNull())
          list.SetProperty("repeat", ParseRepeatState(optionRepeat));
        // Handle "resume" option
        if (list.Size() == 1)
        {
          if (optionResume.isBoolean() && optionResume.asBoolean())
            list[0]->m_lStartOffset = STARTOFFSET_RESUME;
          else if (optionResume.isDouble())
            list[0]->SetProperty("StartPercent", optionResume);
          else if (optionResume.isObject())
            list[0]->m_lStartOffset = (int)(ParseTimeInSeconds(optionResume) * 75.0);
        }

        CApplicationMessenger::Get().MediaPlay(list);
      }

      return ACK;
    }
    else
      return InvalidParams;
  }

  return InvalidParams;
}

JSONRPC_STATUS CPlayerOperations::GoTo(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant to = parameterObject["to"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
      if (to.isString())
      {
        std::string strTo = to.asString();
        int actionID;
        if (strTo == "previous")
          actionID = ACTION_PREV_ITEM;
        else if (strTo == "next")
          actionID = ACTION_NEXT_ITEM;
        else
          return InvalidParams;

        CApplicationMessenger::Get().SendAction(CAction(actionID));
      }
      else if (to.isInteger())
      {
        if (IsPVRChannel())
          CApplicationMessenger::Get().SendAction(CAction(ACTION_CHANNEL_SWITCH, (float)to.asInteger()));
        else
          CApplicationMessenger::Get().PlayListPlayerPlay((int)to.asInteger());
      }
      else
        return InvalidParams;
      break;

    case Picture:
      if (to.isString())
      {
        std::string strTo = to.asString();
        int actionID;
        if (strTo == "previous")
          actionID = ACTION_PREV_PICTURE;
        else if (strTo == "next")
          actionID = ACTION_NEXT_PICTURE;
        else
          return InvalidParams;

        SendSlideshowAction(actionID);
      }
      else
        return FailedToExecute;
      break;

    case None:
    default:
      return FailedToExecute;
  }

  OnPlaylistChanged();
  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CGUIWindowSlideShow *slideshow = NULL;
  CVariant shuffle = parameterObject["shuffle"];
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      int playlistid = GetPlaylist(GetPlayer(parameterObject["playerid"]));
      if (g_playlistPlayer.IsShuffled(playlistid))
      {
        if ((shuffle.isBoolean() && !shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
        {
          CApplicationMessenger::Get().PlayListPlayerShuffle(playlistid, false);
          OnPlaylistChanged();
        }
      }
      else
      {
        if ((shuffle.isBoolean() && shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
        {
          CApplicationMessenger::Get().PlayListPlayerShuffle(playlistid, true);
          OnPlaylistChanged();
        }
      }
      break;
    }

    case Picture:
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (slideshow == NULL)
        return FailedToExecute;
      if (slideshow->IsShuffled())
      {
        if ((shuffle.isBoolean() && !shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
          return FailedToExecute;
      }
      else
      {
        if ((shuffle.isBoolean() && shuffle.asBoolean()) ||
            (shuffle.isString() && shuffle.asString() == "toggle"))
          slideshow->Shuffle();
      }
      break;

    default:
      return FailedToExecute;
  }
  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetRepeat(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      REPEAT_STATE repeat = REPEAT_NONE;
      int playlistid = GetPlaylist(GetPlayer(parameterObject["playerid"]));
      if (parameterObject["repeat"].asString() == "cycle")
      {
        REPEAT_STATE repeatPrev = g_playlistPlayer.GetRepeat(playlistid);
        if (repeatPrev == REPEAT_NONE)
          repeat = REPEAT_ALL;
        else if (repeatPrev == REPEAT_ALL)
          repeat = REPEAT_ONE;
        else
          repeat = REPEAT_NONE;
      }
      else
        repeat = (REPEAT_STATE)ParseRepeatState(parameterObject["repeat"]);

      CApplicationMessenger::Get().PlayListPlayerRepeat(playlistid, repeat);
      OnPlaylistChanged();
      break;
    }

    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetPartymode(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PlayerType player = GetPlayer(parameterObject["playerid"]);
  switch (player)
  {
    case Video:
    case Audio:
    {
      if (IsPVRChannel())
        return FailedToExecute;

      bool change = false;
      PartyModeContext context = PARTYMODECONTEXT_UNKNOWN;
      std::string strContext;
      if (player == Video)
      {
        context = PARTYMODECONTEXT_VIDEO;
        strContext = "video";
      }
      else if (player == Audio)
      {
        context = PARTYMODECONTEXT_MUSIC;
        strContext = "music";
      }

      bool toggle = parameterObject["partymode"].isString();
      if (g_partyModeManager.IsEnabled())
      {
        if (g_partyModeManager.GetType() != context)
          return InvalidParams;

        if (toggle || parameterObject["partymode"].asBoolean() == false)
          change = true;
      }
      else
      {
        if (toggle || parameterObject["partymode"].asBoolean() == true)
          change = true;
      }

      if (change)
        CApplicationMessenger::Get().ExecBuiltIn("playercontrol(partymode(" + strContext + "))");
      break;
    }

    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetAudioStream(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
      if (g_application.m_pPlayer->HasPlayer())
      {
        int index = -1;
        if (parameterObject["stream"].isString())
        {
          std::string action = parameterObject["stream"].asString();
          if (action.compare("previous") == 0)
          {
            index = g_application.m_pPlayer->GetAudioStream() - 1;
            if (index < 0)
              index = g_application.m_pPlayer->GetAudioStreamCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = g_application.m_pPlayer->GetAudioStream() + 1;
            if (index >= g_application.m_pPlayer->GetAudioStreamCount())
              index = 0;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["stream"].isInteger())
          index = (int)parameterObject["stream"].asInteger();

        if (index < 0 || g_application.m_pPlayer->GetAudioStreamCount() <= index)
          return InvalidParams;

        g_application.m_pPlayer->SetAudioStream(index);
      }
      else
        return FailedToExecute;
      break;
      
    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

JSONRPC_STATUS CPlayerOperations::SetSubtitle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  switch (GetPlayer(parameterObject["playerid"]))
  {
    case Video:
      if (g_application.m_pPlayer->HasPlayer())
      {
        int index = -1;
        if (parameterObject["subtitle"].isString())
        {
          std::string action = parameterObject["subtitle"].asString();
          if (action.compare("previous") == 0)
          {
            index = g_application.m_pPlayer->GetSubtitle() - 1;
            if (index < 0)
              index = g_application.m_pPlayer->GetSubtitleCount() - 1;
          }
          else if (action.compare("next") == 0)
          {
            index = g_application.m_pPlayer->GetSubtitle() + 1;
            if (index >= g_application.m_pPlayer->GetSubtitleCount())
              index = 0;
          }
          else if (action.compare("off") == 0)
          {
            g_application.m_pPlayer->SetSubtitleVisible(false);
            return ACK;
          }
          else if (action.compare("on") == 0)
          {
            g_application.m_pPlayer->SetSubtitleVisible(true);
            return ACK;
          }
          else
            return InvalidParams;
        }
        else if (parameterObject["subtitle"].isInteger())
          index = (int)parameterObject["subtitle"].asInteger();

        if (index < 0 || g_application.m_pPlayer->GetSubtitleCount() <= index)
          return InvalidParams;

        g_application.m_pPlayer->SetSubtitle(index);

        // Check if we need to enable subtitles to be displayed
        if (parameterObject["enable"].asBoolean() && !g_application.m_pPlayer->GetSubtitleVisible())
          g_application.m_pPlayer->SetSubtitleVisible(true);
      }
      else
        return FailedToExecute;
      break;
      
    case Audio:
    case Picture:
    default:
      return FailedToExecute;
  }

  return ACK;
}

int CPlayerOperations::GetActivePlayers()
{
  int activePlayers = 0;

  if (g_application.m_pPlayer->IsPlayingVideo() || g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRecording())
    activePlayers |= Video;
  if (g_application.m_pPlayer->IsPlayingAudio() || g_PVRManager.IsPlayingRadio())
    activePlayers |= Audio;
  if (g_windowManager.IsWindowActive(WINDOW_SLIDESHOW))
    activePlayers |= Picture;

  return activePlayers;
}

PlayerType CPlayerOperations::GetPlayer(const CVariant &player)
{
  int activePlayers = GetActivePlayers();
  int playerID;

  switch ((int)player.asInteger())
  {
    case PLAYLIST_VIDEO:
      playerID = Video;
      break;

    case PLAYLIST_MUSIC:
      playerID = Audio;
      break;

    case PLAYLIST_PICTURE:
      playerID = Picture;
      break;

    default:
      playerID = PlayerImplicit;
      break;
  }

  int choosenPlayer = playerID & activePlayers;

  // Implicit order
  if (choosenPlayer & Video)
    return Video;
  else if (choosenPlayer & Audio)
    return Audio;
  else if (choosenPlayer & Picture)
    return Picture;
  else
    return None;
}

int CPlayerOperations::GetPlaylist(PlayerType player)
{
  switch (player)
  {
    case Video:
      return PLAYLIST_VIDEO;

    case Audio:
      return PLAYLIST_MUSIC;

    case Picture:
      return PLAYLIST_PICTURE;

    default:
      return PLAYLIST_NONE;
  }
}

JSONRPC_STATUS CPlayerOperations::StartSlideshow(const std::string path, bool recursive, bool random)
{
  int flags = 0;
  if (recursive)
    flags |= 1;
  if (random)
    flags |= 2;
  else
    flags |= 4;

  CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
  msg.SetStringParam(path);
  CApplicationMessenger::Get().SendGUIMessage(msg, WINDOW_SLIDESHOW, true);

  return ACK;
}

void CPlayerOperations::SendSlideshowAction(int actionID)
{
  CApplicationMessenger::Get().SendAction(CAction(actionID), WINDOW_SLIDESHOW);
}

void CPlayerOperations::OnPlaylistChanged()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

JSONRPC_STATUS CPlayerOperations::GetPropertyValue(PlayerType player, const CStdString &property, CVariant &result)
{
  if (player == None)
    return FailedToExecute;

  int playlist = GetPlaylist(player);

  if (property.Equals("type"))
  {
    switch (player)
    {
      case Video:
        result = "video";
        break;

      case Audio:
        result = "audio";
        break;

      case Picture:
        result = "picture";
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("partymode"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (IsPVRChannel())
        {
          result = false;
          break;
        }

        result = g_partyModeManager.IsEnabled();
        break;

      case Picture:
        result = false;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("speed"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        result = g_application.m_pPlayer->IsPausedPlayback() ? 0 : g_application.m_pPlayer->GetPlaySpeed();
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying() && !slideshow->IsPaused())
          result = slideshow->GetDirection();
        else
          result = 0;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("time"))
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        int ms = 0;
        if (!IsPVRChannel())
          ms = (int)(g_application.GetTime() * 1000.0);
        else
        {
          EPG::CEpgInfoTag epg;
          if (GetCurrentEpg(epg))
            ms = epg.Progress() * 1000;
        }

        MillisecondsToTimeObject(ms, result);
        break;
      }

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("percentage"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
      {
        if (!IsPVRChannel())
          result = g_application.GetPercentage();
        else
        {
          EPG::CEpgInfoTag epg;
          if (GetCurrentEpg(epg))
            result = epg.ProgressPercentage();
          else
            result = 0;
        }
        break;
      }

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->NumSlides() > 0)
          result = (double)slideshow->CurrentSlide() / slideshow->NumSlides();
        else
          result = 0.0;
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("totaltime"))
  {
    switch (player)
    {
      case Video:
      case Audio:
      {
        int ms = 0;
        if (!IsPVRChannel())
          ms = (int)(g_application.GetTotalTime() * 1000.0);
        else
        {
          EPG::CEpgInfoTag epg;
          if (GetCurrentEpg(epg))
            ms = epg.GetDuration() * 1000;
        }
        
        MillisecondsToTimeObject(ms, result);
        break;
      }

      case Picture:
        MillisecondsToTimeObject(0, result);
        break;

      default:
        return FailedToExecute;
    }
  }
  else if (property.Equals("playlistid"))
  {
    result = playlist;
  }
  else if (property.Equals("position"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        if (!IsPVRChannel() && g_playlistPlayer.GetCurrentPlaylist() == playlist)
          result = g_playlistPlayer.GetCurrentSong();
        else
          result = -1;
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying())
          result = slideshow->CurrentSlide() - 1;
        else
          result = -1;
        break;

      default:
        result = -1;
        break;
    }
  }
  else if (property.Equals("repeat"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (IsPVRChannel())
        {
          result = "off";
          break;
        }

        switch (g_playlistPlayer.GetRepeat(playlist))
        {
        case REPEAT_ONE:
          result = "one";
          break;
        case REPEAT_ALL:
          result = "all";
          break;
        default:
          result = "off";
          break;
        }
        break;

      case Picture:
      default:
        result = "off";
        break;
    }
  }
  else if (property.Equals("shuffled"))
  {
    CGUIWindowSlideShow *slideshow = NULL;
    switch (player)
    {
      case Video:
      case Audio:
        if (IsPVRChannel())
        {
          result = false;
          break;
        }

        result = g_playlistPlayer.IsShuffled(playlist);
        break;

      case Picture:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow && slideshow->IsPlaying())
          result = slideshow->IsShuffled();
        else
          result = -1;
        break;

      default:
        result = -1;
        break;
    }
  }
  else if (property.Equals("canseek"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = g_application.m_pPlayer->CanSeek();
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canchangespeed"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = !IsPVRChannel();
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canmove"))
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canzoom"))
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canrotate"))
  {
    switch (player)
    {
      case Picture:
        result = true;
        break;

      case Video:
      case Audio:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canshuffle"))
  {
    switch (player)
    {
      case Video:
      case Audio:
      case Picture:
        result = !IsPVRChannel();
        break;

      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("canrepeat"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        result = !IsPVRChannel();
        break;

      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("currentaudiostream"))
  {
    switch (player)
    {
      case Video:
      case Audio:
        if (g_application.m_pPlayer->HasPlayer())
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = g_application.m_pPlayer->GetAudioStream();
          if (index >= 0)
          {
            SPlayerAudioStreamInfo info;
            g_application.m_pPlayer->GetAudioStreamInfo(index, info);

            result["index"] = index;
            result["name"] = info.name;
            result["language"] = info.language;
            result["codec"] = info.audioCodecName;
            result["bitrate"] = info.bitrate;
            result["channels"] = info.channels;
          }
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
        
      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property.Equals("audiostreams"))
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer->HasPlayer())
        {
          for (int index = 0; index < g_application.m_pPlayer->GetAudioStreamCount(); index++)
          {
            SPlayerAudioStreamInfo info;
            g_application.m_pPlayer->GetAudioStreamInfo(index, info);

            CVariant audioStream(CVariant::VariantTypeObject);
            audioStream["index"] = index;
            audioStream["name"] = info.name;
            audioStream["language"] = info.language;
            audioStream["codec"] = info.audioCodecName;
            audioStream["bitrate"] = info.bitrate;
            audioStream["channels"] = info.channels;

            result.append(audioStream);
          }
        }
        break;
        
      case Audio:
      case Picture:
      default:
        break;
    }
  }
  else if (property.Equals("subtitleenabled"))
  {
    switch (player)
    {
      case Video:
        result = g_application.m_pPlayer->GetSubtitleVisible();
        break;
        
      case Audio:
      case Picture:
      default:
        result = false;
        break;
    }
  }
  else if (property.Equals("currentsubtitle"))
  {
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer->HasPlayer())
        {
          result = CVariant(CVariant::VariantTypeObject);
          int index = CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleStream;
          if (index >= 0)
          {
            SPlayerSubtitleStreamInfo info;
            g_application.m_pPlayer->GetSubtitleStreamInfo(index, info);

            result["index"] = index;
            result["name"] = info.name;
            result["language"] = info.language;
          }
        }
        else
          result = CVariant(CVariant::VariantTypeNull);
        break;
        
      case Audio:
      case Picture:
      default:
        result = CVariant(CVariant::VariantTypeNull);
        break;
    }
  }
  else if (property.Equals("subtitles"))
  {
    result = CVariant(CVariant::VariantTypeArray);
    switch (player)
    {
      case Video:
        if (g_application.m_pPlayer->HasPlayer())
        {
          for (int index = 0; index < g_application.m_pPlayer->GetSubtitleCount(); index++)
          {
            SPlayerSubtitleStreamInfo info;
            g_application.m_pPlayer->GetSubtitleStreamInfo(index, info);

            CVariant subtitle(CVariant::VariantTypeObject);
            subtitle["index"] = index;
            subtitle["name"] = info.name;
            subtitle["language"] = info.language;

            result.append(subtitle);
          }
        }
        break;
        
      case Audio:
      case Picture:
      default:
        break;
    }
  }
  else if (property.Equals("live"))
    result = IsPVRChannel();
  else
    return InvalidParams;

  return OK;
}

int CPlayerOperations::ParseRepeatState(const CVariant &repeat)
{
  REPEAT_STATE state = REPEAT_NONE;
  std::string strState = repeat.asString();

  if (strState.compare("one") == 0)
    state = REPEAT_ONE;
  else if (strState.compare("all") == 0)
    state = REPEAT_ALL;

  return state;
}

double CPlayerOperations::ParseTimeInSeconds(const CVariant &time)
{
  double seconds = 0.0;
  if (time.isObject())
  {
    if (time.isMember("hours"))
      seconds += time["hours"].asInteger() * 60 * 60;
    if (time.isMember("minutes"))
      seconds += time["minutes"].asInteger() * 60;
    if (time.isMember("seconds"))
      seconds += time["seconds"].asInteger();
    if (time.isMember("milliseconds"))
      seconds += time["milliseconds"].asDouble() / 1000.0;
  }

  return seconds;
}

bool CPlayerOperations::IsPVRChannel()
{
  return g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio();
}

bool CPlayerOperations::GetCurrentEpg(EPG::CEpgInfoTag &epg)
{
  if (!g_PVRManager.IsPlayingTV() && !g_PVRManager.IsPlayingRadio())
    return false;

  CPVRChannelPtr currentChannel;
  if (!g_PVRManager.GetCurrentChannel(currentChannel))
    return false;

  if (!currentChannel->GetEPGNow(epg))
    return false;

  return true;
}
