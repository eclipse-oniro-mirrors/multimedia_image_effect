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

#ifndef IMAGE_EFFECT_EFFECT_H
#define IMAGE_EFFECT_EFFECT_H

#include "efilter.h"

namespace OHOS {
namespace Media {
namespace Effect {
class Effect {
public:
    Effect() = default;

    virtual ~Effect() = default;

    /**
     * Adds an EFilter to the collection.
     *
     * @param efilter The EFilter to be added, provided as a shared pointer.
     * This function does not return any value.
     */
    virtual void AddEFilter(const std::shared_ptr<EFilter> &efilter);

    /**
     * Virtual function to insert an EFilter at a specified index.
     * @param efilter A shared pointer to the EFilter object to be inserted.
     * @param index The position at which the EFilter should be inserted.
     * @return ErrorCode indicating the result of the operation.
     */
    virtual ErrorCode InsertEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index);

    /**
     * Removes the specified EFilter from the collection.
     *
     * @param efilter The EFilter to be removed.
     */
    virtual void RemoveEFilter(const std::shared_ptr<EFilter> &efilter);

    /**
     * Removes an EFilter at a specified index.
     *
     * @param index The position of the EFilter to be removed.
     * @return ErrorCode indicating the result of the operation.
     */
    virtual ErrorCode RemoveEFilter(uint32_t index);

    /**
     * Replaces an EFilter at a specified index with a new EFilter.
     *
     * @param efilter A shared pointer to the new EFilter object.
     * @param index The position at which the new EFilter should replace the old one.
     * @return ErrorCode indicating the result of the operation.
     */
    virtual ErrorCode ReplaceEFilter(const std::shared_ptr<EFilter> &efilter, uint32_t index);

    /**
     * Virtual function to start the effect processing.
     * Must be implemented by derived classes.
     *
     * @return ErrorCode indicating the result of the operation.
     */
    virtual ErrorCode Start() = 0;

    /**
     * Virtual function to save the effect configuration.
     * Must be implemented by derived classes.
     *
     * @param res A reference to a shared pointer for storing the result in JSON format.
     * @return ErrorCode indicating the result of the operation.
     */
    virtual ErrorCode Save(EffectJsonPtr &res) = 0;

    /**
     * Retrieves the collection of EFilters.
     *
     * @return A reference to a vector containing shared pointers to EFilter objects.
     */
    std::vector<std::shared_ptr<EFilter>> &GetEFilters()
    {
        return efilters_;
    }
protected:
    // Collection of EFilters
    std::vector<std::shared_ptr<EFilter>> efilters_;
};
} // namespace Effect
} // namespace Media
} // namespace OHOS
#endif // IMAGE_EFFECT_EFFECT_H
