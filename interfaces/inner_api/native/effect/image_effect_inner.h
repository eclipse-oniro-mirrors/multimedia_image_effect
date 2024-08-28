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

#ifndef IMAGE_EFFECT_IMAGE_EFFECT_H
#define IMAGE_EFFECT_IMAGE_EFFECT_H

#include <vector>
#include <mutex>

#include "any.h"
#include "effect.h"
#include "external_window.h"
#include "image_type.h"
#include "surface.h"
#include "pixel_map.h"
#include "render_thread.h"
#include "image_effect_marco_define.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct SurfaceBufferInfo {
    SurfaceBuffer *surfaceBuffer_ = nullptr;
    int64_t timestamp_ = 0;
};

struct DataInfo {
    DataType dataType_ = DataType::UNKNOWN;
    PixelMap *pixelMap_ = nullptr;
    SurfaceBufferInfo surfaceBufferInfo_;
    std::string uri_;
    std::string path_;
};

class ImageEffect : public Effect {
public:
    IMAGE_EFFECT_EXPORT ImageEffect(const char *name = nullptr);
    IMAGE_EFFECT_EXPORT ~ImageEffect();

    IMAGE_EFFECT_EXPORT void AddEFilter(const std::shared_ptr<EFilter> &effect) override;

    IMAGE_EFFECT_EXPORT ErrorCode InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index) override;

    IMAGE_EFFECT_EXPORT void RemoveEFilter(const std::shared_ptr<EFilter> &efilter) override;
    IMAGE_EFFECT_EXPORT ErrorCode RemoveEFilter(uint32_t index) override;

    IMAGE_EFFECT_EXPORT ErrorCode ReplaceEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index) override;

    IMAGE_EFFECT_EXPORT virtual ErrorCode SetInputPixelMap(PixelMap *pixelMap);

    IMAGE_EFFECT_EXPORT ErrorCode Start() override;

    IMAGE_EFFECT_EXPORT ErrorCode Save(EffectJsonPtr &res) override;

    IMAGE_EFFECT_EXPORT static std::shared_ptr<ImageEffect> Restore(std::string &info);

    IMAGE_EFFECT_EXPORT virtual ErrorCode SetOutputPixelMap(PixelMap *pixelMap);

    IMAGE_EFFECT_EXPORT virtual ErrorCode SetOutputSurface(sptr<Surface> &surface);

    IMAGE_EFFECT_EXPORT virtual ErrorCode SetOutNativeWindow(OHNativeWindow *nativeWindow);
    IMAGE_EFFECT_EXPORT sptr<Surface> GetInputSurface();

    IMAGE_EFFECT_EXPORT virtual ErrorCode Configure(const std::string &key, const Plugin::Any &value);

    IMAGE_EFFECT_EXPORT void Stop();

    IMAGE_EFFECT_EXPORT ErrorCode SetInputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer);

    IMAGE_EFFECT_EXPORT ErrorCode SetOutputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer);

    IMAGE_EFFECT_EXPORT ErrorCode SetInputUri(const std::string &uri);

    IMAGE_EFFECT_EXPORT ErrorCode SetOutputUri(const std::string &uri);

    IMAGE_EFFECT_EXPORT ErrorCode SetInputPath(const std::string &path);

    IMAGE_EFFECT_EXPORT ErrorCode SetOutputPath(const std::string &path);

    IMAGE_EFFECT_EXPORT ErrorCode SetExtraInfo(EffectJsonPtr res);

protected:
    IMAGE_EFFECT_EXPORT virtual ErrorCode Render();

    IMAGE_EFFECT_EXPORT static void ClearDataInfo(DataInfo &dataInfo);

    IMAGE_EFFECT_EXPORT static ErrorCode ParseDataInfo(DataInfo &dataInfo, std::shared_ptr<EffectBuffer> &effectBuffer,
        bool isOutputData);

    DataInfo inDateInfo_;
    DataInfo outDateInfo_;

private:
    ErrorCode LockAll(std::shared_ptr<EffectBuffer> &srcEffectBuffer, std::shared_ptr<EffectBuffer> &dstEffectBuffer);

    static void UnLockData(DataInfo &dataInfo);

    void UnLockAll();

    void InitEGLEnv();

    void DestroyEGLEnv();

    IMAGE_EFFECT_EXPORT
    void ConsumerBufferAvailable(sptr<SurfaceBuffer> &buffer, const OHOS::Rect &damages, int64_t timestamp);
    void UpdateProducerSurfaceInfo();

    void ExtInitModule();
    void ExtDeinitModule();

    unsigned long int RequestTaskId();

    void ConsumerBufferWithGPU(sptr<SurfaceBuffer>& buffer);
    void OnBufferAvailableWithCPU(sptr<SurfaceBuffer> &buffer, const OHOS::Rect &damages, int64_t timestamp);
    void OnBufferAvailableToProcess(sptr<SurfaceBuffer> &buffer, sptr<SurfaceBuffer> &outBuffer, int64_t timestamp);

    sptr<Surface> toProducerSurface_;   // from ImageEffect to XComponent
    sptr<Surface> fromProducerSurface_; // to camera hal

    GraphicTransformType toProducerTransform_ = GRAPHIC_ROTATE_BUTT;

    // envSupportIpType
    std::vector<IPType> ipType_ = {
        { IPType::CPU, IPType::GPU },
    };

    std::map<ConfigType, Plugin::Any> config_ = { { ConfigType::IPTYPE, ipType_ } };

    EffectJsonPtr extraInfo_ = nullptr;

    std::string name_;

    class Impl;
    std::shared_ptr<Impl> impl_;
    RenderThread<> *m_renderThread{ nullptr };
    std::atomic_ullong m_currentTaskId{0};

    std::mutex innerEffectMutex;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_IMAGE_EFFECT_H
