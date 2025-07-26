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
#include "effect_trace.h"
#include "effect_json_helper.h"
#include "efilter_factory.h"
#include "memcpy_helper.h"
#include "format_helper.h"
#include "colorspace_helper.h"
#include "render_thread.h"
#include "render_task.h"
#include "render_environment.h"

namespace OHOS {
namespace Media {
namespace Effect {
const std::string EFilter::Parameter::KEY_DEFAULT_VALUE = "default_value";
const std::string START_CACHE_CONFIG = "START_CACHE";
const std::string CANCEL_CACHE_CONFIG = "CANCEL_CACHE";

EFilter::EFilter(const std::string &name) : EFilterBase(name)
{
    cacheConfig_ = std::make_shared<EFilterCacheConfig>();
}

EFilter::~EFilter() = default;

ErrorCode EFilter::ProcessConfig(const std::string &key)
{
    if (START_CACHE_CONFIG.compare(key) == 0) {
        StartCache();
    }
    if (CANCEL_CACHE_CONFIG.compare(key) == 0) {
        CancelCache();
    }
    return ErrorCode::SUCCESS;
}

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

bool EFilter::IsTextureInput()
{
    return false;
}

ErrorCode EFilter::Save(EffectJsonPtr &res)
{
    res->Put("name", name_);
    EffectJsonPtr jsonValues = EffectJsonHelper::CreateObject();
    for (auto value : values_) {
        if (CommonUtils::ParseAnyAndAddToJson(value.first, value.second, jsonValues) !=
            ErrorCode::SUCCESS) {
            EFFECT_LOGE("not support switch to json! key:%{public}s", value.first.c_str());
        }
    }
    if (jsonValues->IsValid()) {
        res->Put("values", jsonValues);
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
    CHECK_AND_RETURN_RET_LOG(pixelFormatCap != nullptr, pixelFormatCap,
        "GetPixelFormatCap: PixelFormatCap fail! name=%{public}s", name.c_str());
    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    if (effectInfo == nullptr) {
        EFFECT_LOGE("GetPixelFormatCap: GetEffectInfo fail! name=%{public}s", name.c_str());
        return pixelFormatCap;
    }

    pixelFormatCap->formats = effectInfo->formats_;
    return pixelFormatCap;
}

std::shared_ptr<HdrFormatCap> GetHdrFormatCap(std::string &name)
{
    std::shared_ptr<HdrFormatCap> hdrFormatCap = std::make_shared<HdrFormatCap>();
    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    if (effectInfo == nullptr) {
        EFFECT_LOGE("GetHdrFormatCap: GetEffectInfo fail! name=%{public}s", name.c_str());
        return hdrFormatCap;
    }

    hdrFormatCap->hdrFormats = effectInfo->hdrFormats_;
    return hdrFormatCap;
}

std::shared_ptr<ColorSpaceCap> GetColorSpaceCap(std::string &name)
{
    std::shared_ptr<ColorSpaceCap> colorSpaceCap = std::make_shared<ColorSpaceCap>();
    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    if (effectInfo == nullptr) {
        EFFECT_LOGE("GetColorSpaceCap: GetEffectInfo fail! name=%{public}s", name.c_str());
        return colorSpaceCap;
    }

    colorSpaceCap->colorSpaces = effectInfo->colorSpaces_;
    return colorSpaceCap;
}

void NegotiateHdrFormat(std::vector<HdrFormat> &hdrFormats, std::unordered_set<HdrFormat> &filtersSupportedHDRFormat)
{
    for (auto it = filtersSupportedHDRFormat.begin(); it != filtersSupportedHDRFormat.end();) {
        if (std::find(hdrFormats.begin(), hdrFormats.end(), *it) == hdrFormats.end()) {
            it = filtersSupportedHDRFormat.erase(it);
        } else {
            ++it;
        }
    }
}

void NegotiateColorSpace(std::vector<EffectColorSpace> &colorSpaces,
    std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace)
{
    for (auto it = filtersSupportedColorSpace.begin(); it != filtersSupportedColorSpace.end();) {
        if (std::find(colorSpaces.begin(), colorSpaces.end(), *it) == colorSpaces.end()) {
            it = filtersSupportedColorSpace.erase(it);
        } else {
            ++it;
        }
    }
}

void EFilter::Negotiate(const std::string &inPort, const std::shared_ptr<Capability> &capability,
    std::shared_ptr<EffectContext> &context)
{
    std::shared_ptr<Capability> outputCap = std::make_shared<Capability>(name_);
    outputCap->pixelFormatCap_ = GetPixelFormatCap(name_);
    outputCap->memNegotiatedCap_ = Negotiate(capability->memNegotiatedCap_, context);
    outputCap->colorSpaceCap_ = GetColorSpaceCap(name_);
    outputCap->hdrFormatCap_ = GetHdrFormatCap(name_);
    logStrategy_ = context->logStrategy_;

    context->capNegotiate_->AddCapability(outputCap);
    context->cacheNegotiate_->NegotiateConfig(cacheConfig_);

    NegotiateColorSpace(outputCap->colorSpaceCap_->colorSpaces, context->filtersSupportedColorSpace_);
    NegotiateHdrFormat(outputCap->hdrFormatCap_->hdrFormats, context->filtersSupportedHdrFormat_);
    EFFECT_LOGD("Negotiate::filtersSupportedHdrFormat count: %{public}d, colorSpace count: %{public}d",
        static_cast<int>(context->filtersSupportedHdrFormat_.size()),
        static_cast<int>(context->filtersSupportedColorSpace_.size()));
    outputCap_ = outputCap;
    outPorts_[0]->Negotiate(outputCap, context);
}

std::shared_ptr<MemoryData> AllocMemory(BufferType allocBufferType, EffectBuffer *buffer)
{
    std::unique_ptr<AbsMemory> absMemory = EffectMemory::CreateMemory(allocBufferType);
    CHECK_AND_RETURN_RET_LOG(absMemory != nullptr, nullptr,
        "memory create fail! allocatorType=%{public}d", allocBufferType);

    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->extraInfo_ != nullptr, nullptr,
        "alloc memory fail, buffer or buffer extraInfo is null!");
    MemoryInfo allocMemInfo = {
        .bufferInfo = *buffer->bufferInfo_,
        .extra = static_cast<void *>(buffer->bufferInfo_->surfaceBuffer_),
    };
    std::shared_ptr<MemoryData> memoryData = absMemory->Alloc(allocMemInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, nullptr,
        "memoryData is null! bufferType=%{public}d", allocBufferType);
    return memoryData;
}

ErrorCode CreateEffectBuffer(EffectBuffer *buffer, std::shared_ptr<MemoryData> &allocMemData,
    std::shared_ptr<EffectBuffer> &effectBuffer)
{
    CHECK_AND_RETURN_RET_LOG(allocMemData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "alloc memory fail!");
    MemoryInfo &allocMemInfo = allocMemData->memoryInfo;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = allocMemInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *buffer->extraInfo_;
    bufferInfo->surfaceBuffer_ = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;
    effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, allocMemData->data, extraInfo);
    return ErrorCode::SUCCESS;
}

std::shared_ptr<EffectBuffer> EFilter::IpTypeConvert(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    IPType runningIPType = context->ipType_;
    IEffectFormat formatType = buffer->bufferInfo_->formatType_;
    std::shared_ptr<PixelFormatCap> pixelFormatCap = GetPixelFormatCap(name_);
    std::map<IEffectFormat, std::vector<IPType>> &formats = pixelFormatCap->formats;
    auto it = formats.find(formatType);
    if (it == formats.end() && runningIPType == IPType::CPU) {
        it = formats.find(IEffectFormat::RGBA8888);
    }
    CHECK_AND_RETURN_RET_LOG(it != formats.end(), buffer,
        "format not support! format=%{public}d, name=%{public}s", formatType, name_.c_str());
    std::shared_ptr<EffectBuffer> source = buffer;
    CHECK_AND_RETURN_RET_LOG(runningIPType != IPType::DEFAULT, buffer, "runningIPType is default");

    if (std::find(it->second.begin(), it->second.end(), runningIPType) == it->second.end()) {
        if (runningIPType == IPType::GPU) {
            source = ConvertFromGPU2CPU(buffer, context, source);
        } else {
            source = ConvertFromCPU2GPU(buffer, context, source);
        }
    }
    return source;
}

std::shared_ptr<EffectBuffer> EFilter::CreateEffectBufferFromTexture(const std::shared_ptr<EffectBuffer> &buffer,
    const std::shared_ptr<EffectContext> &context)
{
    auto texture = buffer->bufferInfo_->tex_;
    MemoryInfo memInfo = {
        .bufferInfo = {
            .width_ = texture->Width(),
            .height_ = texture->Height(),
            .len_ = FormatHelper::CalculateSize(texture->Width(), texture->Height(), IEffectFormat::RGBA8888),
            .formatType_ = IEffectFormat::RGBA8888,
        },
        .bufferType = BufferType::DMA_BUFFER,
    };

    MemoryData* memoryData = context->memoryManager_->AllocMemory(nullptr, memInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, buffer, "Alloc new memory fail!");

    MemoryInfo& allocMemInfo = memoryData->memoryInfo;
    auto bufferInfo = std::make_shared<BufferInfo>();
    CommonUtils::CopyBufferInfo(allocMemInfo.bufferInfo, *bufferInfo);
    auto extraInfo = std::make_shared<ExtraInfo>();
    CommonUtils::CopyExtraInfo(*buffer->extraInfo_, *extraInfo);
    extraInfo->bufferType = allocMemInfo.bufferType;
    bufferInfo->surfaceBuffer_ = static_cast<SurfaceBuffer*>(allocMemInfo.extra);

    auto effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data, extraInfo);
    context->renderEnvironment_->ConvertTextureToBuffer(texture, effectBuffer.get());
    bufferInfo->surfaceBuffer_->InvalidateCache();
    effectBuffer->extraInfo_->dataType = DataType::SURFACE_BUFFER;
    effectBuffer->extraInfo_->bufferType = BufferType::DMA_BUFFER;

