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

#ifndef IMAGE_EFFECT_VPE_HELPER_H
#define IMAGE_EFFECT_VPE_HELPER_H

#include "surface_buffer.h"

namespace OHOS {
namespace Media {
namespace Effect {
class VpeHelper {
public:
    ~VpeHelper();

    VpeHelper(const VpeHelper &) = delete;

    VpeHelper operator = (const VpeHelper &) = delete;

    static int32_t ColorSpaceConverterCreate(int32_t *instance);
    static int32_t ColorSpaceConverterComposeImage(int32_t instance, sptr<SurfaceBuffer> &inputSdrImage,
        sptr<SurfaceBuffer> &inputGainmap, sptr<SurfaceBuffer> &outputHdrImage, bool legacy);
    static int32_t ColorSpaceConverterDecomposeImage(int32_t instance, sptr<SurfaceBuffer> &inputImage,
        sptr<SurfaceBuffer> &outputSdrImage, sptr<SurfaceBuffer> &outputGainmap);
    static int32_t ColorSpaceConverterProcessImage(int32_t instance, sptr<SurfaceBuffer> &inputImage,
        sptr<SurfaceBuffer> &outputSdrImage);
    static int32_t ColorSpaceConverterDestroy(int32_t *instance);

    static int32_t MetadataGeneratorCreate(int32_t *instance);
    static int32_t MetadataGeneratorProcessImage(int32_t instance, sptr<SurfaceBuffer> &inputImage);
    static int32_t MetadataGeneratorDestroy(int32_t *instance);

private:
    VpeHelper();
    static VpeHelper &GetInstance();
    void LoadVpeSo();
    void UnloadVpeSo();

    class Impl;
    std::shared_ptr<Impl> impl_;

    void *vpeHandle_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_VPE_HELPER_H
