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

#include "SmartPlayList.h"
#include "Util.h"
#include "XBDateTime.h"
#include "filesystem/File.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/DatabaseUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "video/VideoDatabase.h"

using namespace std;
using namespace XFILE;

typedef struct
{
  char string[17];
  Field field;
  SortBy sort;
  CSmartPlaylistRule::FIELD_TYPE type;
  StringValidation::Validator validator;
  bool browseable;
  int localizedString;
} translateField;

static const translateField fields[] = {
  { "none",              FieldNone,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 231 },
  { "filename",          FieldFilename,                SortByFile,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 561 },
  { "path",              FieldPath,                    SortByPath,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  573 },
  { "album",             FieldAlbum,                   SortByAlbum,                    CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  558 },
  { "albumartist",       FieldAlbumArtist,             SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  566 },
  { "artist",            FieldArtist,                  SortByArtist,                   CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  557 },
  { "tracknumber",       FieldTrackNumber,             SortByTrackNumber,              CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 554 },
  { "comment",           FieldComment,                 SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 569 },
  { "review",            FieldReview,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 183 },
  { "themes",            FieldThemes,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21895 },
  { "moods",             FieldMoods,                   SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 175 },
  { "styles",            FieldStyles,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 176 },
  { "type",              FieldAlbumType,               SortByAlbumType,                CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 564 },
  { "label",             FieldMusicLabel,              SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21899 },
  { "title",             FieldTitle,                   SortByTitle,                    CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  556 },
  { "sorttitle",         FieldSortTitle,               SortBySortTitle,                CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 171 },
  { "year",              FieldYear,                    SortByYear,                     CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
  { "time",              FieldTime,                    SortByTime,                     CSmartPlaylistRule::SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  { "playcount",         FieldPlaycount,               SortByPlaycount,                CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 567 },
  { "lastplayed",        FieldLastPlayed,              SortByLastPlayed,               CSmartPlaylistRule::DATE_FIELD,     NULL,                                 false, 568 },
  { "inprogress",        FieldInProgress,              SortByNone,                     CSmartPlaylistRule::BOOLEAN_FIELD,  NULL,                                 false, 575 },
  { "rating",            FieldRating,                  SortByRating,                   CSmartPlaylistRule::NUMERIC_FIELD,  CSmartPlaylistRule::ValidateRating,   false, 563 },
  { "votes",             FieldVotes,                   SortByVotes,                    CSmartPlaylistRule::TEXT_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  { "top250",            FieldTop250,                  SortByTop250,                   CSmartPlaylistRule::NUMERIC_FIELD,  NULL,                                 false, 13409 },
  { "mpaarating",        FieldMPAA,                    SortByMPAA,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 20074 },
  { "dateadded",         FieldDateAdded,               SortByDateAdded,                CSmartPlaylistRule::DATE_FIELD,     NULL,                                 false, 570 },
  { "genre",             FieldGenre,                   SortByGenre,                    CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  515 },
  { "plot",              FieldPlot,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 207 },
  { "plotoutline",       FieldPlotOutline,             SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 203 },
  { "tagline",           FieldTagline,                 SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 202 },
  { "set",               FieldSet,                     SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20457 },
  { "director",          FieldDirector,                SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20339 },
  { "actor",             FieldActor,                   SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20337 },
  { "writers",           FieldWriter,                  SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20417 },
  { "airdate",           FieldAirDate,                 SortByYear,                     CSmartPlaylistRule::DATE_FIELD,     NULL,                                 false, 20416 },
  { "hastrailer",        FieldTrailer,                 SortByNone,                     CSmartPlaylistRule::BOOLEAN_FIELD,  NULL,                                 false, 20423 },
  { "studio",            FieldStudio,                  SortByStudio,                   CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  572 },
  { "country",           FieldCountry,                 SortByCountry,                  CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  574 },
  { "tvshow",            FieldTvShowTitle,             SortByTvShowTitle,              CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20364 },
  { "status",            FieldTvShowStatus,            SortByTvShowStatus,             CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 126 },
  { "season",            FieldSeason,                  SortBySeason,                   CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20373 },
  { "episode",           FieldEpisodeNumber,           SortByEpisodeNumber,            CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20359 },
  { "numepisodes",       FieldNumberOfEpisodes,        SortByNumberOfEpisodes,         CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20360 },
  { "numwatched",        FieldNumberOfWatchedEpisodes, SortByNumberOfWatchedEpisodes,  CSmartPlaylistRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 21457 },
  { "videoresolution",   FieldVideoResolution,         SortByVideoResolution,          CSmartPlaylistRule::NUMERIC_FIELD,  NULL,                                 false, 21443 },
  { "videocodec",        FieldVideoCodec,              SortByVideoCodec,               CSmartPlaylistRule::TEXTIN_FIELD,   NULL,                                 false, 21445 },
  { "videoaspect",       FieldVideoAspectRatio,        SortByVideoAspectRatio,         CSmartPlaylistRule::NUMERIC_FIELD,  NULL,                                 false, 21374 },
  { "audiochannels",     FieldAudioChannels,           SortByAudioChannels,            CSmartPlaylistRule::NUMERIC_FIELD,  NULL,                                 false, 21444 },
  { "audiocodec",        FieldAudioCodec,              SortByAudioCodec,               CSmartPlaylistRule::TEXTIN_FIELD,   NULL,                                 false, 21446 },
  { "audiolanguage",     FieldAudioLanguage,           SortByAudioLanguage,            CSmartPlaylistRule::TEXTIN_FIELD,   NULL,                                 false, 21447 },
  { "subtitlelanguage",  FieldSubtitleLanguage,        SortBySubtitleLanguage,         CSmartPlaylistRule::TEXTIN_FIELD,   NULL,                                 false, 21448 },
  { "random",            FieldRandom,                  SortByRandom,                   CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 590 },
  { "playlist",          FieldPlaylist,                SortByPlaylistOrder,            CSmartPlaylistRule::PLAYLIST_FIELD, NULL,                                 true,  559 },
  { "virtualfolder",     FieldVirtualFolder,           SortByNone,                     CSmartPlaylistRule::PLAYLIST_FIELD, NULL,                                 true,  614 },
  { "tag",               FieldTag,                     SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 true,  20459 },
  { "instruments",       FieldInstruments,             SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21892 },
  { "biography",         FieldBiography,               SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21887 },
  { "born",              FieldBorn,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21893 },
  { "bandformed",        FieldBandFormed,              SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21894 },
  { "disbanded",         FieldDisbanded,               SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21896 },
  { "died",              FieldDied,                    SortByNone,                     CSmartPlaylistRule::TEXT_FIELD,     NULL,                                 false, 21897 }
};

static const size_t NUM_FIELDS = sizeof(fields) / sizeof(translateField);

typedef struct
{
  char string[15];
  CSmartPlaylistRule::SEARCH_OPERATOR op;
  int localizedString;
} operatorField;

static const operatorField operators[] = {
  { "contains",        CSmartPlaylistRule::OPERATOR_CONTAINS,          21400 },
  { "doesnotcontain",  CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN,  21401 },
  { "is",              CSmartPlaylistRule::OPERATOR_EQUALS,            21402 },
  { "isnot",           CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL,    21403 },
  { "startswith",      CSmartPlaylistRule::OPERATOR_STARTS_WITH,       21404 },
  { "endswith",        CSmartPlaylistRule::OPERATOR_ENDS_WITH,         21405 },
  { "greaterthan",     CSmartPlaylistRule::OPERATOR_GREATER_THAN,      21406 },
  { "lessthan",        CSmartPlaylistRule::OPERATOR_LESS_THAN,         21407 },
  { "after",           CSmartPlaylistRule::OPERATOR_AFTER,             21408 },
  { "before",          CSmartPlaylistRule::OPERATOR_BEFORE,            21409 },
  { "inthelast",       CSmartPlaylistRule::OPERATOR_IN_THE_LAST,       21410 },
  { "notinthelast",    CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST,   21411 },
  { "true",            CSmartPlaylistRule::OPERATOR_TRUE,              20122 },
  { "false",           CSmartPlaylistRule::OPERATOR_FALSE,             20424 },
  { "between",         CSmartPlaylistRule::OPERATOR_BETWEEN,           21456 }
};

static const size_t NUM_OPERATORS = sizeof(operators) / sizeof(operatorField);

typedef struct
{
  std::string name;
  Field field;
  bool canMix;
  int localizedString;
} group;

static const group groups[] = { { "",           FieldUnknown,   false,    571 },
                                { "none",       FieldNone,      false,    231 },
                                { "sets",       FieldSet,       true,   20434 },
                                { "genres",     FieldGenre,     false,    135 },
                                { "years",      FieldYear,      false,    652 },
                                { "actors",     FieldActor,     false,    344 },
                                { "directors",  FieldDirector,  false,  20348 },
                                { "writers",    FieldWriter,    false,  20418 },
                                { "studios",    FieldStudio,    false,  20388 },
                                { "countries",  FieldCountry,   false,  20451 },
                                { "artists",    FieldArtist,    false,    133 },
                                { "albums",     FieldAlbum,     false,    132 },
                                { "tags",       FieldTag,       false,  20459 },
                              };

static const size_t NUM_GROUPS = sizeof(groups) / sizeof(group);

#define RULE_VALUE_SEPARATOR  " / "

CSmartPlaylistRule::CSmartPlaylistRule()
{
  m_field = FieldNone;
  m_operator = OPERATOR_CONTAINS;
  m_parameter.clear();
}

bool CSmartPlaylistRule::Load(const TiXmlNode *node, const std::string &encoding /* = "UTF-8" */)
{
  if (node == NULL)
    return false;

  const TiXmlElement *element = node->ToElement();
  if (element == NULL)
    return false;

  // format is:
  // <rule field="Genre" operator="contains">parameter</rule>
  // where parameter can either be a string or a list of
  // <value> tags containing a string
  const char *field = element->Attribute("field");
  const char *oper = element->Attribute("operator");
  if (field == NULL || oper == NULL)
    return false;

  m_field = TranslateField(field);
  m_operator = TranslateOperator(oper);

  if (m_operator == OPERATOR_TRUE || m_operator == OPERATOR_FALSE)
    return true;

  const TiXmlNode *parameter = element->FirstChild();
  if (parameter == NULL)
    return false;

  if (parameter->Type() == TiXmlNode::TINYXML_TEXT)
  {
    CStdString utf8Parameter;
    if (encoding.empty()) // utf8
      utf8Parameter = parameter->ValueStr();
    else
      g_charsetConverter.ToUtf8(encoding, parameter->ValueStr(), utf8Parameter);

    if (!utf8Parameter.empty())
      m_parameter.push_back(utf8Parameter);
  }
  else if (parameter->Type() == TiXmlNode::TINYXML_ELEMENT)
  {
    const TiXmlNode *valueNode = element->FirstChild("value");
    while (valueNode != NULL)
    {
      const TiXmlNode *value = valueNode->FirstChild();
      if (value != NULL && value->Type() == TiXmlNode::TINYXML_TEXT)
      {
        CStdString utf8Parameter;
        if (encoding.empty()) // utf8
          utf8Parameter = value->ValueStr();
        else
          g_charsetConverter.ToUtf8(encoding, value->ValueStr(), utf8Parameter);

        if (!utf8Parameter.empty())
          m_parameter.push_back(utf8Parameter);
      }

      valueNode = valueNode->NextSibling("value");
    }
  }
  else
    return false;

  return true;
}

bool CSmartPlaylistRule::Load(const CVariant &obj)
{
  if (!obj.isObject() ||
      !obj.isMember("field") || !obj["field"].isString() ||
      !obj.isMember("operator") || !obj["operator"].isString())
    return false;

  m_field = TranslateField(obj["field"].asString().c_str());
  m_operator = TranslateOperator(obj["operator"].asString().c_str());

  if (m_operator == OPERATOR_TRUE || m_operator == OPERATOR_FALSE)
    return true;

  if (!obj.isMember("value") || (!obj["value"].isString() && !obj["value"].isArray()))
    return false;

  const CVariant &value = obj["value"];
  if (value.isString() && !value.asString().empty())
    m_parameter.push_back(value.asString());
  else if (value.isArray())
  {
    for (CVariant::const_iterator_array val = value.begin_array(); val != value.end_array(); val++)
    {
      if (val->isString() && !val->asString().empty())
        m_parameter.push_back(val->asString());
    }
  }
  else
    return false;

  return true;
}

bool CSmartPlaylistRule::Save(TiXmlNode *parent) const
{
  if (parent == NULL || (m_parameter.empty() && m_operator != OPERATOR_TRUE && m_operator != OPERATOR_FALSE))
    return false;

  TiXmlElement rule("rule");
  rule.SetAttribute("field", TranslateField(m_field).c_str());
  rule.SetAttribute("operator", TranslateOperator(m_operator).c_str());

  for (vector<CStdString>::const_iterator it = m_parameter.begin(); it != m_parameter.end(); it++)
  {
    TiXmlElement value("value");
    TiXmlText text(it->c_str());
    value.InsertEndChild(text);
    rule.InsertEndChild(value);
  }

  parent->InsertEndChild(rule);

  return true;
}

bool CSmartPlaylistRule::Save(CVariant &obj) const
{
  if (obj.isNull() || (m_parameter.empty() && m_operator != OPERATOR_TRUE && m_operator != OPERATOR_FALSE))
    return false;

  obj["field"] = TranslateField(m_field);
  obj["operator"] = TranslateOperator(m_operator);

  obj["value"] = CVariant(CVariant::VariantTypeArray);
  for (vector<CStdString>::const_iterator it = m_parameter.begin(); it != m_parameter.end(); it++)
    obj["value"].push_back(*it);

  return true;
}

Field CSmartPlaylistRule::TranslateField(const char *field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (StringUtils::EqualsNoCase(field, fields[i].string)) return fields[i].field;
  return FieldNone;
}

CStdString CSmartPlaylistRule::TranslateField(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
  return "none";
}

SortBy CSmartPlaylistRule::TranslateOrder(const char *order)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (StringUtils::EqualsNoCase(order, fields[i].string)) return fields[i].sort;
  return SortByNone;
}

CStdString CSmartPlaylistRule::TranslateOrder(SortBy order)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (order == fields[i].sort) return fields[i].string;
  return "none";
}

CSmartPlaylistRule::SEARCH_OPERATOR CSmartPlaylistRule::TranslateOperator(const char *oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (StringUtils::EqualsNoCase(oper, operators[i].string)) return operators[i].op;
  return OPERATOR_CONTAINS;
}

CStdString CSmartPlaylistRule::TranslateOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return operators[i].string;
  return "contains";
}

