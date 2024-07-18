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

#include "image_effect_inner.h"

#include <cassert>
#include <securec.h>
#include <algorithm>
#include <sync_fence.h>

#include "common_utils.h"
#include "effect_log.h"
#include "filter_factory.h"
#include "image_sink_filter.h"
#include "image_source_filter.h"
#include "effect_surface_adapter.h"
#include "pipeline_core.h"
#include "json_helper.h"
#include "efilter_factory.h"
#include "external_loader.h"
#include "effect_context.h"
#include "render_task.h"
#include "colorspace_helper.h"
#include "render_environment.h"
#include "memcpy_helper.h"

#define RENDER_QUEUE_SIZE 8
#define COMMON_TASK_TAG 0

namespace OHOS {
namespace Media {
namespace Effect {

enum class EffectState {
    IDLE,
    RUNNING,
};

class ImageEffect::Impl {
public:
    Impl()
    {
        InitPipeline();
        InitEffectContext();
    }

    void CreatePipeline(std::vector<std::shared_ptr<EFilter>> &efilters);
private:
    void InitPipeline();
    void InitEffectContext();

public:
    std::unique_ptr<EffectSurfaceAdapter> surfaceAdapter_;
    std::shared_ptr<PipelineCore> pipeline_;
    std::shared_ptr<ImageSourceFilter> srcFilter_;
    std::shared_ptr<ImageSinkFilter> sinkFilter_;
    std::shared_ptr<EffectContext> effectContext_;
    EffectState effectState_ = EffectState::IDLE;
};

void ImageEffect::Impl::InitPipeline()
{
    srcFilter_ = FilterFactory::Instance().CreateFilterWithType<ImageSourceFilter>(GET_FILTER_NAME(ImageSourceFilter));
    sinkFilter_ = FilterFactory::Instance().CreateFilterWithType<ImageSinkFilter>(GET_FILTER_NAME(ImageSinkFilter));
    CHECK_AND_RETURN(srcFilter_ != nullptr);
    CHECK_AND_RETURN(sinkFilter_ != nullptr);
}

void ImageEffect::Impl::InitEffectContext()
{
    effectContext_ = std::make_shared<EffectContext>();
    effectContext_->memoryManager_ = std::make_shared<EffectMemoryManager>();
    effectContext_->renderStrategy_ = std::make_shared<RenderStrategy>();
    effectContext_->capNegotiate_ = std::make_shared<CapabilityNegotiate>();
    effectContext_->renderEnvironment_ = std::make_shared<RenderEnvironment>();
    effectContext_->colorSpaceManager_ = std::make_shared<ColorSpaceManager>();
}

void ImageEffect::Impl::CreatePipeline(std::vector<std::shared_ptr<EFilter>> &efilters)
{
    pipeline_ = std::make_shared<PipelineCore>();
    pipeline_->Init(nullptr);

    std::vector<Filter *> filtersToPipeline; // Note: Filters must be inserted in sequence.
    filtersToPipeline.push_back(srcFilter_.get());
    for (const auto &eFilter : efilters) {
        filtersToPipeline.push_back(eFilter.get());
    }
    filtersToPipeline.push_back(sinkFilter_.get());

    ErrorCode res = pipeline_->AddFilters(filtersToPipeline);
    CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "pipeline add filters fail! res=%{public}d", res);

    res = pipeline_->LinkFilters(filtersToPipeline);
    CHECK_AND_RETURN_LOG(res == ErrorCode::SUCCESS, "pipeline link filter fail! res=%{public}d", res);
}

struct EffectParameters {
    EffectParameters(std::shared_ptr<EffectBuffer> &srcEffectBuffer, std::shared_ptr<EffectBuffer> &dstEffectBuffer,
        std::map<ConfigType, Plugin::Any> &config, std::shared_ptr<EffectContext> &effectContext)
        : srcEffectBuffer_(std::move(srcEffectBuffer)),
          dstEffectBuffer_(std::move(dstEffectBuffer)),
          config_(std::move(config)),
          effectContext_(std::move(effectContext)) {};
    std::shared_ptr<EffectBuffer> &&srcEffectBuffer_;
    std::shared_ptr<EffectBuffer> &&dstEffectBuffer_;
    std::map<ConfigType, Plugin::Any> &&config_;
    std::shared_ptr<EffectContext> &&effectContext_;
};

enum class RunningType : int32_t {
    DEFAULT = 0,
    FOREGROUND = 1,
    BACKGROUND = 2,
};

const std::vector<std::string> priorityEFilter_ = {
    "Crop"
};
const std::unordered_map<std::string, ConfigType> configTypeTab_ = {
    { "runningType", ConfigType::IPTYPE },
};
const std::unordered_map<int32_t, std::vector<IPType>> runningTypeTab_{
    { std::underlying_type<RunningType>::type(RunningType::FOREGROUND), { IPType::CPU, IPType::GPU } },
    { std::underlying_type<RunningType>::type(RunningType::BACKGROUND), { IPType::CPU } },
};

ImageEffect::ImageEffect(const char *name)
{
    impl_ = std::make_shared<Impl>();
    if (name != nullptr) {
        name_ = name;
    }
    ExternLoader::Instance()->InitExt();
    ExtInitModule();

    if (m_renderThread == nullptr) {
        auto func = [this]() {
        };
        m_renderThread = new RenderThread<>(RENDER_QUEUE_SIZE, func);
        m_renderThread->Start();
        auto task = std::make_shared<RenderTask<>>([this]() { this->InitEGLEnv(); }, COMMON_TASK_TAG,
            RequestTaskId());
        m_renderThread->AddTask(task);
        task->Wait();
    }
}

ImageEffect::~ImageEffect()
{
    EFFECT_LOGI("ImageEffect destruct!");
    auto task = std::make_shared<RenderTask<>>([this]() { this->DestroyEGLEnv(); }, COMMON_TASK_TAG,
        RequestTaskId());
    m_renderThread->AddTask(task);
    task->Wait();
    ExtDeinitModule();

    impl_->surfaceAdapter_ = nullptr;
    impl_->effectContext_->renderEnvironment_ = nullptr;
    toProducerSurface_ = nullptr;
    fromProducerSurface_ = nullptr;
    m_renderThread->Stop();
}

void ImageEffect::AddEFilter(const std::shared_ptr<EFilter> &efilter)
{
    auto priorityEFilter = std::find_if(priorityEFilter_.begin(), priorityEFilter_.end(),
        [&efilter](const std::string &name) { return name.compare(efilter->GetName()) == 0; });
    if (priorityEFilter == priorityEFilter_.end()) {
        efilters_.emplace_back(efilter);
    } else {
        auto result =
            std::find_if(efilters_.rbegin(), efilters_.rend(), [&priorityEFilter](std::shared_ptr<EFilter> &efilter) {
                return priorityEFilter->compare(efilter->GetName()) == 0;
            });
        if (result == efilters_.rend()) {
            efilters_.insert(efilters_.begin(), efilter);
        } else {
            efilters_.insert(result.base(), efilter);
        }
    }

    impl_->CreatePipeline(efilters_);
}

ErrorCode ImageEffect::InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index)
{
    ErrorCode res = Effect::InsertEFilter(efilter, index);
    if (res == ErrorCode::SUCCESS) {
        impl_->CreatePipeline(efilters_);
    }
    return res;
}

void ImageEffect::RemoveEFilter(const std::shared_ptr<EFilter> &efilter)
{
    Effect::RemoveEFilter(efilter);
    impl_->CreatePipeline(efilters_);
}

ErrorCode ImageEffect::RemoveEFilter(uint32_t index)
{
    ErrorCode res = Effect::RemoveEFilter(index);
    if (res == ErrorCode::SUCCESS) {
        impl_->CreatePipeline(efilters_);
    }
    return res;
}

ErrorCode ImageEffect::ReplaceEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index)
{
    std::unique_lock<std::mutex> lock(bufferAvailableMutex_);
    ErrorCode res = Effect::ReplaceEFilter(efilter, index);
    if (res == ErrorCode::SUCCESS) {
        impl_->CreatePipeline(efilters_);
    }
    return res;
}

unsigned long int ImageEffect::RequestTaskId()
{
    return m_currentTaskId.fetch_add(1);
}

ErrorCode ImageEffect::SetInputPixelMap(PixelMap* pixelMap)
{
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_INVALID_SRC_PIXELMAP, "invalid source pixelMap");

    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::PIXEL_MAP;
    inDateInfo_.pixelMap_ = pixelMap;
    return ErrorCode::SUCCESS;
}

ErrorCode ConfigSourceFilter(std::shared_ptr<ImageSourceFilter> &srcFilter, std::shared_ptr<EffectBuffer> &srcBuffer,
    std::shared_ptr<EffectContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(srcFilter != nullptr, ErrorCode::ERR_INPUT_NULL, "srcFilter is null");

    ErrorCode res = srcFilter->SetSource(srcBuffer, context);
    FALSE_RETURN_MSG_E(res == ErrorCode::SUCCESS, res, "set source fail! res=%{public}d", res);

    return ErrorCode::SUCCESS;
}

ErrorCode ConfigSinkFilter(std::shared_ptr<ImageSinkFilter> &sinkFilter, std::shared_ptr<EffectBuffer> &sinkBuffer)
{
    CHECK_AND_RETURN_RET_LOG(sinkFilter != nullptr, ErrorCode::ERR_INPUT_NULL, "sinkFilter is null");

    ErrorCode res = sinkFilter->SetSink(sinkBuffer);
    FALSE_RETURN_MSG_E(res == ErrorCode::SUCCESS, res, "set sink fail! res=%{public}d", res);

    return ErrorCode::SUCCESS;
}

void GetConfigIPTypes(const std::map<ConfigType, Plugin::Any> &config, std::vector<IPType> &configIPTypes)
{
    auto it = config.find(ConfigType::IPTYPE);
    if (it == config.end()) {
        EFFECT_LOGE("ipType config not set! use default config.");
        configIPTypes = { IPType::CPU, IPType::GPU };
        return;
    }

    ErrorCode result = CommonUtils::ParseAny(it->second, configIPTypes);
    if (result == ErrorCode::SUCCESS) {
        return;
    }
    EFFECT_LOGE("parse ipType fail! use default config.");
    configIPTypes = { IPType::CPU, IPType::GPU };
}

ErrorCode ChooseIPType(const std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    const std::shared_ptr<EffectContext> &context, const std::map<ConfigType, Plugin::Any> &config,
    IPType &runningIPType)
{
    std::vector<IPType> configIPTypes;
    GetConfigIPTypes(config, configIPTypes);

    runningIPType = IPType::DEFAULT;
    IPType priorityIPType = IPType::GPU;
    IEffectFormat effectFormat = srcEffectBuffer->bufferInfo_->formatType_;
    const std::vector<std::shared_ptr<Capability>> &caps = context->capNegotiate_->GetCapabilityList();
    for (const auto &capability : caps) {
        if (capability == nullptr || capability->pixelFormatCap_ == nullptr) {
            continue;
        }
        std::map<IEffectFormat, std::vector<IPType>> &formats = capability->pixelFormatCap_->formats;
        if (runningIPType != IPType::CPU) {
            effectFormat = IEffectFormat::RGBA8888;
        }

        auto it = formats.find(effectFormat);
        if (it == formats.end()) {
            EFFECT_LOGE("effectFormat not support! effectFormat=%{public}d, name=%{public}s",
                effectFormat, capability->name_.c_str());
            return ErrorCode::SUCCESS;
        }

        std::vector<IPType> &ipTypes = it->second;
        if (std::find(configIPTypes.begin(), configIPTypes.end(), priorityIPType) != configIPTypes.end() &&
            std::find(ipTypes.begin(), ipTypes.end(), priorityIPType) != ipTypes.end()) {
            runningIPType = IPType::GPU;
        } else {
            runningIPType = IPType::CPU;
            return ErrorCode::SUCCESS;
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorCode StartPipelineInner(std::shared_ptr<PipelineCore> &pipeline, EffectParameters &effectParameters,
    unsigned long int taskId, RenderThread<> *thread)
{
    auto prom = std::make_shared<std::promise<ErrorCode>>();
    std::future<ErrorCode> fut = prom->get_future();
    auto task = std::make_shared<RenderTask<>>([pipeline, &effectParameters, &prom]() {
        ErrorCode res = pipeline->Prepare();
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("pipeline Prepare fail! res=%{public}d", res);
            prom->set_value(res);
            return;
        }

        res = ColorSpaceHelper::ConvertColorSpace(effectParameters.srcEffectBuffer_, effectParameters.effectContext_);
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("StartPipelineInner:ConvertColorSpace fail! res=%{public}d", res);
            prom->set_value(res);
            return;
        }

        IPType runningIPType;
        res = ChooseIPType(effectParameters.srcEffectBuffer_, effectParameters.effectContext_, effectParameters.config_,
            runningIPType);
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("choose running ip type fail! res=%{public}d", res);
            prom->set_value(res);
            return;
        }
        effectParameters.effectContext_->ipType_ = runningIPType;
        effectParameters.effectContext_->memoryManager_->SetIPType(runningIPType);

        res = pipeline->Start();
        prom->set_value(res);
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("pipeline start fail! res=%{public}d", res);
            return;
        }
    }, 0, taskId);
    thread->AddTask(task);
    task->Wait();
    ErrorCode res = fut.get();
    return res;
}

ErrorCode StartPipeline(std::shared_ptr<PipelineCore> &pipeline, EffectParameters &effectParameters,
    unsigned long int taskId, RenderThread<> *thread)
{
    effectParameters.effectContext_->renderStrategy_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    effectParameters.effectContext_->colorSpaceManager_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    effectParameters.effectContext_->memoryManager_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    ErrorCode res = StartPipelineInner(pipeline, effectParameters, taskId, thread);
    effectParameters.effectContext_->memoryManager_->Deinit();
    effectParameters.effectContext_->colorSpaceManager_->Deinit();
    effectParameters.effectContext_->renderStrategy_->Deinit();
    effectParameters.effectContext_->capNegotiate_->ClearNegotiateResult();
    return res;
}

ErrorCode ImageEffect::Start()
{
    switch (inDateInfo_.dataType_) {
        case DataType::PIXEL_MAP:
        case DataType::SURFACE_BUFFER:
        case DataType::URI:
        case DataType::PATH: {
            impl_->effectState_ = EffectState::RUNNING;
            ErrorCode res = this->Render();
            Stop();
            return res;
        }
        case DataType::SURFACE:
            impl_->effectState_ = EffectState::RUNNING;
            break;
        default:
            EFFECT_LOGE("Not set input data!");
            return ErrorCode::ERR_NOT_SET_INPUT_DATA;
    }
    return ErrorCode::SUCCESS;
}

void ImageEffect::Stop()
{
    std::unique_lock<std::mutex> lock(bufferAvailableMutex_);
    impl_->effectState_ = EffectState::IDLE;
    impl_->effectContext_->memoryManager_->ClearMemory();
}

ErrorCode ImageEffect::SetInputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer)
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, ErrorCode::ERR_INVALID_SRC_SURFACEBUFFER,
        "invalid source surface buffer");

    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::SURFACE_BUFFER;
    inDateInfo_.surfaceBufferInfo_.surfaceBuffer_ = surfaceBuffer;

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer)
{
    ClearDataInfo(outDateInfo_);
    if (surfaceBuffer == nullptr) {
        EFFECT_LOGI("SetOutputSurfaceBuffer: surfaceBuffer set to null!");
        return ErrorCode::SUCCESS;
    }

    outDateInfo_.dataType_ = DataType::SURFACE_BUFFER;
    outDateInfo_.surfaceBufferInfo_.surfaceBuffer_ = surfaceBuffer;

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetInputUri(const std::string &uri)
{
    if (!CommonUtils::EndsWithJPG(uri)) {
        EFFECT_LOGE("SetInputUri: file type is not support! only support jpg/jpeg.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::URI;
    inDateInfo_.uri_ = std::move(uri);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputUri(const std::string &uri)
{
    if (uri.empty()) {
        EFFECT_LOGI("SetOutputUri: uri set to null!");
        ClearDataInfo(outDateInfo_);
        return ErrorCode::SUCCESS;
    }

    if (!CommonUtils::EndsWithJPG(uri)) {
        EFFECT_LOGE("SetOutputUri: file type is not support! only support jpg/jpeg.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(outDateInfo_);
    outDateInfo_.dataType_ = DataType::URI;
    outDateInfo_.uri_ = std::move(uri);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetInputPath(const std::string &path)
{
    if (!CommonUtils::EndsWithJPG(path)) {
        EFFECT_LOGE("SetInputPath: file type is not support! only support jpg/jpeg.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::PATH;
    inDateInfo_.path_ = std::move(path);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputPath(const std::string &path)
{
    if (path.empty()) {
        EFFECT_LOGI("SetOutputPath: path set to null!");
        ClearDataInfo(outDateInfo_);
        return ErrorCode::SUCCESS;
    }

    if (!CommonUtils::EndsWithJPG(path)) {
        EFFECT_LOGE("SetOutputPath: file type is not support! only support jpg/jpeg.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(outDateInfo_);
    outDateInfo_.dataType_ = DataType::PATH;
    outDateInfo_.path_ = std::move(path);

    return ErrorCode::SUCCESS;
}

ErrorCode CheckToRenderPara(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer)
{
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer != nullptr, ErrorCode::ERR_PARSE_FOR_EFFECT_BUFFER_FAIL,
        "invalid srcEffectBuffer");

    // allow developers not to set the out parameter.
    if (dstEffectBuffer == nullptr) {
        return ErrorCode::SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->bufferInfo_ != nullptr && dstEffectBuffer->bufferInfo_ != nullptr,
        ErrorCode::ERR_BUFFER_INFO_NULL,
        "buffer info is null! srcBufferInfo=%{public}d, dstBufferInfo=%{public}d",
        srcEffectBuffer->bufferInfo_ == nullptr, dstEffectBuffer->bufferInfo_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->extraInfo_ != nullptr && dstEffectBuffer->extraInfo_ != nullptr,
        ErrorCode::ERR_EXTRA_INFO_NULL,
        "extra info is null! srcExtraInfo=%{public}d, dstExtraInfo=%{public}d",
        srcEffectBuffer->extraInfo_ == nullptr, dstEffectBuffer->extraInfo_ == nullptr);

    // input and output type is same or not.
    DataType srcDataType = srcEffectBuffer->extraInfo_->dataType;
    DataType dtsDataType = dstEffectBuffer->extraInfo_->dataType;
    CHECK_AND_RETURN_RET_LOG(
        srcDataType == dtsDataType || (srcDataType == DataType::PIXEL_MAP && dtsDataType == DataType::NATIVE_WINDOW),
        ErrorCode::ERR_NOT_SUPPORT_DIFF_DATATYPE,
        "not supported dataType. srcDataType=%{public}d, dstDataType=%{public}d", srcDataType, dtsDataType);

    // color space is same or not.
    if (srcDataType == DataType::PIXEL_MAP) {
        // the format for pixel map is same or not.
        CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->bufferInfo_->formatType_ == dstEffectBuffer->bufferInfo_->formatType_,
            ErrorCode::ERR_NOT_SUPPORT_DIFF_FORMAT,
            "not support different format, srcFormat=%{public}d, dstFormat=%{public}d",
            srcEffectBuffer->bufferInfo_->formatType_, dstEffectBuffer->bufferInfo_->formatType_);

        // color space is same or not.
        EffectColorSpace srcColorSpace = srcEffectBuffer->bufferInfo_->colorSpace_;
        EffectColorSpace dstColorSpace = dstEffectBuffer->bufferInfo_->colorSpace_;
        bool isSrcHdr = ColorSpaceHelper::IsHdrColorSpace(srcColorSpace);
        bool isDstHdr = ColorSpaceHelper::IsHdrColorSpace(dstColorSpace);
        CHECK_AND_RETURN_RET_LOG(isSrcHdr == isDstHdr, ErrorCode::ERR_NOT_SUPPORT_INPUT_OUTPUT_COLORSPACE,
            "not support different colorspace. src=%{public}d, dst=%{public}d", srcColorSpace, dstColorSpace);
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::Render()
{
    if (efilters_.empty()) {
        EFFECT_LOGW("efilters is empty!");
        return ErrorCode::ERR_NOT_FILTERS_WITH_RENDER;
    }

    std::shared_ptr<EffectBuffer> srcEffectBuffer = nullptr;
    std::shared_ptr<EffectBuffer> dstEffectBuffer = nullptr;
    ErrorCode res = LockAll(srcEffectBuffer, dstEffectBuffer);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    res = CheckToRenderPara(srcEffectBuffer, dstEffectBuffer);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    std::shared_ptr<ImageSourceFilter> &sourceFilter = impl_->srcFilter_;
    res = ConfigSourceFilter(sourceFilter, srcEffectBuffer, impl_->effectContext_);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    std::shared_ptr<ImageSinkFilter> &sinkFilter = impl_->sinkFilter_;
    res = ConfigSinkFilter(sinkFilter, dstEffectBuffer);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }
    if (dstEffectBuffer != nullptr) {
        impl_->effectContext_->renderEnvironment_->SetOutputType(dstEffectBuffer->extraInfo_->dataType);
    } else {
        impl_->effectContext_->renderEnvironment_->SetOutputType(srcEffectBuffer->extraInfo_->dataType);
    }

    EffectParameters effectParameters(srcEffectBuffer, dstEffectBuffer, config_, impl_->effectContext_);
    res = StartPipeline(impl_->pipeline_, effectParameters, RequestTaskId(), m_renderThread);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("StartPipeline fail! res=%{public}d", res);
        UnLockAll();
        return res;
    }

    UnLockAll();
    return res;
}

ErrorCode ImageEffect::Save(EffectJsonPtr &res)
{
    EffectJsonPtr effect = JsonHelper::CreateArray();
    for (auto it = efilters_.begin(); it != efilters_.end(); it++) {
        EffectJsonPtr data = JsonHelper::CreateObject();
        std::shared_ptr<EFilter> efilter = *it;
        efilter->Save(data);
        effect->Add(data);
    }

    EffectJsonPtr info  = JsonHelper::CreateObject();
    info->Put("filters", effect);
    info->Put("name", name_);
    if (extraInfo_ != nullptr) {
        extraInfo_->Replace("imageEffect", info);
        res = extraInfo_;
    } else {
        res->Put("imageEffect", info);
    }
    return ErrorCode::SUCCESS;
}

std::shared_ptr<ImageEffect> ImageEffect::Restore(std::string &info)
{
    const EffectJsonPtr root = JsonHelper::ParseJsonData(info);
    CHECK_AND_RETURN_RET_LOG(root->HasElement("imageEffect"), nullptr, "Restore: no imageEffect");
    const EffectJsonPtr &imageInfo = root->GetElement("imageEffect");
    CHECK_AND_RETURN_RET_LOG(imageInfo->HasElement("name"), nullptr, "Restore: imageEffect no name");
    std::string effectName = imageInfo->GetString("name");
    CHECK_AND_RETURN_RET_LOG(!effectName.empty(), nullptr, "Restore: imageEffect get name failed");

    CHECK_AND_RETURN_RET_LOG(imageInfo->HasElement("filters"), nullptr, "Restore: imageEffect no filters");
    std::vector<EffectJsonPtr> efiltersInfo = imageInfo->GetArray("filters");
    CHECK_AND_RETURN_RET_LOG(!efiltersInfo.empty(), nullptr, "Restore: filters not array");

    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(effectName.c_str());
    for (auto &efilterInfo : efiltersInfo) {
        std::string name = efilterInfo->GetString("name");
        CHECK_AND_CONTINUE_LOG(!name.empty(), "Restore: [name] not exist");
        std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Restore(name, efilterInfo, nullptr);
        imageEffect->AddEFilter(efilter);
    }
    return imageEffect;
}

ErrorCode ImageEffect::SetOutputPixelMap(PixelMap* pixelMap)
{
    ClearDataInfo(outDateInfo_);
    if (pixelMap == nullptr) {
        EFFECT_LOGI("SetOutputPixelMap: pixelMap set to null!");
        return ErrorCode::SUCCESS;
    }

    outDateInfo_.dataType_ = DataType::PIXEL_MAP;
    outDateInfo_.pixelMap_ = pixelMap;

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputSurface(sptr<Surface>& surface)
{
    if (surface == nullptr) {
        EFFECT_LOGE("surface is null.");
        return ErrorCode::ERR_INPUT_NULL;
    }
    outDateInfo_.dataType_ = DataType::SURFACE;
    toProducerSurface_ = surface;
    return ErrorCode::SUCCESS;
}

void ImageEffect::UpdateProducerSurfaceInfo()
{
    if (impl_->surfaceAdapter_ == nullptr) {
        EFFECT_LOGE("impl surfaceAdapter is nullptr!");
        return;
    }
    auto transform = impl_->surfaceAdapter_->GetTransform();
    if (transform == toProducerTransform_) {
        return;
    }
    toProducerTransform_ = transform;
    EFFECT_LOGI("Set toProducerSurface transform %{public}d, GRAPHIC_ROTATE_270: %{public}d",
        transform, GRAPHIC_ROTATE_270);

    if (toProducerSurface_ == nullptr) {
        EFFECT_LOGE("toProducerSurface_ is nullptr!");
        return;
    }
    toProducerSurface_->SetTransform(transform);
}

void ImageEffect::ConsumerBufferWithGPU(sptr<SurfaceBuffer>& buffer)
{
    inDateInfo_.surfaceBufferInfo_.surfaceBuffer_ = buffer;
    GraphicTransformType transform = impl_->surfaceAdapter_->GetTransform();
    buffer->SetSurfaceBufferTransform(transform);
    if (impl_->effectState_ == EffectState::RUNNING) {
        impl_->effectContext_->renderEnvironment_->NotifyInputChanged();
        this->Render();
    } else {
        auto task = std::make_shared<RenderTask<>>([buffer, this, transform]() {
            if (impl_->effectContext_->renderEnvironment_->GetEGLStatus() != EGLStatus::READY) {
                impl_->effectContext_->renderEnvironment_->Init();
                impl_->effectContext_->renderEnvironment_->Prepare();
            }
            int tex = GLUtils::CreateTextureFromSurfaceBuffer(buffer);
            impl_->effectContext_->renderEnvironment_->UpdateCanvas();
            impl_->effectContext_->renderEnvironment_->DrawFrame(tex, transform);
        }, COMMON_TASK_TAG, RequestTaskId());
        m_renderThread->AddTask(task);
        task->Wait();
    }
}

void MemoryCopyForSurfaceBuffer(sptr<SurfaceBuffer> &buffer, OHOS::sptr<SurfaceBuffer> outBuffer)
{
    CopyInfo src = {
        .bufferInfo = {
            .width_ = static_cast<uint32_t>(buffer->GetWidth()),
            .height_ = static_cast<uint32_t>(buffer->GetHeight()),
            .len_ = buffer->GetSize(),
            .formatType_ = CommonUtils::SwitchToEffectFormat((GraphicPixelFormat)buffer->GetFormat()),
            .rowStride_ = static_cast<uint32_t>(buffer->GetStride()),
        },
        .data = static_cast<uint8_t *>(buffer->GetVirAddr()),
    };
    CopyInfo dst = {
        .bufferInfo = {
            .width_ = static_cast<uint32_t>(outBuffer->GetWidth()),
            .height_ = static_cast<uint32_t>(outBuffer->GetHeight()),
            .len_ = outBuffer->GetSize(),
            .formatType_ = CommonUtils::SwitchToEffectFormat((GraphicPixelFormat)outBuffer->GetFormat()),
            .rowStride_ = static_cast<uint32_t>(outBuffer->GetStride()),
        },
        .data = static_cast<uint8_t *>(outBuffer->GetVirAddr()),
    };
    MemcpyHelper::CopyData(src, dst);
}

void ImageEffect::ConsumerBufferAvailable(sptr<SurfaceBuffer>& buffer, const OHOS::Rect& damages, int64_t timestamp)
{
    std::unique_lock<std::mutex> lock(bufferAvailableMutex_);
    if (outDateInfo_.dataType_ == DataType::NATIVE_WINDOW) {
        ConsumerBufferWithGPU(buffer);
        return;
    }
    UpdateProducerSurfaceInfo();

    OHOS::sptr<SurfaceBuffer> outBuffer;
    sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;

    BufferRequestConfig requestConfig = {
        .width = buffer->GetWidth(),
        .height = buffer->GetHeight(),
        .strideAlignment = 0x8, // default stride is 8 Bytes.
        .format = buffer->GetFormat(),
        .usage = buffer->GetUsage(),
        .timeout = 0,
        .colorGamut = buffer->GetSurfaceBufferColorGamut(),
        .transform = buffer->GetSurfaceBufferTransform(),
    };

    auto ret = toProducerSurface_->RequestBuffer(outBuffer, syncFence, requestConfig);
    CHECK_AND_RETURN_LOG(ret == 0, "RequestBuffer failed. %{public}d", ret);

    constexpr uint32_t waitForEver = -1;
    (void)syncFence->Wait(waitForEver);

    bool isNeedCpy = true;
    if (impl_->effectState_ == EffectState::RUNNING) {
        inDateInfo_.surfaceBufferInfo_ = {
            .surfaceBuffer_ = buffer,
            .timestamp_ = timestamp,
        };
        outDateInfo_.surfaceBufferInfo_ = {
            .surfaceBuffer_ = outBuffer,
            .timestamp_ = timestamp,
        };
        ErrorCode res = this->Render();
        isNeedCpy = (res != ErrorCode::SUCCESS);
    }

    if (isNeedCpy) {
        MemoryCopyForSurfaceBuffer(buffer, outBuffer);
    }

    BufferFlushConfig flushConfig = {
        .damage = {
            .w = requestConfig.width,
            .h = requestConfig.height,
        },
        .timestamp = timestamp,
    };
    constexpr int32_t invalidFence = -1;
    (void)toProducerSurface_->FlushBuffer(outBuffer, invalidFence, flushConfig);
}

sptr<Surface> ImageEffect::GetInputSurface()
{
    inDateInfo_.dataType_ = DataType::SURFACE;
    if (fromProducerSurface_ != nullptr) {
        return fromProducerSurface_;
    }

    if (impl_->surfaceAdapter_ == nullptr) {
        impl_->surfaceAdapter_ = std::make_unique<EffectSurfaceAdapter>();
    }

    if (impl_->surfaceAdapter_) {
        fromProducerSurface_ = impl_->surfaceAdapter_->GetProducerSurface();
    }

    auto consumerListener = [this](sptr<SurfaceBuffer>& buffer, const OHOS::Rect& damages, int64_t timestamp) {
        ConsumerBufferAvailable(buffer, damages, timestamp);
    };

    if (impl_->surfaceAdapter_) {
        impl_->surfaceAdapter_->SetConsumerListener(std::move(consumerListener));
    }

    return fromProducerSurface_;
}

ErrorCode ImageEffect::SetOutNativeWindow(OHNativeWindow *nativeWindow)
{
    outDateInfo_.dataType_ = DataType::NATIVE_WINDOW;
    impl_->effectContext_->renderEnvironment_->InitEngine(nativeWindow);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::Configure(const std::string &key, const Plugin::Any &value)
{
    auto configTypeIt = std::find_if(configTypeTab_.begin(), configTypeTab_.end(),
        [&key](const std::pair<std::string, ConfigType> &item) { return item.first.compare(key) == 0; });

    CHECK_AND_RETURN_RET_LOG(configTypeIt != configTypeTab_.end(), ErrorCode::ERR_UNSUPPORTED_CONFIG_TYPE,
        "config key is not support! key=%{public}s", key.c_str());

    ConfigType configType = configTypeIt->second;
    switch (configType) {
        case ConfigType::IPTYPE: {
            int32_t runningType;
            ErrorCode result = CommonUtils::ParseAny(value, runningType);
            CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, result,
                "parse any fail! expect type is uint32_t! key=%{public}s", key.c_str());
            auto it = std::find_if(runningTypeTab_.begin(), runningTypeTab_.end(),
                [&runningType](const std::pair<int32_t, std::vector<IPType>> &item) {
                    return item.first == runningType;
                });
            CHECK_AND_RETURN_RET_LOG(it != runningTypeTab_.end(), ErrorCode::ERR_UNSUPPORTED_RUNNINGTYPE,
                "not support runningType! key=%{public}s, runningType=%{public}d", key.c_str(), runningType);
            config_[configType] = it->second;
            break;
        }
        default:
            EFFECT_LOGE("config type is not support! configType=%{public}d", configType);
            return ErrorCode::ERR_UNSUPPORTED_CONFIG_TYPE;
    }
    return ErrorCode::SUCCESS;
}

void ImageEffect::ClearDataInfo(DataInfo &dataInfo)
{
    dataInfo.dataType_ = DataType::UNKNOWN;
    dataInfo.pixelMap_ = nullptr;
    dataInfo.surfaceBufferInfo_.surfaceBuffer_ = nullptr;
    dataInfo.surfaceBufferInfo_.timestamp_ = 0;
    dataInfo.uri_ = "";
    dataInfo.path_ = "";
}

bool IsSameInOutputData(const DataInfo &inDataInfo, const DataInfo &outDataInfo)
{
    if (inDataInfo.dataType_ != outDataInfo.dataType_) {
        return false;
    }

    switch (inDataInfo.dataType_) {
        case DataType::PIXEL_MAP:
            return inDataInfo.pixelMap_ == outDataInfo.pixelMap_;
        case DataType::SURFACE_BUFFER:
            return inDataInfo.surfaceBufferInfo_.surfaceBuffer_ == outDataInfo.surfaceBufferInfo_.surfaceBuffer_;
        case DataType::PATH:
            return inDataInfo.path_ == outDataInfo.path_;
        case DataType::URI:
            return inDataInfo.uri_ == outDataInfo.uri_;
        default:
            return false;
    }
}

ErrorCode ImageEffect::LockAll(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer)
{
    ErrorCode res = ParseDataInfo(inDateInfo_, srcEffectBuffer, false);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("ParseDataInfo inData fail! res=%{public}d", res);
        return res;
    }
    EFFECT_LOGI("input data set, parse data info success! dataType=%{public}d", inDateInfo_.dataType_);

    if (outDateInfo_.dataType_ != DataType::UNKNOWN && !IsSameInOutputData(inDateInfo_, outDateInfo_)) {
        EFFECT_LOGI("output data set, start parse data info. dataType=%{public}d", outDateInfo_.dataType_);
        res = ParseDataInfo(outDateInfo_, dstEffectBuffer, true);
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("ParseDataInfo outData fail! res=%{public}d", res);
            return res;
        }
        EFFECT_LOGI("output data set, parse data info success! dataType=%{public}d", outDateInfo_.dataType_);
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::ParseDataInfo(DataInfo &dataInfo, std::shared_ptr<EffectBuffer> &effectBuffer,
    bool isOutputData)
{
    switch (dataInfo.dataType_) {
        case DataType::PIXEL_MAP:
            return CommonUtils::LockPixelMap(dataInfo.pixelMap_, effectBuffer);
        case DataType::SURFACE:
        case DataType::SURFACE_BUFFER:
            return CommonUtils::ParseSurfaceData(dataInfo.surfaceBufferInfo_.surfaceBuffer_, effectBuffer,
                dataInfo.dataType_, dataInfo.surfaceBufferInfo_.timestamp_);
        case DataType::URI:
            return CommonUtils::ParseUri(dataInfo.uri_, effectBuffer, isOutputData);
        case DataType::PATH:
            return CommonUtils::ParsePath(dataInfo.path_, effectBuffer, isOutputData);
        case DataType::NATIVE_WINDOW:
            return CommonUtils::ParseNativeWindowData(effectBuffer, dataInfo.dataType_);
        case DataType::UNKNOWN:
            EFFECT_LOGW("dataType is unknown! Data is not set!");
            return ErrorCode::ERR_NO_DATA;
        default:
            EFFECT_LOGW("dataType is not support! dataType=%{public}d", dataInfo.dataType_);
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
}

void ImageEffect::UnLockAll()
{
    UnLockData(inDateInfo_);
    UnLockData(outDateInfo_);
}

void ImageEffect::UnLockData(DataInfo &dataInfo)
{
    switch (dataInfo.dataType_) {
        case DataType::PIXEL_MAP: {
            CommonUtils::UnlockPixelMap(dataInfo.pixelMap_);
            return;
        }
        default:
            return;
    }
}

void ImageEffect::InitEGLEnv()
{
    impl_->effectContext_->renderEnvironment_->Init();
    impl_->effectContext_->renderEnvironment_->Prepare();
}

void ImageEffect::DestroyEGLEnv()
{
    if (impl_->effectContext_->renderEnvironment_ == nullptr) {
        return;
    }
    impl_->effectContext_->renderEnvironment_->ReleaseParam();
    impl_->effectContext_->renderEnvironment_->Release();
}

ErrorCode ImageEffect::SetExtraInfo(EffectJsonPtr res)
{
    extraInfo_ = res;
    return ErrorCode::SUCCESS;
}

void ImageEffect::ExtInitModule()
{
}

void ImageEffect::ExtDeinitModule()
{
}
} // namespace Effect
} // namespace Media
} // namespace OHOS