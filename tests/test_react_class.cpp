#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gpio_mocks.h"
#include "react.h"
#include "zephyr_stubs.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief Test fixture for ReactClass.
 */
class ReactClassTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(mockOutput_, configure())
            .WillByDefault(Return(ERR_TYPE_commonErr_OK));

        ASSERT_EQ(reactObj_.init(), ERR_TYPE_commonErr_OK);
    }

    MockGpioOutput mockOutput_;
    ReactClass     reactObj_{mockOutput_};

    /** @brief Drain all pending work items until none remain. */
    void drainAll()
    {
        reactObj_.drainWorkForTest();
    }
};

/* ---------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

/**
 * @brief init() calls configure() on the output GPIO.
 */
TEST_F(ReactClassTest, InitConfiguresOutput)
{
    MockGpioOutput fresh;
    EXPECT_CALL(fresh, configure()).Times(1).WillOnce(Return(ERR_TYPE_commonErr_OK));
    ReactClass freshReact{fresh};
    EXPECT_EQ(freshReact.init(), ERR_TYPE_commonErr_OK);
}

/**
 * @brief HIGH state: LED toggles ON/OFF exactly 3 times (6 setPin calls).
 */
TEST_F(ReactClassTest, HighStateBlinkThreeTimes)
{
    {
        InSequence seq;
        EXPECT_CALL(mockOutput_, setPin(true)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(false)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(true)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(false)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(true)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(false)).Times(1);
    }

    reactObj_.zbusMsgEventHandler(true);
    for (int i = 0; i < 6; ++i) {
        drainAll();
    }
}

/**
 * @brief LOW state: LED turns on then off after hold period.
 */
TEST_F(ReactClassTest, LowStateLedOnThenOff)
{
    {
        InSequence seq;
        EXPECT_CALL(mockOutput_, setPin(true)).Times(1);
        EXPECT_CALL(mockOutput_, setPin(false)).Times(1);
    }

    reactObj_.zbusMsgEventHandler(false);
    drainAll(); /* turns LED on, reschedules */
    drainAll(); /* turns LED off             */
}

/**
 * @brief init() propagates configure() failure.
 */
TEST_F(ReactClassTest, InitFailsWhenConfigureFails)
{
    MockGpioOutput bad;
    EXPECT_CALL(bad, configure()).WillOnce(Return(ERR_TYPE_commonErr_FAIL));
    ReactClass badReact{bad};
    EXPECT_EQ(badReact.init(), ERR_TYPE_commonErr_FAIL);
}
