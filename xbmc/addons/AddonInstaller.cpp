/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "AddonInstaller.h"
#include "Service.h"
#include "utils/log.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/Directory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "ApplicationMessenger.h"
#include "filesystem/FavouritesDirectory.h"
#include "utils/JobManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "guilib/GUIWindowManager.h"      // for callback
#include "GUIUserMessages.h"              // for callback
#include "utils/StringUtils.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogProgress.h"
#include "URL.h"
#include "pvr/PVRManager.h"

using namespace std;
using namespace XFILE;
using namespace ADDON;


struct find_map : public binary_function<CAddonInstaller::JobMap::value_type, unsigned int, bool>
{
  bool operator() (CAddonInstaller::JobMap::value_type t, unsigned int id) const
  {
    return (t.second.jobID == id);
  }
};

CAddonInstaller::CAddonInstaller()
{
  m_repoUpdateJob = 0;
}

CAddonInstaller::~CAddonInstaller()
{
}

CAddonInstaller &CAddonInstaller::Get()
{
  static CAddonInstaller addonInstaller;
  return addonInstaller;
}

void CAddonInstaller::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
    CAddonMgr::Get().FindAddons();

  CSingleLock lock(m_critSection);
  if (strncmp(job->GetType(), "repoupdate", 10) == 0)
  { // repo job finished
    m_repoUpdateDone.Set();
    m_repoUpdateJob = 0;
  }
  else
  { // download job
    JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), bind2nd(find_map(), jobID));
    if (i != m_downloadJobs.end())
      m_downloadJobs.erase(i);
    PrunePackageCache();
  }
  lock.Leave();

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CAddonInstaller::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), bind2nd(find_map(), jobID));
  if (i != m_downloadJobs.end())
  { // update job progress
    i->second.progress = progress;
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM);
    msg.SetStringParam(i->first);
    lock.Leave();
    g_windowManager.SendThreadMessage(msg);
  }
}

bool CAddonInstaller::IsDownloading() const
{
  CSingleLock lock(m_critSection);
  return m_downloadJobs.size() > 0;
}

void CAddonInstaller::GetInstallList(VECADDONS &addons) const
{
  CSingleLock lock(m_critSection);
  vector<CStdString> addonIDs;
  for (JobMap::const_iterator i = m_downloadJobs.begin(); i != m_downloadJobs.end(); ++i)
  {
    if (i->second.jobID)
      addonIDs.push_back(i->first);
  }
  lock.Leave();

  CAddonDatabase database;
  database.Open();
  for (vector<CStdString>::iterator it = addonIDs.begin(); it != addonIDs.end();++it)
  {
    AddonPtr addon;
    if (database.GetAddon(*it, addon))
      addons.push_back(addon);
  }
}

bool CAddonInstaller::GetProgress(const CStdString &addonID, unsigned int &percent) const
{
  CSingleLock lock(m_critSection);
  JobMap::const_iterator i = m_downloadJobs.find(addonID);
  if (i != m_downloadJobs.end())
  {
    percent = i->second.progress;
    return true;
  }
  return false;
}

bool CAddonInstaller::Cancel(const CStdString &addonID)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = m_downloadJobs.find(addonID);
  if (i != m_downloadJobs.end())
  {
    CJobManager::GetInstance().CancelJob(i->second.jobID);
    m_downloadJobs.erase(i);
    return true;
  }
  return false;
}

bool CAddonInstaller::PromptForInstall(const CStdString &addonID, AddonPtr &addon)
{
  // we assume that addons that are enabled don't get to this routine (i.e. that GetAddon() has been called)
  if (CAddonMgr::Get().GetAddon(addonID, addon, ADDON_UNKNOWN, false))
    return false; // addon is installed but disabled, and the user has specifically activated something that needs
                  // the addon - should we enable it?

  // check we have it available
  CAddonDatabase database;
  database.Open();
  if (database.GetAddon(addonID, addon))
  { // yes - ask user if they want it installed
    if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24076), g_localizeStrings.Get(24100),
                                          addon->Name().c_str(), g_localizeStrings.Get(24101)))
      return false;
    if (Install(addonID, true))
    {
      CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (progress)
      {
        progress->SetHeading(13413); // Downloading
        progress->SetLine(0, "");
        progress->SetLine(1, addon->Name());
        progress->SetLine(2, "");
        progress->SetPercentage(0);
        progress->StartModal();
        while (true)
        {
          progress->Progress();
          unsigned int percent;
          if (progress->IsCanceled())
          {
            Cancel(addonID);
            break;
          }
          if (!GetProgress(addonID, percent))
            break;
          progress->SetPercentage(percent);
        }
        progress->Close();
      }
      return CAddonMgr::Get().GetAddon(addonID, addon);
    }
  }
  return false;
}

bool CAddonInstaller::Install(const CStdString &addonID, bool force, const CStdString &referer, bool background)
{
  AddonPtr addon;
  bool addonInstalled = CAddonMgr::Get().GetAddon(addonID, addon, ADDON_UNKNOWN, false);
  if (addonInstalled && !force)
    return true;

  // check whether we have it available in a repository
  CAddonDatabase database;
  database.Open();
  if (database.GetAddon(addonID, addon))
  {
    CStdString repo;
    database.GetRepoForAddon(addonID,repo);
    AddonPtr ptr;
    CAddonMgr::Get().GetAddon(repo,ptr);
    RepositoryPtr therepo = boost::dynamic_pointer_cast<CRepository>(ptr);
    CStdString hash;
    if (therepo)
      hash = therepo->GetAddonHash(addon);
    return DoInstall(addon, hash, addonInstalled, referer, background);
  }
  return false;
}

bool CAddonInstaller::DoInstall(const AddonPtr &addon, const CStdString &hash, bool update, const CStdString &referer, bool background)
{
  // check whether we already have the addon installing
  CSingleLock lock(m_critSection);
  if (m_downloadJobs.find(addon->ID()) != m_downloadJobs.end())
    return false;

  // check whether all the dependencies are available or not
  // TODO: we currently assume that dependencies will install correctly (and each of their dependencies and so on).
  //       it may be better to install the dependencies first to minimise the chance of an addon becoming orphaned due to
  //       missing deps.
  if (!CheckDependencies(addon))
  {
    CGUIDialogKaiToast::QueueNotification(addon->Icon(), addon->Name(), g_localizeStrings.Get(24044), TOAST_DISPLAY_TIME, false);
    return false;
  }

  if (background)
  {
    unsigned int jobID = CJobManager::GetInstance().AddJob(new CAddonInstallJob(addon, hash, update, referer), this);
    m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(jobID)));
  }
  else
  {
    m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(0)));
    lock.Leave();
    CAddonInstallJob job(addon, hash, update, referer);
    if (!job.DoWork())
    { // TODO: dump something to debug log?
      return false;
    }
    lock.Enter();
    JobMap::iterator i = m_downloadJobs.find(addon->ID());
    m_downloadJobs.erase(i);
  }
  return true;
}

bool CAddonInstaller::InstallFromZip(const CStdString &path)
{
  // grab the descriptive XML document from the zip, and read it in
  CFileItemList items;
  // BUG: some zip files return a single item (root folder) that we think is stored, so we don't use the zip:// protocol
  CStdString zipDir;
  URIUtils::CreateArchivePath(zipDir, "zip", path, "");
  if (!CDirectory::GetDirectory(zipDir, items) || items.Size() != 1 || !items[0]->m_bIsFolder)
  {
    CGUIDialogKaiToast::QueueNotification("", path, g_localizeStrings.Get(24045), TOAST_DISPLAY_TIME, false);
    return false;
  }

  // TODO: possibly add support for github generated zips here?
  CStdString archive = URIUtils::AddFileToFolder(items[0]->GetPath(), "addon.xml");

  CXBMCTinyXML xml;
  AddonPtr addon;
  if (xml.LoadFile(archive) && CAddonMgr::Get().LoadAddonDescriptionFromMemory(xml.RootElement(), addon))
  {
    // set the correct path
    addon->Props().path = path;

    // install the addon
    return DoInstall(addon);
  }
  CGUIDialogKaiToast::QueueNotification("", path, g_localizeStrings.Get(24045), TOAST_DISPLAY_TIME, false);
  return false;
}

void CAddonInstaller::InstallFromXBMCRepo(const set<CStdString> &addonIDs)
{
  // first check we have the our repositories up to date (and wait until we do)
  UpdateRepos(false, true);

  // now install the addons
  for (set<CStdString>::const_iterator i = addonIDs.begin(); i != addonIDs.end(); ++i)
    Install(*i);
}

bool CAddonInstaller::CheckDependencies(const AddonPtr &addon)
{
  std::vector<std::string> preDeps;
  preDeps.push_back(addon->ID());
  return CheckDependencies(addon, preDeps);
}

bool CAddonInstaller::CheckDependencies(const AddonPtr &addon,
                                        std::vector<std::string>& preDeps)
{
  if (!addon.get())
    return true; // a NULL addon has no dependencies
  ADDONDEPS deps = addon->GetDeps();
  CAddonDatabase database;
  database.Open();
  for (ADDONDEPS::const_iterator i = deps.begin(); i != deps.end(); ++i)
  {
    const CStdString &addonID = i->first;
    const AddonVersion &version = i->second.first;
    bool optional = i->second.second;
    AddonPtr dep;
    bool haveAddon = CAddonMgr::Get().GetAddon(addonID, dep);
    if ((haveAddon && !dep->MeetsVersion(version)) || (!haveAddon && !optional))
    { // we have it but our version isn't good enough, or we don't have it and we need it
      if (!database.GetAddon(addonID, dep) || !dep->MeetsVersion(version))
      { // we don't have it in a repo, or we have it but the version isn't good enough, so dep isn't satisfied.
        CLog::Log(LOGDEBUG, "Addon %s requires %s version %s which is not available", addon->ID().c_str(), addonID.c_str(), version.c_str());
        return false;
      }
    }
    // at this point we have our dep, or the dep is optional (and we don't have it) so check that it's OK as well
    // TODO: should we assume that installed deps are OK?
    if (dep && std::find(preDeps.begin(), preDeps.end(), dep->ID()) == preDeps.end())
    {
      if (!CheckDependencies(dep, preDeps))
        return false;
      preDeps.push_back(dep->ID());
    }
  }
  return true;
}

void CAddonInstaller::UpdateRepos(bool force, bool wait)
{
  CSingleLock lock(m_critSection);
  if (m_repoUpdateJob)
  {
    if (wait)
    { // wait for our job to complete
      lock.Leave();
      CLog::Log(LOGDEBUG, "%s - waiting for repository update job to finish...", __FUNCTION__);
      m_repoUpdateDone.Wait();
    }
    return;
  }
  // don't run repo update jobs while on the login screen which runs under the master profile
  if((g_windowManager.GetActiveWindow() & WINDOW_ID_MASK) == WINDOW_LOGIN_SCREEN)
    return;
  if (!force && m_repoUpdateWatch.IsRunning() && m_repoUpdateWatch.GetElapsedSeconds() < 600)
    return;
  m_repoUpdateWatch.StartZero();
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_REPOSITORY,addons);
  for (unsigned int i=0;i<addons.size();++i)
  {
    CAddonDatabase database;
    database.Open();
    CDateTime lastUpdate = database.GetRepoTimestamp(addons[i]->ID());
    if (force || !lastUpdate.IsValid() || lastUpdate + CDateTimeSpan(0,6,0,0) < CDateTime::GetCurrentDateTime())
    {
      CLog::Log(LOGDEBUG,"Checking repositories for updates (triggered by %s)",addons[i]->Name().c_str());
      m_repoUpdateJob = CJobManager::GetInstance().AddJob(new CRepositoryUpdateJob(addons), this);
      if (wait)
      { // wait for our job to complete
        lock.Leave();
        CLog::Log(LOGDEBUG, "%s - waiting for this repository update job to finish...", __FUNCTION__);
        m_repoUpdateDone.Wait();
      }
      return;
    }
  }
}

bool CAddonInstaller::HasJob(const CStdString& ID) const
{
  CSingleLock lock(m_critSection);
  return m_downloadJobs.find(ID) != m_downloadJobs.end();
}

void CAddonInstaller::PrunePackageCache()
{
  std::map<CStdString,CFileItemList*> packs;
  int64_t size = EnumeratePackageFolder(packs);
  int64_t limit = (int64_t)g_advancedSettings.m_addonPackageFolderSize*1024*1024;
  if (size < limit)
    return;

  // Prune packages
  // 1. Remove the largest packages, leaving at least 2 for each add-on
  CFileItemList items;
  for (std::map<CStdString,CFileItemList*>::const_iterator it  = packs.begin();
                                                          it != packs.end();++it)
  {
    it->second->Sort(SortByLabel, SortOrderDescending);
    for (int j=2;j<it->second->Size();++j)
      items.Add(CFileItemPtr(new CFileItem(*it->second->Get(j))));
  }
  items.Sort(SortBySize, SortOrderDescending);
  int i=0;
  while (size > limit && i < items.Size())
  {
    size -= items[i]->m_dwSize;
    CFileUtils::DeleteItem(items[i++],true);
  }

  if (size > limit)
  {
    // 2. Remove the oldest packages (leaving least 1 for each add-on)
    items.Clear();
    for (std::map<CStdString,CFileItemList*>::iterator it  = packs.begin();
                                                       it != packs.end();++it)
    {
      if (it->second->Size() > 1)
        items.Add(CFileItemPtr(new CFileItem(*it->second->Get(1))));
    }
    items.Sort(SortByDate, SortOrderAscending);
    i=0;
    while (size > limit && i < items.Size())
    {
      size -= items[i]->m_dwSize;
      CFileUtils::DeleteItem(items[i++],true);
    }
  }
  // clean up our mess
  for (std::map<CStdString,CFileItemList*>::iterator it  = packs.begin();
                                                     it != packs.end();++it)
    delete it->second;
}

int64_t CAddonInstaller::EnumeratePackageFolder(std::map<CStdString,CFileItemList*>& result)
{
  CFileItemList items;
  CDirectory::GetDirectory("special://home/addons/packages/",items,".zip",DIR_FLAG_NO_FILE_DIRS);
  int64_t size=0;
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    size += items[i]->m_dwSize;
    CStdString pack,dummy;
    AddonVersion::SplitFileName(pack,dummy,items[i]->GetLabel());
    if (result.find(pack) == result.end())
      result[pack] = new CFileItemList;
    result[pack]->Add(CFileItemPtr(new CFileItem(*items[i])));
  }

  return size;
}

CAddonInstallJob::CAddonInstallJob(const AddonPtr &addon, const CStdString &hash, bool update, const CStdString &referer)
: m_addon(addon), m_hash(hash), m_update(update), m_referer(referer)
{
}

AddonPtr CAddonInstallJob::GetRepoForAddon(const AddonPtr& addon)
{
  CAddonDatabase database;
  database.Open();
  CStdString repo;
  database.GetRepoForAddon(addon->ID(), repo);
  AddonPtr repoPtr;
  CAddonMgr::Get().GetAddon(repo, repoPtr);

  return repoPtr;
}

bool CAddonInstallJob::DoWork()
{
  AddonPtr repoPtr = GetRepoForAddon(m_addon);
  CStdString installFrom;
  if (!repoPtr || repoPtr->Props().libname.IsEmpty())
  {
    // Addons are installed by downloading the .zip package on the server to the local
    // packages folder, then extracting from the local .zip package into the addons folder
    // Both these functions are achieved by "copying" using the vfs.

    CStdString dest="special://home/addons/packages/";
    CStdString package = URIUtils::AddFileToFolder("special://home/addons/packages/",
                                                URIUtils::GetFileName(m_addon->Path()));

    if (URIUtils::HasSlashAtEnd(m_addon->Path()))
    { // passed in a folder - all we need do is copy it across
      installFrom = m_addon->Path();
    }
    else
    {
      // zip passed in - download + extract
      CStdString path(m_addon->Path());
      if (!m_referer.IsEmpty() && URIUtils::IsInternetStream(path))
      {
        CURL url(path);
        url.SetProtocolOptions(m_referer);
        path = url.Get();
      }
      if (!CFile::Exists(package) && !DownloadPackage(path, dest))
      {
        CFile::Delete(package);
        return false;
      }

      // at this point we have the package - check that it is valid
      if (!CFile::Exists(package) || !CheckHash(package))
      {
        CFile::Delete(package);
        return false;
      }

      // check the archive as well - should have just a single folder in the root
      CStdString archive;
      URIUtils::CreateArchivePath(archive,"zip",package,"");

      CFileItemList archivedFiles;
      CDirectory::GetDirectory(archive, archivedFiles);

      if (archivedFiles.Size() != 1 || !archivedFiles[0]->m_bIsFolder)
      { // invalid package
        CFile::Delete(package);
        return false;
      }
      installFrom = archivedFiles[0]->GetPath();
    }
    repoPtr.reset();
  }

  // run any pre-install functions
  bool reloadAddon = OnPreInstall();

  // perform install
  if (!Install(installFrom, repoPtr))
    return false; // something went wrong

  // run any post-install guff
  OnPostInstall(reloadAddon);

  // and we're done!
  return true;
}

bool CAddonInstallJob::DownloadPackage(const CStdString &path, const CStdString &dest)
{ // need to download/copy the package first
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(path,false)));
  list[0]->Select(true);
  SetFileOperation(CFileOperationJob::ActionReplace, list, dest);
  return CFileOperationJob::DoWork();
}

bool CAddonInstallJob::OnPreInstall()
{
  // check whether this is an active skin - we need to unload it if so
  if (CSettings::Get().GetString("lookandfeel.skin") == m_addon->ID())
  {
    CApplicationMessenger::Get().ExecBuiltIn("UnloadSkin", true);
    return true;
  }

  if (m_addon->Type() == ADDON_SERVICE)
  {
    CAddonDatabase database;
    database.Open();
    bool running = !database.IsAddonDisabled(m_addon->ID()); //grab a current state
    database.DisableAddon(m_addon->ID(),false); // enable it so we can remove it??
    // regrab from manager to have the correct path set
    AddonPtr addon;
    ADDON::CAddonMgr::Get().GetAddon(m_addon->ID(), addon);
    boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
    if (service)
      service->Stop();
    CAddonMgr::Get().RemoveAddon(m_addon->ID()); // remove it
    return running;
  }

  if (m_addon->Type() == ADDON_PVRDLL)
  {
    // stop the pvr manager, so running pvr add-ons are stopped and closed
    PVR::CPVRManager::Get().Stop();
  }
  return false;
}

bool CAddonInstallJob::DeleteAddon(const CStdString &addonFolder)
{
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(addonFolder, true)));
  list[0]->Select(true);
  CFileOperationJob job(CFileOperationJob::ActionDelete, list, "");
  return job.DoWork();
}

bool CAddonInstallJob::Install(const CStdString &installFrom, const AddonPtr& repo)
{
  if (repo)
  {
    CFileItemList dummy;
    CStdString s;
    s.Format("plugin://%s/?action=install"
             "&package=%s&version=%s", repo->ID().c_str(),
                                       m_addon->ID().c_str(),
                                       m_addon->Version().c_str());
    if (!CDirectory::GetDirectory(s, dummy))
      return false;
  }
  else
  {
    CStdString addonFolder(installFrom);
    URIUtils::RemoveSlashAtEnd(addonFolder);
    addonFolder = URIUtils::AddFileToFolder("special://home/addons/",
                                         URIUtils::GetFileName(addonFolder));

    CFileItemList install;
    install.Add(CFileItemPtr(new CFileItem(installFrom, true)));
    install[0]->Select(true);
    CFileOperationJob job(CFileOperationJob::ActionReplace, install, "special://home/addons/");

    AddonPtr addon;
    if (!job.DoWork() || !CAddonMgr::Get().LoadAddonDescription(addonFolder, addon))
    { // failed extraction or failed to load addon description
      CStdString addonID = URIUtils::GetFileName(addonFolder);
      ReportInstallError(addonID, addonID);
      CLog::Log(LOGERROR,"Could not read addon description of %s", addonID.c_str());
      DeleteAddon(addonFolder);
      return false;
    }

    // resolve dependencies
    CAddonMgr::Get().FindAddons(); // needed as GetDeps() grabs directly from c-pluff via the addon manager
    ADDONDEPS deps = addon->GetDeps();
    CStdString referer;
    referer.Format("Referer=%s-%s.zip",addon->ID().c_str(),addon->Version().c_str());
    for (ADDONDEPS::iterator it  = deps.begin(); it != deps.end(); ++it)
    {
      if (it->first.Equals("xbmc.metadata"))
        continue;

      const CStdString &addonID = it->first;
      const AddonVersion &version = it->second.first;
      bool optional = it->second.second;
      AddonPtr dependency;
      bool haveAddon = CAddonMgr::Get().GetAddon(addonID, dependency);
      if ((haveAddon && !dependency->MeetsVersion(version)) || (!haveAddon && !optional))
      { // we have it but our version isn't good enough, or we don't have it and we need it
        bool force=(dependency != NULL);
        // dependency is already queued up for install - ::Install will fail
        // instead we wait until the Job has finished. note that we
        // recall install on purpose in case prior installation failed
        if (CAddonInstaller::Get().HasJob(addonID))
        {
          while (CAddonInstaller::Get().HasJob(addonID))
            Sleep(50);
          force = false;
        }
        // don't have the addon or the addon isn't new enough - grab it (no new job for these)
        if (!CAddonInstaller::Get().Install(addonID, force, referer, false))
        {
          DeleteAddon(addonFolder);
          return false;
        }
      }
    }
  }
  return true;
}

void CAddonInstallJob::OnPostInstall(bool reloadAddon)
{
  if (m_addon->Type() < ADDON_VIZ_LIBRARY && CSettings::Get().GetBool("general.addonnotifications"))
  {
    CGUIDialogKaiToast::QueueNotification(m_addon->Icon(),
                                          m_addon->Name(),
                                          g_localizeStrings.Get(m_update ? 24065 : 24064),
                                          TOAST_DISPLAY_TIME,false,
                                          TOAST_DISPLAY_TIME);
  }
  if (m_addon->Type() == ADDON_SKIN)
  {
    if (reloadAddon || (!m_update && CGUIDialogYesNo::ShowAndGetInput(m_addon->Name(),
                                                        g_localizeStrings.Get(24099),"","")))
    {
      CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
      if (toast)
      {
        toast->ResetTimer();
        toast->Close(true);
      }
      CSettings::Get().SetString("lookandfeel.skin",m_addon->ID().c_str());
    }
  }

  if (m_addon->Type() == ADDON_SERVICE)
  {
    CAddonDatabase database;
    database.Open();
    database.DisableAddon(m_addon->ID(),!reloadAddon); //return it into state it was before OnPreInstall()
    if (reloadAddon) // reload/start it if it was running
    {
      // regrab from manager to have the correct path set
      AddonPtr addon; 
      CAddonMgr::Get().GetAddon(m_addon->ID(), addon);
      boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
      if (service)
        service->Start();
    }
  }

  if (m_addon->Type() == ADDON_REPOSITORY)
  {
    VECADDONS addons;
    addons.push_back(m_addon);
    CJobManager::GetInstance().AddJob(new CRepositoryUpdateJob(addons), &CAddonInstaller::Get());
  }

  if (m_addon->Type() == ADDON_PVRDLL)
  {
    // (re)start the pvr manager
    PVR::CPVRManager::Get().Start(true);
  }
}

void CAddonInstallJob::ReportInstallError(const CStdString& addonID,
                                                const CStdString& fileName)
{
  AddonPtr addon;
  CAddonDatabase database;
  database.Open();
  database.GetAddon(addonID, addon);
  if (addon)
  {
    AddonPtr addon2;
    CAddonMgr::Get().GetAddon(addonID, addon2);
    CGUIDialogKaiToast::QueueNotification(addon->Icon(),
                                          addon->Name(),
                                          g_localizeStrings.Get(addon2 ? 113 : 114),
                                          TOAST_DISPLAY_TIME, false);
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
                                          fileName,
                                          g_localizeStrings.Get(114),
                                          TOAST_DISPLAY_TIME, false);
  }
}

bool CAddonInstallJob::CheckHash(const CStdString& zipFile)
{
  if (m_hash.IsEmpty())
    return true;
  CStdString md5 = CUtil::GetFileMD5(zipFile);
  if (!md5.Equals(m_hash))
  {
    CFile::Delete(zipFile);
    ReportInstallError(m_addon->ID(), URIUtils::GetFileName(zipFile));
    CLog::Log(LOGERROR, "MD5 mismatch after download %s", zipFile.c_str());
    return false;
  }
  return true;
}

CStdString CAddonInstallJob::AddonID() const
{
  return (m_addon) ? m_addon->ID() : "";
}

CAddonUnInstallJob::CAddonUnInstallJob(const AddonPtr &addon)
: m_addon(addon) 
{
}

bool CAddonUnInstallJob::DoWork()
{
  if (m_addon->Type() == ADDON_PVRDLL)
  {
    // stop the pvr manager, so running pvr add-ons are stopped and closed
    PVR::CPVRManager::Get().Stop();
  }
  if (m_addon->Type() == ADDON_SERVICE)
  {
    boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(m_addon);
    if (service)
      service->Stop();
  }

  AddonPtr repoPtr = CAddonInstallJob::GetRepoForAddon(m_addon);
  RepositoryPtr therepo = boost::dynamic_pointer_cast<CRepository>(repoPtr);
  if (therepo && !therepo->Props().libname.IsEmpty())
  {
    CFileItemList dummy;
    CStdString s;
    s.Format("plugin://%s/?action=uninstall"
             "&package=%s", therepo->ID().c_str(), m_addon->ID().c_str());
    if (!CDirectory::GetDirectory(s, dummy))
      return false;
  }
  else
  {
    if (!CAddonInstallJob::DeleteAddon(m_addon->Path()))
      return false;
  }

  OnPostUnInstall();

  return true;
}

void CAddonUnInstallJob::OnPostUnInstall()
{
  if (m_addon->Type() == ADDON_REPOSITORY)
  {
    CAddonDatabase database;
    database.Open();
    database.DeleteRepository(m_addon->ID());
  }

  bool bSave(false);
  CFileItemList items;
  XFILE::CFavouritesDirectory::Load(items);
  for (int i=0; i < items.Size(); ++i)
  {
    if (items[i]->GetPath().Find(m_addon->ID()) > -1)
    {
      items.Remove(items[i].get());
      bSave = true;
    }
  }

  if (bSave)
    CFavouritesDirectory::Save(items);

  if (m_addon->Type() == ADDON_PVRDLL)
  {
    if (CSettings::Get().GetBool("pvrmanager.enabled"))
      PVR::CPVRManager::Get().Start(true);
  }
}
