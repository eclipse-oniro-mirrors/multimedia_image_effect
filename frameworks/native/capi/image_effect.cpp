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
#include "json_helper.h"
#include "native_effect_base.h"
#include "native_common_utils.h"
#include "native_window.h"
#include "surface_buffer.h"

#define MAX_EFILTER_NUMS 100

using namespace OHOS::Media;
using namespace OHOS::Media::Effect;

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

    EFFECT_LOGI("Creat image effect. name=%{public}s", name);
    std::shared_ptr<ImageEffect> imageEffect = std::make_unique<ImageEffect>(name);
    std::unique_ptr<OH_ImageEffect> nativeImageEffect = std::make_unique<OH_ImageEffect>();
    nativeImageEffect->imageEffect_ = imageEffect;
    return nativeImageEffect.release();
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_AddFilter(OH_ImageEffect *imageEffect, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "AddFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, nullptr, "AddFilter: input parameter filterName is null!");
    EFFECT_LOGI("Add filter. name=%{public}s", filterName);
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS, nullptr, "filter nums is out of range!");

    OH_EffectFilter *filter = OH_EffectFilter_Create(filterName);
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, nullptr, "AddFilter: create filter fail! name=%{public}s", filterName);

    imageEffect->imageEffect_->AddEFilter(filter->filter_);
    imageEffect->filters_.emplace_back(filter, filterName);
    return filter;
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_InsertFilter(OH_ImageEffect *imageEffect, uint32_t index, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, nullptr, "InsertFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, nullptr, "InsertFilter: input parameter filterName is null!");
    EFFECT_LOGI("Insert filter. name=%{public}s", filterName);
    CHECK_AND_RETURN_RET_LOG(imageEffect->filters_.size() < MAX_EFILTER_NUMS, nullptr, "filter nums is out of range!");

    OH_EffectFilter *filter = OH_EffectFilter_Create(filterName);
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, nullptr, "InsertFilter: create filter fail! filterName=%{public}s",
        filterName);

    ErrorCode result = imageEffect->imageEffect_->InsertEFilter(filter->filter_, index);
    if (result != ErrorCode::SUCCESS) {
        EFFECT_LOGE("InsertFilter: insert filter fail! result=%{public}d, name=%{public}s", result, filterName);
        OH_EffectFilter_Release(filter);
        return nullptr;
    }

    imageEffect->filters_.emplace_back(filter, filterName);
    return filter;
}

EFFECT_EXPORT
int32_t OH_ImageEffect_RemoveFilter(OH_ImageEffect *imageEffect, const char *filterName)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, 0, "RemoveFilter: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(filterName != nullptr, 0, "RemoveFilter: input parameter nativeEffect is null!");

    int32_t count = 0;
    for (auto it = imageEffect->filters_.begin(); it != imageEffect->filters_.end();) {
        if (it->second.compare(filterName) == 0) {
            imageEffect->imageEffect_->RemoveEFilter(it->first->filter_);
            OH_EffectFilter_Release(it->first);
            it = imageEffect->filters_.erase(it);
            count++;
        } else {
            ++it;
        }
    }
    EFFECT_LOGI("Remove filter. name=%{public}s, count=%{public}d", filterName, count);
    return count;
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
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputSurface: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(nativeWindow != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputSurface: input parameter nativeWindow is null!");

    OHOS::sptr<OHOS::Surface> surface = nativeWindow->surface;

    ErrorCode errorCode = imageEffect->imageEffect_->SetOutputSurface(surface);
    if (errorCode != ErrorCode::SUCCESS) {
        EFFECT_LOGE("SetOutputSurface: set output surface fail! errorCode=%{public}d", errorCode);
        return ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID;
    }

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
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputPixelmap(OH_ImageEffect *imageEffect, OH_PixelmapNative *pixelmap)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPixelmap: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(pixelmap != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputPixelmap: input parameter pixelmap is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetInputPixelMap(NativeCommonUtils::GetPixelMapFromOHPixelmap(pixelmap));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetInputPixelMap: set input pixelmap fail! errorCode=%{public}d", errorCode);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputPixelmap(OH_ImageEffect *imageEffect, OH_PixelmapNative *pixelmap)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputPixelmap: input parameter imageEffect is null!");

    ErrorCode errorCode =
        imageEffect->imageEffect_->SetOutputPixelMap(NativeCommonUtils::GetPixelMapFromOHPixelmap(pixelmap));
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetOutputPixelmap: set output pixelmap fail! errorCode=%{public}d", errorCode);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputNativeBuffer: input parameter imageEffect is null!");
    CHECK_AND_RETURN_RET_LOG(nativeBuffer != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetInputNativeBuffer: input parameter input nativeBuffer is null!");

    OHOS::SurfaceBuffer *surfaceBuffer = OHOS::SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);

    ErrorCode errorCode = imageEffect->imageEffect_->SetInputSurfaceBuffer(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS,
        ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetInputNativeBuffer: set input native buffer fail! errorCode=%{public}d", errorCode);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputNativeBuffer(OH_ImageEffect *imageEffect, OH_NativeBuffer *nativeBuffer)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "SetOutputNativeBuffer: input parameter imageEffect is null!");

    OHOS::SurfaceBuffer *surfaceBuffer = OHOS::SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);

    ErrorCode errorCode = imageEffect->imageEffect_->SetOutputSurfaceBuffer(surfaceBuffer);
    CHECK_AND_RETURN_RET_LOG(errorCode == ErrorCode::SUCCESS, ImageEffect_ErrorCode::EFFECT_PARAM_ERROR,
        "SetOutputNativeBuffer: set output native buffer fail! errorCode=%{public}d", errorCode);
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetInputUri(OH_ImageEffect *imageEffect, const char *uri)
{
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
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_SetOutputUri(OH_ImageEffect *imageEffect, const char *uri)
{
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
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
ImageEffect_ErrorCode OH_ImageEffect_Start(OH_ImageEffect *imageEffect)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, ImageEffect_ErrorCode::EFFECT_ERROR_PARAM_INVALID,
        "Start: input parameter imageEffect is null!");

    ErrorCode errorCode = imageEffect->imageEffect_->Start();
    if (errorCode != ErrorCode::SUCCESS) {
        EFFECT_LOGE("Start: start fail! errorCode=%{public}d", errorCode);
        return ImageEffect_ErrorCode::EFFECT_UNKNOWN;
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

    nlohmann::json effectInfo;
    imageEffect->imageEffect_->Save(effectInfo);
    std::string infoStr;
    try {
        infoStr = effectInfo.dump();
    } catch (nlohmann::json::type_error &err) {
        EFFECT_LOGE("Save: fail to dump json object! error:%{public}s", err.what());
        return ImageEffect_ErrorCode::EFFECT_UNKNOWN;
    }

    char *infoChar = new char[infoStr.length() + 1];
    imageEffect->saveJson = infoChar;
    auto ret = strcpy_s(infoChar, infoStr.length() + 1, infoStr.c_str());
    if (ret != 0) {
        EFFECT_LOGE("Save: strcpy for infoChar failed, ret is %{public}d", ret);
        return ImageEffect_ErrorCode::EFFECT_UNKNOWN;
    }
    *info = infoChar;
    return ImageEffect_ErrorCode::EFFECT_SUCCESS;
}

EFFECT_EXPORT
OH_ImageEffect *OH_ImageEffect_Restore(const char *info)
{
    CHECK_AND_RETURN_RET_LOG(info != nullptr, nullptr, "Restore: input parameter info is null!");
    std::string infoStr = info;
    const nlohmann::json root = nlohmann::json::parse(infoStr, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!root.is_discarded(), nullptr, "Restore: json object is null");
    CHECK_AND_RETURN_RET_LOG(JsonHelper::CheckElementExitstence(root, "imageEffect") == ErrorCode::SUCCESS, nullptr,
        "OH_ImageEffect_Restore no imageEffect");
    nlohmann::json imageInfo = root["imageEffect"];
    CHECK_AND_RETURN_RET_LOG(JsonHelper::CheckElementExitstence(imageInfo, "name") == ErrorCode::SUCCESS, nullptr,
        "OH_ImageEffect_Restore no name");
    std::string effectName;
    CHECK_AND_RETURN_RET_LOG(JsonHelper::GetStringValue(imageInfo, "name", effectName) == ErrorCode::SUCCESS, nullptr,
        "OH_ImageEffect_Restore imageEffect get name failed");
    OH_ImageEffect* ohImageEffect = OH_ImageEffect_Create(effectName.c_str());
    CHECK_AND_RETURN_RET_LOG(ohImageEffect != nullptr, nullptr, "ohImageEffect create failed");
    CHECK_AND_RETURN_RET_LOG(JsonHelper::CheckElementExitstence(imageInfo, "filters") == ErrorCode::SUCCESS, nullptr,
        "OH_ImageEffect_Restore no filters");
    nlohmann::json effects;
    CHECK_AND_RETURN_RET_LOG(JsonHelper::GetArray(imageInfo, "filters", effects) == ErrorCode::SUCCESS, nullptr,
        "OH_ImageEffect_Restore filters not array");

    for (auto &effect : effects) {
        std::string name;
        CHECK_AND_CONTINUE_LOG(JsonHelper::GetStringValue(effect, "name", name) == ErrorCode::SUCCESS,
            "Restore: [name] not exist");
        std::shared_ptr<IFilterDelegate> filterDelegate = EFilterFactory::Instance()->GetDelegate(name);
        if (filterDelegate != nullptr) {
            auto *filter = static_cast<OH_EffectFilter *>(filterDelegate->Restore(effect));
            CHECK_AND_CONTINUE_LOG(filter != nullptr, "Restore: filter restore fail! name=%{public}s", name.c_str());
            ohImageEffect->imageEffect_->AddEFilter(filter->filter_);
            ohImageEffect->filters_.emplace_back(filter, filter->filter_->GetName());
            continue;
        }

        std::unique_ptr<OH_EffectFilter> nativeEFilter = std::make_unique<OH_EffectFilter>();
        std::shared_ptr<EFilter> efilter = EFilterFactory::Instance()->Restore(name, effect, nativeEFilter.get());
        nativeEFilter->filter_ = efilter;
        ohImageEffect->filters_.emplace_back(nativeEFilter.release(), efilter->GetName());
        ohImageEffect->imageEffect_->AddEFilter(efilter);
    }
    ohImageEffect->imageEffect_->SetExtraInfo(root);
    return ohImageEffect;
}

EFFECT_EXPORT
int32_t OH_ImageEffect_GetFilterCount(OH_ImageEffect *imageEffect)
{
    CHECK_AND_RETURN_RET_LOG(imageEffect != nullptr, 0, "GetFilterCount: input parameter imageEffect is null!");
    return static_cast<int32_t>(imageEffect->filters_.size());
}

EFFECT_EXPORT
OH_EffectFilter *OH_ImageEffect_GetFilter(OH_ImageEffect *imageEffect, uint32_t index)
{
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