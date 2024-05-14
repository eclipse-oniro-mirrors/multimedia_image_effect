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

#ifndef RENDERLRUCACHE_H
#define RENDERLRUCACHE_H

#include "base/render_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
template <typename KEY_TYPE, typename VALUE_TYPE, typename SIZE_MEASURER> class RenderFifoCache {
public:
    typedef typename std::pair<KEY_TYPE, VALUE_TYPE> PAIR_TYPE;
    typedef typename std::list<PAIR_TYPE>::iterator ITERATOR_TYPE;

    explicit RenderFifoCache(size_t cacheCapacity) : capacity_(cacheCapacity)
    {
        sizeMeasurer_ = SIZE_MEASURER();
    }

    void Put(const KEY_TYPE &key, const VALUE_TYPE &value)
    {
        valueList_.push_front(PAIR_TYPE(key, value));
        currentSize_ += sizeMeasurer_(value);
        while ((currentSize_ > capacity_) && (valueList_.size() > 1)) {
            auto &last = valueList_.back();
            currentSize_ -= sizeMeasurer_(last.second);
            valueList_.pop_back();
        }
    }

    bool Take(const KEY_TYPE &key, VALUE_TYPE &value)
    {
        auto it =
            std::find_if(valueList_.begin(), valueList_.end(), [key](const PAIR_TYPE &v) { return v.first == key; });
        if (it != valueList_.end()) {
            value = (*it).second;
            valueList_.erase(it);
            currentSize_ -= sizeMeasurer_(value);
            return true;
        }
        return false;
    }

    size_t Size() const
    {
        return currentSize_;
    }

    size_t ReSize(size_t size, bool biasMore = true)
    {
        while (currentSize_ > size) {
            auto &last = valueList_.back();
            size_t ss = sizeMeasurer_(last.second);
            if ((currentSize_ - ss) < size && biasMore) {
                break;
            }
            valueList_.pop_back();
            currentSize_ -= ss;
        }
        return currentSize_;
    }

private:
    std::list<PAIR_TYPE> valueList_;
    size_t capacity_;
    size_t currentSize_ { 0 };
    std::function<size_t(const VALUE_TYPE &v)> sizeMeasurer_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // RENDERLRUCACHE_H