Field CSmartPlaylistRule::TranslateGroup(const char *group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (StringUtils::EqualsNoCase(group, groups[i].name))
      return groups[i].field;
  }

  return FieldUnknown;
}

CStdString CSmartPlaylistRule::TranslateGroup(Field group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return groups[i].name;
  }

  return "";
}

CStdString CSmartPlaylistRule::GetLocalizedField(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return g_localizeStrings.Get(fields[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CSmartPlaylistRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

bool CSmartPlaylistRule::IsFieldBrowseable(Field field)
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].browseable;

  return false;
}

bool CSmartPlaylistRule::Validate(const std::string &input, void *data)
{
  if (data == NULL)
    return true;

  CSmartPlaylistRule *rule = (CSmartPlaylistRule*)data;

  // check if there's a validator for this rule
  StringValidation::Validator validator = NULL;
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
  {
    if (rule->m_field == fields[i].field)
    {
        validator = fields[i].validator;
        break;
    }
  }
  if (validator == NULL)
    return true;

  // split the input into multiple values and validate every value separately
  vector<string> values = StringUtils::Split(input, RULE_VALUE_SEPARATOR);
  for (vector<string>::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    if (!validator(*it, data))
      return false;
  }

  return true;
}

bool CSmartPlaylistRule::ValidateRating(const std::string &input, void *data)
{
  char *end = NULL;
  string strRating = input;
  StringUtils::Trim(strRating);

  double rating = strtod(strRating.c_str(), &end);
  return (end == NULL || *end == '\0') &&
         rating >= 0.0 && rating <= 10.0;
}

vector<Field> CSmartPlaylistRule::GetFields(const CStdString &type)
{
  vector<Field> fields;
  bool isVideo = false;
  if (type == "songs")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldYear);
    fields.push_back(FieldTime);
    fields.push_back(FieldTrackNumber);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldRating);
    fields.push_back(FieldComment);
  }
  else if (type == "albums")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);        // any artist
    fields.push_back(FieldAlbumArtist);  // album artist
    fields.push_back(FieldYear);
    fields.push_back(FieldReview);
    fields.push_back(FieldThemes);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldAlbumType);
    fields.push_back(FieldMusicLabel);
    fields.push_back(FieldRating);
    fields.push_back(FieldPlaycount);
  }
  else if (type == "artists")
  {
    fields.push_back(FieldArtist);
    fields.push_back(FieldGenre);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldInstruments);
    fields.push_back(FieldBiography);
    fields.push_back(FieldBorn);
    fields.push_back(FieldBandFormed);
    fields.push_back(FieldDisbanded);
    fields.push_back(FieldDied);
  }
  else if (type == "tvshows")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTvShowStatus);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldYear);
    fields.push_back(FieldGenre);
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldNumberOfEpisodes);
    fields.push_back(FieldNumberOfWatchedEpisodes);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldTag);
  }
  else if (type == "episodes")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldTvShowTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldAirDate);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldEpisodeNumber);
    fields.push_back(FieldSeason);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  else if (type == "movies")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldPlotOutline);
    fields.push_back(FieldTagline);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldCountry);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldTop250);
    fields.push_back(FieldStudio);
    fields.push_back(FieldTrailer);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldSet);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  else if (type == "musicvideos")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldYear);
    fields.push_back(FieldArtist);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldTime);
    fields.push_back(FieldDirector);
    fields.push_back(FieldStudio);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  if (isVideo)
  {
    fields.push_back(FieldVideoResolution);
    fields.push_back(FieldAudioChannels);
    fields.push_back(FieldVideoCodec);
    fields.push_back(FieldAudioCodec);
    fields.push_back(FieldAudioLanguage);
    fields.push_back(FieldSubtitleLanguage);
    fields.push_back(FieldVideoAspectRatio);
  }
  fields.push_back(FieldPlaylist);
  fields.push_back(FieldVirtualFolder);
  
  return fields;
}

std::vector<SortBy> CSmartPlaylistRule::GetOrders(const CStdString &type)
{
  vector<SortBy> orders;
  orders.push_back(SortByNone);
  if (type == "songs")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);
    orders.push_back(SortByTitle);
    orders.push_back(SortByYear);
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByRating);
  }
  else if (type == "albums")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);        // any artist
    orders.push_back(SortByYear);
    //orders.push_back(SortByThemes);
    //orders.push_back(SortByMoods);
    //orders.push_back(SortByStyles);
    orders.push_back(SortByAlbumType);
    //orders.push_back(SortByMusicLabel);
    orders.push_back(SortByRating);
    orders.push_back(SortByPlaycount);
  }
  else if (type == "artists")
  {
    orders.push_back(SortByArtist);
  }
  else if (type == "tvshows")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByTvShowStatus);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByYear);
    orders.push_back(SortByGenre);
    orders.push_back(SortByNumberOfEpisodes);
    orders.push_back(SortByNumberOfWatchedEpisodes);
    //orders.push_back(SortByPlaycount);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "episodes")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByTvShowTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByYear); // premiered/dateaired
    orders.push_back(SortByEpisodeNumber);
    orders.push_back(SortBySeason);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "movies")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByGenre);
    orders.push_back(SortByCountry);
    orders.push_back(SortByYear); // premiered
    orders.push_back(SortByMPAA);
    orders.push_back(SortByTop250);
    orders.push_back(SortByStudio);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "musicvideos")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByYear);
    orders.push_back(SortByArtist);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByTime);
    orders.push_back(SortByStudio);
    orders.push_back(SortByDateAdded);
  }
  orders.push_back(SortByRandom);
	
  return orders;
}

std::vector<Field> CSmartPlaylistRule::GetGroups(const CStdString &type)
{
  vector<Field> groups;
  groups.push_back(FieldUnknown);

  if (type == "artists")
    groups.push_back(FieldGenre);
  else if (type == "albums")
    groups.push_back(FieldYear);
  if (type == "movies")
  {
    groups.push_back(FieldNone);
    groups.push_back(FieldSet);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldWriter);
    groups.push_back(FieldStudio);
    groups.push_back(FieldCountry);
    groups.push_back(FieldTag);
  }
  else if (type == "tvshows")
  {
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }
  else if (type == "musicvideos")
  {
    groups.push_back(FieldArtist);
    groups.push_back(FieldAlbum);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }

  return groups;
}

CStdString CSmartPlaylistRule::GetLocalizedOperator(SEARCH_OPERATOR oper)
{
  for (unsigned int i = 0; i < NUM_OPERATORS; i++)
    if (oper == operators[i].op) return g_localizeStrings.Get(operators[i].localizedString);
  return g_localizeStrings.Get(16018);
}

CStdString CSmartPlaylistRule::GetLocalizedGroup(Field group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return g_localizeStrings.Get(groups[i].localizedString);
  }

  return g_localizeStrings.Get(groups[0].localizedString);
}

bool CSmartPlaylistRule::CanGroupMix(Field group)
{
  for (unsigned int i = 0; i < NUM_GROUPS; i++)
  {
    if (group == groups[i].field)
      return groups[i].canMix;
  }

  return false;
}

CStdString CSmartPlaylistRule::GetLocalizedRule() const
{
  CStdString rule;
  rule.Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), GetParameter().c_str());
  return rule;
}

CStdString CSmartPlaylistRule::GetParameter() const
{
  return StringUtils::JoinString(m_parameter, RULE_VALUE_SEPARATOR);
}

void CSmartPlaylistRule::SetParameter(const CStdString &value)
{
  m_parameter.clear();
  StringUtils::SplitString(value, RULE_VALUE_SEPARATOR, m_parameter);
}

void CSmartPlaylistRule::SetParameter(const std::vector<CStdString> &values)
{
  m_parameter.assign(values.begin(), values.end());
}

CStdString CSmartPlaylistRule::GetVideoResolutionQuery(const CStdString &parameter) const
{
  CStdString retVal(" in (select distinct idFile from streamdetails where iVideoWidth ");
  int iRes = (int)strtol(parameter.c_str(), NULL, 10);

  int min, max;
  if (iRes >= 1080)     { min = 1281; max = INT_MAX; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }

  switch (m_operator)
  {
    case OPERATOR_EQUALS:
      retVal.AppendFormat(">= %i and iVideoWidth <= %i)", min, max);
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal.AppendFormat("< %i or iVideoWidth > %i)", min, max);
      break;
    case OPERATOR_LESS_THAN:
      retVal.AppendFormat("< %i)", min);
      break;
    case OPERATOR_GREATER_THAN:
      retVal.AppendFormat("> %i)", max);
      break;
    default:
      retVal += ")";
      break;
  }
  return retVal;
}

CStdString CSmartPlaylistRule::GetWhereClause(const CDatabase &db, const CStdString& strType) const
{
  SEARCH_OPERATOR op = m_operator;
  if ((strType == "tvshows" || strType == "episodes") && m_field == FieldYear)
  { // special case for premiered which is a date rather than a year
    // TODO: SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }

  CStdString operatorString, negate;
  if (GetFieldType(m_field) == TEXTIN_FIELD)
  {
    if (op == OPERATOR_DOES_NOT_EQUAL)
      negate = " NOT";
  }
  else
  {
    // the comparison piece
    switch (op)
    {
    case OPERATOR_CONTAINS:
      operatorString = " LIKE '%%%s%%'"; break;
    case OPERATOR_DOES_NOT_CONTAIN:
      negate = " NOT"; operatorString = " LIKE '%%%s%%'"; break;
    case OPERATOR_EQUALS:
      if (GetFieldType(m_field) == NUMERIC_FIELD || GetFieldType(m_field) == SECONDS_FIELD)
        operatorString = " = %s";
      else
        operatorString = " LIKE '%s'";
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      if (GetFieldType(m_field) == NUMERIC_FIELD || GetFieldType(m_field) == SECONDS_FIELD)
        operatorString = " != %s";
      else
      {
        negate = " NOT";
        operatorString = " LIKE '%s'";
      }
      break;
    case OPERATOR_STARTS_WITH:
      operatorString = " LIKE '%s%%'"; break;
    case OPERATOR_ENDS_WITH:
      operatorString = " LIKE '%%%s'"; break;
    case OPERATOR_AFTER:
    case OPERATOR_GREATER_THAN:
    case OPERATOR_IN_THE_LAST:
      operatorString = " > ";
      if (GetFieldType(m_field) == NUMERIC_FIELD || GetFieldType(m_field) == SECONDS_FIELD)
        operatorString += "%s";
      else
        operatorString += "'%s'";
      break;
    case OPERATOR_BEFORE:
    case OPERATOR_LESS_THAN:
    case OPERATOR_NOT_IN_THE_LAST:
      operatorString = " < ";
      if (GetFieldType(m_field) == NUMERIC_FIELD || GetFieldType(m_field) == SECONDS_FIELD)
        operatorString += "%s";
      else
        operatorString += "'%s'";
      break;
    case OPERATOR_TRUE:
      operatorString = " = 1"; break;
    case OPERATOR_FALSE:
      negate = " NOT "; operatorString = " = 0"; break;
    default:
      break;
    }
  }

  // boolean operators don't have any values in m_parameter, they work on the operator
  if (m_operator == OPERATOR_FALSE || m_operator == OPERATOR_TRUE)
  {
    if (strType == "movies")
    {
      if (m_field == FieldInProgress)
        return "movieview.idFile " + negate + " IN (select idFile from bookmark where type = 1)";
      else if (m_field == FieldTrailer)
        return negate + GetField(m_field, strType) + "!= ''";
    }
    else if (strType == "episodes")
    {
      if (m_field == FieldInProgress)
        return "episodeview.idFile " + negate + " IN (select idFile from bookmark where type = 1)";
    }
    else if (strType == "tvshows")
    {
      if (m_field == FieldInProgress)
        return GetField(FieldId, strType) + negate + " IN (select " + GetField(FieldId, strType) + " from tvshowview where "
               "(watchedcount > 0 AND watchedcount < totalCount) OR "
               "(watchedcount = 0 AND " + GetField(FieldId, strType) + " IN "
               "(select episodeview.idShow from episodeview WHERE episodeview.idShow = " + GetField(FieldId, strType) + " AND episodeview.resumeTimeInSeconds > 0)))";
    }
  }

  // The BETWEEN operator is handled special
  if (op == OPERATOR_BETWEEN)
  {
    if (m_parameter.size() != 2)
      return "";

    FIELD_TYPE fieldType = GetFieldType(m_field);
    if (fieldType == NUMERIC_FIELD || m_field == FieldYear)
      return db.PrepareSQL("CAST(%s as DECIMAL(5,1)) BETWEEN %s AND %s", GetField(m_field, strType).c_str(), m_parameter[0].c_str(), m_parameter[1].c_str());
    else if (fieldType == SECONDS_FIELD)
      return db.PrepareSQL("CAST(%s as INTEGER) BETWEEN %s AND %s", GetField(m_field, strType).c_str(), m_parameter[0].c_str(), m_parameter[1].c_str());
    else
      return db.PrepareSQL("%s BETWEEN '%s' AND '%s'", GetField(m_field, strType).c_str(), m_parameter[0].c_str(), m_parameter[1].c_str());
  }

  // now the query parameter
  CStdString wholeQuery;
  for (vector<CStdString>::const_iterator it = m_parameter.begin(); it != m_parameter.end(); /* it++ is done further down */)
  {
    CStdString parameter;
    if (GetFieldType(m_field) == TEXTIN_FIELD)
    {
      CStdStringArray split;
      StringUtils::SplitString(*it, ",", split);
      for (CStdStringArray::iterator itIn = split.begin(); itIn != split.end(); ++itIn)
      {
        if (!parameter.IsEmpty())
          parameter += ",";
        parameter += db.PrepareSQL("'%s'", (*itIn).Trim().c_str());
      }
      parameter = " IN (" + parameter + ")";
    }
    else
      parameter = db.PrepareSQL(operatorString.c_str(), it->c_str());

    if (GetFieldType(m_field) == DATE_FIELD)
    {
      if (m_operator == OPERATOR_IN_THE_LAST || m_operator == OPERATOR_NOT_IN_THE_LAST)
      { // translate time period
        CDateTime date=CDateTime::GetCurrentDateTime();
        CDateTimeSpan span;
        span.SetFromPeriod(*it);
        date-=span;
        parameter = db.PrepareSQL(operatorString.c_str(), date.GetAsDBDate().c_str());
      }
    }
    else if (m_field == FieldTime)
    { // translate time to seconds
      CStdString seconds; seconds.Format("%i", StringUtils::TimeStringToSeconds(*it));
      parameter = db.PrepareSQL(operatorString.c_str(), seconds.c_str());
    }

    CStdString query;
    CStdString table;
    if (strType == "songs")
    {
      table = "songview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idSong FROM song_genre, genre WHERE song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
      else if (m_field == FieldArtist)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idSong FROM song_artist, artist WHERE song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
      else if (m_field == FieldAlbumArtist)
        query = table + ".idAlbum" + negate + " IN (SELECT idAlbum FROM album_artist, artist WHERE album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
      else if (m_field == FieldLastPlayed && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
        query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    }
    else if (strType == "albums")
    {
      table = "albumview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT song.idAlbum FROM song, song_genre, genre WHERE song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
      else if (m_field == FieldArtist)
        query = GetField(FieldId, strType) + negate + " IN (SELECT song.idAlbum FROM song, song_artist, artist WHERE song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
      else if (m_field == FieldAlbumArtist)
        query = GetField(FieldId, strType) + negate + " IN (SELECT album_artist.idAlbum FROM album_artist, artist WHERE album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    }
    else if (strType == "artists")
    {
      table = "artistview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT song_artist.idArtist FROM song_artist, song_genre, genre WHERE song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    }
    else if (strType == "movies")
    {
      table = "movieview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM genrelinkmovie JOIN genre ON genre.idGenre=genrelinkmovie.idGenre WHERE genre.strGenre" + parameter + ")";
      else if (m_field == FieldDirector)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM directorlinkmovie JOIN actors ON actors.idActor=directorlinkmovie.idDirector WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldActor)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM actorlinkmovie JOIN actors ON actors.idActor=actorlinkmovie.idActor WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldWriter)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM writerlinkmovie JOIN actors ON actors.idActor=writerlinkmovie.idWriter WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldStudio)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM studiolinkmovie JOIN studio ON studio.idStudio=studiolinkmovie.idStudio WHERE studio.strStudio" + parameter + ")";
      else if (m_field == FieldCountry)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMovie FROM countrylinkmovie JOIN country ON country.idCountry=countrylinkmovie.idCountry WHERE country.strCountry" + parameter + ")";
      else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
        query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
      else if (m_field == FieldTag)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMedia FROM taglinks JOIN tag ON tag.idTag = taglinks.idTag WHERE tag.strTag" + parameter + " AND taglinks.media_type = 'movie')";
    }
    else if (strType == "musicvideos")
    {
      table = "musicvideoview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMVideo FROM genrelinkmusicvideo JOIN genre ON genre.idGenre=genrelinkmusicvideo.idGenre WHERE genre.strGenre" + parameter + ")";
      else if (m_field == FieldArtist)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMVideo FROM artistlinkmusicvideo JOIN actors ON actors.idActor=artistlinkmusicvideo.idArtist WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldStudio)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMVideo FROM studiolinkmusicvideo JOIN studio ON studio.idStudio=studiolinkmusicvideo.idStudio WHERE studio.strStudio" + parameter + ")";
      else if (m_field == FieldDirector)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMVideo FROM directorlinkmusicvideo JOIN actors ON actors.idActor=directorlinkmusicvideo.idDirector WHERE actors.strActor" + parameter + ")";
      else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
        query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
      else if (m_field == FieldTag)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMedia FROM taglinks JOIN tag ON tag.idTag = taglinks.idTag WHERE tag.strTag" + parameter + " AND taglinks.media_type = 'musicvideo')";
    }
    else if (strType == "tvshows")
    {
      table = "tvshowview";

      if (m_field == FieldGenre)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idShow FROM genrelinktvshow JOIN genre ON genre.idGenre=genrelinktvshow.idGenre WHERE genre.strGenre" + parameter + ")";
      else if (m_field == FieldDirector)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idShow FROM directorlinktvshow JOIN actors ON actors.idActor=directorlinktvshow.idDirector WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldActor)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idShow FROM actorlinktvshow JOIN actors ON actors.idActor=actorlinktvshow.idActor WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldStudio)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetField(m_field, strType) + parameter + ")";
      else if (m_field == FieldMPAA)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idShow FROM tvshowview WHERE " + GetField(m_field, strType) + parameter + ")";
      else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
        query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
      else if (m_field == FieldPlaycount)
        query = "CASE WHEN COALESCE(" + GetField(FieldNumberOfEpisodes, strType) + " - " + GetField(FieldNumberOfWatchedEpisodes, strType) + ", 0) > 0 THEN 0 ELSE 1 END " + parameter;
      else if (m_field == FieldTag)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idMedia FROM taglinks JOIN tag ON tag.idTag = taglinks.idTag WHERE tag.strTag" + parameter + " AND taglinks.media_type = 'tvshow')";
    }
    else if (strType == "episodes")
    {
      table = "episodeview";

      if (m_field == FieldGenre)
        query = table + ".idShow" + negate + " IN (SELECT idShow FROM genrelinktvshow JOIN genre ON genre.idGenre=genrelinktvshow.idGenre WHERE genre.strGenre" + parameter + ")";
      else if (m_field == FieldDirector)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idEpisode FROM directorlinkepisode JOIN actors ON actors.idActor=directorlinkepisode.idDirector WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldActor)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idEpisode FROM actorlinkepisode JOIN actors ON actors.idActor=actorlinkepisode.idActor WHERE actors.strActor" + parameter + ")";
      else if (m_field == FieldWriter)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idEpisode FROM writerlinkepisode JOIN actors ON actors.idActor=writerlinkepisode.idWriter WHERE actors.strActor" + parameter + ")";
      else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
        query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
      else if (m_field == FieldStudio)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idEpisode FROM episodeview WHERE strStudio" + parameter + ")";
      else if (m_field == FieldMPAA)
        query = GetField(FieldId, strType) + negate + " IN (SELECT idEpisode FROM episodeview WHERE mpaa" + parameter + ")";
    }
    if (m_field == FieldVideoResolution)
      query = table + ".idFile" + negate + GetVideoResolutionQuery(*it);
    else if (m_field == FieldAudioChannels)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE iAudioChannels " + parameter + ")";
    else if (m_field == FieldVideoCodec)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE strVideoCodec " + parameter + ")";
    else if (m_field == FieldAudioCodec)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE strAudioCodec " + parameter + ")";
    else if (m_field == FieldAudioLanguage)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE strAudioLanguage " + parameter + ")";
    else if (m_field == FieldSubtitleLanguage)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE strSubtitleLanguage " + parameter + ")";
    else if (m_field == FieldVideoAspectRatio)
      query = table + ".idFile" + negate + " IN (SELECT DISTINCT idFile FROM streamdetails WHERE fVideoAspect " + parameter + ")";
    if (m_field == FieldPlaycount && strType != "songs" && strType != "albums" && strType != "tvshows")
    { // playcount IS stored as NULL OR number IN video db
      if ((m_operator == OPERATOR_EQUALS && it->Equals("0")) ||
          (m_operator == OPERATOR_DOES_NOT_EQUAL && !it->Equals("0")) ||
          (m_operator == OPERATOR_LESS_THAN))
      {
        CStdString field = GetField(FieldPlaycount, strType);
        query = field + " IS NULL OR " + field + parameter;
      }
    }

    if (query.IsEmpty() && m_field != FieldNone)
    {
      string fmt = "%s";
      if (GetFieldType(m_field) == NUMERIC_FIELD)
        fmt = "CAST(%s as DECIMAL(5,1))";
      else if (GetFieldType(m_field) == SECONDS_FIELD)
        fmt = "CAST(%s as INTEGER)";

      query.Format(fmt.c_str(), GetField(m_field,strType).c_str());
      query += negate + parameter;
    }
    
    it++;
    if (query.Equals(negate + parameter))
      query = "1";

    query = "(" + query + ")";
    if (it != m_parameter.end())
      query += " OR ";

    wholeQuery += query;
  }

  return wholeQuery;
}

CStdString CSmartPlaylistRule::GetField(Field field, const CStdString& type)
{
  return DatabaseUtils::GetField(field, DatabaseUtils::MediaTypeFromString(type), DatabaseQueryPartWhere);
}

CSmartPlaylistRuleCombination::CSmartPlaylistRuleCombination()
  : m_type(CombinationAnd)
{ }

CStdString CSmartPlaylistRuleCombination::GetWhereClause(const CDatabase &db, const CStdString& strType, std::set<CStdString> &referencedPlaylists) const
{
  CStdString rule, currentRule;
  
  // translate the combinations into SQL
  for (vector<CSmartPlaylistRuleCombination>::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    if (it != m_combinations.begin())
      rule += m_type == CombinationAnd ? " AND " : " OR ";
    rule += "(" + it->GetWhereClause(db, strType, referencedPlaylists) + ")";
  }

  // translate the rules into SQL
  for (CSmartPlaylistRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if (it->m_field == FieldVirtualFolder)
      continue;

    if (!rule.empty())
      rule += m_type == CombinationAnd ? " AND " : " OR ";
    rule += "(";
    CStdString currentRule;
    if (it->m_field == FieldPlaylist)
    {
      CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(it->m_parameter.at(0), strType);
      if (!playlistFile.IsEmpty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          CStdString playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType().Equals(strType) || (playlist.GetType().Equals("mixed") && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().IsEmpty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetWhereClause(db, referencedPlaylists);
          }
          if (playlist.GetType().Equals(strType))
          {
            if (it->m_operator == CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule.Format("NOT (%s)", playlistQuery.c_str());
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    else
      currentRule = (*it).GetWhereClause(db, strType);
    // if we don't get a rule, we add '1' or '0' so the query is still valid and doesn't fail
    if (currentRule.IsEmpty())
      currentRule = m_type == CombinationAnd ? "'1'" : "'0'";
    rule += currentRule;
    rule += ")";
  }

  return rule;
}

void CSmartPlaylistRuleCombination::GetVirtualFolders(const CStdString& strType, std::vector<CStdString> &virtualFolders) const
{
  for (vector<CSmartPlaylistRuleCombination>::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
    it->GetVirtualFolders(strType, virtualFolders);

  for (CSmartPlaylistRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    if ((it->m_field != FieldVirtualFolder && it->m_field != FieldPlaylist) || it->m_operator != CSmartPlaylistRule::OPERATOR_EQUALS)
      continue;

    CStdString playlistFile = CSmartPlaylistDirectory::GetPlaylistByName(it->m_parameter.at(0), strType);
    if (playlistFile.empty())
      continue;

    if (it->m_field == FieldVirtualFolder)
      virtualFolders.push_back(playlistFile);
    else
    {
      // look for any virtual folders in the expanded playlists
      CSmartPlaylist playlist;
      if (!playlist.Load(playlistFile))
        continue;

      if (CSmartPlaylist::CheckTypeCompatibility(playlist.GetType(), strType))
        playlist.GetVirtualFolders(virtualFolders);
    }
  }
}

bool CSmartPlaylistRuleCombination::Load(const CVariant &obj)
{
  if (!obj.isObject() && !obj.isArray())
    return false;
  
  CVariant child;
  if (obj.isObject())
  {
    if (obj.isMember("and") && obj["and"].isArray())
    {
      m_type = CombinationAnd;
      child = obj["and"];
    }
    else if (obj.isMember("or") && obj["or"].isArray())
    {
      m_type = CombinationOr;
      child = obj["or"];
    }
    else
      return false;
  }
  else
    child = obj;

  for (CVariant::const_iterator_array it = child.begin_array(); it != child.end_array(); it++)
  {
    if (!it->isObject())
      continue;

    if (it->isMember("and") || it->isMember("or"))
    {
      CSmartPlaylistRuleCombination combo;
      if (combo.Load(*it))
        m_combinations.push_back(combo);
    }
    else
    {
      CSmartPlaylistRule rule;
      if (rule.Load(*it))
        m_rules.push_back(rule);
    }
  }

  return true;
}

bool CSmartPlaylistRuleCombination::Save(CVariant &obj) const
{
  if (!obj.isObject() || (m_combinations.empty() && m_rules.empty()))
    return false;

  CVariant comboArray(CVariant::VariantTypeArray);
  if (!m_combinations.empty())
  {
    for (CSmartPlaylistRuleCombinations::const_iterator combo = m_combinations.begin(); combo != m_combinations.end(); combo++)
    {
      CVariant comboObj(CVariant::VariantTypeObject);
      if (combo->Save(comboObj))
        comboArray.push_back(comboObj);
    }

  }
  if (!m_rules.empty())
  {
    for (CSmartPlaylistRules::const_iterator rule = m_rules.begin(); rule != m_rules.end(); rule++)
    {
      CVariant ruleObj(CVariant::VariantTypeObject);
      if (rule->Save(ruleObj))
        comboArray.push_back(ruleObj);
    }
  }

  obj[TranslateCombinationType()] = comboArray;

  return true;
}

std::string CSmartPlaylistRuleCombination::TranslateCombinationType() const
{
  return m_type == CombinationAnd ? "and" : "or";
}

void CSmartPlaylistRuleCombination::AddRule(const CSmartPlaylistRule &rule)
{
  m_rules.push_back(rule);
}

void CSmartPlaylistRuleCombination::AddCombination(const CSmartPlaylistRuleCombination &combination)
{
  m_combinations.push_back(combination);
}

CSmartPlaylist::CSmartPlaylist()
{
  Reset();
}

bool CSmartPlaylist::OpenAndReadName(const CStdString &path)
{
  if (readNameFromPath(path) == NULL)
    return false;

  return !m_playlistName.empty();
}

const TiXmlNode* CSmartPlaylist::readName(const TiXmlNode *root)
{
  if (root == NULL)
    return NULL;

  const TiXmlElement *rootElem = root->ToElement();
  if (rootElem == NULL)
    return NULL;

  if (!root || !StringUtils::EqualsNoCase(root->Value(),"smartplaylist"))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist");
    return NULL;
  }

  // load the playlist type
  const char* type = rootElem->Attribute("type");
  if (type)
    m_playlistType = type;
  // backward compatibility:
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  XMLUtils::GetString(root, "name", m_playlistName);

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromPath(const CStdString &path)
{
  CFileStream file;
  if (!file.Open(path))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s (failed to read file)", path.c_str());
    return NULL;
  }

  m_xmlDoc.Clear();
  file >> m_xmlDoc;

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());
  if (m_playlistName.empty())
  {
    m_playlistName = CUtil::GetTitleFromPath(path);
    if (URIUtils::HasExtension(m_playlistName, ".xsp"))
      URIUtils::RemoveExtension(m_playlistName);
  }

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromXml(const CStdString &xml)
{
  if (xml.empty())
  {
    CLog::Log(LOGERROR, "Error loading empty Smart playlist");
    return NULL;
  }

  m_xmlDoc.Clear();
  if (!m_xmlDoc.Parse(xml))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: %s)", m_xmlDoc.ErrorDesc());
    return NULL;
  }

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());

  return root;
}

bool CSmartPlaylist::load(const TiXmlNode *root)
{
  if (root == NULL)
    return false;

  CStdString encoding;
  XMLUtils::GetEncoding(&m_xmlDoc, encoding);

  return LoadFromXML(root, encoding);
}

bool CSmartPlaylist::Load(const CStdString &path)
{
  return load(readNameFromPath(path));
}

bool CSmartPlaylist::Load(const CVariant &obj)
{
  if (!obj.isObject())
    return false;

  // load the playlist type
  if (obj.isMember("type") && obj["type"].isString())
    m_playlistType = obj["type"].asString();

  // backward compatibility
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  if (obj.isMember("name") && obj["name"].isString())
    m_playlistName = obj["name"].asString();

  if (obj.isMember("rules"))
    m_ruleCombination.Load(obj["rules"]);

  if (obj.isMember("group") && obj["group"].isMember("type") && obj["group"]["type"].isString())
  {
    m_group = obj["group"]["type"].asString();
    if (obj["group"].isMember("mixed") && obj["group"]["mixed"].isBoolean())
      m_groupMixed = obj["group"]["mixed"].asBoolean();
  }

  // now any limits
  if (obj.isMember("limit") && (obj["limit"].isInteger() || obj["limit"].isUnsignedInteger()) && obj["limit"].asUnsignedInteger() > 0)
    m_limit = (unsigned int)obj["limit"].asUnsignedInteger();

  // and order
  if (obj.isMember("order") && obj["order"].isMember("method") && obj["order"]["method"].isString())
  {
    const CVariant &order = obj["order"];
    if (order.isMember("direction") && order["direction"].isString())
      m_orderDirection = StringUtils::EqualsNoCase(order["direction"].asString(), "ascending") ? SortOrderAscending : SortOrderDescending;

    if (order.isMember("ignorefolders") && obj["ignorefolders"].isBoolean())
      m_orderAttributes = obj["ignorefolders"].asBoolean() ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(obj["order"]["method"].asString().c_str());
  }

  return true;
}

bool CSmartPlaylist::LoadFromXml(const CStdString &xml)
{
  return load(readNameFromXml(xml));
}

bool CSmartPlaylist::LoadFromXML(const TiXmlNode *root, const CStdString &encoding)
{
  if (!root)
    return false;

  CStdString tmp;
  if (XMLUtils::GetString(root, "match", tmp))
    m_ruleCombination.SetType(StringUtils::EqualsNoCase(tmp, "all") ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr);

  // now the rules
  const TiXmlNode *ruleNode = root->FirstChild("rule");
  while (ruleNode)
  {
    CSmartPlaylistRule rule;
    if (rule.Load(ruleNode, encoding))
      m_ruleCombination.AddRule(rule);

    ruleNode = ruleNode->NextSibling("rule");
  }

  const TiXmlElement *groupElement = root->FirstChildElement("group");
  if (groupElement != NULL && groupElement->FirstChild() != NULL)
  {
    m_group = groupElement->FirstChild()->ValueStr();
    const char* mixed = groupElement->Attribute("mixed");
    m_groupMixed = mixed != NULL && StringUtils::EqualsNoCase(mixed, "true");
  }

  // now any limits
  // format is <limit>25</limit>
  XMLUtils::GetUInt(root, "limit", m_limit);

  // and order
  // format is <order direction="ascending">field</order>
  const TiXmlElement *order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char *direction = order->Attribute("direction");
    if (direction)
      m_orderDirection = StringUtils::EqualsNoCase(direction, "ascending") ? SortOrderAscending : SortOrderDescending;

    const char *ignorefolders = order->Attribute("ignorefolders");
    if (ignorefolders != NULL)
      m_orderAttributes = StringUtils::EqualsNoCase(ignorefolders, "true") ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::LoadFromJson(const CStdString &json)
{
  if (json.empty())
    return false;

  CVariant obj = CJSONVariantParser::Parse((const unsigned char *)json.c_str(), json.size());
  return Load(obj);
}

bool CSmartPlaylist::Save(const CStdString &path) const
{
  CXBMCTinyXML doc;
  TiXmlDeclaration decl("1.0", "UTF-8", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement xmlRootElement("smartplaylist");
  xmlRootElement.SetAttribute("type",m_playlistType.c_str());
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot)
    return false;

  // add the <name> tag
  XMLUtils::SetString(pRoot, "name", m_playlistName);

  // add the <match> tag
  XMLUtils::SetString(pRoot, "match", m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd ? "all" : "one");

  // add <rule> tags
  for (CSmartPlaylistRules::const_iterator it = m_ruleCombination.m_rules.begin(); it != m_ruleCombination.m_rules.end(); ++it)
    it->Save(pRoot);

  // add <group> tag if necessary
  if (!m_group.empty())
  {
    TiXmlElement nodeGroup("group");
    if (m_groupMixed)
      nodeGroup.SetAttribute("mixed", "true");
    TiXmlText group(m_group.c_str());
    nodeGroup.InsertEndChild(group);
    pRoot->InsertEndChild(nodeGroup);
  }

  // add <limit> tag
  if (m_limit)
    XMLUtils::SetInt(pRoot, "limit", m_limit);

  // add <order> tag
  if (m_orderField != SortByNone)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateOrder(m_orderField).c_str());
    TiXmlElement nodeOrder("order");
    nodeOrder.SetAttribute("direction", m_orderDirection == SortOrderDescending ? "descending" : "ascending");
    if (m_orderAttributes & SortAttributeIgnoreFolders)
      nodeOrder.SetAttribute("ignorefolders", "true");
    nodeOrder.InsertEndChild(order);
    pRoot->InsertEndChild(nodeOrder);
  }
  return doc.SaveFile(path);
}

bool CSmartPlaylist::Save(CVariant &obj, bool full /* = true */) const
{
  if (obj.type() == CVariant::VariantTypeConstNull)
    return false;

  obj.clear();
  // add "type"
  obj["type"] = m_playlistType;

  // add "rules"
  CVariant rulesObj = CVariant(CVariant::VariantTypeObject);
  if (m_ruleCombination.Save(rulesObj))
    obj["rules"] = rulesObj;

  // add "group"
  if (!m_group.empty())
  {
    obj["group"]["type"] = m_group;
    obj["group"]["mixed"] = m_groupMixed;
  }

  // add "limit"
  if (full && m_limit)
    obj["limit"] = m_limit;

  // add "order"
  if (full && m_orderField != SortByNone)
  {
    obj["order"] = CVariant(CVariant::VariantTypeObject);
    obj["order"]["method"] = CSmartPlaylistRule::TranslateOrder(m_orderField);
    obj["order"]["direction"] = m_orderDirection == SortOrderDescending ? "descending" : "ascending";
    obj["order"]["ignorefolders"] = (m_orderAttributes & SortAttributeIgnoreFolders);
  }

  return true;
}

bool CSmartPlaylist::SaveAsJson(CStdString &json, bool full /* = true */) const
{
  CVariant xsp(CVariant::VariantTypeObject);
  if (!Save(xsp, full))
    return false;

  json = CJSONVariantWriter::Write(xsp, true);
  return json.size() > 0;
}

