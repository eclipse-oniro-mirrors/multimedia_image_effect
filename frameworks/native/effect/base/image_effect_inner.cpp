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
#include "render_task.h"
#include "render_environment.h"
#include "v1_1/buffer_handle_meta_key_type.h"
#include "effect_log.h"
#include "effect_trace.h"
#include "native_window.h"

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
    imageEffectFlag_ = STRUCT_IMAGE_EFFECT_CONSTANT;
    impl_ = std::make_shared<Impl>();
    if (name != nullptr) {
        name_ = name;
    }
    ExternLoader::Instance()->InitExt();
    ExtInitModule();

    if (m_renderThread == nullptr) {
        auto func = [this]() {};
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
    imageEffectFlag_ = DESTRUCTOR_IMAGE_EFFECT_CONSTANT;
    EFFECT_LOGI("ImageEffect destruct!");
    impl_->surfaceAdapter_ = nullptr;
    m_renderThread->ClearTask();
    auto task = std::make_shared<RenderTask<>>([this]() { this->DestroyEGLEnv(); }, COMMON_TASK_TAG,
        RequestTaskId());
    m_renderThread->AddTask(task);
    task->Wait();
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
        if (runningIPType == IPType::GPU) {
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
            if (runningIPType == IPType::DEFAULT) {
                runningIPType = IPType::CPU;
            }
            return ErrorCode::SUCCESS;
        }
    }

    return ErrorCode::SUCCESS;
}

ErrorCode StartPipelineInner(std::shared_ptr<PipelineCore> &pipeline, const EffectParameters &effectParameters,
    unsigned long int taskId, RenderThread<> *thread)
{
    if (thread == nullptr) {
        EFFECT_LOGE("pipeline Prepare fail! render thread is nullptr");
        return ErrorCode::ERR_INVALID_OPERATION;
    }
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

ErrorCode StartPipeline(std::shared_ptr<PipelineCore> &pipeline, const EffectParameters &effectParameters,
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
        case DataType::PATH:
        case DataType::PICTURE: {
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
    std::function<bool(DataType, DataType)> dataTypeCheckFunc = [](DataType srcDataType, DataType dstDataType) {
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
    CHECK_AND_RETURN_RET_LOG(dataTypeCheckFunc(srcDataType, dtsDataType), ErrorCode::ERR_NOT_SUPPORT_DIFF_DATATYPE,
        "not supported dataType. srcDataType=%{public}d, dstDataType=%{public}d", srcDataType, dtsDataType);

    // color space is same or not.
    if (srcDataType == DataType::PIXEL_MAP && dtsDataType != DataType::NATIVE_WINDOW) {
        // the format for pixel map is same or not.
        CHECK_AND_RETURN_RET_LOG(srcEffectBuffer->bufferInfo_->formatType_ == dstEffectBuffer->bufferInfo_->formatType_,
            ErrorCode::ERR_NOT_SUPPORT_DIFF_FORMAT,
            "not support different format. srcFormat=%{public}d, dstFormat=%{public}d",
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
    EffectJsonPtr effect = EFFECTJsonHelper::CreateArray();
    for (auto it = efilters_.begin(); it != efilters_.end(); it++) {
        EffectJsonPtr data = EFFECTJsonHelper::CreateObject();
        std::shared_ptr<EFilter> efilter = *it;
        efilter->Save(data);
        effect->Add(data);
    }

    EffectJsonPtr info  = EFFECTJsonHelper::CreateObject();
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
    const EffectJsonPtr root = EFFECTJsonHelper::ParseJsonData(info);
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
        impl_->surfaceAdapter_ = std::make_unique<EffectSurfaceAdapter>();
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
    res = buffer->SetMetadata(key, values);
    CHECK_AND_RETURN_LOG(res == 0, "SetSurfaceBufferHebcAccessType: SetMetadata fail! res=%{public}d", res);
}

bool ImageEffect::OnBufferAvailableToProcess(sptr<SurfaceBuffer> &inBuffer, sptr<SurfaceBuffer> &outBuffer,
    int64_t timestamp)
{
    std::vector<uint32_t> keys = {};
    auto res = inBuffer->ListMetadataKeys(keys);
    for (uint32_t key : keys) {
        std::vector<uint8_t> values;
        res = inBuffer->GetMetadata(key, values);
        if (res != 0) {
            EFFECT_LOGE("GetMetadata fail! key = %{public}d res = %{public}d", key, res);
            continue;
        }
        res = outBuffer->SetMetadata(key, values);
        if (res != 0) {
            EFFECT_LOGE("SetMetadata fail! key = %{public}d res = %{public}d", key, res);
            continue;
        }
    }
    bool isSrcHebcData = IsSurfaceBufferHebc(inBuffer);
    SetSurfaceBufferHebcAccessType(outBuffer,
        isSrcHebcData ? V1_1::HebcAccessType::HEBC_ACCESS_HW_ONLY : V1_1::HebcAccessType::HEBC_ACCESS_CPU_ACCESS);

    CHECK_AND_RETURN_RET_LOG(impl_ != nullptr, true, "OnBufferAvailableToProcess: impl is nullptr.");
    bool isNeedRender = !isSrcHebcData && impl_->effectState_ == EffectState::RUNNING;
    bool isNeedSwap = true;
    if (isNeedRender) {
        inDateInfo_.surfaceBufferInfo_ = {
            .surfaceBuffer_ = inBuffer,
            .timestamp_ = timestamp,
        };
        outDateInfo_.surfaceBufferInfo_ = {
            .surfaceBuffer_ = outBuffer,
            .timestamp_ = timestamp,
        };
        ErrorCode res = this->Render();
        isNeedSwap = (res != ErrorCode::SUCCESS);
    }

    auto detRet = GSError::GSERROR_OK;
    if (isNeedSwap) {
        EFFECT_TRACE_BEGIN("OnBufferAvailableToProcess::SwapBuffers");
        detRet = toProducerSurface_->DetachBufferFromQueue(outBuffer);
        CHECK_AND_RETURN_RET_LOG(detRet == GSError::GSERROR_OK, true,
            "OnBufferAvailableToProcess: detach buffer from producerSurface_ failed");
        detRet = toProducerSurface_->AttachBufferToQueue(inBuffer);
        CHECK_AND_RETURN_RET_LOG(detRet == GSError::GSERROR_OK, true,
            "OnBufferAvailableToProcess: attach buffer from producerSurface_ failed");
        EFFECT_TRACE_END();
    }
    return isNeedSwap;
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

void ImageEffect::FlushBuffer(sptr<SurfaceBuffer>& flushBuffer, int64_t timestamp) {
    EFFECT_TRACE_BEGIN("FlushBuffer::FlushCache");
    (void)flushBuffer->FlushCache();
    EFFECT_TRACE_END();

    BufferFlushConfig flushConfig = {
        .damage = {
            .w = flushBuffer->GetWidth(),
            .h = flushBuffer->GetHeight(),
        },
        .timestamp = timestamp,
    };
    CHECK_AND_RETURN_LOG(imageEffectFlag_ == STRUCT_IMAGE_EFFECT_CONSTANT,
        "ImageEffect::OnBufferAvailableWithCPU ImageEffect not exist.");
    CHECK_AND_RETURN_LOG(toProducerSurface_ != nullptr,
        "ImageEffect::OnBufferAvailableWithCPU: toProducerSurface is nullptr.");
    constexpr int32_t invalidFence = -1;
    toProducerSurface_->FlushBuffer(flushBuffer, invalidFence, flushConfig);
}

bool ImageEffect::OnBufferAvailableWithCPU(sptr<SurfaceBuffer>& inBuffer, sptr<SurfaceBuffer>& outBuffer,
    const OHOS::Rect& damages, int64_t timestamp)
{
    CHECK_AND_RETURN_RET_LOG(inBuffer != nullptr, true, "ImageEffect::OnBufferAvailableWithCPU: inBuffer is nullptr.");
    outDateInfo_.dataType_ = DataType::SURFACE;
    UpdateProducerSurfaceInfo();

    sptr<SyncFence> syncFence = SyncFence::INVALID_FENCE;

    EFFECT_TRACE_BEGIN("inBuffer::InvalidateCache");
    (void)inBuffer->InvalidateCache();
    EFFECT_TRACE_END();

    auto requestConfig = GetBufferRequestConfig(inBuffer);

    CHECK_AND_RETURN_RET_LOG(toProducerSurface_ != nullptr, true,
                             "OnBufferAvailableWithCPU: toProducerSurface is nullptr.");
    auto ret = toProducerSurface_->RequestBuffer(outBuffer, syncFence, requestConfig);
    CHECK_AND_RETURN_RET_LOG(ret == 0 && outBuffer != nullptr, true, "RequestBuffer failed. %{public}d", ret);

		constexpr uint32_t waitForEver = -1;
		(void)syncFence->Wait(waitForEver);

    EFFECT_LOGD("inBuffer: w=%{public}d h=%{public}d stride=%{public}d len=%{public}d usage=%{public}lld",
        inBuffer->GetWidth(), inBuffer->GetHeight(), inBuffer->GetStride(), inBuffer->GetSize(),
        static_cast<unsigned long long>(inBuffer->GetUsage()));
    EFFECT_LOGD("outBuffer: w=%{public}d h=%{public}d stride=%{public}d len=%{public}d usage=%{public}lld",
        outBuffer->GetWidth(), outBuffer->GetHeight(), outBuffer->GetStride(), outBuffer->GetSize(),
        static_cast<unsigned long long>(outBuffer->GetUsage()));

    bool isNeedSwap = OnBufferAvailableToProcess(inBuffer, outBuffer, timestamp);

    auto flushBuffer = (isNeedSwap ? inBuffer : outBuffer);
    FlushBuffer(flushBuffer, timestamp);

    return isNeedSwap;
}

bool ImageEffect::ConsumerBufferAvailable(sptr<SurfaceBuffer>& inBuffer, sptr<SurfaceBuffer>& outBuffer,
    const OHOS::Rect& damages, int64_t timestamp)
{
    std::unique_lock<std::mutex> lock(innerEffectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffectFlag_ == STRUCT_IMAGE_EFFECT_CONSTANT, true,
        "ImageEffect::ConsumerBufferAvailable ImageEffect not exist.");
    return OnBufferAvailableWithCPU(inBuffer, outBuffer, damages, timestamp);
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

    auto consumerListener = [this](sptr<SurfaceBuffer>& inBuffer,
        sptr<SurfaceBuffer>& outBuffer, const OHOS::Rect& damages, int64_t timestamp) {
        return ConsumerBufferAvailable(inBuffer, outBuffer, damages, timestamp);
    };

    if (impl_->surfaceAdapter_) {
        impl_->surfaceAdapter_->SetConsumerListener(std::move(consumerListener));
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
        impl_->surfaceAdapter_ = std::make_unique<EffectSurfaceAdapter>();
    }
    impl_->surfaceAdapter_->SetOutputSurfaceDefaultUsage(toProducerSurface_->GetDefaultUsage());

    toProducerSurface_->SetTransform(GRAPHIC_ROTATE_BUTT);
    return ErrorCode::SUCCESS;
}

ErrorCode ImageEffect::Configure(const std::string &key, const Plugin::Any &value)
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
        case DataType::PICTURE:
            return CommonUtils::ParsePicture(dataInfo.picture_, effectBuffer);
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

ErrorCode ImageEffect::SetInputPicture(Picture *picture)
{
    EFFECT_LOGD("ImageEffect::SetInputPicture");
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ErrorCode::ERR_INPUT_NULL,
        "ImageEffect::SetInputPicture: picture is null!");

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
        EFFECT_LOGI("SetOutputPicture: picture set to null!");
        return ErrorCode::SUCCESS;
    }

    outDateInfo_.dataType_ = DataType::PICTURE;
    outDateInfo_.picture_ = picture;

    return ErrorCode::SUCCESS;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS