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

#ifndef IM_EXTERN_LOADER_H
#define IM_EXTERN_LOADER_H

#include <string>
#include <atomic>
#include <mutex>

#include "image_effect_marco_define.h"
#include "effect_log.h"

namespace OHOS {
namespace Media {
namespace Effect {
// Type alias for initialization function
using InitFunc = void (*)();
// Type alias for deinitialization function
using DeinitFunc = void (*)();
// Type alias for module initialization function
using InitModuleFunc = void (*)();
// Type alias for module deinitialization function
using DeinitModuleFunc = void (*)();

// Class responsible for loading and managing external shared libraries
class ExternLoader {
public:
    ~ExternLoader() = default;

    // Returns a singleton instance of ExternLoader
    IMAGE_EFFECT_EXPORT static ExternLoader *Instance();

    // Checks if the external library is loaded
    IMAGE_EFFECT_EXPORT bool IsExtLoad() const;

    // Loads the external shared library
    IMAGE_EFFECT_EXPORT void LoadExtSo();

    // Retrieves the initialization function
    InitFunc GetInitFunc() const;
    // Retrieves the deinitialization function
    DeinitFunc GetDeinitFunc() const;
    // Retrieves the module initialization function
    InitModuleFunc GetInitModuleFunc() const;
    // Retrieves the module deinitialization function
    DeinitModuleFunc GetDeinitModuleFunc() const;

    // Initializes the external library
    void InitExt();
private:
    ExternLoader() = default;

    // Pointer to the initialization function
    InitFunc initFunc_ = nullptr;
    // Pointer to the deinitialization function
    DeinitFunc deinitFunc_ = nullptr;
    // Pointer to the module initialization function
    InitModuleFunc initModuleFunc_ = nullptr;
    // Pointer to the module deinitialization function
    DeinitModuleFunc deinitModuleFunc_ = nullptr;

    // Atomic flag indicating if the external library is loaded
    std::atomic<bool> isExtLoad_ = false;
    // Atomic flag indicating if the external library has been initialized
    std::atomic<bool> hasInitExt_ = false;

    // Mutex for synchronizing loading of the external shared library
    std::mutex loadExtSo_;
    // Mutex for synchronizing initialization of the external shared library
    std::mutex initExtSo_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS

#endif // IM_EXTERN_LOADER_H