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
    std::shared_ptr<EffectInfo> effectInfo = EFilterFactory::Instance()->GetEffectInfo(name);
    if (effectInfo == nullptr) {
        EFFECT_LOGE("GetPixelFormatCap: GetEffectInfo fail! name=%{public}s", name.c_str());
        return pixelFormatCap;
    }

    pixelFormatCap->formats = effectInfo->formats_;
    return pixelFormatCap;
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
    context->capNegotiate_->AddCapability(outputCap);
    context->cacheNegotiate_->NegotiateConfig(cacheConfig_);
    NegotiateColorSpace(outputCap->colorSpaceCap_->colorSpaces, context->filtersSupportedColorSpace_);
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
        .extra = static_cast<void *>(buffer->extraInfo_->surfaceBuffer),
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
    extraInfo->surfaceBuffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
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

std::shared_ptr<EffectBuffer> EFilter::ConvertFromGPU2CPU(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source)
{
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, buffer, "ConvertFromGPU2CPU fail, buffer is null!");
    EFFECT_LOGE("ConvertFromGPU2CPU");
    context->ipType_ = IPType::CPU;
    MemoryInfo memInfo = {
        .bufferInfo = {
            .width_ = buffer->tex->Width(),
            .height_ = buffer->tex->Height(),
            .len_ = FormatHelper::CalculateSize(buffer->tex->Width(), buffer->tex->Height(), IEffectFormat::RGBA8888),
            .formatType_ = IEffectFormat::RGBA8888,
        },
        .bufferType = BufferType::DMA_BUFFER,
    };
    MemoryData *memoryData = context->memoryManager_->AllocMemory(nullptr, memInfo);
    CHECK_AND_RETURN_RET_LOG(memoryData != nullptr, buffer, "Alloc new memory fail!");
    MemoryInfo &allocMemInfo = memoryData->memoryInfo;
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    *bufferInfo = allocMemInfo.bufferInfo;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *buffer->extraInfo_;
    extraInfo->bufferType = allocMemInfo.bufferType;
    extraInfo->surfaceBuffer = static_cast<SurfaceBuffer *>(allocMemInfo.extra);
    std::shared_ptr<EffectBuffer> input = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data,
        extraInfo);
    context->renderEnvironment_->ConvertTextureToBuffer(buffer->tex, input.get());
    extraInfo->surfaceBuffer->InvalidateCache();
    input->extraInfo_->dataType = DataType::SURFACE_BUFFER;
    input->extraInfo_->bufferType = BufferType::DMA_BUFFER;
    source = input;
    return source;
}

std::shared_ptr<EffectBuffer> EFilter::ConvertFromCPU2GPU(const std::shared_ptr<EffectBuffer> &buffer,
    std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &source)
{
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
        context->renderEnvironment_->Prepare();
    }
    context->ipType_ = IPType::GPU;
    if (source->extraInfo_->surfaceBuffer != nullptr) {
        source->extraInfo_->surfaceBuffer->FlushCache();
    }
    source = context->renderEnvironment_->ConvertBufferToTexture(buffer.get());
    return source;
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
        if (cacheConfig_->GetStatus() == CacheStatus::CACHE_START && !context->cacheNegotiate_->HasCached()) {
            CacheBuffer(source.get(), context);
            cacheConfig_->SetStatus(CacheStatus::CACHE_ENABLED);
        }
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
    if (cacheConfig_->GetStatus() == CacheStatus::CACHE_START && !context->cacheNegotiate_->HasCached()) {
        CacheBuffer(source.get(), context);
        cacheConfig_->SetStatus(CacheStatus::CACHE_ENABLED);
    }
    ErrorCode res = Render(source.get(), output, context);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render inout fail! filterName=%{public}s", name_.c_str());
    return PushData(output, context);
}

ErrorCode EFilter::AllocBuffer(std::shared_ptr<EffectContext> &context,
    const std::shared_ptr<MemNegotiatedCap> &memNegotiatedCap, std::shared_ptr<EffectBuffer> &source,
    std::shared_ptr<EffectBuffer> &effectBuffer) const
{
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
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    *extraInfo = *source->extraInfo_;
    extraInfo->bufferType = allocMemInfo.bufferType;
    extraInfo->surfaceBuffer = (allocMemInfo.bufferType == BufferType::DMA_BUFFER) ?
        static_cast<SurfaceBuffer *>(allocMemInfo.extra) : nullptr;
    effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, memoryData->data, extraInfo);
    return ErrorCode::SUCCESS;
}

ErrorCode EFilter::UseCache(std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr && context->cacheNegotiate_ != nullptr, ErrorCode::ERR_INPUT_NULL,
        "UseCache fail, context or context cacheNegotiate is null");
    context->cacheNegotiate_->UseCache();
    std::shared_ptr<EffectBuffer> cacheBuffer = nullptr;
    GetFilterCache(cacheBuffer, context);
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
    ErrorCode res = Render(cacheBuffer.get(), output, context);
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
        return ColorSpaceHelper::UpdateMetadata(buffer.get());
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
    effectBuffer->tex = buffer->tex;
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
    memInfo.bufferInfo.colorSpace_ = src->bufferInfo_->colorSpace_;
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

ErrorCode EFilter::RenderWithGPU(std::shared_ptr<EffectContext> &context, std::shared_ptr<EffectBuffer> &src,
    std::shared_ptr<EffectBuffer> &dst, bool needModifySource)
{
    if (context->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
        context->renderEnvironment_->Init();
        context->renderEnvironment_->Prepare();
    }
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    context->renderEnvironment_->BeginFrame();
    if (src->bufferInfo_->formatType_ == IEffectFormat::RGBA8888) {
        context->renderEnvironment_->GenMainTex(src, buffer);
    } else {
        context->renderEnvironment_->ConvertYUV2RGBA(src, buffer);
    }

    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    bufferInfo = dst->bufferInfo_;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_shared<ExtraInfo>();
    extraInfo->dataType = DataType::TEX;
    std::shared_ptr<EffectBuffer> effectBuffer = std::make_shared<EffectBuffer>(bufferInfo, nullptr, extraInfo);
    effectBuffer->tex = context->renderEnvironment_->RequestBuffer(bufferInfo->width_, bufferInfo->height_);
    ErrorCode res = Render(buffer.get(), effectBuffer.get(), context);
    if (needModifySource) {
        CommonUtils::ModifyPixelMapPropertyForTexture(dst->extraInfo_->pixelMap, effectBuffer, context);
    } else {
        context->renderEnvironment_->ConvertTextureToBuffer(effectBuffer->tex, dst.get());
    }
    return res;
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

    context->memoryManager_->Init(src, dst); // local variable and not need invoke ClearMemory
    context->renderStrategy_->Init(src, dst);
    context->colorSpaceManager_->Init(src, dst);

    return context;
}

ErrorCode CheckAndUpdateEffectBufferIfNeed(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectContext> &context,
    std::string &name, std::shared_ptr<EffectBuffer> &dst)
{
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
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res,
        "Render CalculateEFilterIPType fail! name=%{public}s", name_.c_str());
    InitContext(context, runningType);
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
        res = ColorSpaceHelper::UpdateMetadata(output == nullptr ? dst.get() : output.get());
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "OnPushDataPortsEmpty: UpdateMetadata fail!");
        if (output != nullptr) {
            MemcpyHelper::CopyData(output.get(), dstBuf);
        }
    } else {
        res = Render(srcBuf, context);
        CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "Render: render with input fail!");
    }
    return ErrorCode::SUCCESS;
}

void EFilter::InitContext(std::shared_ptr<EffectContext> &context, IPType &runningType)
{
    context->ipType_ = runningType;
    context->memoryManager_->SetIPType(runningType);

    context->renderEnvironment_ = std::make_shared<RenderEnvironment>();
    context->renderEnvironment_->Init();
    context->renderEnvironment_->Prepare();
}

ErrorCode EFilter::Render(std::shared_ptr<EffectBuffer> &src, std::shared_ptr<EffectBuffer> &dst)
{
    ErrorCode res = RenderInner(src, dst);
    if (res != ErrorCode::SUCCESS) {
        return res;
    }

    // update exif info
    if (!IsTextureInput()) {
        CommonUtils::UpdateImageExifDateTime(dst->extraInfo_->pixelMap);
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