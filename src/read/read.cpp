#include "read.h"
#include "zbusTopics.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>


ReadClass::ReadClass() { 
    // init dwork with workHandler
     k_work_init_delayable(&dwork_, workHandler);
}

ERR_TYPE_commonErr_E ReadClass::init() {

}

void ReadClass::gpioCallback(void *ctx, bool isHigh) {

    (void) isHigh;

    auto *self = static_cast<ReadClass *>(ctx);
    k_work_reschedule(&self->dwork_, K_MSEC(kDebounceMs));

}

void ReadClass::workHandler(k_work *work) {

    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReadClass        *self = CONTAINER_OF(dw, ReadClass, dwork_);
    self->process();
}

void ReadClass::process() {
    
    const ZBusTopics_gpioStateMsg msg { .isHigh = 0 };
    // publish the state change
    const int ret = zbus_chan_pub(&gpio_state_chan, &msg, K_NO_WAIT);
}