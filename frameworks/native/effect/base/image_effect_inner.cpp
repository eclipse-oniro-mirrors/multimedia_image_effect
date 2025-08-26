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
#include <thread>

#include "qos.h"
#include "metadata_helper.h"
#include "common_utils.h"
#include "filter_factory.h"
#include "image_sink_filter.h"
#include "image_source_filter.h"
#include "effect_surface_adapter.h"
#include "pipeline_core.h"
#include "effect_json_helper.h"
#include "efilter_factory.h"
#include "external_loader.h"
#include "effect_context.h"
#include "colorspace_helper.h"
#include "memcpy_helper.h"

#include "v1_1/buffer_handle_meta_key_type.h"
#include "effect_log.h"
#include "effect_trace.h"
#include "render_task.h"
#include "render_environment.h"
#include "native_window.h"
#include "image_source.h"
#include "capability_negotiate.h"

#define RENDER_QUEUE_SIZE 8
#define COMMON_TASK_TAG 0
namespace OHOS {
namespace Media {
namespace Effect {
using namespace OHOS::HDI::Display::Graphic::Common;

enum class EffectState {
    IDLE,
    RUNNING,
};

const int STRUCT_IMAGE_EFFECT_CONSTANT = 1;
const int DESTRUCTOR_IMAGE_EFFECT_CONSTANT = 2;
const std::string FUNCTION_FLUSH_SURFACE_BUFFER = "flushSurfaceBuffer";

class ImageEffect::Impl {
public:
    Impl()
    {
        InitPipeline();
        InitEffectContext();
    }

    void CreatePipeline(std::vector<std::shared_ptr<EFilter>> &efilters);

    bool CheckEffectSurface() const;
    sptr<IConsumerSurface> GetConsumerSurface() const;
    GSError AcquireConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& syncFence,
        int64_t& timestamp, OHOS::Rect& damages) const;
    GSError ReleaseConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& syncFence) const;
    GSError DetachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;
    GSError AttachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const;
private:
    void InitPipeline();
    void InitEffectContext();

public:
    sptr<EffectSurfaceAdapter> surfaceAdapter_;
    std::shared_ptr<PipelineCore> pipeline_;
    std::shared_ptr<ImageSourceFilter> srcFilter_;
    std::shared_ptr<ImageSinkFilter> sinkFilter_;
    std::shared_ptr<EffectContext> effectContext_;
    EffectState effectState_ = EffectState::IDLE;
    bool isQosEnabled_ = false;
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
    effectContext_->cacheNegotiate_ = std::make_shared<EFilterCacheNegotiate>();
    effectContext_->metaInfoNegotiate_ = std::make_shared<EfilterMetaInfoNegotiate>();
}

void ImageEffect::Impl::CreatePipeline(std::vector<std::shared_ptr<EFilter>> &efilters)
{
    pipeline_ = std::make_shared<PipelineCore>();
    pipeline_->Init(nullptr);

    CHECK_AND_RETURN_LOG(srcFilter_ != nullptr, "srcFilter is null");
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

bool ImageEffect::Impl::CheckEffectSurface() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_ != nullptr, false, "Impl::CheckEffectSurface: surfaceAdapter is nullptr");

    return surfaceAdapter_->CheckEffectSurface();
}

sptr<IConsumerSurface> ImageEffect::Impl::GetConsumerSurface() const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_, nullptr, "Impl::GetConsumerSurface: surfaceAdapter is nullptr");

    return surfaceAdapter_->GetConsumerSurface();
}

GSError ImageEffect::Impl::AcquireConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& syncFence,
    int64_t& timestamp, OHOS::Rect& damages) const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_ != nullptr, GSERROR_NOT_INIT,
        "Impl::AcquireConsumerSurfaceBuffer: surfaceAdapter is nullptr");

    return surfaceAdapter_->AcquireConsumerSurfaceBuffer(buffer, syncFence, timestamp, damages);
}

GSError ImageEffect::Impl::ReleaseConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer,
    const sptr<SyncFence>& syncFence) const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_ != nullptr, GSERROR_NOT_INIT,
        "Impl::ReleaseConsumerSurfaceBuffer: surfaceAdapter is nullptr");

    return surfaceAdapter_->ReleaseConsumerSurfaceBuffer(buffer, syncFence);
}

GSError ImageEffect::Impl::DetachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_ != nullptr, GSERROR_NOT_INIT,
        "Impl::DetachConsumerSurfaceBuffer: surfaceAdapter is nullptr");

    return surfaceAdapter_->DetachConsumerSurfaceBuffer(buffer);
}

GSError ImageEffect::Impl::AttachConsumerSurfaceBuffer(sptr<SurfaceBuffer>& buffer) const
{
    CHECK_AND_RETURN_RET_LOG(surfaceAdapter_ != nullptr, GSERROR_NOT_INIT,
        "Impl::AttachConsumerSurfaceBuffer: surfaceAdapter is nullptr");

    return surfaceAdapter_->AttachConsumerSurfaceBuffer(buffer);
}

struct EffectParameters {
    EffectParameters(std::shared_ptr<EffectBuffer> &srcEffectBuffer, std::shared_ptr<EffectBuffer> &dstEffectBuffer,
        std::map<ConfigType, Any> &config, std::shared_ptr<EffectContext> &effectContext)
        : srcEffectBuffer_(std::move(srcEffectBuffer)),
          dstEffectBuffer_(std::move(dstEffectBuffer)),
          config_(std::move(config)),
          effectContext_(std::move(effectContext)) {};
    std::shared_ptr<EffectBuffer> &&srcEffectBuffer_;
    std::shared_ptr<EffectBuffer> &&dstEffectBuffer_;
    std::map<ConfigType, Any> &&config_;
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
    imageEffectFlag_ = STRUCT_IMAGE_EFFECT_CONSTANT;
    impl_ = std::make_shared<Impl>();
    if (name != nullptr) {
        name_ = name;
    }
    ExternLoader::Instance()->InitExt();
    ExtInitModule();

    if (m_renderThread == nullptr) {
        auto func = [this]() {
            EFFECT_LOGD("ImageEffect has no render work to do!");
        };
        m_renderThread = new RenderThread<>(RENDER_QUEUE_SIZE, func);
        m_renderThread->Start();
        if (name != nullptr && strcmp(name, "Photo") == 0) {
            auto task = std::make_shared<RenderTask<>>([this]() { this->InitEGLEnv(); }, COMMON_TASK_TAG,
                RequestTaskId());
            m_renderThread->AddTask(task);
            task->Wait();
        }
    }
}

ImageEffect::~ImageEffect()
{
    EFFECT_LOGI("ImageEffect destruct enter!");
    if (failureCount_ > 0) {
        EFFECT_LOGE("ImageEffect::SwapBuffers attach fail %{public}d times", failureCount_);
    }
    imageEffectFlag_ = DESTRUCTOR_IMAGE_EFFECT_CONSTANT;
    if (impl_->surfaceAdapter_) {
        impl_->surfaceAdapter_->Destroy();
    }
    m_renderThread->ClearTask();
    auto task = std::make_shared<RenderTask<>>([this]() { this->DestroyEGLEnv(); }, COMMON_TASK_TAG,
        RequestTaskId());
    m_renderThread->AddTask(task);
    task->Wait();
    EFFECT_LOGI("ImageEffect destruct destroy egl env!");
    ExtDeinitModule();
    m_renderThread->Stop();
    delete m_renderThread;

    impl_->effectContext_->renderEnvironment_ = nullptr;
    if (toProducerSurface_) {
        auto res = toProducerSurface_->Disconnect();
        EFFECT_LOGI("ImageEffect::~ImageEffect disconnect res=%{public}d, id=%{public}" PRIu64,
            res, toProducerSurface_->GetUniqueId());
        toProducerSurface_ = nullptr;
    }
    fromProducerSurface_ = nullptr;
    impl_ = nullptr;
    EFFECT_LOGI("ImageEffect destruct end!");
}

