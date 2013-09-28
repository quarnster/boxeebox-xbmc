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

#include <vector>

#include "settings/ISettingCallback.h"
#include "settings/ISettingsHandler.h"
#include "utils/StdString.h"
#include "utils/GlobalsHandling.h"

class TiXmlElement;
namespace ADDON
{
  class IAddon;
}

class DatabaseSettings
{
public:
  void Reset()
  {
    type.clear();
    host.clear();
    port.clear();
    user.clear();
    pass.clear();
    name.clear();
  };
  CStdString type;
  CStdString host;
  CStdString port;
  CStdString user;
  CStdString pass;
  CStdString name;
};

struct TVShowRegexp
{
  bool byDate;
  CStdString regexp;
  int defaultSeason;
  TVShowRegexp(bool d, const CStdString& r, int s = 1)
  {
    byDate = d;
    regexp = r;
    defaultSeason = s;
  }
};

struct RefreshOverride
{
  float fpsmin;
  float fpsmax;

  float refreshmin;
  float refreshmax;

  bool  fallback;
};


struct RefreshVideoLatency
{
  float refreshmin;
  float refreshmax;

  float delay;
};

struct StagefrightConfig
{
  int useAVCcodec;
  int useVC1codec;
  int useVPXcodec;
  int useMP4codec;
  int useMPEG2codec;
  bool useSwRenderer;
  bool useInputDTS;
};

typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

class CAdvancedSettings : public ISettingCallback, public ISettingsHandler
{
  public:
    CAdvancedSettings();

    static CAdvancedSettings* getInstance();

    virtual void OnSettingsLoaded();
    virtual void OnSettingsUnloaded();

    virtual void OnSettingChanged(const CSetting *setting);

    virtual void OnSettingAction(const CSetting *setting);

    void Initialize();
    bool Initialized() { return m_initialized; };
    void AddSettingsFile(const CStdString &filename);
    bool Load();
    void Clear();

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomRegexpReplacers(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);

    int m_audioHeadRoom;
    float m_ac3Gain;
    CStdString m_audioDefaultPlayer;
    float m_audioPlayCountMinimumPercent;
    bool m_dvdplayerIgnoreDTSinWAV;
    int m_audioResample;
    bool m_allowTranscode44100;
    bool m_audioForceDirectSound;
    bool m_audioAudiophile;
    bool m_allChannelStereo;
    bool m_streamSilence;
    CStdString m_audioTranscodeTo;
    float m_limiterHold;
    float m_limiterRelease;

    bool  m_omxHWAudioDecode;
    bool  m_omxDecodeStartWithValidFrame;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    CStdString m_videoPPFFmpegDeint;
    CStdString m_videoPPFFmpegPostProc;
    bool m_videoVDPAUtelecine;
    bool m_videoVDPAUdeintSkipChromaHD;
    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_videoBlackBarColour;
    int m_videoIgnoreSecondsAtStart;
    float m_videoIgnorePercentAtEnd;
    CStdString m_audioHost;
    bool m_audioApplyDrc;

    int   m_videoVDPAUScaling;
    float m_videoNonLinStretchRatio;
    bool  m_videoEnableHighQualityHwScalers;
    float m_videoAutoScaleMaxFps;
    bool  m_videoAllowMpeg4VDPAU;
    bool  m_videoAllowMpeg4VAAPI;
    std::vector<RefreshOverride> m_videoAdjustRefreshOverrides;
    std::vector<RefreshVideoLatency> m_videoRefreshLatency;
    float m_videoDefaultLatency;
    bool m_videoDisableBackgroundDeinterlace;
    int  m_videoCaptureUseOcclusionQuery;
    bool m_DXVACheckCompatibility;
    bool m_DXVACheckCompatibilityPresent;
    bool m_DXVAForceProcessorRenderer;
    bool m_DXVANoDeintProcForProgressive;
    int  m_videoFpsDetect;
    bool m_videoDisableHi10pMultithreading;
    StagefrightConfig m_stagefrightConfig;

