/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "efilter_base.h"

#include "effect_log.h"
#include "filter_factory.h"

namespace OHOS {
namespace Media {
namespace Effect {

std::vector<WorkMode> EFilterBase::GetWorkModes()
{
    return FilterBase::GetWorkModes();
}

ErrorCode EFilterBase::PullData(const std::string &outPort, std::shared_ptr<EffectBuffer> &data)
{
    EFFECT_LOGI("PullData, set sinkBuffer");
    sinkBuffer_ = data;
    return ErrorCode::SUCCESS;
}
}
}
}