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

#include "SettingSection.h"
#include "SettingsManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"

#define XML_CATEGORY    "category"
#define XML_GROUP       "group"

template<class T> void addISetting(const TiXmlNode *node, const T &item, std::vector<T> &items)
{
  if (node == NULL)
    return;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return;

  // check if there is a "before" or "after" attribute to place the setting at a specific position
  int position = -1; // -1 => end, 0 => before, 1 => after
  const char *positionId = element->Attribute("before");
  if (positionId != NULL && strlen(positionId) > 0)
    position = 0;
  else if ((positionId = element->Attribute("after")) != NULL && strlen(positionId) > 0)
    position = 1;

  if (positionId != NULL && strlen(positionId) > 0 && position >= 0)
  {
    for (typename std::vector<T>::iterator it = items.begin(); it != items.end(); ++it)
    {
      if (!StringUtils::EqualsNoCase((*it)->GetId(), positionId))
        continue;

      typename std::vector<T>::iterator positionIt = it;
      if (position == 1)
        ++positionIt;

      items.insert(positionIt, item);
      return;
    }
  }

  items.push_back(item);
}

CSettingGroup::CSettingGroup(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager)
{ }

CSettingGroup::~CSettingGroup()
{
  for (SettingList::const_iterator setting = m_settings.begin(); setting != m_settings.end(); ++setting)
    delete *setting;
  m_settings.clear();
}

bool CSettingGroup::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;

  const TiXmlElement *settingElement = node->FirstChildElement(XML_SETTING);
  while (settingElement != NULL)
  {
    std::string settingId;
    if (CSettingCategory::DeserializeIdentification(settingElement, settingId))
    {
      CSetting *setting = NULL;
      for (SettingList::iterator itSetting = m_settings.begin(); itSetting != m_settings.end(); ++itSetting)
      {
        if ((*itSetting)->GetId() == settingId)
        {
          setting = *itSetting;
          break;
        }
      }
      
      update = (setting != NULL);
      if (!update)
      {
        const char* settingType = settingElement->Attribute(XML_ATTR_TYPE);
        if (settingType == NULL || strlen(settingType) <= 0)
        {
          CLog::Log(LOGERROR, "CSettingGroup: unable to read setting type of \"%s\"", settingId.c_str());
          return false;
        }

        setting = m_settingsManager->CreateSetting(settingType, settingId, m_settingsManager);
        if (setting == NULL)
          CLog::Log(LOGERROR, "CSettingGroup: unknown setting type \"%s\" of \"%s\"", settingType, settingId.c_str());
      }
      
      if (setting == NULL)
        CLog::Log(LOGERROR, "CSettingGroup: unable to create new setting \"%s\"", settingId.c_str());
      else if (!setting->Deserialize(settingElement, update))
      {
        CLog::Log(LOGWARNING, "CSettingGroup: unable to read setting \"%s\"", settingId.c_str());
        if (!update)
          delete setting;
      }
      else if (!update)
        addISetting(settingElement, setting, m_settings);
    }
      
    settingElement = settingElement->NextSiblingElement(XML_SETTING);
  }
    
  return true;
}

SettingList CSettingGroup::GetSettings(SettingLevel level) const
{
  SettingList settings;

  for (SettingList::const_iterator it = m_settings.begin(); it != m_settings.end(); ++it)
  {
    if ((*it)->GetLevel() <= level && (*it)->MeetsRequirements())
      settings.push_back(*it);
  }

  return settings;
}

CSettingCategory::CSettingCategory(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager),
    m_label(-1), m_help(-1),
    m_accessCondition(settingsManager)
{ }

CSettingCategory::~CSettingCategory()
{
  for (SettingGroupList::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
    delete *it;

  m_groups.clear();
}

bool CSettingCategory::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;
    
  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;
    
  int tmp = -1;
  if (element->QueryIntAttribute(XML_ATTR_LABEL, &tmp) == TIXML_SUCCESS && tmp > 0)
    m_label = tmp;
  if (element->QueryIntAttribute(XML_ATTR_HELP, &m_help) == TIXML_SUCCESS && m_help > 0)
    m_help = tmp;

  const TiXmlNode *accessNode = node->FirstChild("access");
  if (accessNode != NULL && !m_accessCondition.Deserialize(accessNode))
    return false;
    
  const TiXmlNode *groupNode = node->FirstChildElement(XML_GROUP);
  while (groupNode != NULL)
  {
    std::string groupId;
    if (CSettingGroup::DeserializeIdentification(groupNode, groupId))
    {
      CSettingGroup *group = NULL;
      for (SettingGroupList::iterator itGroup = m_groups.begin(); itGroup != m_groups.end(); ++itGroup)
      {
        if ((*itGroup)->GetId() == groupId)
        {
          group = *itGroup;
          break;
        }
      }
      
      update = (group != NULL);
      if (!update)
        group = new CSettingGroup(groupId, m_settingsManager);

      if (group->Deserialize(groupNode, update))
      {
        if (!update)
          addISetting(groupNode, group, m_groups);
      }
      else
      {
        CLog::Log(LOGWARNING, "CSettingCategory: unable to read group \"%s\"", groupId.c_str());
        if (!update)
          delete group;
      }
    }
      
    groupNode = groupNode->NextSibling(XML_GROUP);
  }
    
  return true;
}

SettingGroupList CSettingCategory::GetGroups(SettingLevel level) const
{
  SettingGroupList groups;

  for (SettingGroupList::const_iterator it = m_groups.begin(); it != m_groups.end(); ++it)
  {
    if ((*it)->MeetsRequirements() && (*it)->IsVisible() && (*it)->GetSettings(level).size() > 0)
      groups.push_back(*it);
  }

  return groups;
}

bool CSettingCategory::CanAccess() const
{
  return m_accessCondition.Check();
}

CSettingSection::CSettingSection(const std::string &id, CSettingsManager *settingsManager /* = NULL */)
  : ISetting(id, settingsManager),
    m_label(-1), m_help(-1)
{ }

CSettingSection::~CSettingSection()
{
  for (SettingCategoryList::const_iterator it = m_categories.begin(); it != m_categories.end(); ++it)
    delete *it;

  m_categories.clear();
}
  
bool CSettingSection::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  // handle <visible> conditions
  if (!ISetting::Deserialize(node, update))
    return false;
    
  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  int tmp = -1;
  if (element->QueryIntAttribute(XML_ATTR_LABEL, &tmp) == TIXML_SUCCESS && tmp > 0)
    m_label = tmp;
  if (element->QueryIntAttribute(XML_ATTR_HELP, &tmp) == TIXML_SUCCESS && tmp > 0)
    m_help = tmp;
    
  const TiXmlNode *categoryNode = node->FirstChild(XML_CATEGORY);
  while (categoryNode != NULL)
  {
    std::string categoryId;
    if (CSettingCategory::DeserializeIdentification(categoryNode, categoryId))
    {
      CSettingCategory *category = NULL;
      for (SettingCategoryList::iterator itCategory = m_categories.begin(); itCategory != m_categories.end(); ++itCategory)
      {
        if ((*itCategory)->GetId() == categoryId)
        {
          category = *itCategory;
          break;
        }
      }
      
      update = (category != NULL);
      if (!update)
        category = new CSettingCategory(categoryId, m_settingsManager);

      if (category->Deserialize(categoryNode, update))
      {
        if (!update)
          addISetting(categoryNode, category, m_categories);
      }
      else
      {
        CLog::Log(LOGWARNING, "CSettingSection: unable to read category \"%s\"", categoryId.c_str());
        if (!update)
          delete category;
      }
    }
      
    categoryNode = categoryNode->NextSibling(XML_CATEGORY);
  }
    
  return true;
}

SettingCategoryList CSettingSection::GetCategories(SettingLevel level) const
{
  SettingCategoryList categories;

  for (SettingCategoryList::const_iterator it = m_categories.begin(); it != m_categories.end(); ++it)
  {
    if ((*it)->MeetsRequirements() && (*it)->IsVisible() && (*it)->GetGroups(level).size() > 0)
      categories.push_back(*it);
  }

  return categories;
}