    return effectBuffer;
}

std::shared_ptr<EffectBuffer> EFilter::ConvertFromGPU2CPU(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source)
{
    EFFECT_LOGD("EFilter::ConvertFromGPU2CPU enter");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, buffer, "ConvertFromGPU2CPU fail, buffer is null!");
    context->ipType_ = IPType::CPU;

    auto input = CreateEffectBufferFromTexture(buffer, context);
    if (!input) return source;
    input->bufferInfo_->tex_ = buffer->bufferInfo_->tex_;
    input->bufferInfo_->hdrFormat_ = buffer->bufferInfo_->hdrFormat_;
    input->bufferInfo_->pixelMap_ = buffer->bufferInfo_->pixelMap_;
    source = input;

    if (buffer->auxiliaryBufferInfos == nullptr) return source;
    auto gainMapBufferInfoIt = buffer->auxiliaryBufferInfos->find(EffectPixelmapType::GAINMAP);
    if (gainMapBufferInfoIt == buffer->auxiliaryBufferInfos->end()) return source;

    auto gainMapBufferInfo = gainMapBufferInfoIt->second;
    auto tmpGainMapBuffer = std::make_shared<EffectBuffer>(gainMapBufferInfo, nullptr, input->extraInfo_);
    auto gainMapBuffer = CreateEffectBufferFromTexture(tmpGainMapBuffer, context);
    if (!gainMapBuffer) return source;

    gainMapBuffer->bufferInfo_->pixelmapType_ = EffectPixelmapType::GAINMAP;
    gainMapBuffer->bufferInfo_->bufferType_ = gainMapBuffer->extraInfo_->bufferType;
    gainMapBuffer->bufferInfo_->addr_ = gainMapBuffer->buffer_;
    gainMapBuffer->bufferInfo_->pixelMap_ = gainMapBufferInfo->pixelMap_;
    if (source->auxiliaryBufferInfos == nullptr) {
        source->auxiliaryBufferInfos =
            std::make_shared<std::unordered_map<EffectPixelmapType, std::shared_ptr<BufferInfo>>>();
    }
    source->auxiliaryBufferInfos->emplace(EffectPixelmapType::GAINMAP, gainMapBuffer->bufferInfo_);

    return source;
}

std::shared_ptr<EffectBuffer> EFilter::ConvertFromCPU2GPU(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source)
{
    EFFECT_LOGD("EFilter::ConvertFromCPU2GPU enter");
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
        context->renderEnvironment_->Prepare();
    }
    context->ipType_ = IPType::GPU;
    if (source->bufferInfo_->surfaceBuffer_ != nullptr) {
        source->bufferInfo_->surfaceBuffer_->FlushCache();
    }
    source = context->renderEnvironment_->ConvertBufferToTexture(buffer.get());
    return source;
}

