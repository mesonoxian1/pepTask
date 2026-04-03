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
extern "C" void zbusListenerCb(const struct zbus_channel *chan)
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
 */
ZBUS_OBS_DECLARE(react_listener);

/**
 * @brief Construct a ReactClass and initialise the delayable work item.
 */
ReactClass::ReactClass(GpioInterface_GpioOutput &gpio) : gpio_{gpio} {

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
    ERR_TYPE_commonErr_E err = ERR_TYPE_commonErr_OK;

    const int ret = gpio_.configure();

    if(ret != 0) {
        LOG_ERR("IGpioOutput::configure failed: %d", ret);
        err = ERR_TYPE_commonErr_FAIL;
    }
    /*
     * Register observer at runtime — avoids linker iterable section issues
     * when ZBUS_CHAN_DEFINE and ZBUS_CHAN_ADD_OBS are in different TUs. 
     */
    const int obsRet = zbus_chan_add_obs(&gpio_state_chan, &react_listener, K_NO_WAIT);
    if(err == ERR_TYPE_commonErr_OK) {
        if(obsRet != 0) {
            LOG_ERR("zbus_chan_add_obs failed: %d", obsRet);
            err = ERR_TYPE_commonErr_FAIL;
        } else {
            LOG_INF("ReactClass init OK, observer registered");
            err = ERR_TYPE_commonErr_OK;
        }
    }

   return err;
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
    blinkCount_   = (isHigh == true) ? kBlinkToggleCount : 0;
    // call the handler immediatelly
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


    // in case GPIO is set to HIGH - blink the LED 3 times with 100ms between each blink
    if(currentState_ == true) {
        // only enter three-blink sequence once
        if(blinkCount_ > 0) {
            blinkCount_--;
            LOG_DBG("Blink: LED %s (%d left)", (previousLedState == REACT_ledState_OFF) ? "ON" : "OFF", blinkCount_);

            if(previousLedState == REACT_ledState_ON) {
                gpio_.setPin(false);
                previousLedState = REACT_ledState_OFF;
            } else {
                gpio_.setPin(true);
                previousLedState = REACT_ledState_ON;
            }

            k_work_reschedule(&dwork_, K_MSEC(kBlinkPeriodMs));
        } 
    } else {
        // otherwise if the state is low, turn the LED on and keep it on for 500ms.
        if(previousLedState == REACT_ledState_OFF) {
            previousLedState = REACT_ledState_ON;
            gpio_.setPin(true);
            LOG_DBG("LED ON for %d ms (LOW state)", kLowOnMs);
            k_work_reschedule(&dwork_, K_MSEC(kLowOnMs));
        } else {
            previousLedState = REACT_ledState_OFF;
            gpio_.setPin(false);
            LOG_DBG("LED OFF (LOW state complete)");
        }
    }
}