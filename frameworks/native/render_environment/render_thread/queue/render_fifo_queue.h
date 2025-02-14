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

#ifndef IM_RENDER_FIFO_QUEUE_H
#define IM_RENDER_FIFO_QUEUE_H

#include "render_queue_itf.h"

#include <list>

template <typename T> class RenderFifoQueue : public RenderQueueItf<T> {
public:
    ~RenderFifoQueue() = default;

    size_t GetSize() override
    {
        return _list.size();
    }

    bool Push(const T &data) override
    {
        try {
            _list.emplace_back(data);
        } catch (std::bad_array_new_length) {
            return false;
        }
        return true;
    }

    bool Pop(T &result) override
    {
        if (_list.size() == 0) {
            return false; // empty
        }
        result = _list.front();
        _list.pop_front();
        return true;
    }

    bool PopWithCallBack(T &result, std::function<void(T &)> &callback) override
    {
        if (_list.size() == 0) {
            return false; // empty
        }
        result = _list.front();
        _list.pop_front();
        callback(result);
        return true;
    }

    bool Front(T &result) override
    {
        if (_list.size() == 0) {
            return false; // empty
        }
        result = _list.front();
        return true;
    }

    bool Back(T &result) override
    {
        if (_list.size() == 0) {
            return false; // empty
        }
        result = _list.back();
        return true;
    }

    void RemoveAll() override
    {
        _list.clear();
    }

    void Remove(const std::function<bool(T &)> &checkFunc) override
    {
        try {
            _list.remove_if(checkFunc);
        } catch (std::bad_function_call) {
            return;
        }
    }

private:
    std::list<T> _list;
};
#endif