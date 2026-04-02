#pragma once

#include "gpioInterface.h"
#include "errTypes.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

/**
 * @file    gpioZephyr.h
 * @brief   Zephyr hardware implementations of IGpioInput and IGpioOutput.
 *
 * @details These classes wrap the Zephyr GPIO driver API and satisfy the
 *          abstract interfaces defined in gpioInterface.h. They are used
 *          in production firmware; tests use mocks instead.
 *
 *          Only this file and gpioZephyr.cpp include Zephyr GPIO driver
 *          headers — ReadClass and ReactClass remain hardware-agnostic.
 */

/**
 * @brief   POD ISR context for ZephyrGpioInput.
 *
 * @details Stores the gpio_callback, owning instance pointer and work item
 *          in a standard-layout struct. This avoids -Winvalid-offsetof which
 *          would occur if CONTAINER_OF were applied directly to the
 *          non-standard-layout ZephyrGpioInput class.
 */
struct GpioIsrCtx {
    struct gpio_callback cbData;
    void                 *self;
    struct k_work        isrWork;
};

/** @brief Zephyr GPIO input implementation of IGpioInput. */
class ZephyrGpioInput : public GpioInterface_GpioInput
{
public:
    explicit ZephyrGpioInput(const gpio_dt_spec &spec);
    ERR_TYPE_commonErr_E configure(GpioInterface_GpioStateCallback cb) override;
    bool readPin() const override;

private:
    const gpio_dt_spec &spec_;
    GpioIsrCtx         ctx_{};
    GpioInterface_GpioStateCallback userCb_{};

    static void isrHandler(const struct device *dev,
            struct gpio_callback *cb,
            uint32_t pins);
    static void workThunk(k_work *w);
};

/** @brief Zephyr GPIO output implementation of IGpioOutput. */
class ZephyrGpioOutput : public GpioInterface_GpioOutput
{
public:
    explicit ZephyrGpioOutput(const gpio_dt_spec &spec);
    ERR_TYPE_commonErr_E configure() override;
    void setPin(bool state) override;

private:
    const gpio_dt_spec &spec_;
};

extern const gpio_dt_spec btnSpec;
extern const gpio_dt_spec ledSpec;