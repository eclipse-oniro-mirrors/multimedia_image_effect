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

#include "vpe_helper.h"

#include <dlfcn.h>

#include "effect_log.h"
#include "error_code.h"
#include "external_window.h"

namespace OHOS {
namespace Media {
namespace Effect {
using VpeColorSpaceConverterCreate = int32_t (*)(int32_t *);
using VpeColorSpaceConverterComposeImage =
    int32_t (*)(int32_t, OHNativeWindowBuffer*, OHNativeWindowBuffer *, OHNativeWindowBuffer *, bool);
using VpeColorSpaceConverterDecomposeImage =
    int32_t (*)(int32_t, OHNativeWindowBuffer *, OHNativeWindowBuffer *, OHNativeWindowBuffer *);
using VpeColorSpaceConverterProcessImage =
    int32_t (*)(int32_t, OHNativeWindowBuffer *, OHNativeWindowBuffer *);
using VpeColorSpaceConverterDestroy = int32_t (*)(int32_t *);

using VpeMetadataGeneratorCreate = int32_t (*)(int32_t *);
using VpeMetadataGeneratorProcessImage = int32_t (*)(int32_t, OHNativeWindowBuffer *);
using VpeMetadataGeneratorDestroy = int32_t (*)(int32_t *);

class VpeHelper::Impl {
public:
    VpeColorSpaceConverterCreate colorSpaceConverterCreate_ = nullptr;
    VpeColorSpaceConverterComposeImage colorSpaceConverterComposeImage_ = nullptr;
    VpeColorSpaceConverterDecomposeImage colorSpaceConverterDecomposeImage_ = nullptr;
    VpeColorSpaceConverterProcessImage colorSpaceConverterProcessImage_ = nullptr;
    VpeColorSpaceConverterDestroy colorSpaceConverterDestroy_ = nullptr;

    VpeMetadataGeneratorCreate metadataGeneratorCreate_ = nullptr;
    VpeMetadataGeneratorProcessImage metadataGeneratorProcessImage_ = nullptr;
    VpeMetadataGeneratorDestroy metadataGeneratorDestroy_ = nullptr;
};

VpeHelper::VpeHelper()
{
    impl_ = std::make_shared<VpeHelper::Impl>();
    LoadVpeSo();
}

VpeHelper::~VpeHelper()
{
    UnloadVpeSo();
}

void VpeHelper::LoadVpeSo()
{
    EFFECT_LOGI("VpeHelper::LoadVpeSo enter!");
    void *vpeHandle = dlopen("libvideoprocessingengine.z.so", RTLD_NOW);
    if (vpeHandle == nullptr) {
        EFFECT_LOGE("VpeHelper: dlopen libvideoprocessingengine.so failed! dlerror=%{public}s", dlerror());
        return;
    }
    EFFECT_LOGI("VpeHelper: dlopen libvideoprocessingengine.so success!");

    vpeHandle_ = vpeHandle;
    impl_->colorSpaceConverterCreate_ =
        reinterpret_cast<VpeColorSpaceConverterCreate>(dlsym(vpeHandle, "ColorSpaceConverterCreate"));
    impl_->colorSpaceConverterComposeImage_ =
        reinterpret_cast<VpeColorSpaceConverterComposeImage>(dlsym(vpeHandle, "ColorSpaceConverterComposeImage"));
    impl_->colorSpaceConverterDecomposeImage_ =
        reinterpret_cast<VpeColorSpaceConverterDecomposeImage>(dlsym(vpeHandle, "ColorSpaceConverterDecomposeImage"));
    impl_->colorSpaceConverterProcessImage_ =
        reinterpret_cast<VpeColorSpaceConverterProcessImage>(dlsym(vpeHandle, "ColorSpaceConverterProcessImage"));
    impl_->colorSpaceConverterDestroy_ =
        reinterpret_cast<VpeColorSpaceConverterDestroy>(dlsym(vpeHandle, "ColorSpaceConverterDestroy"));

    impl_->metadataGeneratorCreate_ =
        reinterpret_cast<VpeMetadataGeneratorCreate>(dlsym(vpeHandle, "MetadataGeneratorCreate"));
    impl_->metadataGeneratorProcessImage_ =
        reinterpret_cast<VpeMetadataGeneratorProcessImage>(dlsym(vpeHandle, "MetadataGeneratorProcessImage"));
    impl_->metadataGeneratorDestroy_ =
        reinterpret_cast<VpeMetadataGeneratorDestroy>(dlsym(vpeHandle, "MetadataGeneratorDestroy"));
}

void VpeHelper::UnloadVpeSo()
{
    if (vpeHandle_ != nullptr) {
        dlclose(vpeHandle_);
        vpeHandle_ = nullptr;
    }
}

VpeHelper &VpeHelper::GetInstance()
{
    static VpeHelper instance;
    return instance;
}

int32_t VpeHelper::ColorSpaceConverterCreate(int32_t *instance)
{
    VpeColorSpaceConverterCreate &createFunc = GetInstance().impl_->colorSpaceConverterCreate_;
    CHECK_AND_RETURN_RET_LOG(instance != nullptr && createFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "ColorSpaceConverterCreate: function is null!");

    return createFunc(instance);
}

int32_t VpeHelper::ColorSpaceConverterComposeImage(int32_t instance, sptr<SurfaceBuffer> &inputSdrImage,
    sptr<SurfaceBuffer> &inputGainmap, sptr<SurfaceBuffer> &outputHdrImage, bool legacy)
{
    VpeColorSpaceConverterComposeImage &composeImageFunc = GetInstance().impl_->colorSpaceConverterComposeImage_;
    CHECK_AND_RETURN_RET_LOG(composeImageFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "ColorSpaceConverterComposeImage: function is null!");
    OHNativeWindowBuffer *sdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&inputSdrImage);
    OHNativeWindowBuffer *gainmap = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&inputGainmap);
    OHNativeWindowBuffer *hdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&outputHdrImage);
    int32_t res = composeImageFunc(instance, sdr, gainmap, hdr, legacy);
    OH_NativeWindow_DestroyNativeWindowBuffer(hdr);
    OH_NativeWindow_DestroyNativeWindowBuffer(gainmap);
    OH_NativeWindow_DestroyNativeWindowBuffer(sdr);
    return res;
}

int32_t VpeHelper::ColorSpaceConverterDecomposeImage(int32_t instance, sptr<SurfaceBuffer> &inputImage,
    sptr<SurfaceBuffer> &outputSdrImage, sptr<SurfaceBuffer> &outputGainmap)
{
    VpeColorSpaceConverterDecomposeImage &decomposeImageFunc = GetInstance().impl_->colorSpaceConverterDecomposeImage_;
    CHECK_AND_RETURN_RET_LOG(decomposeImageFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "ColorSpaceConverterDecomposeImage: function is null!");
    OHNativeWindowBuffer *hdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&inputImage);
    OHNativeWindowBuffer *sdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&outputSdrImage);
    OHNativeWindowBuffer *gainmap = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&outputGainmap);
    int32_t res = decomposeImageFunc(instance, hdr, sdr, gainmap);
    OH_NativeWindow_DestroyNativeWindowBuffer(gainmap);
    OH_NativeWindow_DestroyNativeWindowBuffer(sdr);
    OH_NativeWindow_DestroyNativeWindowBuffer(hdr);
    return res;
}

int32_t VpeHelper::ColorSpaceConverterProcessImage(int32_t instance, sptr<SurfaceBuffer> &inputImage,
    sptr<SurfaceBuffer> &outputSdrImage)
{
    VpeColorSpaceConverterProcessImage &processImageFunc = GetInstance().impl_->colorSpaceConverterProcessImage_;
    CHECK_AND_RETURN_RET_LOG(processImageFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "ColorSpaceConverterProcessImage: function is null!");
    OHNativeWindowBuffer *hdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&inputImage);
    OHNativeWindowBuffer *sdr = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&outputSdrImage);
    int32_t res = processImageFunc(instance, hdr, sdr);
    OH_NativeWindow_DestroyNativeWindowBuffer(sdr);
    OH_NativeWindow_DestroyNativeWindowBuffer(hdr);
    return res;
}

int32_t VpeHelper::ColorSpaceConverterDestroy(int32_t *instance)
{
    VpeColorSpaceConverterDestroy &destroyFunc = GetInstance().impl_->colorSpaceConverterDestroy_;
    CHECK_AND_RETURN_RET_LOG(instance != nullptr && destroyFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "ColorSpaceConverterDestroy: function is null!");

    EFFECT_LOGI("ColorSpaceConverterDestroy: instance=%{public}d", *instance);
    return destroyFunc(instance);
}

int32_t VpeHelper::MetadataGeneratorCreate(int32_t *instance)
{
    VpeMetadataGeneratorCreate &createFunc = GetInstance().impl_->metadataGeneratorCreate_;
    CHECK_AND_RETURN_RET_LOG(instance != nullptr && createFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "MetadataGeneratorCreate: function is null!");

    return createFunc(instance);
}

int32_t VpeHelper::MetadataGeneratorProcessImage(int32_t instance, sptr<SurfaceBuffer> &inputImage)
{
    VpeMetadataGeneratorProcessImage &processImage = GetInstance().impl_->metadataGeneratorProcessImage_;
    CHECK_AND_RETURN_RET_LOG(processImage != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "MetadataGeneratorProcessImage: function is null!");
    OHNativeWindowBuffer *input = OH_NativeWindow_CreateNativeWindowBufferFromSurfaceBuffer(&inputImage);
    int32_t res = processImage(instance, input);
    OH_NativeWindow_DestroyNativeWindowBuffer(input);
    return res;
}

int32_t VpeHelper::MetadataGeneratorDestroy(int32_t *instance)
{
    VpeMetadataGeneratorDestroy &destroyFunc = GetInstance().impl_->metadataGeneratorDestroy_;
    CHECK_AND_RETURN_RET_LOG(instance != nullptr && destroyFunc != nullptr, (int32_t)ErrorCode::ERR_PARAM_INVALID,
        "MetadataGeneratorDestroy: function is null!");

    EFFECT_LOGI("MetadataGeneratorDestroy: instance=%{public}d", *instance);
    return destroyFunc(instance);
}

} // namespace Effect
} // namespace Media
} // namespace OHOS