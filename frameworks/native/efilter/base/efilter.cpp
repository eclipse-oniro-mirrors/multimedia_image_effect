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

#include "efilter.h"

#include "common_utils.h"
#include "effect_log.h"
#include "json_helper.h"
#include "efilter_factory.h"
#include "memcpy_helper.h"
#include "format_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {
const std::string EFilter::Parameter::KEY_DEFAULT_VALUE = "default_value";
ErrorCode EFilter::SetValue(const std::string &key, Plugin::Any &value)
{
    auto it = values_.find(key);
    if (it == values_.end()) {
        values_.emplace(key, value);
    } else {
        values_[key] = value;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::GetValue(const std::string &key, Plugin::Any &value)
{
    if (key.empty()) {
        return ErrorCode::ERR_INPUT_NULL;
    }
    auto it = values_.find(key);
    if (it == values_.end()) {
        EFFECT_LOGE("value is not set! key=%{public}s", key.c_str());
        return ErrorCode::ERR_NO_VALUE;
    }

    value = it->second;
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::Save(nlohmann::json &res)
{
    res["name"] = name_;
    nlohmann::json jsonValues;
    for (auto value : values_) {
        nlohmann::json jsonValue;
        if (CommonUtils::ParseAnyToJson(value.second, jsonValue) == ErrorCode::SUCCESS) {
            jsonValues[value.first] = jsonValue;
        } else {
            EFFECT_LOGE("not support switch to json! key:%{public}s", value.first.c_str());
        }
    }
    if (!jsonValues.empty()) {
        res["values"] = jsonValues;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::PreRender(IEffectFormat &format)
{
    return ErrorCode::SUCCESS;
}

std::shared_ptr<PixelFormatCap> GetPixelFormatCap(std::string &name)
{
    std::shared_ptr<PixelFormatCap> pixelFormatCap = std::make_shared<PixelFormatCap>();
    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    if (effectInfo == nullptr) {
        EFFECT_LOGE("GetEffectInfo fail! name=%{public}s", name.c_str());
        return pixelFormatCap;
    }

    pixelFormatCap->formats = effectInfo->formats_;
    return pixelFormatCap;
}

void EFilter::Negotiate(const std::string &inPort, const std::shared_ptr<Capability> &capability,
    std::shared_ptr<EffectContext> &context)
{
    std::shared_ptr<Capability> outputCap = std::make_shared<Capability>(name_);
    outputCap->pixelFormatCap_ = GetPixelFormatCap(name_);
    outputCap->memNegotiatedCap_ = Negotiate(capability->memNegotiatedCap_);
    context->capNegotiate_->AddCapability(outputCap);
    outputCap_ = outputCap;
    outPorts_[0]->Negotiate(outputCap, context);
}

ErrorCode EFilter::PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap = outputCap_->memNegotiatedCap_;
    EffectBuffer *output = context->renderStrategy_->ChooseBestOutput(buffer.get(), memNegotiatedCap);
    IPType runningIPType = context->ipType_;
    if (runningIPType != IPType::GPU && buffer.get() == output) {
        EFFECT_LOGD("Render with input. filterName=%{public}s", name_.c_str());
        ErrorCode res = Render(buffer.get(), context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
            "Render input fail! filterName=%{public}s", name_.c_str());
        return ErrorCode::SUCCESS;
    }

    EFFECT_LOGD("Render with input and output. filterName=%{public}s", name_.c_str());
    std::shared_ptr<EffectBuffer> effectBuffer = nullptr;
    if (output == nullptr || buffer.get() == output ||
        (runningIPType == IPType::GPU && output->extraInfo_->bufferType != BufferType::DMA_BUFFER)) {
        MemoryInfo memInfo = {
            .bufferInfo = {
                .width_ = memNegotiatedCap->width,
                .height_ = memNegotiatedCap->height,
                .len_ = FormatHelper::CalculateSize(
                    memNegotiatedCap->width, memNegotiatedCap->height, memNegotiatedCap->format),
                .formatType_ = memNegotiatedCap->format,
            }
        };
        if (runningIPType == IPType::GPU) {
            memInfo.bufferType = BufferType::DMA_BUFFER;
        }
        MemoryData *memoryData = context->memoryManager_->AllocMemory(buffer->buffer_, memInfo);
        CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc new memory fail!");
        MemoryInfo &allocMemInfo = memoryData->memoryInfo;
        std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
        *bufferInfo = allocMemInfo.bufferInfo;
        std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
        *extraInfo = *buffer->extraInfo_;
        extraInfo->bufferType = allocMemInfo.bufferType;
        extraInfo->surfaceBuffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
            static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;
        effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data, extraInfo);
    }
    if (effectBuffer != nullptr) {
        output = effectBuffer.get();
    }
    ErrorCode res = Render(buffer.get(), output, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render inout fail! filterName=%{public}s", name_.c_str());
    return PushData(output, context);
}

ErrorCode OnPushDataPortsEmpty(std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context,
    std::string &name)
{
    EffectBuffer *input = context->renderStrategy_->GetInput();
    if (input == nullptr) {
        EFFECT_LOGE("input effect buffer is null! filterName=%{public}s", name.c_str());
        return ErrorCode::ERR_SRC_EFFECT_BUFFER_NULL;
    }

    // efilter modify input buffer directly
    if (input->buffer_ == buffer->buffer_) {
        return ErrorCode::SUCCESS;
    }

    // efilter create new buffer and inout with the same buffer.
    EffectBuffer *output = context->renderStrategy_->GetOutput();
    if (output == nullptr || input->buffer_ == output->buffer_) {
        return CommonUtils::ModifyPixelMapProperty(buffer->extraInfo_->pixelMap, buffer, context->memoryManager_);
    }
    EFFECT_LOGW("not support different input and output buffer! filterName=%{public}s", name.c_str());
    return ErrorCode::ERR_UNSUPPORTED_INOUT_WITH_DIFF_BUFFER;
}

ErrorCode EFilter::PushData(EffectBuffer *buffer, std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, ErrorCode::ERR_INPUT_NULL,
        "PushData: input effect buffer is null! filterName=%{public}s", name_.c_str());

    std::shared_ptr<EffectBuffer> effectBuffer =
        std::make_shared<EffectBuffer>(buffer->bufferInfo_, buffer->buffer_, buffer->extraInfo_);
    if (outPorts_.empty()) {
        return OnPushDataPortsEmpty(effectBuffer, context, name_);
    }

    outPorts_[0]->PushData(effectBuffer, context);
    return ErrorCode::SUCCESS;
}

std::shared_ptr<MemNegotiatedCap> EFilter::Negotiate(const std::shared_ptr<MemNegotiatedCap> &input)
{
    std::shared_ptr<MemNegotiatedCap> output = input;
    return output;
}

ErrorCode EFilter::CalculateEFilterIPType(IEffectFormat &formatType, IPType &ipType)
{
    std::shared_ptr<PixelFormatCap> pixelFormatCap = GetPixelFormatCap(name_);
    std::map<IEffectFormat, std::vector<IPType>> &formats = pixelFormatCap->formats;
    auto it = formats.find(formatType);
    CHECK_AND_RETURN_RET_LOG(it != formats.end(), ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE,
        "format not support! format=%{public}d, name=%{public}s", formatType, name_.c_str());

    if (std::find(it->second.begin(), it->second.end(), IPType::GPU) == it->second.end()) {
        ipType = IPType::CPU;
    } else {
        ipType = IPType::GPU;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CreateDmaEffectBufferIfNeed(IPType runningType, EffectBuffer *current, EffectBuffer *src,
    std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &effectBuffer)
{
    if (runningType == IPType::CPU) {
        return ErrorCode::SUCCESS;
    }
    if (runningType == IPType::GPU && current->extraInfo_->bufferType == BufferType::DMA_BUFFER) {
        if (current == src || current->buffer_ != src->buffer_) {
            return ErrorCode::SUCCESS;
        }
    }

    MemoryInfo memInfo = {
        .bufferInfo = *current->bufferInfo_,
        .bufferType = BufferType::DMA_BUFFER,
    };
    MemoryData *memData = context->memoryManager_->AllocMemory(src->buffer_, memInfo);
    CHECK_AND_RETURN_RET_LOG(memData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc memory fail!");

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = memData->memoryInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *src->extraInfo_;
    extraInfo->bufferType = memData->memoryInfo.bufferType;
    extraInfo->surfaceBuffer = (memData->memoryInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(memData->memoryInfo.extra) : nullptr;
    effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memData->data, extraInfo);
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::Render(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst)
{
    EffectBuffer *srcBuf = src.get();
    EffectBuffer *dstBuf = dst.get();
    CHECK_AND_RETURN_RET_LOG(srcBuf != nullptr && dstBuf != nullptr, ErrorCode::ERR_INPUT_NULL,
        "src or dst is null! src=%{public}p, dst=%{public}p", srcBuf, dstBuf);

    outPorts_.clear();
    IPType runningType = IPType::DEFAULT;
    ErrorCode res = CalculateEFilterIPType(src->bufferInfo_->formatType_, runningType);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "Render CalculateEFilterIPType fail! name=%{public}s", name_.c_str());

    std::shared_ptr<EffectContext> context = std::make_shared<EffectContext>();
    context->memoryManager_ = std::make_shared<EffectMemoryManager>();
    context->renderStrategy_ = std::make_shared<RenderStrategy>();
    context->ipType_ = runningType;

    context->memoryManager_->Init(src, dst); // local variable and not need invoke ClearMemory
    context->memoryManager_->SetIPType(runningType);
    context->renderStrategy_->Init(srcBuf, dstBuf);

    std::shared_ptr<EffectBuffer> input = nullptr;
    res = CreateDmaEffectBufferIfNeed(runningType, srcBuf, srcBuf, context, input);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "Render CreateDmaEffectBuffer src fail! res=%{public}d, name=%{public}s", res, name_.c_str());
    if (input != nullptr) {
        MemcpyHelper::CopyData(srcBuf, input.get());
    }

    if (runningType == IPType::GPU || src->buffer_ != dst->buffer_) {
        std::shared_ptr<EffectBuffer> output = nullptr;
        res =
            CreateDmaEffectBufferIfNeed(runningType, dstBuf, input == nullptr ? srcBuf : input.get(), context, output);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
            "Render CreateDmaEffectBuffer dst fail! res=%{public}d, name=%{public}s", res, name_.c_str());
        res = Render(input == nullptr ? srcBuf : input.get(), output == nullptr ? dstBuf : output.get(), context);
        if (output != nullptr) {
            MemcpyHelper::CopyData(output.get(), dstBuf);
        }
    } else {
        res = Render(srcBuf, context);
    }
    return res;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS