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

    BufferHandle *GetBufferHandle() const
    {
        return 0;
    };
    int32_t GetWidth() const
    {
        return WIDTH;
    };
    int32_t GetHeight() const
    {
        return HEIGHT;
    };
    int32_t GetStride() const
    {
        return STRIDE;
    };
    int32_t GetFormat() const
    {
        return ::PixelFormat::PIXEL_FMT_RGBA_8888;
    };
    uint64_t GetUsage() const
    {
        return USAGE;
    };
    uint64_t GetPhyAddr() const
    {
        return 0;
    };
    void *GetVirAddr()
    {
        return virAddr;
    };
    int32_t GetFileDescriptor() const
    {
        return 0;
    };
    uint32_t GetSize() const
    {
        return SIZE;
    };

    virtual const GraphicColorGamut &GetSurfaceBufferColorGamut() const
    {
        return m_colorGamut;
    }

    virtual const GraphicTransformType &GetSurfaceBufferTransform() const
    {
        return m_transform;
    }

    virtual void SetSurfaceBufferColorGamut(const GraphicColorGamut &colorGamut) {}

    virtual void SetSurfaceBufferTransform(const GraphicTransformType &transform) {}

    int32_t GetSurfaceBufferWidth() const
    {
        return 0;
    };
    int32_t GetSurfaceBufferHeight() const
    {
        return 0;
    };

    virtual void SetSurfaceBufferWidth(int32_t width) override {}

    virtual void SetSurfaceBufferHeight(int32_t height) override {}

    virtual uint32_t GetSeqNum() const
    {
        return 0;
    };

    // opt EglData
    virtual sptr<EglData> GetEglData() const
    {
        return 0;
    };

    virtual void SetEglData(const sptr<EglData> &data) {}

    virtual void SetExtraData(const sptr<BufferExtraData> &bedata) {}

    virtual const sptr<BufferExtraData> &GetExtraData() const
    {
        return mExtraData;
    }

    virtual GSError WriteToMessageParcel(MessageParcel &parcel)
    {
        return GSERROR_OK;
    }

    virtual GSError ReadFromMessageParcel(MessageParcel &parcel)
    {
        return GSERROR_OK;
    }

    virtual void SetBufferHandle(BufferHandle *handle) {}

    virtual BufferWrapper GetBufferWrapper() override
    {
        return wrapper_;
    }

    virtual void SetBufferWrapper(BufferWrapper wrapper) {}

    // gralloc
    virtual GSError Alloc(const BufferRequestConfig &config)
    {
        return GSERROR_OK;
    }

    virtual GSError Map()
    {
        return GSERROR_OK;
    }

    virtual GSError Unmap()
    {
        return GSERROR_OK;
    }

    virtual GSError FlushCache()
    {
        return GSERROR_OK;
    }

    virtual GSError InvalidateCache()
    {
        return GSERROR_OK;
    }

    // metadata
    virtual GSError SetMetadata(uint32_t key, const std::vector<uint8_t> &value)
    {
        return GSERROR_OK;
    }

    virtual GSError GetMetadata(uint32_t key, std::vector<uint8_t> &value)
    {
        return GSERROR_OK;
    }

    virtual GSError ListMetadataKeys(std::vector<uint32_t> &keys)
    {
        return GSERROR_OK;
    }

    virtual GSError EraseMetadataKey(uint32_t key)
    {
        return GSERROR_OK;
    }

    static SurfaceBuffer *NativeBufferToSurfaceBuffer(OH_NativeBuffer *buffer)
    {
        return nullptr;
    };

    static const SurfaceBuffer *NativeBufferToSurfaceBuffer(OH_NativeBuffer const *buffer)
    {
        return nullptr;
    };

    virtual OH_NativeBuffer *SurfaceBufferToNativeBuffer()
    {
        return nullptr;
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