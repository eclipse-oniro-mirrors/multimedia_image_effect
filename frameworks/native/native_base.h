/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef IMAGE_EFFECT_NATIVE_BASE_H
#define IMAGE_EFFECT_NATIVE_BASE_H

#include <memory>
#include <utility>
#include <vector>

#include "efilter.h"
#include "image_effect.h"
#include "native_efilter.h"

struct OH_EFilter {
    std::shared_ptr<OHOS::Media::Effect::EFilter> filter_ = nullptr;
    void SetParameter(const std::string &key, OHOS::Media::Plugin::Any &param);
    OHOS::Media::Effect::ErrorCode GetParameter(const std::string &key, OHOS::Media::Plugin::Any &param);
    void RemoveParameter(const std::string &key);
private:
    std::unordered_map<std::string, OHOS::Media::Plugin::Any&> params_;
};

struct OH_ImageEffect {
    OH_ImageEffect() = default;
    std::shared_ptr<OHOS::Media::Effect::ImageEffect> imageEffect_ = nullptr;
    char *saveJson = nullptr;
    std::vector<std::pair<OH_EFilter *, std::string>> filters_;
    char *surfaceIdTemp = nullptr;
    ~OH_ImageEffect()
    {
        if (saveJson != nullptr) {
            free(saveJson);
            saveJson = nullptr;
        }
        if (surfaceIdTemp != nullptr) {
            free(surfaceIdTemp);
            surfaceIdTemp = nullptr;
        }
        for (const auto &filter : filters_) {
            OH_EFilter_Release(filter.first);
        }
        filters_.clear();
    }
};

#endif // IMAGE_EFFECT_NATIVE_BASE_H