void EFilter::HandleCacheStart(const std::shared_ptr<EffectBuffer>& source, std::shared_ptr<EffectContext>& context)
{
    if (cacheConfig_->GetStatus() == CacheStatus::CACHE_START && !context->cacheNegotiate_->HasCached()) {
        CacheBuffer(source.get(), context);
        cacheConfig_->SetStatus(CacheStatus::CACHE_ENABLED);
    }
}

ErrorCode EFilter::PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context)
{
    bool needCache = context->cacheNegotiate_->needCache();
    if (needCache && context->cacheNegotiate_->HasCached() && !context->cacheNegotiate_->HasUseCache()) {
        if (cacheConfig_->GetStatus() == CacheStatus::CACHE_USED) {
            return UseCache(context);
        } else if (cacheConfig_->GetStatus() == CacheStatus::NO_CACHE
            || cacheConfig_->GetStatus() == CacheStatus::CACHE_ENABLED) {
            return PushData(buffer.get(), context);
        }
    }
    IPType preIPType = context->ipType_;
    std::shared_ptr<EffectBuffer> source = IpTypeConvert(buffer, context);
    IPType runningIPType = context->ipType_;

    if (runningIPType == IPType::GPU) {
        if (cacheConfig_->GetStatus() == CacheStatus::CACHE_START && !context->cacheNegotiate_->HasCached()) {
            CacheBuffer(source.get(), context);
            cacheConfig_->SetStatus(CacheStatus::CACHE_ENABLED);
        }
        ErrorCode res = Render(source.get(), context);
        return res;
    }
    CHECK_AND_RETURN_RET_LOG(outputCap_ != nullptr, ErrorCode::ERR_INPUT_NULL, "outputCap is null.");
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap = outputCap_->memNegotiatedCap_;
    EffectBuffer *output = preIPType != runningIPType ? source.get()
        : context->renderStrategy_->ChooseBestOutput(source.get(), memNegotiatedCap);
    if (source.get() == output) {
        HandleCacheStart(source, context);
        ErrorCode res = Render(source.get(), context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
            "Render input fail! filterName=%{public}s", name_.c_str());
        return ErrorCode::SUCCESS;
    }

    std::shared_ptr<EffectBuffer> effectBuffer = nullptr;
    if (output == nullptr) {
        AllocBuffer(context, memNegotiatedCap, source, effectBuffer);
    }
    if (effectBuffer != nullptr) {
        output = effectBuffer.get();
    }
    HandleCacheStart(source, context);
    ErrorCode res = Render(source.get(), output, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render inout fail! filterName=%{public}s", name_.c_str());
    return PushData(output, context);
}

ErrorCode EFilter::AllocBuffer(std::shared_ptr<EffectContext> &context,
    const std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap, std::shared_ptr<EffectBuffer> &source,
    std::shared_ptr<EffectBuffer> &effectBuffer) const
{
    EFFECT_TRACE_NAME("EFilter::AllocBuffer");
    MemoryInfo memInfo = {
        .bufferInfo = {
            .width_ = memNegotiatedCap->width,
            .height_ = memNegotiatedCap->height,
            .len_ = FormatHelper::CalculateSize(
                memNegotiatedCap->width, memNegotiatedCap->height, source->bufferInfo_->formatType_),
            .formatType_ = source->bufferInfo_->formatType_,
            .colorSpace_ = source->bufferInfo_->colorSpace_,
        }
    };
    MemoryData *memoryData = context->memoryManager_->AllocMemory(source->buffer_, memInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc new memory fail!");
    MemoryInfo &allocMemInfo = memoryData->memoryInfo;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = allocMemInfo.bufferInfo;
    bufferInfo->fd_ = source->bufferInfo_->fd_;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *source->extraInfo_;
    extraInfo->bufferType = allocMemInfo.bufferType;
    bufferInfo->surfaceBuffer_ = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;
    effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data, extraInfo);

        if (source->auxiliaryBufferInfos != nullptr) {
            effectBuffer->auxiliaryBufferInfos =
                std::make_unique<std::unordered_map<EffectPixelmapType, std::shared_ptr<BufferInfo>>>();
            EFFECT_LOGD("ConvertFromCPU2GPU buffer->auxiliaryBufferInfos != nullptr");
            for (const auto &entry : *source->auxiliaryBufferInfos) {
                std::shared_ptr<BufferInfo> auxiliaryBufferInfo = std::make_shared<BufferInfo>();
                *auxiliaryBufferInfo = *(entry.second);
                effectBuffer->auxiliaryBufferInfos->emplace(entry.first, auxiliaryBufferInfo);
            }
        }
        return ErrorCode::SUCCESS;
}

ErrorCode EFilter::UseCache(std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr && context->cacheNegotiate_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "UseCache fail, context or context cacheNegotiate is null");
    context->cacheNegotiate_->UseCache();
    std::shared_ptr<EffectBuffer> cacheBuffer = nullptr;
    auto res = GetFilterCache(cacheBuffer, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "GetFilterCache fail");
    CHECK_AND_RETURN_RET_LOG(cacheConfig_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "GetIPType fail, cacheConfig_ is null");
    if (cacheConfig_->GetIPType() == IPType::GPU) {
        return Render(cacheBuffer.get(), context);
    }
    CHECK_AND_RETURN_RET_LOG(outputCap_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "get memNegotiatedCap fail, outputCap is null");
    std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap = outputCap_->memNegotiatedCap_;
    EffectBuffer *output = context->renderStrategy_->ChooseBestOutput(cacheBuffer.get(), memNegotiatedCap);
    CHECK_AND_RETURN_RET_LOG(output != nullptr, ErrorCode::ERR_INPUT_NULL, "RChooseBestOutput fail, out is null");
    if (cacheBuffer.get() == output) {
        return Render(cacheBuffer.get(), context);
    }
    res = Render(cacheBuffer.get(), output, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render inout fail! filterName=%{public}s",
        name_.c_str());
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
        ColorSpaceHelper::UpdateMetadata(buffer.get(), context);
        return ErrorCode::SUCCESS;
    }

    // efilter create new buffer and inout with the same buffer.
    EffectBuffer *output = context->renderStrategy_->GetOutput();
    if (output == nullptr || input->buffer_ == output->buffer_) {
        return CommonUtils::ModifyPixelMapProperty(buffer->bufferInfo_->pixelMap_, buffer, context);
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
    effectBuffer->bufferInfo_->tex_ = buffer->bufferInfo_->tex_;
    effectBuffer->auxiliaryBufferInfos = buffer->auxiliaryBufferInfos;
    if (outPorts_.empty()) {
        return OnPushDataPortsEmpty(effectBuffer, context, name_);
    }

    return outPorts_[0]->PushData(effectBuffer, context);
}

std::shared_ptr<MemNegotiatedCap> EFilter::Negotiate(const std::shared_ptr<MemNegotiatedCap> &input,
    std::shared_ptr<EffectContext> &context)
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
    if (runningType == IPType::CPU || src->extraInfo_->dataType == DataType::TEX) {
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
    memInfo.bufferInfo.colorSpace_ = src->bufferInfo_->colorSpace_;
    MemoryData *memData = context->memoryManager_->AllocMemory(src->buffer_, memInfo);
    CHECK_AND_RETURN_RET_LOG(memData != nullptr, ErrorCode::ERR_ALLOC_MEMORY_FAIL, "Alloc memory fail!");

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = memData->memoryInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *src->extraInfo_;
    extraInfo->bufferType = memData->memoryInfo.bufferType;
    bufferInfo->surfaceBuffer_ = (memData->memoryInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(memData->memoryInfo.extra) : nullptr;
    effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memData->data, extraInfo);
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::RenderWithGPU(std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &src,
    std::shared_ptr<EffectBuffer> &dst, bool needModifySource)
{
    if (src->extraInfo_->dataType == DataType::TEX) {
        return Render(src.get(), dst.get(), context);
    }
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
        context->renderEnvironment_->Prepare();
    }
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    context->renderEnvironment_->BeginFrame();
    if (src->bufferInfo_->formatType_ == IEffectFormat::RGBA8888 ||
        src->bufferInfo_->formatType_ == IEffectFormat::RGBA_1010102) {
        context->renderEnvironment_->GenTex(src, buffer);
    } else {
        context->renderEnvironment_->ConvertYUV2RGBA(src, buffer);
    }

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    bufferInfo = dst->bufferInfo_;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    extraInfo->dataType = DataType::TEX;
    std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    effectBuffer->bufferInfo_->tex_ = context->renderEnvironment_->RequestBuffer(bufferInfo->width_,
        bufferInfo->height_, buffer->bufferInfo_->tex_->Format());
    ErrorCode res = Render(buffer.get(), effectBuffer.get(), context);
    if (needModifySource) {
        CommonUtils::ModifyPixelMapPropertyForTexture(dst->bufferInfo_->pixelMap_, effectBuffer, context);
    } else {
        context->renderEnvironment_->ConvertTextureToBuffer(effectBuffer->bufferInfo_->tex_, dst.get());
    }
    return res;
}

void GetSupportedHdrFormat(std::string &name, std::unordered_set<HdrFormat> &filtersSupportedHDRFormat)
{
    filtersSupportedHDRFormat = {
        HdrFormat::SDR,
        HdrFormat::HDR8_GAINMAP,
        HdrFormat::HDR10
    };

    std::shared_ptr<HdrFormatCap> hdrFormatCap = GetHdrFormatCap(name);
    NegotiateHdrFormat(hdrFormatCap->hdrFormats, filtersSupportedHDRFormat);
}

void GetSupportedColorSpace(std::string &name, std::unordered_set<EffectColorSpace> &filtersSupportedColorSpace)
{
    std::unordered_set<EffectColorSpace> allSupportedColorSpaces = ColorSpaceManager::GetAllSupportedColorSpaces();
    std::for_each(allSupportedColorSpaces.begin(), allSupportedColorSpaces.end(), [&](const auto &item) {
        filtersSupportedColorSpace.emplace(item);
    });

    std::shared_ptr<ColorSpaceCap> colorSpaceCap = GetColorSpaceCap(name);
    NegotiateColorSpace(colorSpaceCap->colorSpaces, filtersSupportedColorSpace);
}

std::shared_ptr<EffectContext> CreateEffectContext(std::shared_ptr<EffectBuffer> &src,
    std::shared_ptr<EffectBuffer> &dst, std::string &name)
{
    std::shared_ptr<EffectContext> context = std::make_shared<EffectContext>();
    context->memoryManager_ = std::make_shared<EffectMemoryManager>();
    context->renderStrategy_ = std::make_shared<RenderStrategy>();
    context->colorSpaceManager_ = std::make_shared<ColorSpaceManager>();
    GetSupportedColorSpace(name, context->filtersSupportedColorSpace_);
    GetSupportedHdrFormat(name, context->filtersSupportedHdrFormat_);
    context->memoryManager_->Init(src, dst); // local variable and not need invoke ClearMemory
    context->renderStrategy_->Init(src, dst);
    context->colorSpaceManager_->Init(src, dst);

    return context;
}

ErrorCode CheckAndUpdateEffectBufferIfNeed(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectContext> &context,
    std::string &name, std::shared_ptr<EffectBuffer> &dst)
{
    if (src->bufferInfo_->tex_ != nullptr && dst->bufferInfo_->tex_ != nullptr) {
        unsigned int input = src->bufferInfo_->tex_->GetName();
        unsigned int output = dst->bufferInfo_->tex_->GetName();
        if (input == output) {
            return ErrorCode::ERR_INVALID_TEXTURE;
        }
        ErrorCode res = ColorSpaceHelper::ConvertColorSpace(src, context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
            "CheckAndUpdateEffectBufferIfNeed: ConvertColorSpace fail! res=%{public}d, name=%{public}s",
            res, name.c_str());
        return ErrorCode::SUCCESS;
    }
    if (src->buffer_ == dst->buffer_) {
        dst = src;
    }

    ErrorCode res = ColorSpaceHelper::ConvertColorSpace(src, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "CheckAndUpdateEffectBufferIfNeed: ConvertColorSpace fail! res=%{public}d, name=%{public}s", res, name.c_str());

    if (src->buffer_ != dst->buffer_) {
        // color space is same or not after covert color space if needed.
        EffectColorSpace srcColorSpace = src->bufferInfo_->colorSpace_;
        EffectColorSpace dstColorSpace = dst->bufferInfo_->colorSpace_;
        bool isSrcHdr = ColorSpaceHelper::IsHdrColorSpace(srcColorSpace);
        bool isDstHdr = ColorSpaceHelper::IsHdrColorSpace(dstColorSpace);
        CHECK_AND_RETURN_RET_LOG(isSrcHdr == isDstHdr, ErrorCode::ERR_FILTER_NOT_SUPPORT_INPUT_OUTPUT_COLORSPACE,
            "CheckAndUpdateEffectBufferIfNeed: input and output color space not support! srcRealColorSpace=%{public}d, "
            "dstColorSpace=%{public}d", srcColorSpace, dstColorSpace);
    }

    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::UseTextureInput()
{
    std::shared_ptr<EffectContext> tempContext = std::make_shared<EffectContext>();
    tempContext->ipType_ = IPType::GPU;
    ErrorCode result = Render(nullptr, nullptr, tempContext);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result, "Render: render with input and output fail!");
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::RenderInner(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst)
{
    if (IsTextureInput()) {
        return UseTextureInput();
    }
    EffectBuffer *srcBuf = src.get();
    EffectBuffer *dstBuf = dst.get();
    CHECK_AND_RETURN_RET_LOG(srcBuf != nullptr && dstBuf != nullptr, ErrorCode::ERR_INPUT_NULL, "src or dst is null!");

    outPorts_.clear();
    void *originBuffer = src->buffer_;

    std::shared_ptr<EffectContext> context = CreateEffectContext(src, dst, name_);
    ErrorCode res = CheckAndUpdateEffectBufferIfNeed(src, context, name_, dst);
    CHECK_AND_RETURN_RET(res == ErrorCode::SUCCESS, res);
    bool needMotifySource = (src->buffer_ != originBuffer) && (src->buffer_ == dst->buffer_);

    IPType runningType = IPType::DEFAULT;
    res = CalculateEFilterIPType(src->bufferInfo_->formatType_, runningType);
    if (src->extraInfo_->dataType == DataType::TEX && runningType == IPType::CPU) {
        return ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE;
    }
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "Render CalculateEFilterIPType fail! name=%{public}s", name_.c_str());
    bool isCustomEnv = src->extraInfo_->dataType == DataType::TEX;
    InitContext(context, runningType, isCustomEnv);
    std::shared_ptr<EffectBuffer> input = nullptr;
    res = CreateDmaEffectBufferIfNeed(runningType, srcBuf, srcBuf, context, input);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "Render CreateDmaEffectBuffer src fail! res=%{public}d, name=%{public}s", res, name_.c_str());
    if (input != nullptr) {
        MemcpyHelper::CopyData(srcBuf, input.get());
    }

    if (runningType == IPType::GPU) {
        return RenderWithGPU(context, src, dst, needMotifySource);
    }

    if (src->buffer_ != dst->buffer_) {
        std::shared_ptr<EffectBuffer> output = nullptr;
        res =
            CreateDmaEffectBufferIfNeed(runningType, dstBuf, input == nullptr ? srcBuf : input.get(), context, output);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
            "Render CreateDmaEffectBuffer dst fail! res=%{public}d, name=%{public}s", res, name_.c_str());
        res = Render(input == nullptr ? srcBuf : input.get(), output == nullptr ? dstBuf : output.get(), context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render: render with input and output fail!");
        ColorSpaceHelper::UpdateMetadata(output == nullptr ? dst.get() : output.get(), context);
        if (output != nullptr) {
            MemcpyHelper::CopyData(output.get(), dstBuf);
        }
    } else {
        res = Render(srcBuf, context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render: render with input fail!");
    }
    return ErrorCode::SUCCESS;
}

void EFilter::InitContext(std::shared_ptr<EffectContext> &context, IPType &runningType, bool isCustomEnv)
{
    context->memoryManager_ = std::make_shared<EffectMemoryManager>();
    context->renderStrategy_ = std::make_shared<RenderStrategy>();
    context->capNegotiate_ = std::make_shared<CapabilityNegotiate>();
    context->renderEnvironment_ = std::make_shared<RenderEnvironment>();
    context->renderEnvironment_->Init(isCustomEnv);
    context->renderEnvironment_->Prepare();
    context->colorSpaceManager_ = std::make_shared<ColorSpaceManager>();
    context->cacheNegotiate_ = std::make_shared<EFilterCacheNegotiate>();
    context->metaInfoNegotiate_ = std::make_shared<EfilterMetaInfoNegotiate>();
    context->ipType_ = runningType;
    context->memoryManager_->SetIPType(runningType);
}

ErrorCode EFilter::Render(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst)
{
    ErrorCode res = RenderInner(src, dst);
    if (res != ErrorCode::SUCCESS) {
        return res;
    }

    // update exif info
    if (!IsTextureInput()) {
        CommonUtils::UpdateImageExifDateTime(dst->bufferInfo_->pixelMap_);
    }
    return ErrorCode::SUCCESS;
}
ErrorCode EFilter::StartCache()
{
    cacheConfig_->SetStatus(CacheStatus::CACHE_START);
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::CacheBuffer(EffectBuffer *cache, std::shared_ptr<EffectContext> &context)
{
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::GetFilterCache(std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context)
{
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::ReleaseCache()
{
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::CancelCache()
{
    cacheConfig_->SetStatus(CacheStatus::NO_CACHE);
    cacheConfig_->SetIPType(IPType::DEFAULT);
    return ReleaseCache();
}

} // namespace Effect
} // namespace Media
} // namespace OHOS