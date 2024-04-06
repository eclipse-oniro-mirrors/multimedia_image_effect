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

#include "string_helper.h"

#include <algorithm>

namespace OHOS {
namespace Media {
namespace Effect {
bool StringHelp::EndsWith(const std::string &input, const std::string &suffix)
{
    if (input.length() < suffix.length()) {
        return false;
    }
    return input.compare(input.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool StringHelp::EndsWithIgnoreCase(const std::string &input, const std::string &suffix)
{
    if (input.length() < suffix.length()) {
        return false;
    }

    std::string inputEnd = input.substr(input.length() - suffix.length(), suffix.length());
    std::transform(inputEnd.begin(), inputEnd.end(), inputEnd.begin(), ::tolower);

    std::string suffixLower = suffix;
    std::transform(suffixLower.begin(), suffixLower.end(), suffixLower.begin(), ::tolower);

    return inputEnd.compare(suffixLower) == 0;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS