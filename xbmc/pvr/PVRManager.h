#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <map>

#include "addons/include/xbmc_pvr_types.h"
#include "settings/ISettingCallback.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/JobManager.h"

class CGUIDialogProgressBarHandle;
class CStopWatch;
class CAction;
class CFileItemList;

namespace EPG
{
  class CEpgContainer;
}

namespace PVR
{
  class CPVRClients;
  class CPVRChannel;
  typedef boost::shared_ptr<PVR::CPVRChannel> CPVRChannelPtr;
  class CPVRChannelGroupsContainer;
  class CPVRChannelGroup;
  class CPVRRecording;
  class CPVRRecordings;
  class CPVRTimers;
  class CPVRGUIInfo;
  class CPVRDatabase;
  class CGUIWindowPVRCommon;

  enum ManagerState
  {
    ManagerStateError = 0,
    ManagerStateStopped,
    ManagerStateStarting,
    ManagerStateStopping,
    ManagerStateInterrupted,
    ManagerStateStarted
  };

  enum PlaybackType
  {
    PlaybackTypeAny = 0,
    PlaybackTypeTv,
    PlaybackTypeRadio
  };

  enum ChannelStartLast
  {
    START_LAST_CHANNEL_OFF  = 0,
    START_LAST_CHANNEL_MIN,
    START_LAST_CHANNEL_ON
  };

  #define g_PVRManager       CPVRManager::Get()
  #define g_PVRChannelGroups g_PVRManager.ChannelGroups()
  #define g_PVRTimers        g_PVRManager.Timers()
  #define g_PVRRecordings    g_PVRManager.Recordings()
  #define g_PVRClients       g_PVRManager.Clients()

  typedef boost::shared_ptr<PVR::CPVRChannelGroup> CPVRChannelGroupPtr;

  class CPVRManager : public ISettingCallback, private CThread
  {
    friend class CPVRClients;

  private:
    /*!
     * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
     */
    CPVRManager(void);

  public:
    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    virtual ~CPVRManager(void);

    /*!
     * @brief Get the instance of the PVRManager.
     * @return The PVRManager instance.
     */
    static CPVRManager &Get(void);

    virtual void OnSettingChanged(const CSetting *setting);
    virtual void OnSettingAction(const CSetting *setting);

    /*!
     * @brief Get the channel groups container.
     * @return The groups container.
     */
    CPVRChannelGroupsContainer *ChannelGroups(void) const { return m_channelGroups; }

    /*!
     * @brief Get the recordings container.
     * @return The recordings container.
     */
    CPVRRecordings *Recordings(void) const { return m_recordings; }

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRTimers *Timers(void) const { return m_timers; }

    /*!
     * @brief Get the timers container.
     * @return The timers container.
     */
    CPVRClients *Clients(void) const { return m_addons; }

    /*!
     * @brief Start the PVRManager, which loads all PVR data and starts some threads to update the PVR data.
     * @param bAsync True to (re)start the manager from another thread
     * @param bOpenPVRWindow True to open the PVR window after starting, false otherwise
     */
    void Start(bool bAsync = false, bool bOpenPVRWindow = false);

    /*!
     * @brief Stop the PVRManager and destroy all objects it created.
     */
    void Stop(void);

    /*!
     * @brief Delete PVRManager's objects.
     */
    void Cleanup(void);

    /*!
     * @return True when a PVR window is active, false otherwise.
     */
    bool IsPVRWindowActive(void) const;

    /*!
     * @brief Check whether an add-on can be upgraded or installed without restarting the pvr manager, when the add-on is in use or the pvr window is active
     * @param strAddonId The add-on to check.
     * @return True when the add-on can be installed, false otherwise.
     */
    bool InstallAddonAllowed(const std::string& strAddonId) const;

    /*!
     * @brief Mark an add-on as outdated so it will be upgrade when it's possible again
     * @param strAddonId The add-on to mark as outdated
     * @param strReferer The referer to use when downloading
     */
    void MarkAsOutdated(const std::string& strAddonId, const std::string& strReferer);

    /*!
     * @return True when updated, false when the pvr manager failed to load after the attempt
     */
    bool UpgradeOutdatedAddons(void);

    /*!
     * @brief Get the TV database.
     * @return The TV database.
     */
    CPVRDatabase *GetTVDatabase(void) const { return m_database; }

    /*!
     * @brief Updates the recordings and the "now" and "next" timers.
     */
    void UpdateRecordingsCache(void);

    /*!
     * @brief Get a GUIInfoManager character string.
     * @param dwInfo The string to get.
     * @return The requested string or an empty one if it wasn't found.
     */
    bool TranslateCharInfo(DWORD dwInfo, CStdString &strValue) const;

    /*!
     * @brief Get a GUIInfoManager integer.
     * @param dwInfo The integer to get.
     * @return The requested integer or 0 if it wasn't found.
     */
    int TranslateIntInfo(DWORD dwInfo) const;

    /*!
     * @brief Get a GUIInfoManager boolean.
     * @param dwInfo The boolean to get.
     * @return The requested boolean or false if it wasn't found.
     */
    bool TranslateBoolInfo(DWORD dwInfo) const;

    /*!
     * @brief Show the player info.
     * @param iTimeout Hide the player info after iTimeout seconds.
     * @todo not really the right place for this :-)
     */
    void ShowPlayerInfo(int iTimeout);

    /*!
     * @brief Reset the TV database to it's initial state and delete all the data inside.
     * @param bResetEPGOnly True to only reset the EPG database, false to reset both PVR and EPG.
     */
    void ResetDatabase(bool bResetEPGOnly = false);

    /*!
     * @brief Check if a TV channel, radio channel or recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @return True while the PVRManager is initialising.
     */
    bool IsInitialising(void) const;

    /*!
     * @brief Return the channel that is currently playing.
     * @param channel The channel or NULL if none is playing.
     * @return True if a channel is playing, false otherwise.
     */
    bool GetCurrentChannel(CPVRChannelPtr &channel) const;

    /*!
     * @brief Return the EPG for the channel that is currently playing.
     * @param channel The EPG or NULL if no channel is playing.
     * @return The amount of results that was added or -1 if none.
     */
    int GetCurrentEpg(CFileItemList &results) const;

    /*!
     * @brief Check whether the PVRManager has fully started.
     * @return True if started, false otherwise.
     */
    bool IsStarted(void) const;

    /*!
     * @brief Check whether EPG tags for channels have been created.
     * @return True if EPG tags have been created, false otherwise.
     */
    bool EpgsCreated(void) const;

    /*!
     * @brief Reset the playing EPG tag.
     */
    void ResetPlayingTag(void);

    /*!
     * @brief Switch to the given channel.
     * @param channel The channel to switch to.
     * @param bPreview True to show a preview, false otherwise.
     * @return Trrue if the switch was successful, false otherwise.
     */
    bool PerformChannelSwitch(const CPVRChannel &channel, bool bPreview);

    /*!
     * @brief Close an open PVR stream.
     */
    void CloseStream(void);

    /*!
     * @brief Open a stream from the given channel.
     * @param channel The channel to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenLiveStream(const CFileItem &channel);

    /*!
     * @brief Open a stream from the given recording.
     * @param tag The recording to open.
     * @return True if the stream was opened, false otherwise.
     */
    bool OpenRecordedStream(const CPVRRecording &tag);

    /*!
     * @brief Start recording on a given channel if it is not already recording, stop if it is.
     * @param channel the channel to start/stop recording.
     * @return True if the recording was started or stopped successfully, false otherwise.
     */
    bool ToggleRecordingOnChannel(unsigned int iChannelId);

    /*!
     * @brief Start or stop recording on the channel that is currently being played.
     * @param bOnOff True to start recording, false to stop.
     * @return True if the recording was started or stopped successfully, false otherwise.
     */
    bool StartRecordingOnPlayingChannel(bool bOnOff);

    /*!
     * @brief Get the channel number of the previously selected channel.
     * @return The requested channel number or -1 if it wasn't found.
     */
    int GetPreviousChannel(void);

    /*!
     * @brief Check whether there are active timers.
     * @return True if there are active timers, false otherwise.
     */
    bool HasTimers(void) const;

    /*!
     * @brief Check whether there are active recordings.
     * @return True if there are active recordings, false otherwise.
     */
    bool IsRecording(void) const;

    /*!
     * @brief Check whether the pvr backend is idle.
     * @return True if there are no active timers/recordings/wake-ups within the configured time span.
     */
    bool IsIdle(void) const;

    /*!
     * @brief Set the current playing group, used to load the right channel.
     * @param group The new group.
     */
    void SetPlayingGroup(CPVRChannelGroupPtr group);

    /*!
     * @brief Get the current playing group, used to load the right channel.
     * @param bRadio True to get the current radio group, false to get the current TV group.
     * @return The current group or the group containing all channels if it's not set.
     */
    CPVRChannelGroupPtr GetPlayingGroup(bool bRadio = false);

    /*!
     * @brief Let the background thread create epg tags for all channels.
     */
    void TriggerEpgsCreate(void);

    /*!
     * @brief Let the background thread update the recordings list.
     */
    void TriggerRecordingsUpdate(void);

    /*!
     * @brief Let the background thread update the timer list.
     */
    void TriggerTimersUpdate(void);

    /*!
     * @brief Let the background thread update the channel list.
     */
    void TriggerChannelsUpdate(void);

    /*!
     * @brief Let the background thread update the channel groups list.
     */
    void TriggerChannelGroupsUpdate(void);

    /*!
     * @brief Let the background thread save the current video settings.
     */
    void TriggerSaveChannelSettings(void);

    /*!
     * @brief Update the channel that is currently active.
     * @param item The new channel.
     * @return True if it was updated correctly, false otherwise.
     */
    bool UpdateItem(CFileItem& item);

    /*!
     * @brief Switch to a channel given it's channel number.
     * @param iChannelNumber The channel number to switch to.
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelSwitch(unsigned int iChannelNumber);

    /*!
     * @brief Switch to the next channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelUp(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, true); }

    /*!
     * @brief Switch to the previous channel in this group.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview If true, don't do the actual switch but just update channel pointers.
     *                Used to display event info while doing "fast channel switching"
     * @return True if the channel was switched, false otherwise.
     */
    bool ChannelDown(unsigned int *iNewChannelNumber, bool bPreview = false) { return ChannelUpDown(iNewChannelNumber, bPreview, false); }

    /*!
     * @brief Get the total duration of the currently playing LiveTV item.
     * @return The total duration in milliseconds or NULL if no channel is playing.
     */
    int GetTotalTime(void) const;

    /*!
     * @brief Get the current position in milliseconds since the start of a LiveTV item.
     * @return The position in milliseconds or NULL if no channel is playing.
     */
    int GetStartTime(void) const;

    /*!
     * @brief Start playback on a channel.
     * @param channel The channel to start to play.
     * @param bPreview If true, open minimised.
     * @return True if playback was started, false otherwise.
     */
    bool StartPlayback(const CPVRChannel *channel, bool bPreview = false);

    /*!
     * @brief Start playback of the last used channel, and if it fails use first channel in the current channelgroup.
     * @param type The type of playback to be started (any, radio, tv). See PlaybackType enum
     * @return True if playback was started, false otherwise.
     */
    bool StartPlayback(PlaybackType type = PlaybackTypeAny);

    /*!
     * @brief Update the current playing file in the guiinfomanager and application.
     */
    void UpdateCurrentFile(void);

    /*!
     * @brief Check whether names are still correct after the language settings changed.
     */
    void LocalizationChanged(void);

