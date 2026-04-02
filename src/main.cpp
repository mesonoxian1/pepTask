#include "zbusTopics.h"
#include "gpioInterface.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

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

/**
 * @brief GPIO spec for the input button resolved from the device tree.
 *
 * @details Populated at compile time from the @c button0 node defined in
 *          boards/native_sim.overlay. Contains the GPIO device pointer,
 *          pin number, and flags (GPIO_ACTIVE_LOW | GPIO_PULL_UP).
 */
static const gpio_dt_spec btnSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);

/**
 * @brief GPIO spec for the output LED resolved from the device tree.
 *
 * @details Populated at compile time from the @c led0 node defined in
 *          boards/native_sim.overlay. Contains the GPIO device pointer,
 *          pin number, and flags (GPIO_ACTIVE_HIGH).
 */
static const gpio_dt_spec ledSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);


int main(void)
{
    LOG_INF("Starting... ");

    return 0;
}