void CSmartPlaylist::Reset()
{
  m_ruleCombination.m_combinations.clear();
  m_ruleCombination.m_rules.clear();
  m_ruleCombination.SetType(CSmartPlaylistRuleCombination::CombinationAnd);
  m_limit = 0;
  m_orderField = SortByNone;
  m_orderDirection = SortOrderNone;
  m_orderAttributes = SortAttributeNone;
  m_playlistType = "songs"; // sane default
  m_group.clear();
  m_groupMixed = false;
}

void CSmartPlaylist::SetName(const CStdString &name)
{
  m_playlistName = name;
}

void CSmartPlaylist::SetType(const CStdString &type)
{
  m_playlistType = type;
}

bool CSmartPlaylist::IsVideoType() const
{
  return IsVideoType(m_playlistType);
}

bool CSmartPlaylist::IsMusicType() const
{
  return IsMusicType(m_playlistType);
}

bool CSmartPlaylist::IsVideoType(const CStdString &type)
{
  return type == "movies" || type == "tvshows" || type == "episodes" ||
         type == "musicvideos" || type == "mixed";
}

bool CSmartPlaylist::IsMusicType(const CStdString &type)
{
  return type == "artists" || type == "albums" ||
         type == "songs" || type == "mixed";
}

CStdString CSmartPlaylist::GetWhereClause(const CDatabase &db, set<CStdString> &referencedPlaylists) const
{
  return m_ruleCombination.GetWhereClause(db, GetType(), referencedPlaylists);
}

void CSmartPlaylist::GetVirtualFolders(std::vector<CStdString> &virtualFolders) const
{
  m_ruleCombination.GetVirtualFolders(GetType(), virtualFolders);
}

CStdString CSmartPlaylist::GetSaveLocation() const
{
  if (m_playlistType == "mixed")
    return "mixed";
  if (IsMusicType())
    return "music";
  // all others are video
  return "video";
}

void CSmartPlaylist::GetAvailableFields(const std::string &type, std::vector<std::string> &fieldList)
{
  vector<Field> typeFields = CSmartPlaylistRule::GetFields(type);
  for (vector<Field>::const_iterator field = typeFields.begin(); field != typeFields.end(); field++)
  {
    for (unsigned int i = 0; i < NUM_FIELDS; i++)
    {
      if (*field == fields[i].field)
        fieldList.push_back(fields[i].string);
    }
  }
}

void CSmartPlaylist::GetAvailableOperators(std::vector<std::string> &operatorList)
{
  for (unsigned int index = 0; index < NUM_OPERATORS; index++)
    operatorList.push_back(operators[index].string);
}

bool CSmartPlaylist::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.m_rules.empty() && m_ruleCombination.m_combinations.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit <= 0 && m_orderField == SortByNone && m_orderDirection == SortOrderNone;

  return empty;
}

bool CSmartPlaylist::CheckTypeCompatibility(const CStdString &typeLeft, const CStdString &typeRight)
{
  if (typeLeft.Equals(typeRight))
    return true;

  if (typeLeft.Equals("mixed") &&
     (typeRight.Equals("songs") || typeRight.Equals("musicvideos")))
    return true;

  if (typeRight.Equals("mixed") &&
     (typeLeft.Equals("songs") || typeLeft.Equals("musicvideos")))
    return true;

  return false;
}
