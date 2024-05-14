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

#ifndef IM_RENDER_WORK_ITF_H
#define IM_RENDER_WORK_ITF_H

#include "render_task_itf.h"

template <typename TASK> class RenderWorkerItf {
public:
    typedef TASK TaskType;
    RenderWorkerItf() = default;
    virtual ~RenderWorkerItf() = default;
    RenderWorkerItf(const RenderWorkerItf &) = delete;
    virtual RenderWorkerItf &operator = (const RenderWorkerItf &) = delete;
    virtual void AddTask(const TASK &, bool overwrite = false) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

protected:
    virtual void Run() = 0;
};
#endif // IM_RENDER_WORK_ITF_H