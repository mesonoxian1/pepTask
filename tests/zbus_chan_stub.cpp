/**
 * @file zbus_chan_stub.cpp
 * @brief Provides gpio_state_chan channel instance for the test build.
 *
 * In the Zephyr build ZBUS_CHAN_DEFINE lives in main.cpp.
 * In the test build main.cpp is excluded so we define the channel here.
 */
#include "zephyr_stubs.h"
#include "zbusTopics.h"

ZBUS_CHAN_DEFINE(
    gpio_state_chan,
    struct ZBusTopics_gpioStateMsg,
    NULL,
    NULL,
    ZBUS_OBSERVERS_EMPTY,
    ZBUS_MSG_INIT(.isHigh = false)
);

// react_listener defined here for test build (main.cpp is excluded)
extern "C" void zbusListenerCb(const struct zbus_channel *chan);
ZBUS_LISTENER_DEFINE(react_listener, zbusListenerCb);