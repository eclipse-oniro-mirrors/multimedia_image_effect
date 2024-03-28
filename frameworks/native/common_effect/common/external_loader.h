/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef IM_EXTERN_LOADER_H
#define IM_EXTERN_LOADER_H

#include <string>

#include "effect_log.h"
#include "native_base.h"

namespace OHOS {
namespace Media {
namespace Effect {
using CreateImageEffectExtFunc = OH_ImageEffect* (*)(const char*);
using InitFunc = void (*)();
using DeinitFunc = void (*)();
using InitModuleFunc = void (*)();
using DeinitModuleFunc = void (*)();
class ExternLoader {
public:
    ~ExternLoader() = default;

    static ExternLoader *Instance();

    bool IsExtLoad() const;

    void LoadExtSo();

    CreateImageEffectExtFunc GetCreateImageEffectExtFunc() const;

    InitFunc GetInitFunc() const;
    DeinitFunc GetDeinitFunc() const;
    InitModuleFunc GetInitModuleFunc() const;
    DeinitModuleFunc GetDeinitModuleFunc() const;

    void InitExt();
private:
    ExternLoader() = default;

    CreateImageEffectExtFunc createImageEffectExtFunc_ = nullptr;

    InitFunc initFunc_ = nullptr;
    DeinitFunc deinitFunc_ = nullptr;
    InitModuleFunc initModuleFunc_ = nullptr;
    DeinitModuleFunc deinitModuleFunc_ = nullptr;

    bool isExtLoad_ = false;
    bool hasInitExt_ = false;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IM_EXTERN_LOADER_H