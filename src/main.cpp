#include "zbusTopics.h"
#include "gpioInterface.h"
#include "read.h"
#include "react.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#define SIMULATION_THREAD_ENABLED   (FALSE)


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

/* ---------------------------------------------------------------------------
 * Device-tree GPIO specs
 * -------------------------------------------------------------------------- */

static const gpio_dt_spec btnSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);

static const gpio_dt_spec ledSpec =
    GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

/* ---------------------------------------------------------------------------
 * POD ISR context — avoids CONTAINER_OF on non-standard-layout ZephyrGpioInput
 * -------------------------------------------------------------------------- */

struct GpioIsrCtx {
    struct gpio_callback cbData;  /* must be first */
    void                *self;
    struct k_work        isrWork;
};

/* ---------------------------------------------------------------------------
 * Concrete Zephyr GPIO implementations
 * -------------------------------------------------------------------------- */

class ZephyrGpioInput : public GpioInterface_GpioInput
{
public:
    explicit ZephyrGpioInput(const gpio_dt_spec &spec) : spec_{spec} {

        ctx_.self = this;
        k_work_init(&ctx_.isrWork, workThunk);
    }

    ERR_TYPE_commonErr_E configure(GpioInterface_GpioStateCallback cb) override {

        ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;
        userCb_ = cb;

        if (device_is_ready(spec_.port) == false) {
            LOG_ERR("GPIO input device not ready");
            err = ERR_TYPE_commonErr_FAIL; 
        }

        int ret = gpio_pin_configure_dt(&spec_, GPIO_INPUT | GPIO_PULL_UP);
       
        if (ret != 0) { 
            err = ERR_TYPE_commonErr_FAIL; 
        }

        gpio_init_callback(&ctx_.cbData, isrHandler, BIT(spec_.pin));
        ret = gpio_add_callback(spec_.port, &ctx_.cbData);
   
        if (ret != 0) {
            err = ERR_TYPE_commonErr_FAIL; 
        }

        gpio_pin_interrupt_configure_dt(&spec_, GPIO_INT_EDGE_BOTH);

        return err;
    }

    bool readPin() const override
    {
        const bool state = gpio_pin_get_dt(&spec_) > 0;
        LOG_INF("Input GPIO state -> %s", state ? "HIGH" : "LOW");
        return state;
    }

private:
    const gpio_dt_spec &spec_;
    GpioIsrCtx          ctx_{};
    GpioInterface_GpioStateCallback   userCb_{};

    static void isrHandler(const struct device *, struct gpio_callback *cb, uint32_t pins) {

        GpioIsrCtx *ctx = reinterpret_cast<GpioIsrCtx *>(cb);
        k_work_submit(&ctx->isrWork);
    }

    static void workThunk(k_work *w) {

        GpioIsrCtx      *ctx  = CONTAINER_OF(w, GpioIsrCtx, isrWork);
        ZephyrGpioInput *self = static_cast<ZephyrGpioInput *>(ctx->self);
        self->userCb_(self->readPin());
    }
};

class ZephyrGpioOutput : public GpioInterface_GpioOutput
{
public:
    explicit ZephyrGpioOutput(const gpio_dt_spec &spec) : spec_{spec} {

    }

    ERR_TYPE_commonErr_E configure() override
    {
        ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;

        if(device_is_ready(spec_.port) == false) {
            LOG_ERR("GPIO output device not ready");
            err = ERR_TYPE_commonErr_FAIL;
        }

        gpio_pin_configure_dt(&spec_, GPIO_OUTPUT_INACTIVE);
        return err;
    }

    void setPin(bool state) override
    {
        gpio_pin_set_dt(&spec_, state ? 1 : 0);
        LOG_INF("LED GPIO control -> %s", state ? "ON" : "OFF");
    }

private:
    const gpio_dt_spec &spec_;
};

#if(SIMULATION_THREAD_ENABLED == TRUE) SIMULATION_THREAD_ENABLED
/* ---------------------------------------------------------------------------
 * Simulation thread
 *
 * gpio_emul_input_set sets RAW electrical level.
 * Button is GPIO_ACTIVE_LOW so:
 *   raw 0 -> logical HIGH (button pressed)
 *   raw 1 -> logical LOW  (button released)
 * -------------------------------------------------------------------------- */
 
static void simThread(void *, void *, void *)
{
    k_sleep(K_SECONDS(1));
    
    // simulate input button
    for(; ; ) {
        LOG_INF("--- Button PRESS (logical HIGH) ---");
        gpio_emul_input_set(btnSpec.port, btnSpec.pin, 0);
        // delay for 3 s
        k_sleep(K_SECONDS(3));
 
        LOG_INF("--- Button RELEASE (logical LOW) ---");
        gpio_emul_input_set(btnSpec.port, btnSpec.pin, 1);
        // delay for 3 s
        k_sleep(K_SECONDS(3));
    }
}
 
K_THREAD_DEFINE(sim_tid, 1024, simThread, NULL, NULL, NULL, 5, 0, 0);
#endif

/* ---------------------------------------------------------------------------
 * Application objects
 * -------------------------------------------------------------------------- */

static ZephyrGpioInput  gpioIn{btnSpec};
static ZephyrGpioOutput gpioOut{ledSpec};
static ReadClass        readObj{gpioIn};
static ReactClass       reactObj{gpioOut};

//! ZBus listener — defined here so the linker always includes it
extern "C" void zbusListenerCb(const struct zbus_channel *chan);
ZBUS_LISTENER_DEFINE(react_listener, zbusListenerCb);

int main(void) {

    ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;
    LOG_INF("Starting... ");

    if (readObj.init() != ERR_TYPE_commonErr_OK) {
        LOG_ERR("ReadClass::init failed");
        err = ERR_TYPE_commonErr_FAIL;
    }
 
    if (reactObj.init() != ERR_TYPE_commonErr_OK) {
        LOG_ERR("ReactClass::init failed");
        err = ERR_TYPE_commonErr_FAIL;
    }
 
    LOG_INF("Init complete — waiting for GPIO events");
    return err;
}