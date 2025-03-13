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

#include "port.h"
#include "filter.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Effect {
namespace Test {

class TestPort : public testing::Test {
public:
    TestPort() = default;

    ~TestPort() override = default;
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    void SetUp() override{}

    void TearDown() override{}
};

HWTEST_F(TestPort, PullData001, TestSize.Level1) {
    InfoTransfer *filterPtr = nullptr;

    InPort inPort(filterPtr);
    std::shared_ptr<EffectBuffer> buffer = nullptr;
    ErrorCode result = inPort.PullData(buffer);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    OutPort outPort(filterPtr);
    result = outPort.PullData(buffer);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    EmptyInPort emptyInPort;
    result = emptyInPort.PullData(buffer);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    EmptyOutPort emptyOutPort;
    result = emptyOutPort.PullData(buffer);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    std::shared_ptr<Port> port = nullptr;
    result = outPort.Connect(port);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = outPort.Disconnect();
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    result = emptyInPort.Connect(port);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = emptyOutPort.Connect(port);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    std::vector<WorkMode> modes;
    WorkMode outMode;
    result = outPort.Activate(modes, outMode);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = emptyInPort.Activate(modes, outMode);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    result = emptyOutPort.Activate(modes, outMode);
    EXPECT_NE(result, ErrorCode::SUCCESS);

    std::shared_ptr<Port> res = outPort.GetPeerPort();
    EXPECT_EQ(res, nullptr);

    res = inPort.GetPeerPort();
    EXPECT_EQ(res, nullptr);

    const std::shared_ptr<Capability> capability;
    std::shared_ptr<EffectContext> context;
    emptyInPort.Negotiate(capability, context);

    emptyOutPort.Negotiate(capability, context);

    std::string capabilityName = "test";
    const std::shared_ptr<Capability> capabilityShared = std::make_shared<Capability>(capabilityName);
    std::shared_ptr<EffectContext> contextShared = std::make_shared<EffectContext>();
    emptyOutPort.Negotiate(capabilityShared, contextShared);

    emptyInPort.Negotiate(capabilityShared, contextShared);
}

HWTEST_F(TestPort, Port_001, TestSize.Level1) {
    InfoTransfer *filterPtr = nullptr;
    Port *inPort = new InPort(filterPtr);
    std::shared_ptr<Port> res = inPort->Port::GetPeerPort();
    EXPECT_EQ(res, nullptr);

    WorkMode mode = inPort->Port::GetWorkMode();
    EXPECT_EQ(mode, WorkMode::PUSH);

    ErrorCode result = inPort->Disconnect();
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    std::shared_ptr<Port> port = nullptr;
    result = inPort->Connect(port);
    EXPECT_EQ(result, ErrorCode::SUCCESS);

    delete inPort;
    inPort = nullptr;
}
}
}
}
}
