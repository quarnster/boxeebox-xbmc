/*
 *      Copyright (C) 2013 Team XBMC
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

#include "SettingsManager.h"
#include "SettingSection.h"
#include "Setting.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#define XML_ROOT        "settings"
#define XML_SECTION     "section"


CSettingsManager::CSettingsManager()
  : m_initialized(false), m_loaded(false)
{ }

CSettingsManager::~CSettingsManager()
{
  // first clear all registered settings handler and subsettings
  // implementations because we can't be sure that they are still valid
  m_settingsHandlers.clear();
  m_subSettings.clear();
  m_settingCreators.clear();

  Clear();
}

bool CSettingsManager::Initialize(const TiXmlElement *root)
{
  CSingleLock lock(m_critical);
  if (m_initialized || root == NULL)
    return false;

  if (!StringUtils::EqualsNoCase(root->ValueStr(), XML_ROOT))
  {
    CLog::Log(LOGERROR, "CSettingsManager: error reading settings definition: doesn't contain <settings> tag");
    return false;
  }

  const TiXmlNode *sectionNode = root->FirstChild(XML_SECTION);
  while (sectionNode != NULL)
  {
    std::string sectionId;
    if (CSettingSection::DeserializeIdentification(sectionNode, sectionId))
    {
      CSettingSection *section = NULL;
      SettingSectionMap::iterator itSection = m_sections.find(sectionId);
      bool update = (itSection != m_sections.end());
      if (!update)
        section = new CSettingSection(sectionId, this);
      else
        section = itSection->second;

      if (section->Deserialize(sectionNode, update))
      {
        section->CheckRequirements();
        if (!update)
          m_sections[section->GetId()] = section;

        // get all settings and add them to the settings map
        for (SettingCategoryList::const_iterator categoryIt = section->GetCategories().begin(); categoryIt != section->GetCategories().end(); ++categoryIt)
        {
          (*categoryIt)->CheckRequirements();
          for (SettingGroupList::const_iterator groupIt = (*categoryIt)->GetGroups().begin(); groupIt != (*categoryIt)->GetGroups().end(); ++groupIt)
          {
            (*groupIt)->CheckRequirements();
            for (SettingList::const_iterator settingIt = (*groupIt)->GetSettings().begin(); settingIt != (*groupIt)->GetSettings().end(); ++settingIt)
            {
              (*settingIt)->CheckRequirements();

              const std::string &settingId = (*settingIt)->GetId();
              SettingMap::iterator setting = m_settings.find(settingId);
              if (setting == m_settings.end())
              {
                Setting tmpSetting = { NULL };
                std::pair<SettingMap::iterator, bool> tmpIt = m_settings.insert(make_pair(settingId, tmpSetting));
                setting = tmpIt.first;
              }
                
              if (setting->second.setting == NULL)
              {
                setting->second.setting = *settingIt;
                (*settingIt)->m_callback = this;
              }
            }
          }
        }
      }
      else
      {
        CLog::Log(LOGWARNING, "CSettingsManager: unable to read section \"%s\"", sectionId.c_str());
        if (!update)
          delete section;
      }
    }
      
    sectionNode = sectionNode->NextSibling(XML_SECTION);
  }

  for (SettingMap::iterator itSettingDep = m_settings.begin(); itSettingDep != m_settings.end(); ++itSettingDep)
  {
    if (itSettingDep->second.setting == NULL)
      continue;

    const SettingDependencies& deps = itSettingDep->second.setting->GetDependencies();
    for (SettingDependencies::const_iterator depIt = deps.begin(); depIt != deps.end(); ++depIt)
    {
      std::set<std::string> settingIds = depIt->GetSettings();
      for (std::set<std::string>::const_iterator itSettingId = settingIds.begin(); itSettingId != settingIds.end(); ++itSettingId)
      {
        SettingMap::iterator setting = m_settings.find(*itSettingId);
        if (setting == m_settings.end())
          continue;

        bool newDep = true;
        SettingDependencies &settingDeps = setting->second.dependencies[itSettingDep->first];
        for (SettingDependencies::const_iterator itDeps = settingDeps.begin(); itDeps != settingDeps.end(); ++itDeps)
        {
          if (itDeps->GetType() == depIt->GetType())
          {
            newDep = false;
            break;
          }
        }

        if (newDep)
          settingDeps.push_back(*depIt);
      }
    }
  }

  return true;
}

bool CSettingsManager::Load(const TiXmlElement *root, bool &updated, bool triggerEvents /* = true */, std::map<std::string, CSetting*> *loadedSettings /* = NULL */)
{
  CSingleLock lock(m_critical);
  if (m_loaded || root == NULL)
    return false;

  if (triggerEvents && !OnSettingsLoading())
    return false;

  if (!Deserialize(root, loadedSettings))
    return false;

  bool ret = true;
  // load any ISubSettings implementations
  if (triggerEvents)
    ret = Load(root);

  updated = UpdateSettings(root);

  if (triggerEvents)
    OnSettingsLoaded();

  return ret;
}

bool CSettingsManager::Save(TiXmlNode *root) const
{
  CSingleLock lock(m_critical);
  if (!m_initialized || root == NULL)
    return false;

  if (!OnSettingsSaving())
    return false;

  if (!Serialize(root))
  {
    CLog::Log(LOGERROR, "CSettingsManager: failed to save settings");
    return false;
  }

  // save any ISubSettings implementations
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); ++it)
  {
    if (!(*it)->Save(root))
      return false;
  }

  OnSettingsSaved();

  return true;
}

void CSettingsManager::Unload()
{
  CSingleLock lock(m_critical);
  if (!m_loaded)
    return;

  // needs to be set before calling CSetting::Reset() to avoid calls to
  // OnSettingChanging() and OnSettingChanged()
  m_loaded = false;

  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    setting->second.setting->Reset();

  OnSettingsUnloaded();
}

void CSettingsManager::Clear()
{
  CSingleLock lock(m_critical);
  Unload();

  m_settings.clear();
  for (SettingSectionMap::iterator section = m_sections.begin(); section != m_sections.end(); ++section)
    delete section->second;
  m_sections.clear();

  OnSettingsCleared();

  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); ++it)
    (*it)->Clear();

  m_initialized = false;
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, const std::string &settingId)
{
  if (node == NULL)
    return false;

  CSetting *setting = GetSetting(settingId);
  if (setting == NULL)
    return false;

  return LoadSetting(node, setting);
}

void CSettingsManager::SetInitialized()
{
  CSingleLock lock(m_critical);
  if (m_initialized)
    return;

  m_initialized = true;

  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); )
  {
    SettingMap::iterator tmpIterator = setting++;
    if (tmpIterator->second.setting == NULL)
      m_settings.erase(tmpIterator);
  }
}

void CSettingsManager::RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList)
{
  CSingleLock lock(m_critical);
  if (callback == NULL)
    return;

  for (std::set<std::string>::const_iterator settingIt = settingList.begin(); settingIt != settingList.end(); ++settingIt)
  {
    std::string id = *settingIt;
    StringUtils::ToLower(id);

    SettingMap::iterator setting = m_settings.find(id);
    if (setting == m_settings.end())
    {
      if (m_initialized)
        continue;

      Setting tmpSetting = { NULL };
      std::pair<SettingMap::iterator, bool> tmpIt = m_settings.insert(make_pair(id, tmpSetting));
      setting = tmpIt.first;
    }

    setting->second.callbacks.insert(callback);
  }
}

void CSettingsManager::UnregisterCallback(ISettingCallback *callback)
{
  CSingleLock lock(m_critical);
  for (SettingMap::iterator settingIt = m_settings.begin(); settingIt != m_settings.end(); ++settingIt)
    settingIt->second.callbacks.erase(callback);
}

void CSettingsManager::RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator)
{
  CSingleLock lock(m_critical);
  if (settingType.empty() || settingCreator == NULL)
    return;

  SettingCreatorMap::const_iterator creatorIt = m_settingCreators.find(settingType);
  if (creatorIt == m_settingCreators.end())
    m_settingCreators.insert(make_pair(settingType, settingCreator));
}

void CSettingsManager::RegisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.insert(settingsHandler);
}

void CSettingsManager::UnregisterSettingsHandler(ISettingsHandler *settingsHandler)
{
  if (settingsHandler == NULL)
    return;

  CSingleLock lock(m_critical);
  m_settingsHandlers.erase(settingsHandler);
}

void CSettingsManager::RegisterSubSettings(ISubSettings *subSettings)
{
  CSingleLock lock(m_critical);
  if (subSettings == NULL)
    return;

  m_subSettings.insert(subSettings);
}

void CSettingsManager::UnregisterSubSettings(ISubSettings *subSettings)
{
  CSingleLock lock(m_critical);
  if (subSettings == NULL)
    return;

  m_subSettings.erase(subSettings);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, IntegerSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == NULL)
    return;

  RegisterSettingOptionsFiller(identifier, (void*)optionsFiller, SettingOptionsFillerTypeInteger);
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, StringSettingOptionsFiller optionsFiller)
{
  if (identifier.empty() || optionsFiller == NULL)
    return;

  RegisterSettingOptionsFiller(identifier, (void*)optionsFiller, SettingOptionsFillerTypeString);
}

void CSettingsManager::UnregisterSettingOptionsFiller(const std::string &identifier)
{
  m_optionsFillers.erase(identifier);
}

void* CSettingsManager::GetSettingOptionsFiller(const CSetting *setting)
{
  if (setting == NULL)
    return NULL;

  // get the option filler's identifier
  std::string filler;
  if (setting->GetType() == SettingTypeInteger)
    filler = ((const CSettingInt*)setting)->GetOptionsFiller();
  else if (setting->GetType() == SettingTypeString)
    filler = ((const CSettingString*)setting)->GetOptionsFiller();

  if (filler.empty())
    return NULL;

  // check if such an option filler is known
  SettingOptionsFillerMap::const_iterator fillerIt = m_optionsFillers.find(filler);
  if (fillerIt == m_optionsFillers.end())
    return NULL;

  if (fillerIt->second.filler == NULL)
    return NULL;

  // make sure the option filler's type matches the setting's type
  switch (fillerIt->second.type)
  {
    case SettingOptionsFillerTypeInteger:
    {
      if (setting->GetType() != SettingTypeInteger)
        return NULL;

      break;
    }
    
    case SettingOptionsFillerTypeString:
    {
      if (setting->GetType() != SettingTypeString)
        return NULL;

      break;
    }

    default:
      return NULL;
  }

  return fillerIt->second.filler;
}

CSetting* CSettingsManager::GetSetting(const std::string &id) const
{
  CSingleLock lock(m_critical);
  if (id.empty())
    return NULL;

  std::string settingId = id;
  StringUtils::ToLower(settingId);

  SettingMap::const_iterator setting = m_settings.find(settingId);
  if (setting != m_settings.end())
    return setting->second.setting;

  CLog::Log(LOGDEBUG, "CSettingsManager: requested setting (%s) was not found.", id.c_str());
  return NULL;
}

CSettingSection* CSettingsManager::GetSection(const std::string &section) const
{
  CSingleLock lock(m_critical);
  if (section.empty())
    return NULL;

  std::string sectionId = section;
  StringUtils::ToLower(sectionId);

  SettingSectionMap::const_iterator sectionIt = m_sections.find(sectionId);
  if (sectionIt != m_sections.end())
    return sectionIt->second;

  CLog::Log(LOGDEBUG, "CSettingsManager: requested setting section (%s) was not found.", section.c_str());
  return NULL;
}

SettingDependencyMap CSettingsManager::GetDependencies(const std::string &id) const
{
  CSingleLock lock(m_critical);
  SettingMap::const_iterator setting = m_settings.find(id);
  if (setting == m_settings.end())
    return SettingDependencyMap();

  return setting->second.dependencies;
}

SettingDependencyMap CSettingsManager::GetDependencies(const CSetting *setting) const
{
  CSingleLock lock(m_critical);
  if (setting == NULL)
    return SettingDependencyMap();

  return GetDependencies(setting->GetId());
}

bool CSettingsManager::GetBool(const std::string &id) const
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeBool)
  {
    // Backward compatibility (skins use this setting)
    if (setting == NULL && StringUtils::EqualsNoCase(id, "lookandfeel.enablemouse"))
      return GetBool("input.enablemouse");

    return false;
  }

  return ((CSettingBool*)setting)->GetValue();
}

bool CSettingsManager::SetBool(const std::string &id, bool value)
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeBool)
    return false;

  return ((CSettingBool*)setting)->SetValue(value);
}

bool CSettingsManager::ToggleBool(const std::string &id)
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeBool)
    return false;

  return SetBool(id, !((CSettingBool*)setting)->GetValue());
}

int CSettingsManager::GetInt(const std::string &id) const
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeInteger)
    return 0;

  return ((CSettingInt*)setting)->GetValue();
}

bool CSettingsManager::SetInt(const std::string &id, int value)
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeInteger)
    return false;

  return ((CSettingInt*)setting)->SetValue(value);
}

double CSettingsManager::GetNumber(const std::string &id) const
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeNumber)
    return 0.0;

  return ((CSettingNumber*)setting)->GetValue();
}

bool CSettingsManager::SetNumber(const std::string &id, double value)
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeNumber)
    return false;

  return ((CSettingNumber*)setting)->SetValue(value);
}

std::string CSettingsManager::GetString(const std::string &id) const
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeString)
    return "";

  return ((CSettingString*)setting)->GetValue();
}

bool CSettingsManager::SetString(const std::string &id, const std::string &value)
{
  CSingleLock lock(m_critical);
  CSetting *setting = GetSetting(id);
  if (setting == NULL || setting->GetType() != SettingTypeString)
    return false;

  return ((CSettingString*)setting)->SetValue(value);
}

void CSettingsManager::AddCondition(const std::string &condition)
{
  if (condition.empty())
    return;

  m_conditions.AddCondition(condition);
}

void CSettingsManager::AddCondition(const std::string &identifier, SettingConditionCheck condition)
{
  if (identifier.empty() || condition == NULL)
    return;

  m_conditions.AddCondition(identifier, condition);
}
  
bool CSettingsManager::Serialize(TiXmlNode *parent) const
{
  if (parent == NULL)
    return false;

  for (SettingMap::const_iterator it = m_settings.begin(); it != m_settings.end(); ++it)
  {
    if (it->second.setting->GetType() == SettingTypeAction)
      continue;

    std::vector<std::string> parts = StringUtils::Split(it->first, ".");
    if (parts.size() != 2 || parts.at(0).empty() || parts.at(1).empty())
    {
      CLog::Log(LOGWARNING, "CSettingsManager: unable to save setting \"%s\"", it->first.c_str());
      continue;
    }
      
    TiXmlNode *sectionNode = parent->FirstChild(parts.at(0));
    if (sectionNode == NULL)
    {
      TiXmlElement sectionElement(parts.at(0));
      sectionNode = parent->InsertEndChild(sectionElement);
        
      if (sectionNode == NULL)
      {
        CLog::Log(LOGWARNING, "CSettingsManager: unable to write <%s> tag", parts.at(0).c_str());
        continue;
      }
    }
      
    TiXmlElement settingElement(parts.at(1));
    TiXmlNode *settingNode = sectionNode->InsertEndChild(settingElement);
    if (settingNode == NULL)
    {
      CLog::Log(LOGWARNING, "CSetting: unable to write <%s> tag in <%s>", parts.at(1).c_str(), parts.at(0).c_str());
      continue;
    }
      
    TiXmlText value(it->second.setting->ToString());
    settingNode->InsertEndChild(value);
  }

  return true;
}
  
bool CSettingsManager::Deserialize(const TiXmlNode *node, std::map<std::string, CSetting*> *loadedSettings /* = NULL */)
{
  if (node == NULL)
    return false;

  for (SettingMap::iterator it = m_settings.begin(); it != m_settings.end(); ++it)
  {
    if (LoadSetting(node, it->second.setting) && loadedSettings != NULL)
      loadedSettings->insert(make_pair(it->first, it->second.setting));
  }

  return true;
}

bool CSettingsManager::OnSettingChanging(const CSetting *setting)
{
  CSingleLock lock(m_critical);
  if (setting == NULL)
    return false;

  if (!m_loaded)
    return true;

  SettingMap::const_iterator settingIt = m_settings.find(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin();
        callback != settingData.callbacks.end();
        ++callback)
  {
    if (!(*callback)->OnSettingChanging(setting))
      return false;
  }

  return true;
}
  
