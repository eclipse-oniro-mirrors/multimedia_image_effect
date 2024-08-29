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

#include "image_effect.h"

#include "effect_log.h"
#include "efilter_factory.h"
#include "external_loader.h"
#include "image_effect_inner.h"
#include "effect_json_helper.h"
#include "native_effect_base.h"
#include "native_common_utils.h"
#include "native_window.h"
#include "event_report.h"

#define MAX_EFILTER_NUMS 100

using namespace OHOS::Media;
using namespace OHOS::Media::Effect;

namespace {
    std::mutex effectMutex_;
}

#ifdef __cplusplus
extern "C" {
#endif

EFFECT_EXPORT
OH_ImageEffect *OH_ImageEffect_Create(const char *name)
{
    if (!ExternLoader::Instance()->IsExtLoad()) {
        ExternLoader::Instance()->LoadExtSo();
    }
    auto func = ExternLoader::Instance()->GetCreateImageEffectExtFunc();
    if (func) {
        void* image = func(name);
        if (image != nullptr) {
            return static_cast<OH_ImageEffect *>(image);
        }
    } else {
        EFFECT_LOGE("OH_ImageEffect_Create: shared lib so not find function!");
    }

    EFFECT_LOGI("Creat image effect");
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(name);
    std::unique_ptr<OH_ImageEffect> nativeImageEffect = std::make_unique<OH_ImageEffect>();
    nativeImageEffect->imageEffect_ = imageEffect;
    return nativeImageEffect.release();
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_AddFilter(OH_ImageEffect *imageEffect, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "AddFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS, nullptr,
        "AddFilter: filter nums is out of range!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, nullptr, "AddFilter: input parameter filterName is null!");

    OH_EffectFilter *filter = OH_EffectFilter_Create(filterName);
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, nullptr, "AddFilter: create filter fail! name=%{public}s", filterName);

    filter->isCreatedBySystem_ = true;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_AddFilterByFilter(imageEffect, filter);
    if (errorCode != ImageEffect_ErrorCode::EFFECT_SUCCESS) {
        OH_EffectFilter_Release(filter);
        filter = nullptr;
    }
    return filter;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_AddFilterByFilter(OH_ImageEffect *imageEffect, OH_EffectFilter *filter)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "AddFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "AddFilter: input parameter filter is null!");
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS,
        ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID, "AddFilterByFilter: filter nums is out of range!");

    EFFECT_LOGI("Add filter. name=%{public}s", filter->filter_->GetName().c_str());

    imageEffect->imageEffect_->AddEFilter(filter->filter_);
    imageEffect->filters_.emplace_back(filter, filter->filter_->GetName());

    EventInfo eventInfo = {
        .filterName = filter->filter_->GetName(),
    };
    EventReport::ReportHiSysEvent(ADD_FILTER_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_InsertFilter(OH_ImageEffect *imageEffect, uint32_t index, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "InsertFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS, nullptr,
        "InsertFilter: filter nums is out of range!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, nullptr, "InsertFilter: input parameter filterName is null!");

    OH_EffectFilter *filter = OH_EffectFilter_Create(filterName);
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, nullptr, "InsertFilter: create filter fail! filterName=%{public}s",
        filterName);

    filter->isCreatedBySystem_ = true;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_InsertFilterByFilter(imageEffect, index, filter);
    if (errorCode != ImageEffect_ErrorCode::EFFECT_SUCCESS) {
        OH_EffectFilter_Release(filter);
        filter = nullptr;
    }
    return filter;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_InsertFilterByFilter(OH_ImageEffect *imageEffect, uint32_t index,
    OH_EffectFilter *filter)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InsertFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS,
        ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID, "InsertFilter: filter nums is out of range!");
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InsertFilter: input parameter filterName is null!");
    CHECK_AND_RETURN_RET_LOG(index <= static_cast<uint32_t>(imageEffect->filters_.size()),
        ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "InsertFilter: input parameter index is invalid! index=%{public}d, efilterSize=%{public}zu",
        index, imageEffect->filters_.size());

    std::string filterName = filter->filter_->GetName();
    EFFECT_LOGI("Insert filter. name=%{public}s", filterName.c_str());

    ErrorCode result = imageEffect->imageEffect_->InsertEFilter(filter->filter_, index);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("InsertFilter: insert filter fail! result=%{public}d, name=%{public}s", result, filterName.c_str());
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    imageEffect->filters_.emplace(imageEffect->filters_.begin() + index, filter, filterName);
    EventInfo eventInfo = {
        .filterName = filterName,
    };
    EventReport::ReportHiSysEvent(ADD_FILTER_STATISTIC, eventInfo);

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
int32_t OH_ImageEffect_RemoveFilter(OH_ImageEffect *imageEffect, const char *filterName)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, 0, "RemoveFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, 0, "RemoveFilter: input parameter nativeEffect is null!");

    int32_t count = 0;
    for (auto it = imageEffect->filters_.begin(); it != imageEffect->filters_.end();) {
        if (it->second.compare(filterName) == 0) {
            imageEffect->imageEffect_->RemoveEFilter(it->first->filter_);
            auto filter = it->first;
            if (filter != nullptr && filter->isCreatedBySystem_) {
                OH_EffectFilter_Release(filter);
            }
            it = imageEffect->filters_.erase(it);
            count++;
        } else {
            ++it;
        }
    }
    EFFECT_LOGI("Remove filter. name=%{public}s, count=%{public}d", filterName, count);

    EventInfo eventInfo = {
        .filterName = filterName,
        .filterNum = count,
    };
    EventReport::ReportHiSysEvent(REMOVE_FILTER_STATISTIC, eventInfo);
    return count;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_RemoveFilterByIndex(OH_ImageEffect *imageEffect, uint32_t index)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RemoveFilterByIndex: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(index < static_cast<uint32_t>(imageEffect->filters_.size()),
        ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RemoveFilterByIndex: input parameter index is invalid! index=%{public}d, efilterSize=%{public}zu",
        index, imageEffect->filters_.size());

    ErrorCode result = imageEffect->imageEffect_->RemoveEFilter(index);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("RemoveFilterByIndex: remove filter fail! result=%{public}d", result);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    const auto &filter = imageEffect->filters_.at(index);
    auto ohEFilter = filter.first;

    CHECK_AND_RETURN_RET_LOG(ohEFilter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RemoveFilterByIndex: ohEFilter is null!");
    CHECK_AND_RETURN_RET_LOG(ohEFilter->filter_ != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "RemoveFilterByIndex: filter of ohEFilter is null!");

    std::string filterName = ohEFilter->filter_->GetName();
    if (ohEFilter->isCreatedBySystem_) {
        OH_EffectFilter_Release(ohEFilter);
    }
    imageEffect->filters_.erase(imageEffect->filters_.begin() + index);
    EFFECT_LOGI("Remove filter by index. name=%{public}s, index=%{public}d", filterName.c_str(), index);

    EventInfo eventInfo = {
        .filterName = filterName,
        .filterNum = 1,
    };
    EventReport::ReportHiSysEvent(REMOVE_FILTER_STATISTIC, eventInfo);

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_ReplaceFilter(OH_ImageEffect *imageEffect, uint32_t index, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "ReplaceFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(index < static_cast<uint32_t>(imageEffect->filters_.size()), nullptr,
        "ReplaceFilter: input parameter index is invalid! index=%{public}d, efilterSize=%{public}zu",
        index, imageEffect->filters_.size());

    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, nullptr, "ReplaceFilter: input parameter filterName is null!");

    OH_EffectFilter *filter = OH_EffectFilter_Create(filterName);
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, nullptr,
        "ReplaceFilter: create filter fail! name=%{public}s", filterName);

    filter->isCreatedBySystem_ = true;
    ImageEffect_ErrorCode errorCode = OH_ImageEffect_ReplaceFilterByFilter(imageEffect, index, filter);
    if (errorCode != ImageEffect_ErrorCode::EFFECT_SUCCESS) {
        OH_EffectFilter_Release(filter);
        filter = nullptr;
    }
    return filter;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_ReplaceFilterByFilter(OH_ImageEffect *imageEffect, uint32_t index,
    OH_EffectFilter *filter)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "ReplaceFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(index < static_cast<uint32_t>(imageEffect->filters_.size()),
        ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "ReplaceFilter: input parameter index is invalid! index=%{public}d, efilterSize=%{public}zu",
        index, imageEffect->filters_.size());
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "ReplaceFilter: input parameter filterName is null!");

    EFFECT_LOGI("Replace filter. index=%{public}d, name=%{public}s", index, filter->filter_->GetName().c_str());

    ErrorCode result = imageEffect->imageEffect_->ReplaceEFilter(filter->filter_, index);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("ReplaceFilter fail! result=%{public}d, index=%{public}d", result, index);
        return ImageEffect_ErrorCode ::EFFECT_ERROR_PARAM_INVALID;
    }

    auto &originFilter = imageEffect->filters_.at(index);
    if (originFilter.first->isCreatedBySystem_) {
        OH_EffectFilter_Release(originFilter.first);
    }
    imageEffect->filters_[index] = std::pair<OH_EffectFilter *, std::string>(filter, filter->filter_->GetName());

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Configure(OH_ImageEffect *imageEffect, const char *key,
    const ImageEffect_Any *value)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Configure: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(key != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Configure: input parameter key is null!");
    CHECK_AND_RETURN_RET_LOG(value != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Configure: input parameter value is null!");

    Plugin::Any any;
    ErrorCode result = NativeCommonUtils::ParseOHAny(value, any);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "Configure: parse oh any fail! result=%{public}d, key=%{public}s, dataType=%{public}d",
        result, key, value->dataType);

    result = imageEffect->imageEffect_->Configure(key, any);
    CHECK_AND_RETURN_RET_LOG(result == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "Configure: config fail! result=%{public}d, key=%{public}s, dataType=%{public}d", result, key, value->dataType);

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputSurface(OH_ImageEffect *imageEffect, NativeWindow *nativeWindow)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputSurface: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(nativeWindow != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputSurface: input parameter nativeWindow is null!");

    ErrorCode errorCode = imageEffect->imageEffect_->SetOutNativeWindow(nativeWindow);
    if (errorCode != ErrorCode::SUCCESS) {
        EFFECT_LOGE("SetOutputSurface: set output surface fail! errorCode=%{public}d", errorCode);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

    EventInfo eventInfo = {
        .dataType = EventDataType::SURFACE,
    };
    EventReport::ReportHiSysEvent(INPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_GetInputSurface(OH_ImageEffect *imageEffect, NativeWindow **nativeWindow)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "GetInputSurface: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(nativeWindow != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "GetInputSurface: input parameter surfaceId is null!");

    OHOS::sptr<OHOS::Surface> surface = imageEffect->imageEffect_->GetInputSurface();
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, ImageEffect_ErrorCode::EFFECT_UNKNOWN,
        "GetInputSurface: get input surface fail! surface is null!");

    *nativeWindow = OH_NativeWindow_CreateNativeWindow(&surface);

    EventInfo eventInfo = {
        .dataType = EventDataType::SURFACE,
    };
    EventReport::ReportHiSysEvent(OUTPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputPixelmap(OH_ImageEffect *imageEffect, OH_PixelmapNative *pixelmap)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPixelmap: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(pixelmap != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPixelmap: input parameter pixelmap is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetInputPixelMap(NativeCommonUtils::GetPixelMapFromOHPixelmap(pixelmap));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetInputPixelMap: set input pixelmap fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::PIXEL_MAP,
    };
    EventReport::ReportHiSysEvent(INPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputPixelmap(OH_ImageEffect *imageEffect, OH_PixelmapNative *pixelmap)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputPixelmap: input parameter imageEffect is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetOutputPixelMap(NativeCommonUtils::GetPixelMapFromOHPixelmap(pixelmap));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetOutputPixelmap: set output pixelmap fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::PIXEL_MAP,
    };
    EventReport::ReportHiSysEvent(OUTPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputNativeBuffer: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(nativeBuffer != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputNativeBuffer: input parameter input nativeBuffer is null!");

    OHOS::SurfaceBuffer *surfaceBuffer = OHOS::SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);

    ErrorCode errorCode = imageEffect->imageEffect_->SetInputSurfaceBuffer(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS,
        ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetInputNativeBuffer: set input native buffer fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::SURFACE_BUFFER,
    };
    EventReport::ReportHiSysEvent(INPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputNativeBuffer: input parameter imageEffect is null!");

    OHOS::SurfaceBuffer *surfaceBuffer = OHOS::SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);

    ErrorCode errorCode = imageEffect->imageEffect_->SetOutputSurfaceBuffer(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetOutputNativeBuffer: set output native buffer fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::SURFACE_BUFFER,
    };
    EventReport::ReportHiSysEvent(OUTPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputUri(OH_ImageEffect *imageEffect, const char *uri)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    EFFECT_LOGD("Set input uri");
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputUri: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(uri != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputUri: input parameter input uri is null!");

    ErrorCode errorCode = imageEffect->imageEffect_->SetInputUri(uri);
    if (errorCode != ErrorCode::SUCCESS) {
        EFFECT_LOGE("SetInputUri: set input uri fail! errorCode=%{public}d", errorCode);
        return ImageEffect_ErrorCode::EFFECT_PARAM_ERROR;
    }

    EventInfo eventInfo = {
        .dataType = EventDataType::URI,
    };
    EventReport::ReportHiSysEvent(INPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputUri(OH_ImageEffect *imageEffect, const char *uri)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    EFFECT_LOGD("Set output uri.");
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputUri: input parameter imageEffect is null!");

    std::string strUri;
    if (uri != nullptr) {
        strUri = uri;
    }
    ErrorCode errorCode = imageEffect->imageEffect_->SetOutputUri(strUri);
    if (errorCode != ErrorCode::SUCCESS) {
        EFFECT_LOGE("SetOutputUri: set output uri fail! errorCode=%{public}d", errorCode);
        return ImageEffect_ErrorCode::EFFECT_PARAM_ERROR;
    }

    EventInfo eventInfo = {
        .dataType = EventDataType::URI,
    };
    EventReport::ReportHiSysEvent(OUTPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputPicture(OH_ImageEffect *imageEffect, OH_PictureNative *picture)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPicture: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPicture: input parameter picture is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetInputPicture(NativeCommonUtils::GetPictureFromNativePicture(picture));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetInputPicture: set input picture fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::PICTURE,
    };
    EventReport::ReportHiSysEvent(INPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputPicture(OH_ImageEffect *imageEffect, OH_PictureNative *picture)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputPicture: input parameter imageEffect is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetOutputPicture(NativeCommonUtils::GetPictureFromNativePicture(picture));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetOutputPicture: set output picture fail! errorCode=%{public}d", errorCode);

    EventInfo eventInfo = {
        .dataType = EventDataType::PICTURE,
    };
    EventReport::ReportHiSysEvent(OUTPUT_DATA_TYPE_STATISTIC, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Start(OH_ImageEffect *imageEffect)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    if (imageEffect == nullptr) {
        ImageEffect_ErrorCode errorCode = ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
        NativeCommonUtils::ReportEventStartFailed(errorCode, "OH_ImageEffect_Start: imageEffect is null");
        EFFECT_LOGE("Start: input parameter imageEffect is null!");
        return errorCode;
    }

    ErrorCode errorCode = imageEffect->imageEffect_->Start();
    if (errorCode != ErrorCode::SUCCESS) {
        ImageEffect_ErrorCode res = NativeCommonUtils::ConvertStartResult(errorCode);
        NativeCommonUtils::ReportEventStartFailed(res, "OH_ImageEffect_Start fail!");
        EFFECT_LOGE("Start: start fail! errorCode=%{public}d", errorCode);
        return res;
    }

    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Stop(OH_ImageEffect *imageEffect)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Stop: input parameter imageEffect is null!");
    imageEffect->imageEffect_->Stop();
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Release(OH_ImageEffect *imageEffect)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Release: input parameter imageEffect is null!");
    delete imageEffect;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Save(OH_ImageEffect *imageEffect, char **info)
{
    EFFECT_LOGD("Save effect.");
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Save: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(info != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Save: input parameter info is null!");

    EffectJsonPtr effectInfo = EFFECTJsonHelper::CreateObject();
    imageEffect->imageEffect_->Save(effectInfo);
    std::string infoStr = effectInfo->ToString();

    char *infoChar = new char[infoStr.length() + 1];
    imageEffect->saveJson = infoChar;
    auto ret = strcpy_s(infoChar, infoStr.length() + 1, infoStr.c_str());
    if (ret != 0) {
        EFFECT_LOGE("Save: strcpy for infoChar failed, ret is %{public}d", ret);
        return ImageEffect_ErrorCode::EFFECT_UNKNOWN;
    }
    *info = infoChar;

    EventInfo eventInfo;
    EventReport::ReportHiSysEvent(SAVE_IMAGE_EFFECT_BEHAVIOR, eventInfo);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_ImageEffect *OH_ImageEffect_Restore(const char *info)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, nullptr, "Restore: input parameter info is null!");
    std::string infoStr = info;
    const EffectJsonPtr root = EFFECTJsonHelper::ParseJsonData(infoStr);
    CHECK_AND_RETURN_RET_LOG(root->HasElement("imageEffect"), nullptr, "OH_ImageEffect_Restore no imageEffect");
    EffectJsonPtr imageInfo = root->GetElement("imageEffect");
    CHECK_AND_RETURN_RET_LOG(imageInfo != nullptr, nullptr, "OH_ImageEffect_Restore imageInfo is nullptr");
    CHECK_AND_RETURN_RET_LOG(imageInfo->HasElement("name"), nullptr, "OH_ImageEffect_Restore no name");

    std::string effectName = imageInfo->GetString("name");
    CHECK_AND_RETURN_RET_LOG(!effectName.empty(), nullptr, "OH_ImageEffect_Restore imageEffect get name failed");
    OH_ImageEffect* ohImageEffect = OH_ImageEffect_Create(effectName.c_str());
    CHECK_AND_RETURN_RET_LOG(ohImageEffect != nullptr, nullptr, "ohImageEffect create failed");
    CHECK_AND_RETURN_RET_LOG(imageInfo->HasElement("filters"), nullptr, "OH_ImageEffect_Restore no filters");
    EffectJsonPtr filters = imageInfo->GetElement("filters");
    CHECK_AND_RETURN_RET_LOG(filters != nullptr, nullptr, "OH_ImageEffect_Restore filters is null!");
    CHECK_AND_RETURN_RET_LOG(filters->IsArray(), nullptr, "OH_ImageEffect_Restore filters not array");
    std::vector<EffectJsonPtr> effects = filters->GetArray();

    for (auto &effect : effects) {
        std::string name = effect->GetString("name");
        CHECK_AND_CONTINUE_LOG(!name.empty(), "Restore: [name] not exist");
        std::shared_ptr<IFilterDelegate> filterDelegate = EFilterFactory::Instance()->GetDelegate(name);
        if (filterDelegate != nullptr) {
            auto *filter = static_cast<OH_EffectFilter *>(filterDelegate->Restore(effect));
            CHECK_AND_CONTINUE_LOG(filter != nullptr, "Restore: filter restore fail! name=%{public}s", name.c_str());
            filter->isCreatedBySystem_ = true;
            ohImageEffect->imageEffect_->AddEFilter(filter->filter_);
            ohImageEffect->filters_.emplace_back(filter, filter->filter_->GetName());
            continue;
        }

        std::unique_ptr<OH_EffectFilter> nativeEFilter = std::make_unique<OH_EffectFilter>();
        std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Restore(name, effect, nativeEFilter.get());
        CHECK_AND_CONTINUE_LOG(efilter != nullptr, "Restore: efilter restore fail! name=%{public}s", name.c_str());
        nativeEFilter->filter_ = efilter;
        nativeEFilter->isCreatedBySystem_ = true;
        ohImageEffect->filters_.emplace_back(nativeEFilter.release(), efilter->GetName());
        ohImageEffect->imageEffect_->AddEFilter(efilter);
    }
    ohImageEffect->imageEffect_->SetExtraInfo(root);

    EventInfo eventInfo;
    EventReport::ReportHiSysEvent(RESTORE_IMAGE_EFFECT_BEHAVIOR, eventInfo);
    return ohImageEffect;
}

EFFECT_EXPORT
int32_t OH_ImageEffect_GetFilterCount(OH_ImageEffect *imageEffect)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, 0, "GetFilterCount: input parameter imageEffect is null!");
    return static_cast<int32_t>(imageEffect->filters_.size());
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_GetFilter(OH_ImageEffect *imageEffect, uint32_t index)
{
    std::unique_lock<std::mutex> lock(effectMutex_);
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "GetFilter: input parameter imageEffect is null!");
    if (index >= static_cast<uint32_t>(imageEffect->filters_.size())) {
        EFFECT_LOGE("GetFilter: input parameter index is invalid!");
        return nullptr;
    }
    return imageEffect->filters_.at(index).first;
}

#ifdef __cplusplus
}
#endif