    CStdString m_videoDefaultPlayer;
    CStdString m_videoDefaultDVDPlayer;
    float m_videoPlayCountMinimumPercent;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_songInfoDuration;
    int m_logLevel;
    int m_logLevelHint;
    int m_extraLogLevels;
    CStdString m_cddbAddress;

    //airtunes + airplay
    bool m_logEnableAirtunes;
    int m_airTunesPort;
    int m_airPlayPort;

    bool m_handleMounting;

    bool m_fullScreenOnMovieStart;
    CStdString m_cachePath;
    CStdString m_videoCleanDateTimeRegExp;
    CStdStringArray m_videoCleanStringRegExps;
    CStdStringArray m_videoExcludeFromListingRegExps;
    CStdStringArray m_moviesExcludeFromScanRegExps;
    CStdStringArray m_tvshowExcludeFromScanRegExps;
    CStdStringArray m_audioExcludeFromListingRegExps;
    CStdStringArray m_audioExcludeFromScanRegExps;
    CStdStringArray m_pictureExcludeFromListingRegExps;
    CStdStringArray m_videoStackRegExps;
    CStdStringArray m_folderStackRegExps;
    CStdStringArray m_trailerMatchRegExps;
    SETTINGS_TVSHOWLIST m_tvshowEnumRegExps;
    CStdString m_tvshowMultiPartEnumRegExp;
    typedef std::vector< std::pair<CStdString, CStdString> > StringMapping;
    StringMapping m_pathSubstitutions;
    int m_remoteDelay; ///< \brief number of remote messages to ignore before repeating
    float m_controllerDeadzone;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    unsigned int m_fanartRes; ///< \brief the maximal resolution to cache fanart at (assumes 16x9)
    unsigned int m_imageRes;  ///< \brief the maximal resolution to cache images at (assumes 16x9)
    /*! \brief the maximal size to cache thumbs at, assuming square
     Used for actual thumbs (eg bookmark thumbs, picture thumbs) rather than cover art which uses m_imageRes instead
     */
    unsigned int GetThumbSize() const { return m_imageRes / 2; };
    bool m_useDDSFanart;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    bool m_sambastatfiles;

    bool m_bHTTPDirectoryStatFilesize;

    bool m_bFTPThumbs;

    CStdString m_musicThumbs;
    CStdString m_fanartImages;

    bool m_bMusicLibraryHideAllItems;
    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    bool m_bMusicLibraryCleanOnUpdate;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    std::vector<CStdString> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryImportWatchedState;
    bool m_bVideoLibraryImportResumePoint;

    bool m_bVideoScannerIgnoreErrors;
    int m_iVideoLibraryDateAdded;

    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    int m_iTuxBoxStreamtsPort;
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;
    bool m_bTuxBoxZapstream;
    int m_iTuxBoxZapstreamPort;

    int m_iMythMovieLength;         // minutes

    int m_iEpgLingerTime;           // minutes
    int m_iEpgUpdateCheckInterval;  // seconds
    int m_iEpgCleanupInterval;      // seconds
    int m_iEpgActiveTagCheckInterval; // seconds
    int m_iEpgRetryInterruptedUpdateInterval; // seconds
    int m_iEpgUpdateEmptyTagsInterval; // seconds
    bool m_bEpgDisplayUpdatePopup;
    bool m_bEpgDisplayIncrementalUpdatePopup;

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds

    int m_curlconnecttimeout;
    int m_curllowspeedtime;
    int m_curlretries;
    bool m_curlDisableIPV6;

    bool m_fullScreen;
    bool m_startFullScreen;
    bool m_showExitButton; /* Ideal for appliances to hide a 'useless' button */
    bool m_canWindowed;
    bool m_splashImage;
    bool m_alwaysOnTop;  /* makes xbmc to run always on top .. osx/win32 only .. */
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool m_AllowD3D9Ex;
    bool m_ForceD3D9Ex;
    bool m_AllowDynamicTextures;
    unsigned int m_RestrictCapsMask;
    float m_sleepBeforeFlip; ///< if greather than zero, XBMC waits for raster to be this amount through the frame prior to calling the flip
    bool m_bVirtualShares;

    float m_karaokeSyncDelayCDG; // seems like different delay is needed for CDG and MP3s
    float m_karaokeSyncDelayLRC;
    bool m_karaokeChangeGenreForKaraokeSongs;
    bool m_karaokeKeepDelay; // store user-changed song delay in the database
    int m_karaokeStartIndex; // auto-assign numbering start from this value
    bool m_karaokeAlwaysEmptyOnCdgs; // always have empty background on CDG files
    bool m_karaokeUseSongSpecificBackground; // use song-specific video or image if available instead of default
    CStdString m_karaokeDefaultBackgroundType; // empty string or "vis", "image" or "video"
    CStdString m_karaokeDefaultBackgroundFilePath; // only for "image" or "video" types above

    CStdString m_cpuTempCmd;
    CStdString m_gpuTempCmd;

    /* PVR/TV related advanced settings */
    int m_iPVRTimeCorrection;     /*!< @brief correct all times (epg tags, timer tags, recording tags) by this amount of minutes. defaults to 0. */
    int m_iPVRInfoToggleInterval; /*!< @brief if there are more than 1 pvr gui info item available (e.g. multiple recordings active at the same time), use this toggle delay in milliseconds. defaults to 3000. */
    bool m_bPVRShowEpgInfoOnEpgItemSelect; /*!< @brief when selecting an EPG fileitem, show the EPG info dialog if this setting is true. start playback on the selected channel if false */
    int m_iPVRMinVideoCacheLevel;      /*!< @brief cache up to this level in the video buffer buffer before resuming playback if the buffers run dry */
    int m_iPVRMinAudioCacheLevel;      /*!< @brief cache up to this level in the audio buffer before resuming playback if the buffers run dry */
    bool m_bPVRCacheInDvdPlayer; /*!< @brief true to use "CACHESTATE_PVR" in CDVDPlayer (default) */
    bool m_bPVRChannelIconsAutoScan; /*!< @brief automatically scan user defined folder for channel icons when loading internal channel groups */
    bool m_bPVRAutoScanIconsUserSet; /*!< @brief mark channel icons populated by auto scan as "user set" */
    int m_iPVRNumericChannelSwitchTimeout; /*!< @brief time in ms before the numeric dialog auto closes when confirmchannelswitch is disabled */

    bool m_measureRefreshrate; //when true the videoreferenceclock will measure the refreshrate when direct3d is used
                               //otherwise it will use the windows refreshrate

    DatabaseSettings m_databaseMusic; // advanced music database setup
    DatabaseSettings m_databaseVideo; // advanced video database setup
    DatabaseSettings m_databaseTV;    // advanced tv database setup
    DatabaseSettings m_databaseEpg;   /*!< advanced EPG database setup */

    bool m_guiVisualizeDirtyRegions;
    int  m_guiAlgorithmDirtyRegions;
    int  m_guiDirtyRegionNoFlipTimeout;
    unsigned int m_addonPackageFolderSize;

    unsigned int m_cacheMemBufferSize;
    bool m_alwaysForceBuffer;
    float m_readBufferFactor;

    bool m_jsonOutputCompact;
    unsigned int m_jsonTcpPort;

    bool m_enableMultimediaKeys;
    std::vector<CStdString> m_settingsFiles;
    void ParseSettingsFile(const CStdString &file);

    float GetDisplayLatency(float refreshrate);
    bool m_initialized;

    void SetDebugMode(bool debug);
    void SetExtraLogsFromAddon(ADDON::IAddon* addon);

    // runtime settings which cannot be set from advancedsettings.xml
    CStdString m_pictureExtensions;
    CStdString m_musicExtensions;
    CStdString m_videoExtensions;
    CStdString m_discStubExtensions;

    CStdString m_stereoscopicflags_sbs;
    CStdString m_stereoscopicflags_tab;

    CStdString m_logFolder;

    CStdString m_userAgent;
};

XBMC_GLOBAL(CAdvancedSettings,g_advancedSettings);