void CSettingsManager::OnSettingChanged(const CSetting *setting)
{
  CSingleLock lock(m_critical);
  if (!m_loaded || setting == NULL)
    return;
    
  SettingMap::const_iterator settingIt = m_settings.find(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();
    
  for (CallbackSet::iterator callback = settingData.callbacks.begin();
        callback != settingData.callbacks.end();
        ++callback)
    (*callback)->OnSettingChanged(setting);

  // now handle any settings which depend on the changed setting
  const SettingDependencyMap& deps = GetDependencies(setting);
  for (SettingDependencyMap::const_iterator depsIt = deps.begin(); depsIt != deps.end(); ++depsIt)
  {
    for (SettingDependencies::const_iterator depIt = depsIt->second.begin(); depIt != depsIt->second.end(); ++depIt)
      UpdateSettingByDependency(depsIt->first, *depIt);
  }
}

void CSettingsManager::OnSettingAction(const CSetting *setting)
{
  CSingleLock lock(m_critical);
  if (!m_loaded || setting == NULL)
    return;

  SettingMap::const_iterator settingIt = m_settings.find(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin();
        callback != settingData.callbacks.end();
        ++callback)
    (*callback)->OnSettingAction(setting);
}

bool CSettingsManager::OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode)
{
  CSingleLock lock(m_critical);
  if (setting == NULL)
    return false;

  SettingMap::const_iterator settingIt = m_settings.find(setting->GetId());
  if (settingIt == m_settings.end())
    return false;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  bool ret = false;
  for (CallbackSet::iterator callback = settingData.callbacks.begin();
        callback != settingData.callbacks.end();
        ++callback)
    ret |= (*callback)->OnSettingUpdate(setting, oldSettingId, oldSettingNode);

  return ret;
}

void CSettingsManager::OnSettingPropertyChanged(const CSetting *setting, const char *propertyName)
{
  CSingleLock lock(m_critical);
  if (!m_loaded || setting == NULL)
    return;

  SettingMap::const_iterator settingIt = m_settings.find(setting->GetId());
  if (settingIt == m_settings.end())
    return;

  Setting settingData = settingIt->second;
  // now that we have a copy of the setting's data, we can leave the lock
  lock.Leave();

  for (CallbackSet::iterator callback = settingData.callbacks.begin();
        callback != settingData.callbacks.end();
        ++callback)
    (*callback)->OnSettingPropertyChanged(setting, propertyName);
}

CSetting* CSettingsManager::CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager /* = NULL */) const
{
  if (StringUtils::EqualsNoCase(settingType, "boolean"))
    return new CSettingBool(settingId, (CSettingsManager*)this);
  else if (StringUtils::EqualsNoCase(settingType, "integer"))
    return new CSettingInt(settingId, (CSettingsManager*)this);
  else if (StringUtils::EqualsNoCase(settingType, "number"))
    return new CSettingNumber(settingId, (CSettingsManager*)this);
  else if (StringUtils::EqualsNoCase(settingType, "string"))
    return new CSettingString(settingId, (CSettingsManager*)this);
  else if (StringUtils::EqualsNoCase(settingType, "action"))
    return new CSettingAction(settingId, (CSettingsManager*)this);

  SettingCreatorMap::const_iterator creator = m_settingCreators.find(settingType);
  if (creator != m_settingCreators.end())
    return creator->second->CreateSetting(settingType, settingId, (CSettingsManager*)this);

  return NULL;
}

bool CSettingsManager::OnSettingsLoading()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); ++it)
  {
    if (!(*it)->OnSettingsLoading())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsLoaded()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); ++it)
    (*it)->OnSettingsLoaded();
}

bool CSettingsManager::OnSettingsSaving() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); ++it)
  {
    if (!(*it)->OnSettingsSaving())
      return false;
  }

  return true;
}

void CSettingsManager::OnSettingsSaved() const
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); ++it)
    (*it)->OnSettingsSaved();
}

void CSettingsManager::OnSettingsCleared()
{
  CSingleLock lock(m_critical);
  for (SettingsHandlers::const_iterator it = m_settingsHandlers.begin(); it != m_settingsHandlers.end(); ++it)
    (*it)->OnSettingsCleared();
}

bool CSettingsManager::Load(const TiXmlNode *settings)
{
  bool ok = true;
  for (std::set<ISubSettings*>::const_iterator it = m_subSettings.begin(); it != m_subSettings.end(); ++it)
    ok &= (*it)->Load(settings);

  return ok;
}

bool CSettingsManager::LoadSetting(const TiXmlNode *node, CSetting *setting)
{
  if (node == NULL || setting == NULL)
    return false;

  if (setting->GetType() == SettingTypeAction)
    return false;

  const std::string &settingId = setting->GetId();

  std::vector<std::string> parts = StringUtils::Split(settingId, ".");
  if (parts.size() != 2 || parts.at(0).empty() || parts.at(1).empty())
  {
    CLog::Log(LOGWARNING, "CSettingsManager: unable to load setting \"%s\"", settingId.c_str());
    return false;
  }

  const TiXmlNode *sectionNode = node->FirstChild(parts.at(0));
  if (sectionNode == NULL)
    return false;

  const TiXmlNode *settingNode = sectionNode->FirstChild(parts.at(1));
  if (settingNode == NULL)
    return false;

  if (!setting->FromString(settingNode->FirstChild() != NULL ? settingNode->FirstChild()->ValueStr() : StringUtils::Empty))
  {
    CLog::Log(LOGWARNING, "CSettingsManager: unable to read value of setting \"%s\"", settingId.c_str());
    return false;
  }

  return true;
}

bool CSettingsManager::UpdateSettings(const TiXmlNode *root)
{
  bool updated = false;

  for (SettingMap::iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
  {
    const std::set<CSettingUpdate>& updates = setting->second.setting->GetUpdates();
    if (updates.empty())
      continue;

    for (std::set<CSettingUpdate>::const_iterator update = updates.begin(); update != updates.end(); ++update)
      updated |= UpdateSetting(root, setting->second.setting, *update);
  }

  return updated;
}

bool CSettingsManager::UpdateSetting(const TiXmlNode *node, CSetting *setting, const CSettingUpdate& update)
{
  if (node == NULL || setting == NULL || update.GetType() == SettingUpdateTypeNone)
    return false;

  bool updated = false;
  const char *oldSetting = NULL;
  const TiXmlNode *oldSettingNode = NULL;
  if (update.GetType() == SettingUpdateTypeRename)
  {
    if (update.GetValue().empty())
      return false;

    oldSetting = update.GetValue().c_str();
    std::vector<std::string> parts = StringUtils::Split(oldSetting, ".");
    if (parts.size() != 2 || parts.at(0).empty() || parts.at(1).empty())
      return false;

    const TiXmlNode *sectionNode = node->FirstChild(parts.at(0));
    if (sectionNode == NULL)
      return false;

    oldSettingNode = sectionNode->FirstChild(parts.at(1));
    if (oldSettingNode == NULL)
      return false;

    if (setting->FromString(oldSettingNode->FirstChild() != NULL ? oldSettingNode->FirstChild()->ValueStr() : StringUtils::Empty))
      updated = true;
    else
      CLog::Log(LOGWARNING, "CSetting: unable to update \"%s\" through automatically renaming from \"%s\"", setting->GetId().c_str(), oldSetting);
  }

  updated |= OnSettingUpdate(setting, oldSetting, oldSettingNode);
  return updated;
}

void CSettingsManager::UpdateSettingByDependency(const std::string &settingId, const CSettingDependency &dependency)
{
  CSetting *setting = GetSetting(settingId);
  if (setting == NULL)
    return;

  switch (dependency.GetType())
  {
    case SettingDependencyTypeEnable:
      // just trigger the property changed callback and a call to
      // CSetting::IsEnabled() will automatically determine the new
      // enabled state
      OnSettingPropertyChanged(setting, "enabled");
      break;

    case SettingDependencyTypeUpdate:
    {
      SettingType type = (SettingType)setting->GetType();
      if (type == SettingTypeInteger)
      {
        CSettingInt *settingInt = ((CSettingInt*)setting);
        if (settingInt->GetOptionsType() == SettingOptionsTypeDynamic)
          settingInt->UpdateDynamicOptions();
      }
      else if (type == SettingTypeString)
      {
        CSettingString *settingString = ((CSettingString*)setting);
        if (settingString->GetOptionsType() == SettingOptionsTypeDynamic)
          settingString->UpdateDynamicOptions();
      }
      break;
    }

    case SettingDependencyTypeVisible:
      // just trigger the property changed callback and a call to
      // CSetting::IsVisible() will automatically determine the new
      // visible state
      OnSettingPropertyChanged(setting, "visible");
      break;

    case SettingDependencyTypeNone:
    default:
      break;
  }
}

void CSettingsManager::RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type)
{
  SettingOptionsFillerMap::const_iterator it = m_optionsFillers.find(identifier);
  if (it != m_optionsFillers.end())
    return;

  SettingOptionsFiller optionsFiller = { filler, type };
  m_optionsFillers.insert(make_pair(identifier, optionsFiller));
}
