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

#include "external_loader.h"

#include <dlfcn.h>

namespace OHOS {
namespace Media {
namespace Effect {
ExternLoader *ExternLoader::Instance()
{
    static ExternLoader instance;
    return &instance;
}

void ExternLoader::LoadExtSo()
{
    std::unique_lock<std::mutex> lock(loadExtSo_);
    EFFECT_LOGI("EFilterFactory:LoadExtSo enter!");
    if (isExtLoad_) {
        return;
    }
    isExtLoad_ = true;
    void *effectExtHandle = dlopen("/system/lib64/platformsdk/libimage_effect_ext.so", RTLD_NOW);
    if (effectExtHandle == nullptr) {
        EFFECT_LOGE("EFilterFactory: dlopen libimage_effect_ext.so failed! dlerror=%{public}s", dlerror());
        isExtLoad_ = false;
        return;
    }
    EFFECT_LOGI("EFilterFactory: dlopen libimage_effect_ext.so success!");

    createImageEffectExtFunc_ = reinterpret_cast<CreateImageEffectExtFunc>(dlsym(effectExtHandle,
        "CreateImageEffectEXT"));
    initFunc_ = reinterpret_cast<InitModuleFunc>(dlsym(effectExtHandle, "Init"));
    deinitFunc_ = reinterpret_cast<InitModuleFunc>(dlsym(effectExtHandle, "Deinit"));
    initModuleFunc_ = reinterpret_cast<InitModuleFunc>(dlsym(effectExtHandle, "InitModule"));
    deinitModuleFunc_ = reinterpret_cast<InitModuleFunc>(dlsym(effectExtHandle, "DeinitModule"));
}

bool ExternLoader::IsExtLoad() const
{
    return isExtLoad_;
}

CreateImageEffectExtFunc ExternLoader::GetCreateImageEffectExtFunc() const
{
    return createImageEffectExtFunc_;
}

InitFunc ExternLoader::GetInitFunc() const
{
    return initFunc_;
}

InitFunc ExternLoader::GetDeinitFunc() const
{
    return deinitFunc_;
}

InitModuleFunc ExternLoader::GetInitModuleFunc() const
{
    return initModuleFunc_;
}

InitModuleFunc ExternLoader::GetDeinitModuleFunc() const
{
    return deinitModuleFunc_;
}

void ExternLoader::InitExt()
{
    std::unique_lock<std::mutex> lock(initExtSo_);
    if (!IsExtLoad()) {
        LoadExtSo();
    }

    if (hasInitExt_) {
        return;
    }

    hasInitExt_ = true;
    auto initFunc = ExternLoader::Instance()->GetInitFunc();
    if (initFunc) {
        initFunc();
    } else {
        EFFECT_LOGE("EFilterFactory: shared lib so not find function!");
    }
}
} // namespace Effect
} // namespace Media
} // namespace OHOS