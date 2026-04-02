#include "gpioZephyr.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_zephyr, LOG_LEVEL_INF);


/**
 * @brief GPIO spec for the input button resolved from the device tree.
 *
 * @details Populated at compile time from the button0 node defined in
 *          boards/native_sim.overlay. Contains the GPIO device pointer,
 *          pin number, and flags (GPIO_ACTIVE_LOW | GPIO_PULL_UP).
 */
const gpio_dt_spec btnSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);

/**
 * @brief GPIO spec for the output LED resolved from the device tree.
 *
 * @details Populated at compile time from the led0 node defined in
 *          boards/native_sim.overlay. Contains the GPIO device pointer,
 *          pin number, and flags (GPIO_ACTIVE_HIGH).
 */
const gpio_dt_spec ledSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

/* ============================================================
 * ZephyrGpioInput
 * ============================================================ */

/**
 * @brief Construct from a device-tree GPIO spec.
 *
 * @param spec  GPIO spec obtained via GPIO_DT_SPEC_GET().
 */
ZephyrGpioInput::ZephyrGpioInput(const gpio_dt_spec &spec)
    : spec_{spec}
{
    ctx_.self = this;
    k_work_init(&ctx_.isrWork, workThunk);
}

/**
 * @brief    Configure the pin as input pull-up and register an edge callback.
 *
 * @param cb Invoked from work-queue context on every logical state change.
 * @return   ERR_TYPE_commonErr_OK on success, ERR_TYPE_commonErr_FAIL otherwise.
 */
ERR_TYPE_commonErr_E ZephyrGpioInput::configure(GpioInterface_GpioStateCallback cb)
{
    userCb_ = cb;

    if (!device_is_ready(spec_.port)) {
        LOG_ERR("GPIO input device not ready");
       // return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&spec_, GPIO_INPUT | GPIO_PULL_UP);
    if (ret != 0) { return ERR_TYPE_commonErr_FAIL; }

    gpio_init_callback(&ctx_.cbData, isrHandler, BIT(spec_.pin));
    ret = gpio_add_callback(spec_.port, &ctx_.cbData);
    if (ret != 0) { return ERR_TYPE_commonErr_FAIL; }

     gpio_pin_interrupt_configure_dt(&spec_, GPIO_INT_EDGE_BOTH);

    return ERR_TYPE_commonErr_OK;
}

/**
 * @brief   Read the current logical state of the input pin.
 *
 * @return  true if the pin is logically high, false if low.
 */
bool ZephyrGpioInput::readPin() const
{
    const bool state = gpio_pin_get_dt(&spec_) > 0;
    LOG_INF("readPin() -> %s", state ? "HIGH" : "LOW");
    return state;
}

/**
 * @brief Static ISR handler — submits isrWork to exit ISR context immediately.
 *
 * @param dev   GPIO device pointer (unused).
 * @param cb    Pointer to the embedded gpio_callback inside GpioIsrCtx.
 * @param pins  Bitmask of triggered pins (unused).
 */
void ZephyrGpioInput::isrHandler(const struct device * /*dev*/,
                                  struct gpio_callback *cb,
                                  uint32_t /*pins*/)
{
    GpioIsrCtx *ctx = reinterpret_cast<GpioIsrCtx *>(cb);
    k_work_submit(&ctx->isrWork);
}

/**
 * @brief Static work-queue thunk — recovers instance via GpioIsrCtx and
 *        invokes the user callback with the current pin state.
 *
 * @param w  Pointer to the embedded k_work inside GpioIsrCtx.
 */
void ZephyrGpioInput::workThunk(k_work *w)
{
    GpioIsrCtx      *ctx  = CONTAINER_OF(w, GpioIsrCtx, isrWork);
    ZephyrGpioInput *self = static_cast<ZephyrGpioInput *>(ctx->self);
    self->userCb_(self->readPin());
}

/* ============================================================
 * ZephyrGpioOutput
 * ============================================================ */

/**
 * @brief Construct from a device-tree GPIO spec.
 *
 * @param spec  GPIO spec obtained via GPIO_DT_SPEC_GET().
 */
ZephyrGpioOutput::ZephyrGpioOutput(const gpio_dt_spec &spec)
    : spec_{spec}
{}

/**
 * @brief Configure the pin as a push-pull digital output, initially low.
 *
 * @return ERR_TYPE_commonErr_OK on success, ERR_TYPE_commonErr_FAIL otherwise.
 */
ERR_TYPE_commonErr_E ZephyrGpioOutput::configure()
{
    ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;

    if (!device_is_ready(spec_.port)) {
        LOG_ERR("GPIO output device not ready");
        ERR_TYPE_commonErr_FAIL;
    }

    gpio_pin_configure_dt(&spec_, GPIO_OUTPUT_INACTIVE);
    return err;
}

/**
 * @brief Drive the output pin to a logical level.
 *
 * @param state  true = high (LED on), false = low (LED off).
 */
void ZephyrGpioOutput::setPin(bool state)
{
    gpio_pin_set_dt(&spec_, state ? 1 : 0);
    LOG_INF("LED -> %s", state ? "ON" : "OFF");
}