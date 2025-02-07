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

#ifndef IMAGE_EFFECT_EFFECT_JSON_HELPER_H
#define IMAGE_EFFECT_EFFECT_JSON_HELPER_H

#include <memory>
#include <string>
#include <vector>

#include "image_effect_marco_define.h"

struct cJSON;

namespace OHOS {
namespace Media {
namespace Effect {

class EffectJson;
using EffectJsonPtr = std::shared_ptr<EffectJson>;
using Json = cJSON;

class EffectJson {
public:
    IMAGE_EFFECT_EXPORT explicit EffectJson(Json *json, bool isRoot = true);
    IMAGE_EFFECT_EXPORT ~EffectJson();

    IMAGE_EFFECT_EXPORT bool IsBool() const;
    IMAGE_EFFECT_EXPORT bool IsNumber() const;
    IMAGE_EFFECT_EXPORT bool IsString() const;
    IMAGE_EFFECT_EXPORT bool IsArray() const;
    IMAGE_EFFECT_EXPORT bool IsObject() const;
    IMAGE_EFFECT_EXPORT bool IsValid() const;
    IMAGE_EFFECT_EXPORT bool IsNull() const;
    IMAGE_EFFECT_EXPORT bool HasElement(const std::string &key) const;

    IMAGE_EFFECT_EXPORT EffectJsonPtr GetElement(const std::string &key);
    IMAGE_EFFECT_EXPORT int32_t GetInt(const std::string &key, int32_t defaultValue = 0);
    IMAGE_EFFECT_EXPORT uint32_t GetUInt(const std::string &key, uint32_t defaultValue = 0);
    IMAGE_EFFECT_EXPORT float GetFloat(const std::string &key, float defaultValue = 0.0f);
    IMAGE_EFFECT_EXPORT double GetDouble(const std::string &key, double defaultValue = 0.0);
    IMAGE_EFFECT_EXPORT bool GetBool(const std::string &key, bool defaultValue = false);
    IMAGE_EFFECT_EXPORT std::string GetString(const std::string &key, const std::string &defaultValue = "");
    IMAGE_EFFECT_EXPORT std::vector<EffectJsonPtr> GetArray(const std::string &key);

    IMAGE_EFFECT_EXPORT int32_t GetInt();
    IMAGE_EFFECT_EXPORT uint32_t GetUInt();
    IMAGE_EFFECT_EXPORT float GetFloat();
    IMAGE_EFFECT_EXPORT double GetDouble();
    IMAGE_EFFECT_EXPORT bool GetBool();
    IMAGE_EFFECT_EXPORT std::string GetString();
    IMAGE_EFFECT_EXPORT std::vector<EffectJsonPtr> GetArray();

    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, EffectJsonPtr &json);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, int32_t value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, uint32_t value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, float value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, double value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, bool value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, const std::string &value);
    IMAGE_EFFECT_EXPORT bool Put(const std::string &key, const char *value);

    // add for array
    IMAGE_EFFECT_EXPORT bool Add(EffectJsonPtr &json) const;
    IMAGE_EFFECT_EXPORT bool Add(int32_t value) const;
    IMAGE_EFFECT_EXPORT bool Add(uint32_t value) const;
    IMAGE_EFFECT_EXPORT bool Add(float value) const;
    IMAGE_EFFECT_EXPORT bool Add(double value) const;
    IMAGE_EFFECT_EXPORT bool Add(bool value) const;
    IMAGE_EFFECT_EXPORT bool Add(const std::string &value) const;
    IMAGE_EFFECT_EXPORT bool Add(const char *value) const;

    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, EffectJsonPtr &json);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, int32_t value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, uint32_t value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, float value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, double value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, bool value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, const std::string &value);
    IMAGE_EFFECT_EXPORT bool Replace(const std::string &key, const char *value);

    IMAGE_EFFECT_EXPORT std::string ToString() const;
private:
    bool Put(const std::string &key, Json *json, bool isAllowDelete = true);
    bool Add(Json *json, bool isAllowDelete = true) const;
    bool Replace(const std::string &key, Json *json, bool isAllowDelete = true);
    Json *json_ = nullptr;
    bool isRoot_ = false;
};

class EffectJsonHelper final {
public:
    IMAGE_EFFECT_EXPORT static EffectJsonPtr ParseJsonData(const std::string &data);
    IMAGE_EFFECT_EXPORT static EffectJsonPtr CreateObject(bool isRoot = true);
    IMAGE_EFFECT_EXPORT static EffectJsonPtr CreateArray(bool isRoot = true);
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_EFFECT_JSON_HELPER_H