#include "zbusTopics.h"
#include "gpioInterface.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "gpioZephyr.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// extern declarations for the device tree specs defined in gpioZephyr.cpp
extern const gpio_dt_spec buttonSpec;
extern const gpio_dt_spec ledSpec;

/**
 * @brief ZBus channel carrying GpioStateMsg state change notifications.
 *
 * @details This is the communication backbone between ReadClass
 *          (publisher) and ReactClass (observer). Defined once here in
 *          main.cpp; all other translation units reference it via
 *          ZBUS_CHAN_DECLARE in zbusTopics.h.
 *
 *          Channel parameters:
 *          - Name: gpio_state_chan
 *          - Message type:  GpioStateMsg
 *          - Validator:     none
 *          - User data:     none
 *          - Observers:     added at runtime via zbus_chan_add_obs()
 *          - Initial value: isHigh = false
 */
ZBUS_CHAN_DEFINE(
    gpio_state_chan,
    struct ZBusTopics_gpioStateMsg,
    NULL,
    NULL,
    ZBUS_OBSERVERS_EMPTY,
    ZBUS_MSG_INIT(.isHigh = false)
);

int main(void)
{
    LOG_INF("Starting... ");

    return 0;
}