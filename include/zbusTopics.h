#pragma once

#include <zephyr/zbus/zbus.h>

/**
 * @file zbus_topics.h
 * @brief ZBus channel and message definitions shared across the application.
 *
 * Only one translation unit must own the ZBUS_CHAN_DEFINE; all others use
 * ZBUS_CHAN_DECLARE. The definition lives in src/main.cpp.
 */

//! Payload published on GPIO state change
struct ZBusTopics_gpioStateMsg {
    bool isHigh; //!< State of the input pin - TRUE - HIGH, FALSE - LOW
};

/**
 * ZBus channel carrying GpioStateMsg notifications.
 * Published by ReadClass; observed by ReactClass.
 */
ZBUS_CHAN_DECLARE(gpio_state_chan);
