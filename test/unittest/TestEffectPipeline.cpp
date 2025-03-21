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
#include "efilter_factory.h"
#include "test_common.h"
#include "filter_base.h"
#include "transfer.h"

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

HWTEST_F(TestEffectPipeline, FilterBaseNamePort001, TestSize.Level1)
{
    FilterBase filterBase("testName_");
    std::string mime = "image/png";
    std::string expectedPortName = "image_1";
    std::string portName = filterBase.NamePort(mime);

    EXPECT_EQ(portName, expectedPortName);
    EXPECT_EQ(filterBase.portTypeCntMap_["testName_image"], 1);

    mime = "";
    expectedPortName = "default_1";
    portName = filterBase.NamePort(mime);
    EXPECT_EQ(portName, expectedPortName);
    EXPECT_EQ(filterBase.portTypeCntMap_["testName_default"], 1);
}

HWTEST_F(TestEffectPipeline, FilterBaseGetRouteInPort001, TestSize.Level1)
{
    FilterBase filterBase("testNmae_");
    PInPort result = filterBase.GetRouteInPort("test");
    EXPECT_EQ(result, nullptr);

    filterBase.routeMap_.emplace_back("test", "test");
    result = filterBase.GetRouteInPort("test");
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(TestEffectPipeline, FilterBaseGetRouteOutPort001, TestSize.Level1)
{
    FilterBase filterBase("testNmae_");
    POutPort result = filterBase.GetRouteOutPort("test");
    EXPECT_EQ(result, nullptr);

    filterBase.routeMap_.emplace_back("test", "test");
    result = filterBase.GetRouteOutPort("test");
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(TestEffectPipeline, FilterBaseOnEvent001, TestSize.Level1)
{
    FilterBase filterBase("testNmae_");
    const Event &event = Event{ "name_", EventType::EVENT_COMPLETE, { nullptr } };
    ASSERT_NO_THROW(filterBase.OnEvent(event));

    EventReceiver *eventReceiver{ nullptr };
    filterBase.eventReceiver_ = eventReceiver;
    ASSERT_NO_THROW(filterBase.OnEvent(event));
}

HWTEST_F(TestEffectPipeline, FilterBaseGetNextFilters001, TestSize.Level1)
{
    FilterBase filterBase("testNmae_");
    InfoTransfer *filterPtr = nullptr;
    OutPort port(filterPtr);
    auto outPort = std::make_shared<OutPort>(port);
    filterBase.outPorts_.push_back(outPort);
    std::vector<Filter *> filters = filterBase.GetNextFilters();
    EXPECT_TRUE(filters.empty());
}

HWTEST_F(TestEffectPipeline, FilterBaseGetPreFilters001, TestSize.Level1)
{
    FilterBase filterBase("testNmae_");
    InfoTransfer *filterPtr = nullptr;
    InPort port(filterPtr);
    auto inPort = std::make_shared<InPort>(port);
    filterBase.inPorts_.push_back(inPort);
    std::vector<Filter *> filters = filterBase.GetPreFilters();
    EXPECT_TRUE(filters.empty());

    filterBase.UnlinkPrevFilters();

    auto result = filterBase.FindPort(filterBase.inPorts_, "test");
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(TestEffectPipeline, InPortPullData001, TestSize.Level1)
{
    std::shared_ptr<BufferInfo> bufferInfo = std::make_unique<BufferInfo>();
    void *add = nullptr;
    std::shared_ptr<ExtraInfo> extraInfo = std::make_unique<ExtraInfo>();
    std::shared_ptr<EffectBuffer> data = std::make_shared<EffectBuffer>(bufferInfo, add, extraInfo);

    InfoTransfer *filterPtr = nullptr;
    InPort port(filterPtr);
    auto inPort = std::make_shared<InPort>(port);

    std::shared_ptr<InPort> prevPort = std::make_shared<InPort>(nullptr);
    inPort->prevPort_ = prevPort;

    ErrorCode result = inPort->PullData(data);
    ASSERT_EQ(result, ErrorCode::ERR_PIPELINE_INVALID_FILTER_PORT);
}

HWTEST_F(TestEffectPipeline, InPortActivate001, TestSize.Level1)
{
    std::shared_ptr<EFilter> eFilter = EFilterFactory::Instance()->Create(BRIGHTNESS_EFILTER);
    InPort port(eFilter.get());
    auto inPort = std::make_shared<InPort>(port);

    std::shared_ptr<InPort> prevPort = std::make_shared<InPort>(port);
    inPort->prevPort_ = prevPort;

    std::vector<WorkMode> modes;
    WorkMode outMode;
    ErrorCode result = inPort->Activate(modes, outMode);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_PARAMETER_VALUE);
}

HWTEST_F(TestEffectPipeline, Start_001, TestSize.Level1)
{
    std::shared_ptr<PipelineCore> pipeline = std::make_shared<PipelineCore>();
    ErrorCode result = pipeline->Start();
    EXPECT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(TestEffectPipeline, Filter_001, TestSize.Level1)
{
    std::shared_ptr<PipelineCore> pipeline = std::make_shared<PipelineCore>();
    ErrorCode result = pipeline->AddFilter(nullptr);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    std::vector<Filter *> filterIn;
    result = pipeline->AddFilter(filterIn);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = pipline->RemoveFilterChain(nullptr);
    EXPECT_NE(result, ErrorCode::SUCCESS);
}

HWTEST_F(TestEffectPipeline, Port_001, TestSize.Level1)
{
    std::shared_ptr<PipelineCore> pipeline = std::make_shared<PipelineCore>();
    InfoTransfer *filterPtr1 = nullptr;
    OutPort port1(filterPtr1);
    auto outPort = std::make_shared<OutPort>(port1);

    InfoTransfer *filterPtr2 = nullptr;
    InPort port2(filterPtr2);
    auto inPort = std::make_shared<InPort>(port2);

    ErrorCode result = pipeline->LinkPorts(outPort, inPort);
    EXPECT_NE(result, ErrorCode::SUCCESS);
}
}
}
}
}
