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

#include "render_strategy.h"

#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
void RenderStrategy::Init(EffectBuffer *src, EffectBuffer *dst)
{
    src_ = src;
    dst_ = dst;
}

EffectBuffer *ChooseBufOnSetInput(EffectBuffer *buffer, EffectBuffer *src,
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap)
{
    return buffer;
}

EffectBuffer *ChooseBufOnSetInOutput(EffectBuffer *buffer, EffectBuffer *src, EffectBuffer *dst,
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap)
{
    if (buffer->buffer_ == dst->buffer_) {
        return buffer;
    }

    std::shared_ptr<BufferInfo> &bufferInfo = buffer->bufferInfo_;
    std::shared_ptr<BufferInfo> &dstBufferInfo = dst->bufferInfo_;
    CHECK_AND_RETURN_RET_LOG(bufferInfo != nullptr && dstBufferInfo != nullptr, buffer,
        "dst or input buffer info is null! src_=%{public}p, buffer=%{public}p", bufferInfo.get(), dstBufferInfo.get());
    CHECK_AND_RETURN_RET_LOG(memNegotiatedCap != nullptr, buffer, "memNegotiatedCap is null!");
    EFFECT_LOGD("ChooseBufOnSetInOutput: w=%{public}d, h=%{public}d, dstW=%{public}d, dstH=%{public}d, "
        "negoW=%{public}d, negoH=%{public}d", bufferInfo->width_, bufferInfo->height_,
        dstBufferInfo->width_, dstBufferInfo->height_, memNegotiatedCap->width, memNegotiatedCap->height);

    if (memNegotiatedCap->width == dstBufferInfo->width_ && memNegotiatedCap->height == dstBufferInfo->height_) {
        return dst;
    }

    // not allow to modify src while set input and output
    if (src->buffer_ == buffer->buffer_) {
        return nullptr;
    }

    return buffer;
}

EffectBuffer *RenderStrategy::ChooseBestOutput(EffectBuffer *buffer,
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap)
{
    CHECK_AND_RETURN_RET_LOG(src_ != nullptr && buffer != nullptr, buffer,
        "src or input buffer is null! src_=%{public}p, buffer=%{public}p", src_, buffer);

    if (dst_ == nullptr || dst_->buffer_ == nullptr || src_->buffer_ == dst_->buffer_) {
        return ChooseBufOnSetInput(buffer, src_, memNegotiatedCap);
    }
    return ChooseBufOnSetInOutput(buffer, src_, dst_, memNegotiatedCap);
}

EffectBuffer *RenderStrategy::GetInput()
{
    return src_;
}

EffectBuffer *RenderStrategy::GetOutput()
{
    return dst_;
}

void RenderStrategy::Deinit()
{
    src_ = nullptr;
    dst_ = nullptr;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS