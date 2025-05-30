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
#include "effect_trace.h"

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
    EFFECT_TRACE_NAME("ExternLoader::LoadExtSo");
    std::unique_lock<std::mutex> lock(loadExtSo_);
    EFFECT_LOGI("EFilterFactory:LoadExtSo enter!");
    if (isExtLoad_) {
        return;
    }

    void *effectExtHandle = dlopen("libimage_effect_ext.so", RTLD_NOW);
    if (effectExtHandle == nullptr) {
        EFFECT_LOGE("EFilterFactory: dlopen libimage_effect_ext.so failed! dlerror=%{public}s", dlerror());
        isExtLoad_ = false;
        return;
    }
    EFFECT_LOGI("EFilterFactory: dlopen libimage_effect_ext.so success!");

    bool allSymbolsLoaded = true;  // 用于跟踪所有符号是否加载成功

    void* initFunc = dlsym(effectExtHandle, "Init");
    if (!initFunc) {
        EFFECT_LOGE("EFilterFactory: dlsym Init failed! dlerror=%{public}s", dlerror());
        allSymbolsLoaded = false;
    }

    void* deinitFunc = dlsym(effectExtHandle, "Deinit");
    if (!deinitFunc) {
        EFFECT_LOGE("EFilterFactory: dlsym Deinit failed! dlerror=%{public}s", dlerror());
        allSymbolsLoaded = false;
    }

    void* initModuleFunc = dlsym(effectExtHandle, "InitMoudle");
    if (!initModuleFunc) {
        EFFECT_LOGE("EFilterFactory: dlsym InitMoudle failed! dlerror=%{public}s", dlerror());
        allSymbolsLoaded = false;
    }

    void* deinitModuleFunc = dlsym(effectExtHandle, "DeinitModule");
    if (!deinitModuleFunc) {
        EFFECT_LOGE("EFilterFactory: dlsym DeinitModule failed! dlerror=%{public}s", dlerror());
        allSymbolsLoaded = false;
    }

    // 如果dlsym调用成功， 将临时指针赋值给类成员
    if (allSymbolsLoaded) {
        initFunc_ = reinterpret_cast<InitModuleFunc>(initFunc);
        deinitFunc_ = reinterpret_cast<InitModuleFunc>(deinitFunc);
        initModuleFunc_ = reinterpret_cast<InitModuleFunc>(initModuleFunc);
        deinitModuleFunc_ = reinterpret_cast<InitModuleFunc>(deinitModuleFunc);
        isExtLoad_ = true;
    } else {
        // 如果有任何dlsym调用失败， 关闭动态链接库
        dlclose(effectExtHandle);
        effectExtHandle = nullptr;
        isExtLoad_ = false;
        EFFECT_LOGE("EFilterFactory: LoadExtSo failed due to dlsym errors.");
    }
}

bool ExternLoader::IsExtLoad() const
{
    return isExtLoad_;
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

    auto initFunc = ExternLoader::Instance()->GetInitFunc();
    if (initFunc) {
        initFunc();
    } else {
        EFFECT_LOGE("EFilterFactory: shared lib so not find function!");
    }
    hasInitExt_ = true;
}
} // namespace Effect
} // namespace Media
} // namespace OHOS