    /*!
     * @brief Check if a TV channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @brief Check if a radio channel is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @brief Check if a recording is playing.
     * @return True if it's playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @return True when a channel scan is currently running, false otherwise.
     */
    bool IsRunningChannelScan(void) const;

    /*!
     * @brief Open a selection dialog and start a channel scan on the selected client.
     */
    void StartChannelScan(void);

    /*!
     * @brief Try to find missing channel icons automatically
     */
    void SearchMissingChannelIcons(void);

    /*!
     * @brief Persist the current channel settings in the database.
     */
    void SaveCurrentChannelSettings(void);

    /*!
     * @brief Load the settings for the current channel from the database.
     */
    void LoadCurrentChannelSettings(void);

    /*!
     * @brief Check if channel is parental locked. Ask for PIN if neccessary.
     * @param channel The channel to open.
     * @return True if channel is unlocked (by default or PIN unlocked), false otherwise.
     */
    bool CheckParentalLock(const CPVRChannel &channel);

    /*!
     * @brief Check if parental lock is overriden at the given moment.
     * @param channel The channel to open.
     * @return True if parental lock is overriden, false otherwise.
     */
    bool IsParentalLocked(const CPVRChannel &channel);

    /*!
     * @brief Open Numeric dialog to check for parental PIN.
     * @param strTitle Override the title of the dialog if set.
     * @return True if entered PIN was correct, false otherwise.
     */
    bool CheckParentalPIN(const char *strTitle = NULL);

    /*!
     * @brief Executes "pvrpowermanagement.setwakeupcmd"
     */
    bool SetWakeupCommand(void);

    /*!
     * @brief Wait until the pvr manager is loaded
     * @return True when loaded, false otherwise
     */
    bool WaitUntilInitialised(void);

    /*!
     * @brief Handle PVR specific cActions
     * @param action The action to process
     * @return True if action could be handled, false otherwise.
     */
    bool OnAction(const CAction &action);

    static void SettingOptionsPvrStartLastChannelFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);

    /*!
     * @brief Create EPG tags for all channels in internal channel groups
     * @return True if EPG tags where created successfully, false otherwise
     */
    bool CreateChannelEpgs(void);

  protected:
    /*!
     * @brief PVR update and control thread.
     */
    virtual void Process(void);

  private:
    /*!
     * @brief Load at least one client and load all other PVR data after loading the client.
     * If some clients failed to load here, the pvrmanager will retry to load them every second.
     * @return If at least one client and all pvr data was loaded, false otherwise.
     */
    bool Load(void);

    /*!
     * @brief Update all recordings.
     */
    void UpdateRecordings(void);

    /*!
     * @brief Update all timers.
     */
    void UpdateTimers(void);

    /*!
     * @brief Update all channels.
     */
    void UpdateChannels(void);

    /*!
     * @brief Update all channel groups and channels in them.
     */
    void UpdateChannelGroups(void);

    /*!
     * @brief Reset all properties.
     */
    void ResetProperties(void);

    /*!
     * @brief Called by ChannelUp() and ChannelDown() to perform a channel switch.
     * @param iNewChannelNumber The new channel number after the switch.
     * @param bPreview Preview window if true.
     * @param bUp Go one channel up if true, one channel down if false.
     * @return True if the switch was successful, false otherwise.
     */
    bool ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp);

    /*!
     * @brief Stop the EPG and PVR threads but do not remove their data.
     */
    void StopUpdateThreads(void);

    /*!
     * @brief Restart the EPG and PVR threads after they've been stopped by StopUpdateThreads()
     */
    bool StartUpdateThreads(void);

    /*!
     * @brief Continue playback on the last channel if it was stored in the database.
     * @return True if playback was continued, false otherwise.
     */
    bool ContinueLastChannel(void);

    /*!
     * @brief Show or update the progress dialog.
     * @param strText The current status.
     * @param iProgress The current progress in %.
     */
    void ShowProgressDialog(const CStdString &strText, int iProgress);

    /*!
     * @brief Hide the progress dialog if it's visible.
     */
    void HideProgressDialog(void);

    void ExecutePendingJobs(void);

    bool IsJobPending(const char *strJobName) const;

    ManagerState GetState(void) const;

    void SetState(ManagerState state);
    /** @name containers */
    //@{
    CPVRChannelGroupsContainer *    m_channelGroups;               /*!< pointer to the channel groups container */
    CPVRRecordings *                m_recordings;                  /*!< pointer to the recordings container */
    CPVRTimers *                    m_timers;                      /*!< pointer to the timers container */
    CPVRClients *                   m_addons;                      /*!< pointer to the pvr addon container */
    CPVRGUIInfo *                   m_guiInfo;                     /*!< pointer to the guiinfo data */
    //@}

    CCriticalSection                m_critSectionTriggers;         /*!< critical section for triggered updates */
    CEvent                          m_triggerEvent;                /*!< triggers an update */
    std::vector<CJob *>             m_pendingUpdates;              /*!< vector of pending pvr updates */

    CFileItem *                     m_currentFile;                 /*!< the PVR file that is currently playing */
    CPVRDatabase *                  m_database;                    /*!< the database for all PVR related data */
    CCriticalSection                m_critSection;                 /*!< critical section for all changes to this class, except for changes to triggers */
    bool                            m_bFirstStart;                 /*!< true when the PVR manager was started first, false otherwise */
    bool                            m_bIsSwitchingChannels;        /*!< true while switching channels */
    bool                            m_bEpgsCreated;                /*!< true if epg data for channels has been created */
    CGUIDialogProgressBarHandle *   m_progressHandle;              /*!< progress dialog that is displayed while the pvrmanager is loading */

    CCriticalSection                m_managerStateMutex;
    ManagerState                    m_managerState;
    CStopWatch                     *m_parentalTimer;
    bool                            m_bOpenPVRWindow;
    std::map<std::string, std::string> m_outdatedAddons;
  };

  class CPVREpgsCreateJob : public CJob
  {
  public:
    CPVREpgsCreateJob(void) {}
    virtual ~CPVREpgsCreateJob() {}
    virtual const char *GetType() const { return "pvr-create-epgs"; }

    virtual bool DoWork();
  };

  class CPVRRecordingsUpdateJob : public CJob
  {
  public:
    CPVRRecordingsUpdateJob(void) {}
    virtual ~CPVRRecordingsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-recordings"; }

    virtual bool DoWork();
  };

  class CPVRTimersUpdateJob : public CJob
  {
  public:
    CPVRTimersUpdateJob(void) {}
    virtual ~CPVRTimersUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-timers"; }

    virtual bool DoWork();
  };

  class CPVRChannelsUpdateJob : public CJob
  {
  public:
    CPVRChannelsUpdateJob(void) {}
    virtual ~CPVRChannelsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-channels"; }

    virtual bool DoWork();
  };

  class CPVRChannelGroupsUpdateJob : public CJob
  {
  public:
    CPVRChannelGroupsUpdateJob(void) {}
    virtual ~CPVRChannelGroupsUpdateJob() {}
    virtual const char *GetType() const { return "pvr-update-channelgroups"; }

    virtual bool DoWork();
  };

  class CPVRChannelSettingsSaveJob : public CJob
  {
  public:
    CPVRChannelSettingsSaveJob(void) {}
    virtual ~CPVRChannelSettingsSaveJob() {}
    virtual const char *GetType() const { return "pvr-save-channelsettings"; }

    bool DoWork();
  };

  class CPVRChannelSwitchJob : public CJob
  {
  public:
    CPVRChannelSwitchJob(CFileItem* previous, CFileItem* next) : m_previous(previous), m_next(next) {}
    virtual ~CPVRChannelSwitchJob() {}
    virtual const char *GetType() const { return "pvr-channel-switch"; }

    virtual bool DoWork();
  private:
    CFileItem* m_previous;
    CFileItem* m_next;
  };
}
