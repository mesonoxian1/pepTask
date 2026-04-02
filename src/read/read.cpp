#include "read.h"
#include "zbusTopics.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

/**
 * @brief Construct a ReadClass and initialise the delayable work item.
 */
ReadClass::ReadClass() { 
    // init dwork with workHandler
     k_work_init_delayable(&dwork_, workHandler);
}

/**
 * @brief   Initialise the ReadClass and configure the GPIO input.
 *
 * @return  ERR_TYPES_commonErr_OK on success, ERR_TYPES_commonErr_FAIL on failure.
 */
ERR_TYPE_commonErr_E ReadClass::init() {

    return ERR_TYPES_commonErr_OK;
}

/**
 * @brief   Static GPIO edge callback — defers processing to the work queue.
 *
 * @details Invoked from work-queue context by the concrete GpioInput
 *          implementation on every edge. Reschedules the debounce timer,
 *          resetting it on rapid successive edges so only the final stable
 *          state is processed. The isHigh parameter is intentionally ignored
 *          here — the actual pin state is read fresh in process() after the
 *          debounce window expires.
 *
 * @param ctx     Pointer to the owning ReadClass instance.
 * @param isHigh  Current logical pin state (unused).
 */
static void ReadClass::gpioCallback(void *ctx, bool isHigh) {

    (void) isHigh;

    auto *self = static_cast<ReadClass *>(ctx);
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
static void ReadClass::workHandler(k_work *work) {

    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReadClass        *self = CONTAINER_OF(dw, ReadClass, dwork_);
    self->process();
}

/**
 * @brief   Debounce expiry handler — reads pin state and publishes via ZBus.
 *
 * @details Called in work-queue context after the debounce window expires.
 *          Reads the current logical pin state, publishes a ZBusTopics_gpioStateMsg
 *          on gpio_state_chan if the state has changed since the last publish.
 */
void ReadClass::process() {
    
    const ZBusTopics_gpioStateMsg msg { .isHigh = 0 };
    // publish the state change
    const int ret = zbus_chan_pub(&gpio_state_chan, &msg, K_NO_WAIT);
}