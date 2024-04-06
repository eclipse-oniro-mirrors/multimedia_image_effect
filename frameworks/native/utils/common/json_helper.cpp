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

#include "effect_log.h"
#include "error_code.h"

namespace OHOS {
namespace Media {
namespace Effect {
using nlohmann::json;
using std::string;

json JsonHelper::nullJson_;

ErrorCode JsonHelper::CheckElementExitstence(const json &jsonObject, const string &key)
{
    ErrorCode errorCode;
    GetJsonElement(jsonObject, key, errorCode);
    return errorCode;
}

ErrorCode JsonHelper::GetStringValue(const nlohmann::json &jsonObject, const std::string &key, std::string &value)
{
    ErrorCode result;
    const json &jsonString = GetJsonElement(jsonObject, key, result);
    if (result != ErrorCode::SUCCESS) {
        return result;
    }

    if (!jsonString.is_string()) {
        EFFECT_LOGE("not a string value type for key = %{public}s", key.c_str());
        return ErrorCode::ERR_JSON_DATA_TYPE;
    }

    value = jsonString;
    return ErrorCode::SUCCESS;
}

ErrorCode JsonHelper::GetFloatValue(const json &jsonObject, const string &key, float &value)
{
    ErrorCode result;
    const json &jsonFloat = GetJsonElement(jsonObject, key, result);
    if (result != ErrorCode::SUCCESS) {
        return result;
    }

    if (!jsonFloat.is_number_float()) {
        EFFECT_LOGE("not a float value type for key = %{public}s", key.c_str());
        return ErrorCode::ERR_JSON_DATA_TYPE;
    }

    value = jsonFloat;
    return ErrorCode::SUCCESS;
}

ErrorCode JsonHelper::GetInt32Value(const json &jsonObject, const string &key, int32_t &value)
{
    ErrorCode result;
    const json &jsonInt = GetJsonElement(jsonObject, key, result);
    if (result != ErrorCode::SUCCESS) {
        return result;
    }

    if (!jsonInt.is_number_integer()) {
        EFFECT_LOGE("not a int value type for key = %{public}s", key.c_str());
        return ErrorCode::ERR_JSON_DATA_TYPE;
    }

    value = jsonInt;
    return ErrorCode::SUCCESS;
}

ErrorCode JsonHelper::GetArray(const json &jsonObject, const string &key, json &value)
{
    ErrorCode result;
    const json &jsonArray = GetJsonElement(jsonObject, key, result);
    if (result != ErrorCode::SUCCESS) {
        return result;
    }

    if (!jsonArray.is_array()) {
        EFFECT_LOGE("not a array value type for key = %{public}s", key.c_str());
        return ErrorCode::ERR_JSON_DATA_TYPE;
    }

    value = jsonArray;
    return ErrorCode::SUCCESS;
}

const json &JsonHelper::GetJsonElement(const json &jsonObject, const string &key, ErrorCode &errorCode)
{
    if (!jsonObject.is_object()) {
        EFFECT_LOGE("not a json object for key = %{public}s", key.c_str());
        errorCode = ErrorCode::ERR_JSON_DATA_TYPE;
        return nullJson_;
    }
    auto iter = jsonObject.find(key);
    if (iter == jsonObject.end()) {
        EFFECT_LOGD("failed to find key = %{public}s", key.c_str());
        errorCode = ErrorCode::ERR_JSON_NO_TARGET;
        return nullJson_;
    }

    errorCode = ErrorCode::SUCCESS;
    return *iter;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS