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

#ifndef IMAGE_EFFECT_MOCK_SURFACE_BUFFER_H
#define IMAGE_EFFECT_MOCK_SURFACE_BUFFER_H

#include "gmock/gmock.h"
#include "surface_buffer.h"
#include "display_type.h"

struct BufferWrapper {};

constexpr int32_t WIDTH = 1920;
constexpr int32_t HEIGHT = 1080;
constexpr int32_t STRIDE = WIDTH * 4;
constexpr uint64_t USAGE = 43;
constexpr uint32_t SIZE = STRIDE * HEIGHT;

namespace OHOS {
namespace Media {
namespace Effect {
class MockSurfaceBuffer : public SurfaceBuffer {
public:
    MockSurfaceBuffer()
    {
        virAddr = malloc(SIZE);
    };

    ~MockSurfaceBuffer()
    {
        free(virAddr);
        virAddr = nullptr;
    };

    BufferHandle *GetBufferHandle() const override
    {
        return 0;
    };

    int32_t GetWidth() const override
    {
        return WIDTH;
    };

    int32_t GetHeight() const override
    {
        return HEIGHT;
    };

    int32_t GetStride() const override
    {
        return STRIDE;
    };

    int32_t GetFormat() const override
    {
        return ::PixelFormat::PIXEL_FMT_RGBA_8888;
    };

    uint64_t GetUsage() const override
    {
        return USAGE;
    };

    uint64_t GetPhyAddr() const override
    {
        return 0;
    };

    void *GetVirAddr() override
    {
        return virAddr;
    };

    int32_t GetFileDescriptor() const override
    {
        return 0;
    };

    uint32_t GetSize() const override
    {
        return SIZE;
    };

    const GraphicColorGamut &GetSurfaceBufferColorGamut() const override
    {
        return m_colorGamut;
    }

    const GraphicTransformType &GetSurfaceBufferTransform() const override
    {
        return m_transform;
    }

    void SetSurfaceBufferColorGamut(const GraphicColorGamut &colorGamut) override {}

    void SetSurfaceBufferTransform(const GraphicTransformType &transform) override {}

    int32_t GetSurfaceBufferWidth() const override
    {
        return 0;
    };

    int32_t GetSurfaceBufferHeight() const override
    {
        return 0;
    };

    void SetSurfaceBufferWidth(int32_t width) override {}

    void SetSurfaceBufferHeight(int32_t height) override {}

    uint32_t GetSeqNum() const override
    {
        return 0;
    };

    // opt EglData
    sptr<EglData> GetEglData() const override
    {
        return 0;
    };

    void SetEglData(const sptr<EglData> &data) override {}

    void SetExtraData(sptr<BufferExtraData> bedata) override {}

    sptr<BufferExtraData> GetExtraData() const override
    {
        return mExtraData;
    }

    GSError WriteToMessageParcel(MessageParcel &parcel) override
    {
        return GSERROR_OK;
    }

    GSError ReadFromMessageParcel(MessageParcel &parcel) override
    {
        return GSERROR_OK;
    }

    void SetBufferHandle(BufferHandle *handle) override {}

    BufferWrapper GetBufferWrapper() override
    {
        return wrapper_;
    }

    void SetBufferWrapper(BufferWrapper wrapper) override {}

    // gralloc
    GSError Alloc(const BufferRequestConfig &config) override
    {
        return GSERROR_OK;
    }

    GSError Map() override
    {
        return GSERROR_OK;
    }

    GSError Unmap() override
    {
        return GSERROR_OK;
    }

    GSError FlushCache() override
    {
        return GSERROR_OK;
    }

    GSError InvalidateCache() override
    {
        return GSERROR_OK;
    }

    // metadata
    GSError SetMetadata(uint32_t key, const std::vector<uint8_t> &value) override
    {
        return GSERROR_OK;
    }

    GSError GetMetadata(uint32_t key, std::vector<uint8_t> &value) override
    {
        return GSERROR_OK;
    }

    GSError ListMetadataKeys(std::vector<uint32_t> &keys) override
    {
        return GSERROR_OK;
    }

    GSError EraseMetadataKey(uint32_t key) override
    {
        return GSERROR_OK;
    }

    OH_NativeBuffer *SurfaceBufferToNativeBuffer() override
    {
        return reinterpret_cast<OH_NativeBuffer *>(this);
    }

private:
    void *virAddr;

    GraphicColorGamut m_colorGamut;
    GraphicTransformType m_transform;
    sptr<BufferExtraData> mExtraData;
    BufferWrapper wrapper_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IMAGE_EFFECT_MOCK_SURFACE_BUFFER_H