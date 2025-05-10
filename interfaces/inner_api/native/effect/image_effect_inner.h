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
#include <unordered_set>

#include "any.h"
#include "effect.h"
#include "external_window.h"
#include "image_type.h"
#include "surface.h"
#include "pixel_map.h"
#include "image_effect_marco_define.h"
#include "render_thread.h"
#include "picture.h"

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
    Picture *picture_ = nullptr;
};

struct BufferProcessInfo {
    sptr<SurfaceBuffer> inBuffer_;
    sptr<SurfaceBuffer> outBuffer_;
    sptr<SyncFence> inBufferSyncFence_;
    sptr<SyncFence> outBufferSyncFence_;
    bool isSrcHebcData_ = false;
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

    IMAGE_EFFECT_EXPORT ErrorCode SetInputPicture(Picture *picture);

    IMAGE_EFFECT_EXPORT ErrorCode SetOutputPicture(Picture *picture);

protected:
    IMAGE_EFFECT_EXPORT virtual ErrorCode Render();

    IMAGE_EFFECT_EXPORT static void ClearDataInfo(DataInfo &dataInfo);

    IMAGE_EFFECT_EXPORT static ErrorCode ParseDataInfo(DataInfo &dataInfo, std::shared_ptr<EffectBuffer> &effectBuffer,
        bool isOutputData, IEffectFormat format, LOG_STRATEGY strategy = LOG_STRATEGY::NORMAL);

    DataInfo inDateInfo_;
    DataInfo outDateInfo_;

private:
    ErrorCode LockAll(std::shared_ptr<EffectBuffer> &srcEffectBuffer, std::shared_ptr<EffectBuffer> &dstEffectBuffer,
        IEffectFormat format);

    void RemoveGainMapIfNeed() const;

    static void UnLockData(DataInfo &dataInfo);
    static BufferRequestConfig GetBufferRequestConfig(const sptr<SurfaceBuffer>& buffer);

    void UnLockAll();

    void InitEGLEnv();

    void DestroyEGLEnv();

    IMAGE_EFFECT_EXPORT
    void ConsumerBufferAvailable();
    void UpdateProducerSurfaceInfo();

    void ExtInitModule();
    void ExtDeinitModule();

    unsigned long int RequestTaskId();

    void ConsumerBufferWithGPU(sptr<SurfaceBuffer>& buffer);
    void OnBufferAvailableWithCPU();
    bool RenderBuffer(sptr<SurfaceBuffer>& inBuffer, sptr<SurfaceBuffer>& outBuffer, int64_t& timestamp);
    GSError FlushBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence, bool isNeedAttach, bool sendFence,
        int64_t& timestamp);
    GSError ReleaseBuffer(sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence);
    void ProcessRender(BufferProcessInfo& bufferProcessInfo, bool& isNeedSwap, int64_t& timestamp);
    void ProcessSwapBuffers(BufferProcessInfo& bufferProcessInfo, int64_t& timestamp);

    ErrorCode GetImageInfo(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
        std::shared_ptr<ExifMetadata> &exifMetadata);
    ErrorCode ConfigureFilters(std::shared_ptr<EffectBuffer> srcEffectBuffer,
        std::shared_ptr<EffectBuffer> dstEffectBuffer);
    ErrorCode GetImageInfoFromPixelMap(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
            std::shared_ptr<ExifMetadata> &exifMetadata) const;
    ErrorCode GetImageInfoFromSurface(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat) const;
    ErrorCode GetImageInfoFromPath(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
        std::shared_ptr<ExifMetadata> &exifMetadata) const;
    ErrorCode GetImageInfoFromPicture(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat,
        std::shared_ptr<ExifMetadata> &exifMetadata) const;

    void UpdateConsumerBuffersNumber();
    void UpdateCycleBuffersNumber();

    void SetPathToSink();

    sptr<Surface> toProducerSurface_;   // from ImageEffect to XComponent
    sptr<Surface> fromProducerSurface_; // to camera hal
    volatile int32_t imageEffectFlag_ = 0;
    bool setCycleBuffersNumber_ = false;
    bool setConsumerBufferSize_ = false;

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
    std::mutex innerEffectMutex_;
    RenderThread<> *m_renderThread{ nullptr };
    std::atomic_ullong m_currentTaskId{0};
    bool needPreFlush_ = false;
    uint32_t failureCount_ = 0;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_IMAGE_EFFECT_H
