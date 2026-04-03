#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "zephyr_stubs.h"
#include "zbusTopics.h"
#include "gpio_mocks.h"
#include "read.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief Test fixture for ReadClass.
 *
 * Wires a MockGpioInput into ReadClass and captures the GpioStateCallback
 * so individual tests can fire simulated edges.
 */
class ReadClassTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(mockInput_, configure(_))
            .WillByDefault([this](GpioInterface_GpioStateCallback cb) {
                return mockInput_.captureAndReturn(cb);
            });

        ASSERT_EQ(readObj_.init(), ERR_TYPE_commonErr_OK);
    }

    MockGpioInput mockInput_;
    ReadClass     readObj_{mockInput_};
};

/* ---------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

/**
 * @brief init() calls configure() on the GPIO input exactly once.
 */
TEST_F(ReadClassTest, InitCallsConfigure)
{
    MockGpioInput freshInput;
    EXPECT_CALL(freshInput, configure(_))
        .Times(1)
        .WillOnce([&](GpioInterface_GpioStateCallback cb) {
            return freshInput.captureAndReturn(cb);
        });

    ReadClass fresh{freshInput};
    EXPECT_EQ(fresh.init(), ERR_TYPE_commonErr_OK);
}

/**
 * @brief A single edge schedules the debounce work item.
 *        After draining, the current pin state is read and published.
 */
TEST_F(ReadClassTest, EdgeTriggersDebounceAndPublish)
{
    EXPECT_CALL(mockInput_, readPin()).WillOnce(Return(true));

    mockInput_.simulateEdge(true);
    drainDelayable(&readObj_.dwork_);

    ZBusTopics_gpioStateMsg msg{};
    ASSERT_EQ(zbus_chan_read(&gpio_state_chan, &msg, K_NO_WAIT), 0);
    EXPECT_TRUE(msg.isHigh);
}

/**
 * @brief HIGH to LOW transition publishes LOW state.
 */
TEST_F(ReadClassTest, TransitionHighToLowPublishesLow)
{
    EXPECT_CALL(mockInput_, readPin())
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    mockInput_.simulateEdge(true);
    drainDelayable(&readObj_.dwork_);

    mockInput_.simulateEdge(false);
    drainDelayable(&readObj_.dwork_);

    ZBusTopics_gpioStateMsg msg{};
    ASSERT_EQ(zbus_chan_read(&gpio_state_chan, &msg, K_NO_WAIT), 0);
    EXPECT_FALSE(msg.isHigh);
}

/**
 * @brief Rapid edges before debounce expires result in only one readPin() call.
 */
TEST_F(ReadClassTest, RapidEdgesDebounced)
{
    EXPECT_CALL(mockInput_, readPin()).Times(1).WillOnce(Return(true));

    mockInput_.simulateEdge(false);
    mockInput_.simulateEdge(true);
    mockInput_.simulateEdge(false);

    drainDelayable(&readObj_.dwork_);
}

/**
 * @brief Same state twice does not publish a second time.
 */
TEST_F(ReadClassTest, DuplicateStateNotPublished)
{
    EXPECT_CALL(mockInput_, readPin())
        .WillOnce(Return(true))
        .WillOnce(Return(true));

    mockInput_.simulateEdge(true);
    drainDelayable(&readObj_.dwork_);

    ZBusTopics_gpioStateMsg before{};
    zbus_chan_read(&gpio_state_chan, &before, K_NO_WAIT);

    mockInput_.simulateEdge(true);
    drainDelayable(&readObj_.dwork_);

    ZBusTopics_gpioStateMsg after{};
    zbus_chan_read(&gpio_state_chan, &after, K_NO_WAIT);

    EXPECT_EQ(before.isHigh, after.isHigh);
}
