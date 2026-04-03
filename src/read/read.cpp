#include "read.h"
#include "zbusTopics.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(read_class, LOG_LEVEL_DBG);

/*!
 * @brief   Construct a ReadClass bound to a GPIO input abstraction.
 *
 * @param   gpio    Reference to a GpioInterface_GpioInput implementation.
 *                  Must remain valid for the lifetime of this object.
 */
ReadClass::ReadClass(GpioInterface_GpioInput &gpio) : gpio_{gpio} { 
    
    // init dwork with workHandler - work handler will be called after reschedule is intiated
    k_work_init_delayable(&dwork_, workHandler);
}

/**
 * @brief   Configure the GPIO input and pass the callback for edge-change detection.
 *
 * @return  ERR_TYPE_commonErr_OK on success, ERR_TYPE_commonErr_FAIL on failure.
 */
ERR_TYPE_commonErr_E ReadClass::init() {

    ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;
   
    GpioInterface_GpioStateCallback cb{};
    cb.func = &ReadClass::gpioCallback;
    cb.ctx  = this;

    err = gpio_.configure(cb);

    if(err != ERR_TYPE_commonErr_OK) {
        LOG_ERR("IGpioInput::configure failed: %d", err);
    }

    return err;
}

/*!
 * @brief   Static GPIO edge callback — defers processing to the work queue.
 *
 * @details Reschedules the debounce timer on every edge. Rapid successive
 *          edges reset the timer so only the final stable state is processed.
 *          The isHigh parameter is intentionally ignored — the actual pin
 *          state is read fresh in process() after the debounce window expires.
 *
 * @param   ctx     Pointer to the owning ReadClass instance.
 * @param   isHigh  Current logical pin state (unused).
 */
void ReadClass::gpioCallback(void *ctx, bool isHigh) {

    (void) isHigh;

    auto *self = static_cast<ReadClass *>(ctx);
    // debounce is acchieved using a reschedule - take an actual reading later
    k_work_reschedule(&self->dwork_, K_MSEC(kDebounceMs));
}

/**
 * @brief   Static work-queue thunk — recovers the ReadClass instance and calls process().
 *
 * @details Uses CONTAINER_OF to walk from the k_work pointer back to the owning
 *          ReadClass via the embedded k_work_delayable member dwork_.
 *
 * @param work  Pointer to the embedded k_work inside dwork_.
 */
void ReadClass::workHandler(k_work *work) {

    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReadClass        *self = CONTAINER_OF(dw, ReadClass, dwork_);
    self->process();
}

/*!
 * @brief   Debounce expiry handler — reads pin state and publishes via ZBus.
 *
 * @details Called in work-queue context after the debounce window expires.
 *          Reads the current logical pin state and publishes a
 *          ZBusTopics_gpioStateMsg on gpio_state_chan if the state has
 *          changed since the last publish.
 */
void ReadClass::process() {
    
    int ret;
    const bool state = gpio_.readPin();
    const ZBusTopics_gpioStateMsg msg { 
        .isHigh = state 
    };

    if(state != lastState) {
        // store the state for the next state change check
        lastState = state;
    
        LOG_INF("GPIO input: %s", state ? "HIGH" : "LOW");
        // publish the state change
        ret = zbus_chan_pub(&gpio_state_chan, &msg, K_NO_WAIT);
        
        LOG_INF("zbus_chan_pub ret = %d, isHigh = %d", ret, state);
        
        if(ret != 0) {
            LOG_ERR("zbus_chan_pub failed: %d", ret);
        }
    }
}
