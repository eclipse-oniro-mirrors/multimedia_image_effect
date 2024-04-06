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

#ifndef IMAGE_EFFECT_JSON_HELPER_H
#define IMAGE_EFFECT_JSON_HELPER_H

#include <string>

#include "effect_log.h"
#include "error_code.h"
#include "nlohmann/json.hpp"

namespace OHOS {
namespace Media {
namespace Effect {
class JsonHelper final {
public:
    static ErrorCode CheckElementExitstence(const nlohmann::json &jsonObject, const std::string &key);
    static ErrorCode GetStringValue(const nlohmann::json &jsonObject, const std::string &key, std::string &value);
    static ErrorCode GetFloatValue(const nlohmann::json &jsonObject, const std::string &key, float &value);
    static ErrorCode GetInt32Value(const nlohmann::json &jsonObject, const std::string &key, int32_t &value);
    static ErrorCode GetArray(const nlohmann::json &jsonObject, const std::string &key, nlohmann::json &value);

private:
    static const nlohmann::json &GetJsonElement(const nlohmann::json &jsonObject, const std::string &key,
        ErrorCode &errorCode);
    static nlohmann::json nullJson_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_JSON_HELPER_H