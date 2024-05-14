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

#include "graphic/render_general_program.h"
#include "graphic/gl_utils.h"

#include "effect_trace.h"

namespace OHOS {
namespace Media {
namespace Effect {
RenderGeneralProgram::RenderGeneralProgram(RenderContext *context, const std::string &vss, const std::string &fss)
    : RenderProgram(context), vss_(vss), fss_(fss)
{
    SetTag("RenderGeneralProgram");
}

bool RenderGeneralProgram::Init()
{
    EFFECT_TRACE_NAME("Init RenderGeneralProgram");
    program_ = GLUtils::CreateProgram(vss_, fss_);
    SetReady(true);
    return program_ <= 0 ? false : true;
}

bool RenderGeneralProgram::Release()
{
    if (IsReady()) {
        glDeleteProgram(program_);
        program_ = 0;
        SetReady(false);
    }
    return true;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS