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

#include "memcpy_helper.h"

#include "securec.h"
#include "effect_log.h"
#include "format_helper.h"

namespace OHOS {
namespace Media {
namespace Effect {
void MemcpyHelper::CopyData(CopyInfo &src, CopyInfo &dst)
{
    uint8_t *srcBuffet = src.data;
    uint8_t *dstBuffer = dst.data;
    CHECK_AND_RETURN_LOG(srcBuffet != nullptr && dstBuffer != nullptr,
        "Input addr is null! srcAddr=%{public}p, dstAddr=%{public}p", srcBuffet, dstBuffer);
    if (srcBuffet == dstBuffer) {
        EFFECT_LOGD("Buffer is same, not need copy.");
        return;
    }

    BufferInfo &srcInfo = src.bufferInfo;
    BufferInfo &dstInfo = dst.bufferInfo;
    EFFECT_LOGD("CopyData: srcAddr=%{public}p, srcH=%{public}d, srcFormat=%{public}d, srcStride=%{public}d, "
        "srcLen=%{public}d, dstAddr=%{public}p, dstH=%{public}d, dstFormat=%{public}d, dstStride=%{public}d, "
        "dstLen=%{public}d", src.data, srcInfo.height_, srcInfo.formatType_, srcInfo.rowStride_, srcInfo.len_,
        dst.data, dstInfo.height_, dstInfo.formatType_, dstInfo.rowStride_, dstInfo.len_);
    uint32_t srcRowStride = srcInfo.rowStride_;
    uint32_t dstRowStride = dstInfo.rowStride_;
    uint32_t srcBufferLen = srcInfo.len_;
    uint32_t dstBufferLen = dstInfo.len_;

    // direct copy the date while the size is same.
    if (srcRowStride == dstRowStride && srcBufferLen == dstBufferLen) {
        errno_t ret = memcpy_s(dstBuffer, dstBufferLen, srcBuffet, srcBufferLen);
        if (ret != 0) {
            EFFECT_LOGE("CopyData memcpy_s failed. ret=%{public}d, dstBuf=%{public}p, dstBufLen=%{public}d,"
                " srcBuf=%{public}p, srcBufLen=%{public}d", ret, dstBuffer, dstBufferLen, srcBuffet, srcBufferLen);
        }
        return;
    }

    // copy by row
    uint32_t srcRowCount = FormatHelper::CalculateDataRowCount(srcInfo.height_, srcInfo.formatType_);
    uint32_t dstRowCount = FormatHelper::CalculateDataRowCount(dstInfo.height_, dstInfo.formatType_);
    uint32_t rowCount = srcRowCount > dstRowCount ? dstRowCount : srcRowCount;
    uint32_t count = srcRowStride > dstRowStride ? dstRowStride : srcRowStride;
    if (rowCount * dstRowStride > dstBufferLen || rowCount * srcRowStride > srcBufferLen) {
        EFFECT_LOGE("Out of buffer available range! Copy fail! srcH=%{public}d, srcFormat=%{public}d, "
            "srcStride=%{public}d, srcLen=%{public}d, dstH=%{public}d, dstFormat=%{public}d, dstStride=%{public}d, "
            "dstLen=%{public}d", srcInfo.height_, srcInfo.formatType_, srcInfo.rowStride_, srcInfo.len_,
            dstInfo.height_, dstInfo.formatType_, dstInfo.rowStride_, dstInfo.len_);
        return;
    }
    for (uint32_t i = 0; i < rowCount; i++) {
        errno_t ret = memcpy_s(dstBuffer + i * dstRowStride, dstRowStride, srcBuffet + i * srcRowStride, count);
        if (ret != 0) {
            EFFECT_LOGE("CopyData: copy by row memcpy_s failed. ret=%{public}d, row=%{public}d, srcH=%{public}d, "
                "srcFormat=%{public}d, srcStride=%{public}d, srcLen=%{public}d, dstH=%{public}d, dstFormat=%{public}d, "
                "dstStride=%{public}d, dstLen=%{public}d", ret, i,
                srcInfo.height_, srcInfo.formatType_, srcInfo.rowStride_, srcInfo.len_,
                dstInfo.height_, dstInfo.formatType_, dstInfo.rowStride_, dstInfo.len_);
            continue;
        }
    }
}

void CreateCopyInfoByEffectBuffer(EffectBuffer *buffer, CopyInfo &info)
{
    info = {
        .bufferInfo = *buffer->bufferInfo_,
        .data = static_cast<uint8_t *>(buffer->buffer_),
    };
}

void MemcpyHelper::CopyData(EffectBuffer *src, EffectBuffer *dst)
{
    CHECK_AND_RETURN_LOG(src != nullptr && dst != nullptr,
        "Input effect buffer is null! src=%{public}p, dst=%{public}p", src, dst);

    if (src == dst) {
        EFFECT_LOGD("EffectBuffer is same, not need copy.");
        return;
    }

    CopyInfo srcCopyInfo;
    CreateCopyInfoByEffectBuffer(src, srcCopyInfo);

    CopyInfo dstCopyInfo;
    CreateCopyInfoByEffectBuffer(dst, dstCopyInfo);

    CopyData(srcCopyInfo, dstCopyInfo);
}

void MemcpyHelper::CopyData(EffectBuffer *src, CopyInfo &dst)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "Input src effect buffer is null! src=%{public}p", src);
    CopyInfo srcCopyInfo;
    CreateCopyInfoByEffectBuffer(src, srcCopyInfo);

    CopyData(srcCopyInfo, dst);
}

void MemcpyHelper::CopyData(CopyInfo &src, EffectBuffer *dst)
{
    CHECK_AND_RETURN_LOG(dst != nullptr, "Input dst effect buffer is null! dst=%{public}p", dst);
    CopyInfo dstCopyInfo;
    CreateCopyInfoByEffectBuffer(dst, dstCopyInfo);

    CopyData(src, dstCopyInfo);
}

void CreateCopyInfoByMemoryData(MemoryData *memoryData, CopyInfo &info)
{
    info = {
        .bufferInfo = memoryData->memoryInfo.bufferInfo,
        .data = static_cast<uint8_t *>(memoryData->data),
    };
}

void MemcpyHelper::CopyData(EffectBuffer *buffer, MemoryData *memoryData)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr && memoryData != nullptr,
        "Input is null! buffer=%{public}p, memoryData=%{public}p", buffer, memoryData);
    CopyInfo dstCopyInfo;
    CreateCopyInfoByMemoryData(memoryData, dstCopyInfo);

    MemcpyHelper::CopyData(buffer, dstCopyInfo);
}

void MemcpyHelper::CopyData(MemoryData *src, MemoryData *dst)
{
    CHECK_AND_RETURN_LOG(src != nullptr && dst != nullptr,
        "Input memory data is null! src=%{public}p, dst=%{public}p", src, dst);
    if (src == dst) {
        EFFECT_LOGD("MemoryData is same, not need copy.");
        return;
    }

    CopyInfo srcCopyInfo;
    CreateCopyInfoByMemoryData(src, srcCopyInfo);

    CopyInfo dstCopyInfo;
    CreateCopyInfoByMemoryData(dst, dstCopyInfo);

    CopyData(srcCopyInfo, dstCopyInfo);
}
} // namespace Effect
} // namespace Media
} // namespace OHOS