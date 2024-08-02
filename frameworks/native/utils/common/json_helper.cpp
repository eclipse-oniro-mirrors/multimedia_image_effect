/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "json_helper.h"

#include "cJSON.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {

namespace {
#define EFFECT_JSON_FALSE_RETURN(cond)                                                                  \
    do {                                                                                                \
        if (!(cond)) {                                                                                  \
            EFFECT_LOGE("[%{public}s:%{public}d] Effect json cond false!", __FUNCTION__, __LINE__);     \
            return false;                                                                               \
        }                                                                                               \
    } while (0)

#define EFFECT_JSON_FALSE_RETURN_WITH_WORK(cond, work)                                                  \
    do {                                                                                                \
        if (!(cond)) {                                                                                  \
            EFFECT_LOGE("[%{public}s:%{public}d] Effect json cond false!", __FUNCTION__, __LINE__);     \
            work;                                                                                       \
            return false;                                                                               \
        }                                                                                               \
    } while (0)
}

EffectJson::EffectJson(Json *json, bool isRoot) : json_(json), isRoot_(isRoot) {}

EffectJson::~EffectJson()
{
    if (json_ != nullptr && isRoot_) {
        cJSON_Delete(json_);
    }
    json_ = nullptr;
}

bool EffectJson::IsBool() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsBool(json_));
}

bool EffectJson::IsNumber() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsNumber(json_));
}

bool EffectJson::IsString() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsString(json_));
}

bool EffectJson::IsArray() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsArray(json_));
}

bool EffectJson::IsObject() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsObject(json_));
}

bool EffectJson::IsValid() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return !static_cast<bool>(cJSON_IsInvalid(json_));
}

bool EffectJson::IsNull() const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_IsNull(json_));
}

bool EffectJson::HasElement(const std::string &key) const
{
    EFFECT_JSON_FALSE_RETURN(json_ != nullptr);
    return static_cast<bool>(cJSON_HasObjectItem(json_, key.c_str()));
}

EffectJsonPtr EffectJson::GetElement(const std::string &key)
{
    if (!HasElement(key)) {
        return nullptr;
    }
    cJSON *element = cJSON_GetObjectItemCaseSensitive(json_, key.c_str());
    return std::make_shared<EffectJson>(element, false);
}

int32_t EffectJson::GetInt()
{
    if (!IsNumber()) {
        return 0;
    }
    return static_cast<int32_t>(cJSON_GetNumberValue(json_));
}

int32_t EffectJson::GetInt(const std::string &key, int32_t defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsNumber()) {
        return defaultValue;
    }

    return element->GetInt();
}

uint32_t EffectJson::GetUInt()
{
    if (!IsNumber()) {
        return 0;
    }
    return static_cast<int32_t>(cJSON_GetNumberValue(json_));
}

uint32_t EffectJson::GetUInt(const std::string &key, uint32_t defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsNumber()) {
        return defaultValue;
    }

    return element->GetUInt();
}

float EffectJson::GetFloat()
{
    if (!IsNumber()) {
        return 0.0f;
    }
    return static_cast<float>(cJSON_GetNumberValue(json_));
}

float EffectJson::GetFloat(const std::string &key, float defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsNumber()) {
        return defaultValue;
    }

    return element->GetFloat();
}

double EffectJson::GetDouble()
{
    if (!IsNumber()) {
        return 0;
    }
    return cJSON_GetNumberValue(json_);
}

double EffectJson::GetDouble(const std::string &key, double defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsNumber()) {
        return defaultValue;
    }

    return element->GetDouble();
}

bool EffectJson::GetBool()
{
    if (!IsBool()) {
        return false;
    }
    return static_cast<bool>(cJSON_IsTrue(json_));
}

bool EffectJson::GetBool(const std::string &key, bool defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsBool()) {
        return defaultValue;
    }

    return element->GetBool();
}

std::string EffectJson::GetString()
{
    if (!IsString()) {
        return "";
    }
    return cJSON_GetStringValue(json_);
}

std::string EffectJson::GetString(const std::string &key, const std::string &defaultValue)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsString()) {
        return defaultValue;
    }

    return element->GetString();
}

std::vector<EffectJsonPtr> EffectJson::GetArray()
{
    std::vector<EffectJsonPtr> elements;
    if (!IsArray()) {
        return elements;
    }

    cJSON *element = nullptr;
    cJSON_ArrayForEach(element, json_) {
        elements.push_back(std::make_shared<EffectJson>(element, false));
    }
    return elements;
}

std::vector<EffectJsonPtr> EffectJson::GetArray(const std::string &key)
{
    auto element = GetElement(key);
    if (element == nullptr || element->IsNull() || !element->IsArray()) {
        return {};
    }

    return element->GetArray();
}

void DeleteJson(cJSON *json, bool isAllowDelete = true)
{
    if (isAllowDelete && json != nullptr) {
        cJSON_Delete(json);
    }
}

bool EffectJson::Put(const std::string &key, Json *json, bool isAllowDelete)
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json_ != nullptr);

    bool res = static_cast<bool>(cJSON_AddItemToObject(json_, key.c_str(), json));
    EFFECT_JSON_FALSE_RETURN_WITH_WORK(res, DeleteJson(json, isAllowDelete));
    return true;
}

bool EffectJson::Put(const std::string &key, EffectJsonPtr &json)
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json->json_ != nullptr);

    cJSON *jsonObject = json->json_;
    bool isAllowDelete = false;
    if (json->isRoot_) {
        jsonObject = cJSON_Duplicate(json->json_, true);
        isAllowDelete = true;
    }
    return Put(key, jsonObject, isAllowDelete);
}

bool EffectJson::Put(const std::string &key, int32_t value)
{
    return Put(key, static_cast<double>(value));
}

bool EffectJson::Put(const std::string &key, uint32_t value)
{
    return Put(key, static_cast<double>(value));
}

bool EffectJson::Put(const std::string &key, float value)
{
    return Put(key, static_cast<double>(value));
}

bool EffectJson::Put(const std::string &key, double value)
{
    cJSON *child = cJSON_CreateNumber(value);
    return Put(key, child);
}

bool EffectJson::Put(const std::string &key, bool value)
{
    cJSON *child = cJSON_CreateBool(value);
    return Put(key, child);
}

bool EffectJson::Put(const std::string &key, const std::string &value)
{
    cJSON *child = cJSON_CreateString(value.c_str());
    return Put(key, child);
}

bool EffectJson::Put(const std::string &key, const char *value)
{
    EFFECT_JSON_FALSE_RETURN(value != nullptr);
    cJSON *child = cJSON_CreateString(value);
    return Put(key, child);
}

bool EffectJson::Add(Json *json, bool isAllowDelete) const
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json_ != nullptr);

    bool res = static_cast<bool>(cJSON_AddItemToArray(json_, json));
    EFFECT_JSON_FALSE_RETURN_WITH_WORK(res, DeleteJson(json, isAllowDelete));
    return true;
}

bool EffectJson::Add(EffectJsonPtr &json) const
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json->json_ != nullptr);

    cJSON *jsonObject = json->json_;
    bool isAllowDelete = false;
    if (json->isRoot_) {
        jsonObject = cJSON_Duplicate(json->json_, true);
        isAllowDelete = true;
    }
    return Add(jsonObject, isAllowDelete);
}

bool EffectJson::Add(int32_t value) const
{
    return Add(static_cast<double>(value));
}

bool EffectJson::Add(uint32_t value) const
{
    return Add(static_cast<double>(value));
}

bool EffectJson::Add(float value) const
{
    return Add(static_cast<double>(value));
}

bool EffectJson::Add(double value) const
{
    EFFECT_JSON_FALSE_RETURN(IsArray());

    cJSON *child = cJSON_CreateNumber(value);
    return Add(child);
}

bool EffectJson::Add(bool value) const
{
    EFFECT_JSON_FALSE_RETURN(IsArray());

    cJSON *child = cJSON_CreateBool(value);
    return Add(child);
}

bool EffectJson::Add(const std::string &value) const
{
    EFFECT_JSON_FALSE_RETURN(IsArray());

    cJSON *child = cJSON_CreateString(value.c_str());
    return Add(child);
}

bool EffectJson::Add(const char *value) const
{
    EFFECT_JSON_FALSE_RETURN(IsArray());
    EFFECT_JSON_FALSE_RETURN(value != nullptr);

    cJSON *child = cJSON_CreateString(value);
    return Add(child);
}

bool EffectJson::Replace(const std::string &key, Json *json, bool jsonObject)
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json_ != nullptr);

    bool res =
        static_cast<bool>(cJSON_ReplaceItemInObjectCaseSensitive(json_, key.c_str(), json));
    EFFECT_JSON_FALSE_RETURN_WITH_WORK(res, DeleteJson(json, jsonObject));
    return true;
}

bool EffectJson::Replace(const std::string &key, EffectJsonPtr &json)
{
    EFFECT_JSON_FALSE_RETURN(json != nullptr && json->json_ != nullptr);

    cJSON *jsonObject = json->json_;
    bool isAllowDelete = false;
    if (json->isRoot_) {
        jsonObject = cJSON_Duplicate(json->json_, true);
        isAllowDelete = true;
    }
    return Replace(key, jsonObject, isAllowDelete);
}

bool EffectJson::Replace(const std::string &key, int32_t value)
{
    return Replace(key, static_cast<double>(value));
}

bool EffectJson::Replace(const std::string &key, uint32_t value)
{
    return Replace(key, static_cast<double>(value));
}

bool EffectJson::Replace(const std::string &key, float value)
{
    return Replace(key, static_cast<double>(value));
}

bool EffectJson::Replace(const std::string &key, double value)
{
    cJSON *child = cJSON_CreateNumber(value);
    return Replace(key, child);
}

bool EffectJson::Replace(const std::string &key, bool value)
{
    cJSON *child = cJSON_CreateBool(value);
    return Replace(key, child);
}

bool EffectJson::Replace(const std::string &key, const std::string &value)
{
    cJSON *child = cJSON_CreateString(value.c_str());
    return Replace(key, child);
}

bool EffectJson::Replace(const std::string &key, const char *value)
{
    EFFECT_JSON_FALSE_RETURN(value != nullptr);
    cJSON *child = cJSON_CreateString(value);
    return Replace(key, child);
}

std::string EffectJson::ToString() const
{
    std::string ret;
    if (json_ == nullptr) {
        return ret;
    }
    char *jsonData = cJSON_PrintUnformatted(json_);
    if (jsonData != nullptr) {
        ret = jsonData;
        cJSON_free(jsonData);
    }
    return ret;
}

EffectJsonPtr JsonHelper::ParseJsonData(const std::string &data)
{
    cJSON *json = cJSON_Parse(data.c_str());
    return std::make_shared<EffectJson>(json);
}

EffectJsonPtr JsonHelper::CreateObject(bool isRoot)
{
    cJSON *json = cJSON_CreateObject();
    return std::make_shared<EffectJson>(json, isRoot);
}

EffectJsonPtr JsonHelper::CreateArray(bool isRoot)
{
    cJSON *json = cJSON_CreateArray();
    return std::make_shared<EffectJson>(json, isRoot);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS