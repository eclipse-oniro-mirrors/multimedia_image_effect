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

#include "capability_negotiate.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
static const std::vector<IEffectFormat> FORMAT_PRIORITY_TABLE = {
    IEffectFormat::YUVNV12,
    IEffectFormat::YUVNV21,
    IEffectFormat::RGBA8888,
    IEffectFormat::RGBA_1010102,
    IEffectFormat::YCBCR_P010,
    IEffectFormat::YCRCB_P010
};
std::vector<std::shared_ptr<Capability>> &CapabilityNegotiate::GetCapabilityList()
{
    return caps_;
}

void CapabilityNegotiate::AddCapability(std::shared_ptr<Capability> &capability)
{
    caps_.emplace_back(capability);
}

void CapabilityNegotiate::ClearNegotiateResult()
{
    caps_.clear();
}

IEffectFormat CalculateHighPriorityFormat(const std::vector<IEffectFormat> &srcFormats,
    const std::vector<IEffectFormat> &dstFormats)
{
    for (const auto &format : srcFormats) {
        if (std::find(dstFormats.begin(), dstFormats.end(), format) != dstFormats.end()) {
            return format;
        }
    }
    return dstFormats[0];
}

std::vector<IEffectFormat> CalculateNegotiateFormats(const std::vector<std::vector<IEffectFormat>> &negotiateFormats)
{
    if (negotiateFormats.empty()) {
        EFFECT_LOGE("calculate negotiate, format is null.");
        return {};
    }

    std::vector<IEffectFormat> intersectFormats = negotiateFormats[0];
    if (negotiateFormats.size() == 1) {
        return intersectFormats;
    }

    for (size_t i = 1; i < negotiateFormats.size(); ++i) {
        std::vector<IEffectFormat> tempIntersection;
        std::set_intersection(intersectFormats.begin(), intersectFormats.end(),
                              negotiateFormats[i].begin(), negotiateFormats[i].end(),
                              std::back_inserter(tempIntersection));
        intersectFormats = tempIntersection;
    }

    return intersectFormats;
}

IEffectFormat CapabilityNegotiate::NegotiateFormat(std::vector<std::shared_ptr<Capability>> capabilities)
{
    std::vector<std::vector<IEffectFormat>> allNegotiateFormats;
    for (const auto &cap : capabilities) {
        if (cap->pixelFormatCap_) {
            std::vector<IEffectFormat> formats;
            for (const auto &[effectFormat, ipTypes] : cap->pixelFormatCap_->formats) {
                formats.push_back(effectFormat);
            }
            allNegotiateFormats.push_back(formats);
        }
    }
    std::vector<IEffectFormat> intersectionFormats = CalculateNegotiateFormats(allNegotiateFormats);
    CHECK_AND_RETURN_RET_LOG(!allNegotiateFormats.empty(), IEffectFormat::DEFAULT,
        "get calculate negotiate formats is null");
    CHECK_AND_RETURN_RET_LOG(!intersectionFormats.empty(), allNegotiateFormats[0][0],
        "get intersection formats is null");
    IEffectFormat format = CalculateHighPriorityFormat(FORMAT_PRIORITY_TABLE, intersectionFormats);
    return format;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS