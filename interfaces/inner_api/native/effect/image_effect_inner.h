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
#include <queue>
#include <optional>
#include <condition_variable>
#include <utility>

#include "any.h"
#include "effect.h"
#include "external_window.h"
#include "image_type.h"
#include "surface.h"
#include "pixel_map.h"
#include "image_effect_marco_define.h"
#include "render_thread.h"
#include "picture.h"

#define TIME_FOR_WAITING_BUFFER 2500

namespace OHOS {
namespace Media {
namespace Effect {
struct SurfaceBufferInfo {
    SurfaceBuffer *surfaceBuffer_ = nullptr;
    int64_t timestamp_ = 0;
};

struct TextureInfo {
    int32_t textureId_ = 0;
    int32_t colorSpace_ = 0;
};

struct DataInfo {
    DataType dataType_ = DataType::UNKNOWN;
    PixelMap *pixelMap_ = nullptr;
    SurfaceBufferInfo surfaceBufferInfo_;
    TextureInfo textureInfo_;
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

struct BufferEntry {
    uint32_t seqNum_;
    sptr<SurfaceBuffer> buffer_;
    sptr<SyncFence> syncFence_;
    int64_t timestamp_;
};

template <typename T>
class ThreadSafeBufferQueue {
public:
    explicit ThreadSafeBufferQueue(size_t max_capacity = std::numeric_limits<size_t>::max())
        : max_capacity_(std::max<size_t>(1, max_capacity)) {}

    ThreadSafeBufferQueue(const ThreadSafeBufferQueue&) = delete;
    ThreadSafeBufferQueue& operator=(const ThreadSafeBufferQueue&) = delete;

    bool TryPush(T&& element, bool wait = true,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(TIME_FOR_WAITING_BUFFER))
    {
        std::unique_lock lock(mutex_);

        if (!wait) {
            return queue_.size() < max_capacity_ ? CommitPush(std::forward<T>(element), lock) : false;
        }

        if (!WaitForSpace(lock, timeout)) {
            return false;
        }

        return CommitPush(std::forward<T>(element), lock);
    }

    std::optional<T> TryPop(bool wait = true, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
    {
        std::unique_lock lock(mutex_);
        if (!wait) {
            return !queue_.empty() ? CommitPop(lock) : std::nullopt;
        }

        if (!WaitForData(lock, timeout)) {
            return std::nullopt;
        }

        return CommitPop(lock);
    }

    size_t Size() const
    {
        std::lock_guard lock(mutex_);
        return queue_.size();
    }

private:
    template<typename Rep = int, typename Period = std::milli>
    bool WaitForSpace(std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& timeout)
    {
        if (timeout > std::chrono::duration<Rep, Period>::zero()) {
            return not_full_cv_.wait_for(lock, timeout, [this] {
                return queue_.size() < max_capacity_;
            });
        }
        not_full_cv_.wait(lock, [this] {
            return queue_.size() < max_capacity_;
        });
        return true;
    }

    template<typename Rep = int, typename Period = std::milli>
    bool WaitForData(std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& timeout)
    {
        if (timeout > std::chrono::duration<Rep, Period>::zero()) {
            return not_empty_cv_.wait_for(lock, timeout, [this] {
                return !queue_.empty();
            });
        }
        not_empty_cv_.wait(lock, [this] {
            return !queue_.empty();
        });
        return true;
    }

    bool CommitPush(T&& element, std::unique_lock<std::mutex>& lock)
    {
        queue_.emplace(std::forward<T>(element));
        lock.unlock();
        not_empty_cv_.notify_one();
        return true;
    }

    std::optional<T> CommitPop(std::unique_lock<std::mutex>& lock)
    {
        T element = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        not_full_cv_.notify_one();
        return element;
    }

    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable not_full_cv_;
    std::condition_variable not_empty_cv_;
    const size_t max_capacity_;
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

    IMAGE_EFFECT_EXPORT ErrorCode SetInputTexture(int32_t textureId, int32_t colorSpace);

    IMAGE_EFFECT_EXPORT ErrorCode SetOutputTexture(int32_t textureId);

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
    bool SubmitRenderTask(BufferEntry&& entry);
    void RenderBuffer();
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
    ErrorCode GetImageInfoFromTexInfo(uint32_t &width, uint32_t &height, PixelFormat &pixelFormat) const;

    void UpdateConsumerBuffersNumber();
    void UpdateCycleBuffersNumber();

    void SetPathToSink();

    ErrorCode InitEffectBuffer(std::shared_ptr<EffectBuffer> &srcEffectBuffer,
        std::shared_ptr<EffectBuffer> &dstEffectBuffer, IEffectFormat format);

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
    std::shared_ptr<ThreadSafeBufferQueue<BufferEntry>> bufferPool_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_IMAGE_EFFECT_H
