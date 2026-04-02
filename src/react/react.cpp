#include "react.h"
#include "zbusTopics.h"
#include "errTypes.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(react_class, LOG_LEVEL_DBG);

static ReactClass *reactInstance {nullptr};

void zbusListenerCb(const struct zbus_channel *chan)
{
    if (reactInstance != nullptr) {
        const ZBusTopics_gpioStateMsg *msg =
            static_cast<const ZBusTopics_gpioStateMsg *>(zbus_chan_const_msg(chan));
    
        LOG_INF("ZBus received: isHigh = %d", msg->isHigh);
        reactInstance->zbusMsgEventHandler(msg->isHigh);
    }

}

ZBUS_LISTENER_DEFINE(react_listener, zbusListenerCb);

ReactClass::ReactClass() {

    k_work_init_delayable(&dwork_, workHandler);
}

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
        success = ERR_TYPES_commonErr_OK;
    } else {
        LOG_INF("ReactClass init OK, observer registered");
        success = ERR_TYPES_commonErr_FAIL;
    }

   return success;
}

void ReactClass::zbusMsgEventHandler(bool isHigh) {

    currentState_ = isHigh;
    blinkCount_   = isHigh ? kBlinkToggleCount : 0;
    k_work_reschedule(&dwork_, K_NO_WAIT);
}

void ReactClass::workHandler(k_work *work) {

    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReactClass       *self = CONTAINER_OF(dw, ReactClass, dwork_);
    self->process();
}

void ReactClass::process() {

}