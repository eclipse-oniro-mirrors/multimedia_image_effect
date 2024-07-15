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

#include "gtest/gtest.h"

#include "pipeline_core.h"
#include "effect.h"
#include "efilter_factory.h"
#include "test_common.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

class TestEffectPipeline : public testing::Test {
public:
    TestEffectPipeline() = default;

    ~TestEffectPipeline() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override{}

    void TearDown() override{}
};

HWTEST_F(TestEffectPipeline, EffectPipelineStandard001, TestSize.Level1) {
    std::shared_ptr<PipelineCore> pipeline = std::make_shared<PipelineCore>();
    pipeline->Init(nullptr);

    std::vector<Filter *> filtersToPipeline;
    std::shared_ptr<EFilter> eFilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    filtersToPipeline.push_back(eFilter.get());

    ErrorCode result = pipeline->AddFilters(filtersToPipeline);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    std::shared_ptr<EFilter> contrastEFilter = EFilterFactory::Instance()->Create(CONTRAST_EFILTER);
    result = pipeline->AddFilter(contrastEFilter.get());
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = pipeline->LinkFilters(filtersToPipeline);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = pipeline->RemoveFilter(eFilter.get());
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = pipeline->RemoveFilterChain(contrastEFilter.get());
    EXPECT_EQ(result, ErrorCode::SUCCESS);
}
}
}
}
}