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

#include "any.h"
#include "effect.h"
#include "external_window.h"
#include "image_type.h"
#include "surface.h"
#include "pixel_map.h"

namespace OHOS {
namespace Media {
namespace Effect {
struct DataInfo {
    DataType dataType_ = DataType::UNKNOWN;
    PixelMap *pixelMap_ = nullptr;
    SurfaceBuffer *surfaceBuffer_ = nullptr;
    std::string uri_;
    std::string path_;
};

class ImageEffect : public Effect {
public:
    ImageEffect(const char *name = nullptr);
    ~ImageEffect();

    void AddEFilter(const std::shared_ptr<EFilter> &effect) override;

    ErrorCode InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index) override;

    void RemoveEFilter(const std::shared_ptr<EFilter> &efilter) override;

    virtual ErrorCode SetInputPixelMap(PixelMap *pixelMap);

    ErrorCode Start() override;

    ErrorCode Save(nlohmann::json &res) override;

    static std::shared_ptr<ImageEffect> Restore(std::string &info);

    virtual ErrorCode SetOutputPixelMap(PixelMap *pixelMap);

    virtual ErrorCode SetOutputSurface(sptr<Surface> &surface);
    sptr<Surface> GetInputSurface();

    virtual ErrorCode Configure(const std::string &key, const Plugin::Any &value);

    void Stop();

    ErrorCode SetInputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer);

    ErrorCode SetOutputSurfaceBuffer(OHOS::SurfaceBuffer *surfaceBuffer);

    ErrorCode SetInputUri(const std::string &uri);

    ErrorCode SetOutputUri(const std::string &uri);

    ErrorCode SetInputPath(const std::string &path);

    ErrorCode SetOutputPath(const std::string &path);

    ErrorCode SetExtraInfo(nlohmann::json res);

protected:
    virtual ErrorCode Render();

    static void ClearDataInfo(DataInfo &dataInfo);

    static ErrorCode ParseDataInfo(DataInfo &dataInfo, std::shared_ptr<EffectBuffer> &effectBuffer,
        bool isOutputData);

    DataInfo inDateInfo_;
    DataInfo outDateInfo_;

private:
    ErrorCode LockAll(std::shared_ptr<EffectBuffer> &srcEffectBuffer, std::shared_ptr<EffectBuffer> &dstEffectBuffer);

    static void UnLockData(DataInfo &dataInfo);

    void UnLockAll();

    void InitEGLEnv();

    void DestroyEGLEnv();

    void ConsumerBufferAvailable(sptr<SurfaceBuffer> &buffer, const OHOS::Rect &damages, int64_t timestamp);
    void UpdateProducerSurfaceInfo();

    void ExtInitModule();
    void ExtDeinitModule();

    sptr<Surface> toProducerSurface_;   // from ImageEffect to XComponent
    sptr<Surface> fromProducerSurface_; // to camera hal

    GraphicTransformType toProducerTransform_ = GRAPHIC_ROTATE_BUTT;

    // envSupportIpType
    std::vector<IPType> ipType_ = {
        { IPType::CPU, IPType::GPU },
    };

    std::map<ConfigType, Plugin::Any> config_ = { { ConfigType::IPTYPE, ipType_ } };

    nlohmann::json extraInfo_ = nullptr;

    std::string name_;

    class Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_IMAGE_EFFECT_H
