#include "react.h"
#include "zbusTopics.h"
#include "errTypes.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

/**
 * @brief   Registers the Zephyr logging module for this translation unit.
 *
 * @details Associates all LOG_INF, LOG_ERR, LOG_DBG calls in react.cpp with
 *          the module name "react_class". Log lines from this file will appear
 *          as <inf> react_class: ... in the output.
 *          LOG_LEVEL_DBG enables all log levels including debug messages.
 */
LOG_MODULE_REGISTER(react_class, LOG_LEVEL_DBG);

static ReactClass *reactInstance {nullptr};

/**
 * @brief   ZBus listener callback invoked by the VDED dispatcher on channel publish.
 *
 * @details Reads the message directly via zbus_chan_const_msg() since the channel
 *          is already locked by the dispatcher during callback execution.
 *          Forwards the GPIO state to the ReactClass instance via zbusMsgEventHandler().
 *
 * @param chan  Pointer to the ZBus channel that was published to.
 */
void zbusListenerCb(const struct zbus_channel *chan)
{
    if (reactInstance != nullptr) {
        const ZBusTopics_gpioStateMsg *msg =
            static_cast<const ZBusTopics_gpioStateMsg *>(zbus_chan_const_msg(chan));
    
        LOG_INF("ZBus received: isHigh = %d", msg->isHigh);
        reactInstance->zbusMsgEventHandler(msg->isHigh);
    }

}

/**
 * @brief   Defines the ZBus listener observer for the ReactClass.
 *
 * @details Creates a static zbus_observer struct named react_listener with
 *          zbusListenerCb as the callback. Must be in the same translation
 *          unit as ZBUS_CHAN_ADD_OBS or registered via zbus_chan_add_obs()
 *          at runtime to ensure correct linker section population.
 */
ZBUS_LISTENER_DEFINE(react_listener, zbusListenerCb);

/**
 * @brief Construct a ReactClass and initialise the delayable work item.
 */
ReactClass::ReactClass() {

    k_work_init_delayable(&dwork_, workHandler);
}

/**
 * @brief   Initialise the ReactClass and register the ZBus observer at runtime.
 *
 * @details Stores the instance pointer for use by the static ZBus listener callback,
 *          then registers react_listener on gpio_state_chan via zbus_chan_add_obs().
 *          Runtime registration is used to avoid linker iterable section issues when
 *          ZBUS_CHAN_DEFINE and ZBUS_CHAN_ADD_OBS are in different translation units.
 *
 * @return  ERR_TYPES_commonErr_OK on success, ERR_TYPES_commonErr_FAIL on failure.
 */
ERR_TYPE_commonErr_E ReactClass::init() {

    reactInstance = this;
    ERR_TYPE_commonErr_E success;
    /*
     * Register observer at runtime — avoids linker iterable section issues
     * when ZBUS_CHAN_DEFINE and ZBUS_CHAN_ADD_OBS are in different TUs. 
     */
    const int obs_ret = zbus_chan_add_obs(&gpio_state_chan, &react_listener, K_NO_WAIT);

    if (obs_ret != 0) {
        LOG_ERR("zbus_chan_add_obs failed: %d", obs_ret);
        success = ERR_TYPE_commonErr_FAIL;
    } else {
        LOG_INF("ReactClass init OK, observer registered");
        success = ERR_TYPE_commonErr_OK;
    }

   return success;
}

/**
 * @brief   Handle an incoming ZBus GPIO state message.
 *
 * @details Latches the new state and resets the blink counter, then schedules
 *          the work handler immediately via k_work_reschedule. GPIO is never
 *          driven directly here — all LED control happens in process().
 *
 * @param isHigh  true if the GPIO input is logically high, false if low.
 */
void ReactClass::zbusMsgEventHandler(bool isHigh) {

    currentState_ = isHigh;
    blinkCount_   = isHigh ? kBlinkToggleCount : 0;
    k_work_reschedule(&dwork_, K_NO_WAIT);
}

/**
 * @brief   Static work-queue thunk — recovers the ReactClass instance and calls process().
 *
 * @details Uses CONTAINER_OF to walk from the k_work pointer back to the owning
 *          ReactClass via the embedded k_work_delayable member dwork_.
 *
 * @param work  Pointer to the embedded k_work inside dwork_.
 */
void ReactClass::workHandler(k_work *work) {

    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReactClass       *self = CONTAINER_OF(dw, ReactClass, dwork_);
    self->process();
}

void ReactClass::process() {

}