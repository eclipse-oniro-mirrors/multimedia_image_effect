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

#ifndef IMAGE_EFFECT_EFFECT_LOG_H
#define IMAGE_EFFECT_EFFECT_LOG_H

#include <hilog/log.h>

#ifdef __FILE_NAME__
#define LOG_FILE_NAME __FILE_NAME__
#else
#define LOG_FILE_NAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002B6A

#undef LOG_TAG
#define LOG_TAG "ImageEffect"

#ifndef LOG_LABEL
#define LOG_LABEL                     \
    {                                 \
        LOG_CORE, LOG_DOMAIN, LOG_TAG \
    }
#endif

#define EFFECT_LOG(func, fmt, args...)                                                                            \
    do {                                                                                                          \
        (void)func(LOG_CORE, "[%{public}s()@%{public}s:%{public}d] " fmt, __FILE_NAME__, __FUNCTION__, __LINE__,  \
            ##args);                                                                                              \
    } while (0)

#define EFFECT_LOGF(fmt, ...) HILOG_FATAL(LOG_CORE, fmt, ##__VA_ARGS__)
#define EFFECT_LOGE(fmt, ...) HILOG_ERROR(LOG_CORE, fmt, ##__VA_ARGS__)
#define EFFECT_LOGW(fmt, ...) HILOG_WARN(LOG_CORE, fmt, ##__VA_ARGS__)
#define EFFECT_LOGI(fmt, ...) HILOG_INFO(LOG_CORE, fmt, ##__VA_ARGS__)
#define EFFECT_LOGD(fmt, ...) EFFECT_LOG(HILOG_DEBUG, fmt, ##__VA_ARGS__)

#define CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...) \
    do {                                              \
        if (!(cond)) {                                \
            EFFECT_LOGE(fmt, ##__VA_ARGS__);          \
            return ret;                               \
        }                                             \
    } while (0)

#define CHECK_AND_RETURN_LOG(cond, fmt, ...) \
    do {                                     \
        if (!(cond)) {                       \
            EFFECT_LOGE(fmt, ##__VA_ARGS__); \
            return;                          \
        }                                    \
    } while (0)

#define CHECK_AND_CONTINUE_LOG(cond, fmt, ...) \
    if (!(cond)) {                             \
        EFFECT_LOGE(fmt, ##__VA_ARGS__);       \
        continue;                              \
    }

#define CHECK_AND_RETURN_RET(cond, ret)               \
    do {                                              \
        if (!(cond)) {                                \
            return ret;                               \
        }                                             \
    } while (0)

#ifndef CHECK_AND_RETURN
#define CHECK_AND_RETURN(exec)                        \
    do {                                              \
        bool returnValue = (exec);                    \
        if (!returnValue) {                           \
            EFFECT_LOGE("CHECK_AND_RETURN " #exec);   \
            return;                                   \
        }                                             \
    } while (0)
#endif

#endif // IMAGE_EFFECT_EFFECT_LOG_H