void ImageEffect::AddEFilter(const std::shared_ptr<EFilter> &efilter)
{
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
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
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
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
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
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
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    EFFECT_LOGD("ImageEffect::SetInputPixelMap");
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_INVALID_SRC_PIXELMAP, "invalid source pixelMap");
    impl_->effectContext_->renderEnvironment_->NotifyInputChanged();

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

ErrorCode ConfigSinkFilter(std::shared_ptr<ImageSinkFilter> &sinkFilter, std::shared_ptr<EffectBuffer> &sinkBuffer,
    sptr<Surface> &toXComponentSurface)
{
    CHECK_AND_RETURN_RET_LOG(sinkFilter != nullptr, ErrorCode::ERR_INPUT_NULL, "sinkFilter is null");

    ErrorCode res = sinkFilter->SetSink(sinkBuffer);
    FALSE_RETURN_MSG_E(res == ErrorCode::SUCCESS, res, "set sink fail! res=%{public}d", res);
    res = sinkFilter->SetXComponentSurface(toXComponentSurface);
    FALSE_RETURN_MSG_E(res == ErrorCode::SUCCESS, res, "SetRenderSurface fail! res=%{public}d", res);

    return ErrorCode::SUCCESS;
}

void GetConfigIPTypes(const std::map<ConfigType, Any> &config, std::vector<IPType> &configIPTypes)
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

void AdjustEffectFormat(IEffectFormat& effectFormat) {
    switch (effectFormat) {
        case IEffectFormat::RGBA_1010102:
        case IEffectFormat::YCBCR_P010:
        case IEffectFormat::YCRCB_P010:
            effectFormat = IEffectFormat::RGBA_1010102;
            break;
        default:
            effectFormat = IEffectFormat::RGBA8888;
            break;
    }
}

ErrorCode ChooseIPType(const std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    const std::shared_ptr<EffectContext> &context, const std::map<ConfigType, Any> &config,
    IPType &runningIPType)
{
    std::vector<IPType> configIPTypes;
    GetConfigIPTypes(config, configIPTypes);
    bool isTextureInput = srcEffectBuffer->extraInfo_->dataType == DataType::TEX;

    runningIPType = isTextureInput ? IPType::GPU : IPType::DEFAULT;
    IPType priorityIPType = IPType::GPU;
    IEffectFormat effectFormat = srcEffectBuffer->bufferInfo_->formatType_;
    const std::vector<std::shared_ptr<Capability>> &caps = context->capNegotiate_->GetCapabilityList();
    for (const auto &capability : caps) {
        if (capability == nullptr || capability->pixelFormatCap_ == nullptr) {
            continue;
        }
        std::map<IEffectFormat, std::vector<IPType>> &formats = capability->pixelFormatCap_->formats;

        if (runningIPType == IPType::GPU && !isTextureInput) {
            AdjustEffectFormat(effectFormat);
        }

        auto it = formats.find(effectFormat);
        if (it == formats.end()) {
            EFFECT_LOGE("effectFormat not support! effectFormat=%{public}d, name=%{public}s",
                effectFormat, capability->name_.c_str());
            return isTextureInput ? ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE : ErrorCode::SUCCESS;
        }

        std::vector<IPType> &ipTypes = it->second;
        if (std::find(configIPTypes.begin(), configIPTypes.end(), priorityIPType) != configIPTypes.end() &&
            std::find(ipTypes.begin(), ipTypes.end(), priorityIPType) != ipTypes.end()) {
            runningIPType = IPType::GPU;
        } else {
            if (runningIPType == IPType::DEFAULT) {
                runningIPType = IPType::CPU;
            }
            return isTextureInput ? ErrorCode::ERR_UNSUPPORTED_IPTYPE_FOR_EFFECT : ErrorCode::SUCCESS;
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ProcessPipelineTask(std::shared_ptr<PipelineCore> pipeline, const EffectParameters &effectParameters)
{
    EFFECT_TRACE_NAME("ProcessPipelineTask");
    EFFECT_TRACE_BEGIN("ConvertColorSpace");
    ErrorCode res = ColorSpaceHelper::ConvertColorSpace(effectParameters.srcEffectBuffer_,
        effectParameters.effectContext_);
    EFFECT_TRACE_END();
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("ProcessPipelineTask:ConvertColorSpace fail! res=%{public}d", res);
        return res;
    }

    IPType runningIPType;
    res = ChooseIPType(effectParameters.srcEffectBuffer_, effectParameters.effectContext_, effectParameters.config_,
        runningIPType);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("choose running ip type fail! res=%{public}d", res);
        return res;
    }
    if (effectParameters.effectContext_->renderEnvironment_->GetEGLStatus() != EGLStatus::READY
        && runningIPType == IPType::GPU) {
        bool isCustomEnv = effectParameters.srcEffectBuffer_->extraInfo_->dataType == DataType::TEX;
        effectParameters.effectContext_->renderEnvironment_->Init(isCustomEnv);
        effectParameters.effectContext_->renderEnvironment_->Prepare();
    }
    effectParameters.effectContext_->ipType_ = runningIPType;
    effectParameters.effectContext_->memoryManager_->SetIPType(runningIPType);

    res = pipeline->Start();
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("pipeline start fail! res=%{public}d", res);
        return res;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode StartPipelineInner(std::shared_ptr<PipelineCore> &pipeline, const EffectParameters &effectParameters,
    unsigned long int taskId, RenderThread<> *thread, bool isNeedCreateThread = false)
{
    if (thread == nullptr) {
        EFFECT_LOGE("pipeline Prepare fail! render thread is nullptr");
        return ErrorCode::ERR_INVALID_OPERATION;
    }

    if (!isNeedCreateThread) {
        return ProcessPipelineTask(pipeline, effectParameters);
    } else {
        auto prom = std::make_shared<std::promise<ErrorCode>>();
        std::future<ErrorCode> fut = prom->get_future();
        auto task = std::make_shared<RenderTask<>>([pipeline, &effectParameters, &prom]() {
            auto res = ProcessPipelineTask(pipeline, effectParameters);
            prom->set_value(res);
            return;
        }, 0, taskId);
        thread->AddTask(task);
        task->Wait();
        ErrorCode res = fut.get();
        return res;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode StartPipeline(std::shared_ptr<PipelineCore> &pipeline, const EffectParameters &effectParameters,
    unsigned long int taskId, RenderThread<> *thread, bool isNeedCreateThread = false)
{
    effectParameters.effectContext_->renderStrategy_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    effectParameters.effectContext_->colorSpaceManager_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    effectParameters.effectContext_->memoryManager_->Init(effectParameters.srcEffectBuffer_,
        effectParameters.dstEffectBuffer_);
    ErrorCode res = StartPipelineInner(pipeline, effectParameters, taskId, thread, isNeedCreateThread);
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
        case DataType::PATH:
        case DataType::PICTURE:
        case DataType::TEX: {
            impl_->effectState_ = EffectState::RUNNING;
            ErrorCode res = this->Render();
            Stop();
            return res;
        }
        case DataType::SURFACE:
            impl_->effectState_ = EffectState::RUNNING;
            if (impl_->surfaceAdapter_) {
                impl_->surfaceAdapter_->ConsumerRequestCpuAccess(true);
            }
            break;
        default:
            EFFECT_LOGE("Not set input data!");
            return ErrorCode::ERR_NOT_SET_INPUT_DATA;
    }
    return ErrorCode::SUCCESS;
}

void ImageEffect::Stop()
{
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    impl_->effectState_ = EffectState::IDLE;
    if (impl_->surfaceAdapter_) {
        impl_->surfaceAdapter_->ConsumerRequestCpuAccess(false);
    }
    impl_->effectContext_->logStrategy_ = LOG_STRATEGY::NORMAL;
    impl_->effectContext_->memoryManager_->ClearMemory();
}

ErrorCode ImageEffect::SetInputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer)
{
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, ErrorCode::ERR_INVALID_SRC_SURFACEBUFFER,
        "invalid source surface buffer");
    if (needPreFlush_) {
        surfaceBuffer->FlushCache();
    }

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
    EFFECT_LOGD("ImageEffect::SetInputUri");
    if (!CommonUtils::EndsWithJPG(uri) && !CommonUtils::EndsWithHEIF(uri)) {
        EFFECT_LOGE("SetInputUri: file type is not support! only support jpg/jpeg and heif.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::URI;
    inDateInfo_.uri_ = std::move(uri);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputUri(const std::string &uri)
{
    EFFECT_LOGD("ImageEffect::SetOutputUri");
    if (uri.empty()) {
        EFFECT_LOGI("SetOutputUri: uri set to null!");
        ClearDataInfo(outDateInfo_);
        return ErrorCode::SUCCESS;
    }

    if (!CommonUtils::EndsWithJPG(uri) && !CommonUtils::EndsWithHEIF(uri)) {
        EFFECT_LOGE("SetOutputUri: file type is not support! only support jpg/jpeg and heif.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(outDateInfo_);
    outDateInfo_.dataType_ = DataType::URI;
    outDateInfo_.uri_ = std::move(uri);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetInputPath(const std::string &path)
{
    EFFECT_LOGD("ImageEffect::SetInputPath");
    if (!CommonUtils::EndsWithJPG(path) && !CommonUtils::EndsWithHEIF(path)) {
        EFFECT_LOGE("SetInputPath: file type is not support! only support jpg/jpeg and heif.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::PATH;
    inDateInfo_.path_ = std::move(path);

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputPath(const std::string &path)
{
    EFFECT_LOGD("ImageEffect::SetOutputPath");
    if (path.empty()) {
        EFFECT_LOGI("SetOutputPath: path set to null!");
        ClearDataInfo(outDateInfo_);
        return ErrorCode::SUCCESS;
    }

    if (!CommonUtils::EndsWithJPG(path) && !CommonUtils::EndsWithHEIF(path)) {
        EFFECT_LOGE("SetOutputPath: file type is not support! only support jpg/jpeg and heif.");
        return ErrorCode::ERR_FILE_TYPE_NOT_SUPPORT;
    }
    ClearDataInfo(outDateInfo_);
    outDateInfo_.dataType_ = DataType::PATH;
    outDateInfo_.path_ = std::move(path);

    return ErrorCode::SUCCESS;
}

ErrorCode CheckPixelmapColorSpace(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer)
{
    // the format for pixel map is same or not.
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->bufferInfo_->formatType_ == dstEffectBuffer->bufferInfo_->formatType_,
        ErrorCode::ERR_NOT_SUPPORT_DIFF_FORMAT,
        "not support different format. srcFormat=%{public}d, dstFormat=%{public}d",
        srcEffectBuffer->bufferInfo_->formatType_, dstEffectBuffer->bufferInfo_->formatType_);

    if (srcEffectBuffer->extraInfo_->dataType == DataType::TEX) {
        return ErrorCode::SUCCESS;
    }

    // color space is same or not.
    EffectColorSpace srcColorSpace = srcEffectBuffer->bufferInfo_->colorSpace_;
    EffectColorSpace dstColorSpace = dstEffectBuffer->bufferInfo_->colorSpace_;
    bool isSrcHdr = ColorSpaceHelper::IsHdrColorSpace(srcColorSpace);
    bool isDstHdr = ColorSpaceHelper::IsHdrColorSpace(dstColorSpace);
    CHECK_AND_RETURN_RET_LOG(isSrcHdr == isDstHdr, ErrorCode::ERR_NOT_SUPPORT_INPUT_OUTPUT_COLORSPACE,
        "not support different colorspace. src=%{public}d, dst=%{public}d", srcColorSpace, dstColorSpace);
    return ErrorCode::SUCCESS;
}

static bool dataTypeCheckFunc(DataType srcDataType, DataType dstDataType) {
    if (srcDataType == dstDataType) {
        return true;
    }
    std::vector<std::pair<DataType, DataType>> extraSupportTab = {
        { DataType::PIXEL_MAP, DataType::NATIVE_WINDOW },
        { DataType::PICTURE, DataType::NATIVE_WINDOW },
    };
    return extraSupportTab.end() != std::find_if(extraSupportTab.begin(), extraSupportTab.end(),
        [&srcDataType, &dstDataType](const std::pair<DataType, DataType> &data) {
        return data.first == srcDataType && data.second == dstDataType;
    });
};

ErrorCode CheckToRenderPara(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer)
{
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer != nullptr, ErrorCode::ERR_PARSE_FOR_EFFECT_BUFFER_FAIL,
        "invalid srcEffectBuffer");
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->bufferInfo_ != nullptr, ErrorCode::ERR_BUFFER_INFO_NULL,
        "buffer info is null! srcBufferInfo=%{public}d", srcEffectBuffer->bufferInfo_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->extraInfo_ != nullptr, ErrorCode::ERR_EXTRA_INFO_NULL,
        "extra info is null! srcExtraInfo=%{public}d", srcEffectBuffer->extraInfo_ == nullptr);

    // allow developers not to set the out parameter.
    if (dstEffectBuffer == nullptr) {
        if (srcEffectBuffer->extraInfo_->dataType == DataType::TEX) {
            return ErrorCode::ERR_NO_DST_TEX;
        }
        return ErrorCode::SUCCESS;
    }

    CHECK_AND_RETURN_RET_LOG(dstEffectBuffer->bufferInfo_ != nullptr, ErrorCode::ERR_BUFFER_INFO_NULL,
        "buffer info is null! srcBufferInfo=%{public}d", dstEffectBuffer->bufferInfo_ == nullptr);
    CHECK_AND_RETURN_RET_LOG(dstEffectBuffer->extraInfo_ != nullptr, ErrorCode::ERR_EXTRA_INFO_NULL,
        "extra info is null! dstExtraInfo=%{public}d", dstEffectBuffer->extraInfo_ == nullptr);

    if (dstEffectBuffer->bufferInfo_->tex_ != nullptr && dstEffectBuffer->bufferInfo_->tex_->Width() == 0
        && dstEffectBuffer->bufferInfo_->tex_->Height() == 0) {
        return ErrorCode::SUCCESS;
    }

    // input and output type is same or not.
    DataType srcDataType = srcEffectBuffer->extraInfo_->dataType;
    DataType dtsDataType = dstEffectBuffer->extraInfo_->dataType;
    CHECK_AND_RETURN_RET_LOG(dataTypeCheckFunc(srcDataType, dtsDataType), ErrorCode::ERR_NOT_SUPPORT_DIFF_DATATYPE,
        "not supported dataType. srcDataType=%{public}d, dstDataType=%{public}d", srcDataType, dtsDataType);

    if (srcEffectBuffer->bufferInfo_->tex_ != nullptr && dstEffectBuffer->bufferInfo_->tex_ != nullptr) {
        unsigned int input = srcEffectBuffer->bufferInfo_->tex_->GetName();
        unsigned int output = dstEffectBuffer->bufferInfo_->tex_->GetName();
        if (input == output) {
            return ErrorCode::ERR_INVALID_TEXTURE;
        }
    }

    // color space is same or not.
    if (srcDataType == DataType::PIXEL_MAP && dtsDataType != DataType::NATIVE_WINDOW) {
        return CheckPixelmapColorSpace(srcEffectBuffer, dstEffectBuffer);
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfoFromPixelMap(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
    std::shared_ptr<ExifMetadata> &exifMetadata) const
{
    width = static_cast<uint32_t>(inDateInfo_.pixelMap_->GetWidth());
    height = static_cast<uint32_t>(inDateInfo_.pixelMap_->GetHeight());
    pixelFormat = inDateInfo_.pixelMap_->GetPixelFormat();
    exifMetadata = inDateInfo_.pixelMap_->GetExifMetadata();
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfoFromSurface(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat) const
{
    width = static_cast<uint32_t>(inDateInfo_.surfaceBufferInfo_.surfaceBuffer_->GetWidth());
    height = static_cast<uint32_t>(inDateInfo_.surfaceBufferInfo_.surfaceBuffer_->GetHeight());
    pixelFormat = static_cast<PixelFormat>(inDateInfo_.surfaceBufferInfo_.surfaceBuffer_->GetFormat());
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfoFromPath(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
    std::shared_ptr<ExifMetadata> &exifMetadata) const
{
    auto path = inDateInfo_.dataType_ == DataType::URI ? CommonUtils::UrlToPath(inDateInfo_.uri_) : inDateInfo_.path_;
    std::shared_ptr<ImageSource> imageSource = CommonUtils::GetImageSourceFromPath(path);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, ErrorCode::ERR_CREATE_IMAGESOURCE_FAIL,
        "CreateImageSource fail! path=%{public}s", path.c_str());
    ImageInfo info;
    imageSource->GetImageInfo(info);
    width = static_cast<uint32_t>(info.size.width);
    height = static_cast<uint32_t>(info.size.height);
    pixelFormat = info.pixelFormat;
    exifMetadata = imageSource->GetExifMetadata();
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfoFromPicture(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
    std::shared_ptr<ExifMetadata> &exifMetadata) const
{
    std::shared_ptr<PixelMap> pixelMap = inDateInfo_.picture_->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, ErrorCode::ERR_GET_MAIN_PIXELMAP_FROM_PICTURE_FAIL,
        "GetImageInfoFromPicture pixelMap is nullptr!");
    width = static_cast<uint32_t>(pixelMap->GetWidth());
    height = static_cast<uint32_t>(pixelMap->GetHeight());
    pixelFormat = pixelMap->GetPixelFormat();
    exifMetadata = inDateInfo_.picture_->GetExifMetadata();
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfoFromTexInfo(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat) const
{
    int32_t texId = inDateInfo_.textureInfo_.textureId_;
    width = GLUtils::GetTexWidth(texId);
    CHECK_AND_RETURN_RET_LOG(width > 0, ErrorCode::ERR_INVALID_TEXTURE, "input tex width is invalid");
    height = GLUtils::GetTexHeight(texId);
    CHECK_AND_RETURN_RET_LOG(height > 0, ErrorCode::ERR_INVALID_TEXTURE, "input tex height is invalid");
    pixelFormat = CommonUtils::SwitchGLFormatToPixelFormat(GLUtils::GetTexFormat(texId));
    CHECK_AND_RETURN_RET_LOG(pixelFormat != PixelFormat::UNKNOWN, ErrorCode::ERR_UNSUPPORTED_FORMAT_TYPE,
        "input tex pixelFormat is invalid");
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::GetImageInfo(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
    std::shared_ptr<ExifMetadata> &exifMetadata)
{
    ErrorCode errorCode = ErrorCode::SUCCESS;
    switch (inDateInfo_.dataType_) {
        case DataType::PIXEL_MAP: {
            errorCode = GetImageInfoFromPixelMap(width, height, pixelFormat, exifMetadata);
            CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "GetImageInfoFromPixelMap fail!");
            break;
        }
        case DataType::SURFACE:
        case DataType::SURFACE_BUFFER: {
            errorCode = GetImageInfoFromSurface(width, height, pixelFormat);
            CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "GetImageInfoFromSurface fail!");
            break;
        }
        case DataType::URI:
        case DataType::PATH: {
            errorCode = GetImageInfoFromPath(width, height, pixelFormat, exifMetadata);
            CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "GetImageInfoFromPath fail!");
            break;
        }
        case DataType::NATIVE_WINDOW:
            width = 0;
            height = 0;
            break;
        case DataType::PICTURE: {
            errorCode = GetImageInfoFromPicture(width, height, pixelFormat, exifMetadata);
            CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "GetImageInfoFromPicture fail!");
            break;
        }
        case DataType::TEX: {
            errorCode = GetImageInfoFromTexInfo(width, height, pixelFormat);
            CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, errorCode, "GetImageInfoFromTexInfo fail!");
            break;
        }
        case DataType::UNKNOWN:
            EFFECT_LOGE("dataType is unknown! DataType is not set!");
            return  ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
        default:
            EFFECT_LOGE("dataType is not support! dataType=%{public}d", static_cast<int>(inDateInfo_.dataType_));
            return ErrorCode::ERR_UNSUPPORTED_DATA_TYPE;
    }
    return errorCode;
}

ErrorCode ImageEffect::InitEffectBuffer(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer, IEffectFormat format)
{
    ErrorCode res = LockAll(srcEffectBuffer, dstEffectBuffer, format);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    res = CheckToRenderPara(srcEffectBuffer, dstEffectBuffer);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::ConfigureFilters(std::shared_ptr<EffectBuffer> srcEffectBuffer,
    std::shared_ptr<EffectBuffer> dstEffectBuffer) {
    std::shared_ptr<ImageSourceFilter> &sourceFilter = impl_->srcFilter_;
    ErrorCode res = ConfigSourceFilter(sourceFilter, srcEffectBuffer, impl_->effectContext_);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    std::shared_ptr<ImageSinkFilter> &sinkFilter = impl_->sinkFilter_;
    res = ConfigSinkFilter(sinkFilter, dstEffectBuffer, toProducerSurface_);
    if (res != ErrorCode::SUCCESS) {
        UnLockAll();
        return res;
    }

    return ErrorCode::SUCCESS;
}

void ImageEffect::SetPathToSink()
{
    if (inDateInfo_.dataType_ == DataType::URI) {
        impl_->sinkFilter_->inPath_ = CommonUtils::UrlToPath(inDateInfo_.uri_);
    } else if (inDateInfo_.dataType_ == DataType::PATH) {
        impl_->sinkFilter_->inPath_ = inDateInfo_.path_;
    }
}

ErrorCode ImageEffect::Render()
{
    EFFECT_TRACE_NAME("ImageEffect::Render");
    CHECK_AND_RETURN_RET_LOG(!efilters_.empty(), ErrorCode::ERR_NOT_FILTERS_WITH_RENDER, "efilters is empty");

    uint32_t width = 0;
    uint32_t height = 0;
    PixelFormat pixelFormat = PixelFormat::RGBA_8888;
    std::shared_ptr<ExifMetadata> exifMetadata = nullptr;

    ErrorCode res = GetImageInfo(width, height, pixelFormat, exifMetadata);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "set image info fail! res = %{public}d", res);
    IEffectFormat format = CommonUtils::SwitchToEffectFormat(pixelFormat);
    impl_->effectContext_->exifMetadata_ = exifMetadata;

    std::shared_ptr<ImageSourceFilter> &sourceFilter = impl_->srcFilter_;
    sourceFilter->SetNegotiateParameter(width, height, format, impl_->effectContext_);

    res = impl_->pipeline_->Prepare();
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "pipeline prepare fail! res=%{public}d", res);

    RemoveGainMapIfNeed();
    if (inDateInfo_.dataType_ == DataType::URI || inDateInfo_.dataType_ == DataType::PATH) {
        const std::vector<std::shared_ptr<Capability>> &capabilities =
            impl_->effectContext_->capNegotiate_->GetCapabilityList();
        format = CapabilityNegotiate::NegotiateFormat(capabilities);
    }
    EFFECT_LOGD("image effect render, negotiate format=%{public}d", format);
    SetPathToSink();

    std::shared_ptr<EffectBuffer> srcEffectBuffer = nullptr;
    std::shared_ptr<EffectBuffer> dstEffectBuffer = nullptr;
    res = InitEffectBuffer(srcEffectBuffer, dstEffectBuffer, format);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "init effectBuffer fail! res=%{puiblic}d", res);

    res = ConfigureFilters(srcEffectBuffer, dstEffectBuffer);
    CHECK_AND_RETURN_RET_LOG(res == ErrorCode::SUCCESS, res, "configure filters fail! res=%{puiblic}d", res);

    std::shared_ptr<EffectBuffer> outBuffer = dstEffectBuffer != nullptr ? dstEffectBuffer : srcEffectBuffer;
    impl_->effectContext_->renderEnvironment_->SetOutputType(outBuffer->extraInfo_->dataType);
    EffectParameters effectParameters(srcEffectBuffer, dstEffectBuffer, config_, impl_->effectContext_);
    bool isNeedCreateThread = !impl_->isQosEnabled_ && srcEffectBuffer->extraInfo_->dataType != DataType::TEX;
    res = StartPipeline(impl_->pipeline_, effectParameters, RequestTaskId(), m_renderThread, isNeedCreateThread);
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
    EffectJsonPtr effect = EffectJsonHelper::CreateArray();
    for (auto it = efilters_.begin(); it != efilters_.end(); it++) {
        EffectJsonPtr data = EffectJsonHelper::CreateObject();
        std::shared_ptr<EFilter> efilter = *it;
        efilter->Save(data);
        effect->Add(data);
    }

    EffectJsonPtr info  = EffectJsonHelper::CreateObject();
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
    EFFECT_TRACE_NAME("ImageEffect::Restore");
    const EffectJsonPtr root = EffectJsonHelper::ParseJsonData(info);
    CHECK_AND_RETURN_RET_LOG(root->HasElement("imageEffect"), nullptr, "Restore: no imageEffect");
    const EffectJsonPtr &imageInfo = root->GetElement("imageEffect");
    CHECK_AND_RETURN_RET_LOG(imageInfo != nullptr, nullptr, "Restore: imageInfo is null!");
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
        CHECK_AND_RETURN_RET_LOG(efilter != nullptr, nullptr,
            "Restore: efilter restore fail! name=%{public}s", name.c_str());
        imageEffect->AddEFilter(efilter);
    }
    return imageEffect;
}

ErrorCode ImageEffect::SetOutputPixelMap(PixelMap* pixelMap)
{
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    EFFECT_LOGD("ImageEffect::SetOutputPixelMap");
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
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    if (surface == nullptr) {
        EFFECT_LOGE("surface is null.");
        return ErrorCode::ERR_INPUT_NULL;
    }
    outDateInfo_.dataType_ = DataType::SURFACE;
    toProducerSurface_ = surface;

    if (impl_->surfaceAdapter_ == nullptr) {
        impl_->surfaceAdapter_ = sptr<EffectSurfaceAdapter>::MakeSptr();
    }
    impl_->surfaceAdapter_->SetOutputSurfaceDefaultUsage(toProducerSurface_->GetDefaultUsage());

    toProducerSurface_->SetTransform(GRAPHIC_ROTATE_BUTT);
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

void MemoryCopyForSurfaceBuffer(sptr<SurfaceBuffer> &buffer, OHOS::sptr<SurfaceBuffer> &outBuffer)
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

bool IsSurfaceBufferHebc(sptr<SurfaceBuffer> &buffer)
{
    V1_1::BufferHandleAttrKey key = V1_1::BufferHandleAttrKey::ATTRKEY_ACCESS_TYPE;
    std::vector<uint8_t> values;
    auto res = buffer->GetMetadata(key, values);
    CHECK_AND_RETURN_RET(res == 0, false);

    V1_1::HebcAccessType hebcAccessType = V1_1::HebcAccessType::HEBC_ACCESS_UNINIT;
    res = MetadataHelper::ConvertVecToMetadata(values, hebcAccessType);
    CHECK_AND_RETURN_RET(res == 0, false);

    if (hebcAccessType == V1_1::HEBC_ACCESS_HW_ONLY) {
        EFFECT_LOGD("IsSurfaceBufferHebc: surface buffer is Hebc data!");
        return true;
    }
    return false;
}

void SetSurfaceBufferHebcAccessType(sptr<SurfaceBuffer> &buffer, V1_1::HebcAccessType hebcAccessType)
{
    std::vector<uint8_t> values;
    auto res = MetadataHelper::ConvertMetadataToVec(hebcAccessType, values);
    CHECK_AND_RETURN_LOG(res == 0, "SetSurfaceBufferHebcAccessType: ConvertVecToMetadata fail! res=%{public}d", res);

    V1_1::BufferHandleAttrKey key = V1_1::BufferHandleAttrKey::ATTRKEY_ACCESS_TYPE;
    res = buffer->SetMetadata(key, values, false);
    CHECK_AND_RETURN_LOG(res == 0, "SetSurfaceBufferHebcAccessType: SetMetadata fail! res=%{public}d", res);
}

void SetSurfaceBufferRequestHebcAccessType(sptr<SurfaceBuffer> &buffer, V1_1::HebcAccessType hebcAccessType)
{
    std::vector<uint8_t> values;
    auto res = MetadataHelper::ConvertMetadataToVec(hebcAccessType, values);
    CHECK_AND_RETURN_LOG(res == 0,
        "SetSurfaceBufferRequestHebcAccessType: ConvertVecToMetadata fail! res=%{public}d", res);

    V1_1::BufferHandleAttrKey key = V1_1::BufferHandleAttrKey::ATTRKEY_REQUEST_ACCESS_TYPE;
    res = buffer->SetMetadata(key, values, false);
    CHECK_AND_RETURN_LOG(res == 0,
        "SetSurfaceBufferRequestHebcAccessType: SetMetadata fail! res=%{public}d", res);
}

void ImageEffect::RenderBuffer()
{
    auto entry = bufferPool_->TryPop(false);
    CHECK_AND_RETURN_LOG(entry != std::nullopt, "ProcessRender: No available buffer in bufferPool!");
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    inDateInfo_.surfaceBufferInfo_ = {
        .surfaceBuffer_ = entry->buffer_,
        .timestamp_ = entry->timestamp_,
    };
    outDateInfo_.surfaceBufferInfo_ = {
        .surfaceBuffer_ = entry->buffer_,
        .timestamp_ = entry->timestamp_,
    };
    impl_->effectContext_->renderEnvironment_->NotifyInputChanged();
    this->Render();
    if (impl_->effectContext_->logStrategy_ == LOG_STRATEGY::NORMAL) {
        impl_->effectContext_->logStrategy_ = LOG_STRATEGY::LIMITED;
    }

    EFFECT_LOGD("ProcessRender: FlushBuffer: %{public}d", entry->buffer_->GetSeqNum());
    auto ret = FlushBuffer(entry->buffer_, entry->syncFence_, true, true, entry->timestamp_);
    CHECK_AND_RETURN_LOG(ret == GSError::GSERROR_OK, "ProcessRender: FlushBuffer fail! ret=%{public}d", ret);
}

BufferRequestConfig ImageEffect::GetBufferRequestConfig(const sptr<SurfaceBuffer>& buffer)
{
    return {
        .width = buffer->GetWidth(),
        .height = buffer->GetHeight(),
        .strideAlignment = 0x8,
        .format = buffer->GetFormat(),
        .usage = buffer->GetUsage(),
        .timeout = 0,
        .colorGamut = buffer->GetSurfaceBufferColorGamut(),
        .transform = buffer->GetSurfaceBufferTransform(),
    };
}

GSError ImageEffect::FlushBuffer(sptr<SurfaceBuffer>& flushBuffer, sptr<SyncFence>& syncFence, bool isNeedAttach,
    bool isSendFence, int64_t& timestamp)
{
    BufferFlushConfig flushConfig = {
        .damage = {
            .w = flushBuffer->GetWidth(),
            .h = flushBuffer->GetHeight(),
        },
        .timestamp = timestamp,
    };

    CHECK_AND_RETURN_RET_LOG(imageEffectFlag_ == STRUCT_IMAGE_EFFECT_CONSTANT, GSERROR_NOT_INIT,
        "FlushBuffer: ImageEffect not exist.");
    CHECK_AND_RETURN_RET_LOG(toProducerSurface_ != nullptr, GSERROR_NOT_INIT,
        "FlushBuffer: toProducerSurface is nullptr.");

    auto ret = GSError::GSERROR_OK;
    const sptr<SyncFence> invalidFence = SyncFence::InvalidFence();
    if (isNeedAttach) {
        ret = toProducerSurface_->AttachAndFlushBuffer(flushBuffer, isSendFence ? syncFence : invalidFence,
            flushConfig, false);
        if (ret != GSError::GSERROR_OK) {
            EFFECT_LOGE("AttachAndFlushBuffer: attach and flush buffer failed. %{public}d", ret);
        }
    } else {
        ret = toProducerSurface_->FlushBuffer(flushBuffer, isSendFence ? syncFence : invalidFence, flushConfig);
        if (ret != GSError::GSERROR_OK) {
            EFFECT_LOGE("FlushBuffer: flush buffer failed. %{public}d", ret);
        }
    }

    return ret;
}

GSError ImageEffect::ReleaseBuffer(sptr<OHOS::SurfaceBuffer> &buffer, sptr<OHOS::SyncFence> &fence)
{
    auto ret = impl_->ReleaseConsumerSurfaceBuffer(buffer, fence);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ReleaseBuffer: ReleaseConsumerSurfaceBuffer failed. %{public}d", ret);
    }
    return ret;
}

bool ImageEffect::SubmitRenderTask(BufferEntry &&entry)
{
    bool success = bufferPool_->TryPush(std::move(entry));
    EFFECT_LOGD("SubmitRenderTask: bufferPool size: %{public}d", (int)bufferPool_->Size());
    CHECK_AND_RETURN_RET_LOG(m_renderThread, true, "SubmitRenderTask: m_renderThread is null!");
    CHECK_AND_RETURN_RET_LOG(success, true, "SubmitRenderTask: bufferPool push failed!");

    auto task = std::make_shared<RenderTask<>>([this]() {
        RenderBuffer();
    }, COMMON_TASK_TAG + 1, m_currentTaskId.fetch_add(1));
    m_renderThread->AddTask(task);
    return false;
}

void ImageEffect::ProcessRender(BufferProcessInfo& bufferProcessInfo, bool& isNeedSwap, int64_t& timestamp)
{
    auto& [inBuffer, outBuffer, inBufferSyncFence, outBufferSyncFence, isSrcHebcData] = bufferProcessInfo;

    constexpr uint32_t waitForEver = -1;
    (void)inBufferSyncFence->Wait(waitForEver);
    CHECK_AND_RETURN_LOG(inBuffer, "ProcessRender: inBuffer is nullptr!");

    {
        EFFECT_TRACE_BEGIN("ProcessRender: InvalidateCache");
        (void)inBuffer->InvalidateCache();
        EFFECT_TRACE_END();
    }

    CHECK_AND_RETURN_LOG(toProducerSurface_ != nullptr, "ProcessRender: toProducerSurface is nullptr.");
    auto requestConfig = GetBufferRequestConfig(inBuffer);
    auto ret = toProducerSurface_->RequestAndDetachBuffer(outBuffer, outBufferSyncFence, requestConfig);
    if (ret != 0 || outBuffer == nullptr) {
        EFFECT_LOGE("ProcessRender::RequestAndDetachBuffer failed. %{public}d", ret);
        return;
    }
    EFFECT_LOGD("ProcessRender: inBuffer: %{public}d, outBuffer: %{public}d",
        inBuffer->GetSeqNum(), outBuffer->GetSeqNum());

    ret = impl_->DetachConsumerSurfaceBuffer(inBuffer);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessRender: DetachConsumerSurfaceBuffer failed. %{public}d", ret);
        ReleaseBuffer(inBuffer, inBufferSyncFence);
        FlushBuffer(outBuffer, outBufferSyncFence, true, true, timestamp);
        return;
    }

    SetSurfaceBufferRequestHebcAccessType(outBuffer, V1_1::HebcAccessType::HEBC_ACCESS_CPU_ACCESS);
    ret = impl_->AttachConsumerSurfaceBuffer(outBuffer);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessRender: AttachConsumerSurfaceBuffer failed. %{public}d", ret);
    }

    ret = ReleaseBuffer(outBuffer, outBufferSyncFence);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessRender: ReleaseConsumerSurfaceBuffer failed. %{public}d", ret);
        impl_->DetachConsumerSurfaceBuffer(outBuffer);
        return;
    }

    SetSurfaceBufferHebcAccessType(inBuffer, isSrcHebcData ?
        V1_1::HebcAccessType::HEBC_ACCESS_HW_ONLY : V1_1::HebcAccessType::HEBC_ACCESS_CPU_ACCESS);
    isNeedSwap = SubmitRenderTask({inBuffer->GetSeqNum(), inBuffer, inBufferSyncFence, timestamp});
}

void ImageEffect::ProcessSwapBuffers(BufferProcessInfo& bufferProcessInfo, int64_t& timestamp)
{
    sptr<SurfaceBuffer> inBuffer = bufferProcessInfo.inBuffer_;
    sptr<SurfaceBuffer> outBuffer = bufferProcessInfo.outBuffer_;
    sptr<SyncFence> inBufferSyncFence = bufferProcessInfo.inBufferSyncFence_;
    sptr<SyncFence> outBufferSyncFence = bufferProcessInfo.outBufferSyncFence_;
    // inBuffer aquired from impl
    auto requestConfig = GetBufferRequestConfig(inBuffer);
    auto ret = toProducerSurface_->RequestAndDetachBuffer(outBuffer, outBufferSyncFence, requestConfig);
    if (ret != 0 || outBuffer == nullptr) {
        EFFECT_LOGE("ProcessSwapBuffers::RequestAndDetachBuffer failed. %{public}d", ret);
        ReleaseBuffer(inBuffer, inBufferSyncFence);
        return;
    }
    CHECK_AND_RETURN_LOG(inBuffer != nullptr && outBuffer != nullptr,
        "ProcessSwapBuffers: inBuffer or outBuffer is nullptr");
    // outBuffer requestAndDetached from XC
    EFFECT_LOGD("ProcessSwapBuffers: inBuffer: %{public}d, outBuffer: %{public}d",
        inBuffer->GetSeqNum(), outBuffer->GetSeqNum());

    CHECK_AND_RETURN_LOG(impl_, "ProcessSwapBuffers: impl_ is nullptr");
    ret = impl_->DetachConsumerSurfaceBuffer(inBuffer);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessSwapBuffers: DetachConsumerSurfaceBuffer failed. %{public}d", ret);
        ReleaseBuffer(inBuffer, inBufferSyncFence);
        FlushBuffer(outBuffer, outBufferSyncFence, true, true, timestamp);
        return;
    }
    // inBuffer detached from impl
    ret = impl_->AttachConsumerSurfaceBuffer(outBuffer);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessSwapBuffers: AttachConsumerSurfaceBuffer failed. %{public}d", ret);
        failureCount_++;
        impl_->AttachConsumerSurfaceBuffer(inBuffer);
        ReleaseBuffer(inBuffer, inBufferSyncFence);
        FlushBuffer(outBuffer, inBufferSyncFence, true, false, timestamp);
        return;
    }
    // outBuffer attached in Impl
    ret = FlushBuffer(inBuffer, inBufferSyncFence, true, true, timestamp);
    if (ret != GSError::GSERROR_OK) {
        EFFECT_LOGE("ProcessSwapBuffers: FlushBuffer failed. %{public}d", ret);
        ReleaseBuffer(outBuffer, outBufferSyncFence);
        return;
    }
    // inBuffer attachAndFlushed in XC
    ret = ReleaseBuffer(outBuffer, outBufferSyncFence);
    // outBuffer release in impl
    CHECK_AND_RETURN_LOG(ret == GSError::GSERROR_OK,
        "ProcessSwapBuffers: ReleaseConsumerSurfaceBuffer failed. %{public}d", ret);
}

void ImageEffect::OnBufferAvailableWithCPU()
{
    sptr<SurfaceBuffer> inBuffer = nullptr;
    sptr<SurfaceBuffer> outBuffer = nullptr;
    int64_t timestamp = 0;
    OHOS::Rect damages = {};
    sptr<SyncFence> inBufferSyncFence = SyncFence::INVALID_FENCE;

    CHECK_AND_RETURN_LOG(impl_->CheckEffectSurface(),
        "OnBufferAvailableToProcess: onBufferAvailableToProcess consumerSurface_ not exist");

    auto ret = impl_->AcquireConsumerSurfaceBuffer(inBuffer, inBufferSyncFence, timestamp, damages);
    CHECK_AND_RETURN_LOG(ret == 0 && inBuffer != nullptr, "AcquireBuffer failed. %{public}d", ret);

    outDateInfo_.dataType_ = DataType::SURFACE;
    UpdateProducerSurfaceInfo();

    bool isSrcHebcData = false;
    CHECK_AND_RETURN_LOG(impl_ != nullptr, "OnBufferAvailableWithCPU: impl is nullptr.");
    UpdateConsumerBuffersNumber();
    UpdateCycleBuffersNumber();
    bool isNeedSwap = true;
    bool isNeedRender = impl_->effectState_ == EffectState::RUNNING;
    if (isNeedRender) {
        isSrcHebcData = IsSurfaceBufferHebc(inBuffer);
        isNeedRender = !isSrcHebcData;
    }

    BufferProcessInfo bufferProcessInfo = {
        .inBuffer_ = inBuffer,
        .outBuffer_ = outBuffer,
        .inBufferSyncFence_ = inBufferSyncFence,
        .outBufferSyncFence_ = SyncFence::INVALID_FENCE,
        .isSrcHebcData_ = isSrcHebcData,
    };

    if (isNeedRender) {
        EFFECT_TRACE_BEGIN("ProcessRender");
        ProcessRender(bufferProcessInfo, isNeedSwap, timestamp);
        EFFECT_TRACE_END();
    }
    if (isNeedSwap) {
        EFFECT_TRACE_BEGIN("ProcessSwapBuffers");
        ProcessSwapBuffers(bufferProcessInfo, timestamp);
        EFFECT_TRACE_END();
    }
}

void ImageEffect::ConsumerBufferAvailable()
{
    CHECK_AND_RETURN_LOG(imageEffectFlag_ == STRUCT_IMAGE_EFFECT_CONSTANT,
        "ImageEffect::ConsumerBufferAvailable ImageEffect not exist.");
    if (!impl_->isQosEnabled_) {
        OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
        impl_->isQosEnabled_ = true;
    }
    std::unique_lock<std::mutex> lock(consumerListenerMutex_);
    CHECK_AND_RETURN_LOG(imageEffectFlag_ == STRUCT_IMAGE_EFFECT_CONSTANT,
        "ImageEffect::ConsumerBufferAvailable ImageEffect not exist.");
    OnBufferAvailableWithCPU();
}

sptr<Surface> ImageEffect::GetInputSurface()
{
    inDateInfo_.dataType_ = DataType::SURFACE;
    if (fromProducerSurface_ != nullptr) {
        return fromProducerSurface_;
    }

    if (impl_->surfaceAdapter_ == nullptr) {
        impl_->surfaceAdapter_ = sptr<EffectSurfaceAdapter>::MakeSptr();
    }

    if (impl_->surfaceAdapter_) {
        fromProducerSurface_ = impl_->surfaceAdapter_->GetProducerSurface();
    }

    auto consumerListener = [this]() {
        return ConsumerBufferAvailable();
    };

    if (impl_->surfaceAdapter_) {
        impl_->surfaceAdapter_->SetConsumerListener(std::move(consumerListener));
    }

    auto consumerSurface = impl_->GetConsumerSurface();
    if (fromProducerSurface_ && consumerSurface) {
        int maxBufferPoolSize = std::max((int)fromProducerSurface_->GetQueueSize(),
            (int)consumerSurface->GetQueueSize());
        maxBufferPoolSize = std::min(maxBufferPoolSize, RENDER_QUEUE_SIZE);
        EFFECT_LOGD("GetInputSurface: maxBufferPoolSize=%{public}d", maxBufferPoolSize);
        bufferPool_ = std::make_shared<ThreadSafeBufferQueue<BufferEntry>>(maxBufferPoolSize);
    }

    return fromProducerSurface_;
}

ErrorCode ImageEffect::SetOutNativeWindow(OHNativeWindow *nativeWindow)
{
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    CHECK_AND_RETURN_RET_LOG(nativeWindow != nullptr, ErrorCode::ERR_INPUT_NULL, "nativeWindow is nullptr");
    OHOS::sptr<OHOS::Surface> surface = nativeWindow->surface;
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ErrorCode::ERR_INPUT_NULL, "surface is nullptr");
    toProducerSurface_ = surface;
    outDateInfo_.dataType_ = DataType::NATIVE_WINDOW;
    impl_->effectContext_->renderEnvironment_->InitEngine(nativeWindow);
    if (impl_->surfaceAdapter_ == nullptr) {
        impl_->surfaceAdapter_ = sptr<EffectSurfaceAdapter>::MakeSptr();
    }
    impl_->surfaceAdapter_->SetOutputSurfaceDefaultUsage(toProducerSurface_->GetDefaultUsage());

    toProducerSurface_->SetTransform(GRAPHIC_ROTATE_BUTT);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::Configure(const std::string &key, const Any &value)
{
    if (FUNCTION_FLUSH_SURFACE_BUFFER.compare(key) == 0) {
        EFFECT_LOGI("ImageEffect Configure FlushCache");
        needPreFlush_ = true;
        return ErrorCode::SUCCESS;
    }
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
        case DataType::PICTURE:
            return inDataInfo.picture_ == outDataInfo.picture_;
        default:
            return false;
    }
}

void ImageEffect::RemoveGainMapIfNeed() const
{
    auto context = impl_->effectContext_;
    const auto& supportedHdrFormats = context->filtersSupportedHdrFormat_;
    const bool needRemove = supportedHdrFormats.find(HdrFormat::HDR8_GAINMAP) == supportedHdrFormats.end();
    if (!needRemove) {
        return;
    }

    EFFECT_LOGI("RemoveGainMapIfNeed: gainMap is not supported, remove it!");
    if (outDateInfo_.dataType_ == DataType::PICTURE && outDateInfo_.picture_ &&
        outDateInfo_.picture_->HasAuxiliaryPicture(AuxiliaryPictureType::GAINMAP)) {
        outDateInfo_.picture_->DropAuxiliaryPicture(AuxiliaryPictureType::GAINMAP);
    } else if (inDateInfo_.dataType_ == DataType::PICTURE && inDateInfo_.picture_ &&
        inDateInfo_.picture_->HasAuxiliaryPicture(AuxiliaryPictureType::GAINMAP)) {
        inDateInfo_.picture_->DropAuxiliaryPicture(AuxiliaryPictureType::GAINMAP);
    }
}

ErrorCode ImageEffect::LockAll(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
    std::shared_ptr<EffectBuffer> &dstEffectBuffer, IEffectFormat format)
{
    ErrorCode res = ParseDataInfo(inDateInfo_, srcEffectBuffer, false, format, impl_->effectContext_->logStrategy_);
    if (res != ErrorCode::SUCCESS) {
        EFFECT_LOGE("ParseDataInfo inData fail! res=%{public}d", res);
        return res;
    }
    EFFECT_LOGD("input data set, parse data info success! dataType=%{public}d", inDateInfo_.dataType_);

    if (outDateInfo_.dataType_ != DataType::UNKNOWN && !IsSameInOutputData(inDateInfo_, outDateInfo_)) {
        EFFECT_LOGD("output data set, start parse data info. dataType=%{public}d", outDateInfo_.dataType_);
        res = ParseDataInfo(outDateInfo_, dstEffectBuffer, true, format, impl_->effectContext_->logStrategy_);
        if (res != ErrorCode::SUCCESS) {
            EFFECT_LOGE("ParseDataInfo outData fail! res=%{public}d", res);
            return res;
        }
        EFFECT_LOGD("output data set, parse data info success! dataType=%{public}d", outDateInfo_.dataType_);
    }

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::ParseDataInfo(DataInfo &dataInfo, std::shared_ptr<EffectBuffer> &effectBuffer,
    bool isOutputData, IEffectFormat format, LOG_STRATEGY strategy)
{
    switch (dataInfo.dataType_) {
        case DataType::PIXEL_MAP:
            return CommonUtils::LockPixelMap(dataInfo.pixelMap_, effectBuffer);
        case DataType::SURFACE:
        case DataType::SURFACE_BUFFER:
            return CommonUtils::ParseSurfaceData(dataInfo.surfaceBufferInfo_.surfaceBuffer_, effectBuffer,
                dataInfo.dataType_, strategy, dataInfo.surfaceBufferInfo_.timestamp_);
        case DataType::URI:
            return CommonUtils::ParseUri(dataInfo.uri_, effectBuffer, isOutputData, format);
        case DataType::PATH:
            return CommonUtils::ParsePath(dataInfo.path_, effectBuffer, isOutputData, format);
        case DataType::NATIVE_WINDOW:
            return CommonUtils::ParseNativeWindowData(effectBuffer, dataInfo.dataType_);
        case DataType::PICTURE:
            return CommonUtils::ParsePicture(dataInfo.picture_, effectBuffer);
        case DataType::TEX:
            return CommonUtils::ParseTex(dataInfo.textureInfo_.textureId_, dataInfo.textureInfo_.colorSpace_,
                effectBuffer);
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
    EFFECT_TRACE_NAME("ImageEffect::InitEGLEnv");
    impl_->effectContext_->renderEnvironment_->Init();
    impl_->effectContext_->renderEnvironment_->Prepare();
}

void ImageEffect::DestroyEGLEnv()
{
    EFFECT_LOGI("ImageEffect DestroyEGLEnv enter!");
    if (impl_->effectContext_->renderEnvironment_ == nullptr) {
        return;
    }
    impl_->effectContext_->renderEnvironment_->ReleaseParam();
    impl_->effectContext_->renderEnvironment_->Release();
    EFFECT_LOGI("ImageEffect DestroyEGLEnv end!");
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

ErrorCode ImageEffect::SetInputPicture(Picture *picture)
{
    EFFECT_LOGD("ImageEffect::SetInputPicture");
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ImageEffect::SetInputPicture: picture is null!");

    impl_->effectContext_->renderEnvironment_->NotifyInputChanged();
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::PICTURE;
    inDateInfo_.picture_ = picture;

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputPicture(Picture *picture)
{
    EFFECT_LOGD("ImageEffect::SetOutputPicture");
    ClearDataInfo(outDateInfo_);

    if (picture == nullptr) {
        EFFECT_LOGI("SetOutputPicture:picture set to null!");
        return ErrorCode::SUCCESS;
    }

    outDateInfo_.dataType_ = DataType::PICTURE;
    outDateInfo_.picture_ = picture;

    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetInputTexture(int32_t textureId, int32_t colorSpace)
{
    ClearDataInfo(inDateInfo_);
    inDateInfo_.dataType_ = DataType::TEX;
    CHECK_AND_RETURN_RET_LOG(textureId > 0, ErrorCode::ERR_INPUT_NULL,
        "ImageEffect::SetInputTexture: tex is invalid!");
    inDateInfo_.textureInfo_.textureId_ = textureId;
    inDateInfo_.textureInfo_.colorSpace_ = colorSpace;
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::SetOutputTexture(int32_t textureId)
{
    ClearDataInfo(outDateInfo_);
    outDateInfo_.dataType_ = DataType::TEX;
    CHECK_AND_RETURN_RET_LOG(textureId > 0, ErrorCode::ERR_INPUT_NULL,
        "ImageEffect::SetInputTexture: tex is invalid!");
    outDateInfo_.textureInfo_.textureId_ = textureId;
    return ErrorCode::SUCCESS;
}

void ImageEffect::UpdateConsumerBuffersNumber()
{
    if (setConsumerBufferSize_) {
        return;
    }

    if (!toProducerSurface_ || !fromProducerSurface_) {
        EFFECT_LOGE("UpdateConsumerBuffersNumber: toProducerSurface_ or fromProducerSurface_ is null!");
        return;
    }

    auto errorCode = toProducerSurface_->SetQueueSize(fromProducerSurface_->GetQueueSize());
    if (errorCode != 0) {
        EFFECT_LOGE("UpdateConsumerBuffersNumber: SetQueueSize failed! code: %{public}d", errorCode);
        return;
    }

    EFFECT_LOGI("UpdateConsumerBuffersNumber: SetQueueSize success! ConsumedrBuffersNumber: %{public}d",
        fromProducerSurface_->GetQueueSize());
    setConsumerBufferSize_ = true;
    return;
}

void ImageEffect::UpdateCycleBuffersNumber()
{
    if (setCycleBuffersNumber_) {
        return;
    }

    if (!toProducerSurface_ || !fromProducerSurface_) {
        EFFECT_LOGE("UpdateCycleBuffersNumber: toProducerSurface_ or fromProducerSurface_ is null!");
        return;
    }

    uint32_t cycleBuffersNumber = toProducerSurface_->GetQueueSize() + fromProducerSurface_->GetQueueSize();
    auto errorCode = toProducerSurface_->SetCycleBuffersNumber(cycleBuffersNumber);
    if (errorCode != 0) {
        EFFECT_LOGE("UpdateCycleBuffersNumber: SetCycleBuffersNumber failed! code: %{public}d", errorCode);
        return;
    }

    EFFECT_LOGI("UpdateCycleBuffersNumber: SetCycleBuffersNumber success! cycleBuffersNumber: %{public}d",
        cycleBuffersNumber);
    setCycleBuffersNumber_ = true;
    return;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS
