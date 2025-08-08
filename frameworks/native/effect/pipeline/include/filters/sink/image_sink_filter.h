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

#ifndef IE_PIPELINE_FILTERS_IMAGE_SINK_FILTER_H
#define IE_PIPELINE_FILTERS_IMAGE_SINK_FILTER_H

#include <surface.h>
#include "filter_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
class ImageSinkFilter : public FilterBase {
public:
    explicit ImageSinkFilter(const std::string &name) : FilterBase(name)
    {
        filterType_ = FilterType::OUTPUT_SINK;
    }

    ~ImageSinkFilter() override
    {
        if (hdrSurfaceBuffer_) {
            hdrSurfaceBuffer_->DecStrongRef(hdrSurfaceBuffer_);
            hdrSurfaceBuffer_ = nullptr;
        }

        DestoryTexureCache();
    }

    struct TextureCacheSeq {
        TextureCacheSeq() : texId_(0), eglImage_(nullptr), eglSync_(nullptr) {}
        TextureCacheSeq(unsigned int texId, void* img, void* sync)
            : texId_(texId), eglImage_(img), eglSync_(sync) {}
        unsigned int texId_;
        void* eglImage_;
        void* eglSync_;
    };

    virtual ErrorCode SetSink(const std::shared_ptr<EffectBuffer> &sink);

    void DestoryTexureCache();

    ErrorCode SetXComponentSurface(sptr<Surface> &surface);

    ErrorCode SetParameter(int32_t key, const Media::Any &value) override
    {
        return FilterBase::SetParameter(key, value);
    }

    ErrorCode GetParameter(int32_t key, Media::Any &value) override
    {
        return FilterBase::GetParameter(key, value);
    }

    ErrorCode Start() override;

    void Negotiate(const std::string& inPort, const std::shared_ptr<Capability> &capability,
        std::shared_ptr<EffectContext> &context) override;

    void FlushBufferToScreen(sptr<SurfaceBuffer> &outBuffer, sptr<SyncFence> &fence) const;

    void RequestBufferFromScreen(BufferRequestConfig &requestConfig, sptr<SurfaceBuffer> &outBuffer,
        sptr<SyncFence> &syncFence) const;

    sptr<SurfaceBuffer> GetOrCreateSurfaceBuffer(const BufferRequestConfig &requestConfig);

    static BufferRequestConfig CreateBaseBufferConfig(int32_t width, int32_t height, GraphicPixelFormat format,
        GraphicTransformType transform, GraphicColorGamut colorGamut);

    ErrorCode SurfaceRenderFlow(SurfaceBuffer* srcBuffer, BufferRequestConfig &requestConfig,
        MetaDataMap& hdrMetaDataMap, const int32_t &colorSpaceType,
        const std::shared_ptr<EffectContext> &context) const;

    ErrorCode TextureRenderFlow(RenderTexturePtr texture, BufferRequestConfig &requestConfig,
        MetaDataMap &hdrMetaDataMap, const int32_t &colorSpaceType,
        const std::shared_ptr<EffectContext> &context);

    ErrorCode RenderToDisplay(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context);
    ErrorCode RenderHdr10(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context);
    ErrorCode Render8GainMap(const std::shared_ptr<EffectBuffer> &buffer, std::shared_ptr<EffectContext> &context);

    ErrorCode ProcessDisplayForNoTex(const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context);

    ErrorCode PushData(const std::string &inPort, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context) override;

    ErrorCode PackToFile(const std::string &path, const std::shared_ptr<Picture> &picture);

    ErrorCode SaveUrlData(const std::string &url, const std::shared_ptr<EffectBuffer> &buffer);

    ErrorCode SaveUrlData(const std::string &url, const std::shared_ptr<Picture> &picture);

    ErrorCode SavePathData(const std::string &path, const std::shared_ptr<EffectBuffer> &buffer);

    ErrorCode SavePathData(const std::string &path, const std::shared_ptr<Picture> &picture);

    ErrorCode SaveInputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &buffer,
        std::shared_ptr<EffectContext> &context);

    ErrorCode SavaOutputData(EffectBuffer *src, const std::shared_ptr<EffectBuffer> &inputBuffer,
        std::shared_ptr<EffectBuffer> &outputBuffer, std::shared_ptr<EffectContext> &context);

    ErrorCode SaveData(const std::shared_ptr<EffectBuffer> &inputBuffer, std::shared_ptr<EffectBuffer> &outputBuffer,
        std::shared_ptr<EffectContext> &context);

    std::string inPath_;

private:
    void OnEvent(const Event &event) override {}

    sptr<Surface> toXComponentSurface_;
    std::unordered_map<uint32_t, TextureCacheSeq> texureCacheSeqs_;
    sptr<SurfaceBuffer> hdrSurfaceBuffer_ = nullptr;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif