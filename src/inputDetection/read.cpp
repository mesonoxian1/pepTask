#include "read.h"
#include "zbusTopics.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(read_class, LOG_LEVEL_DBG);

ReadClass::ReadClass(IGpioInput &gpio)
    : gpio_{gpio}
{
    k_work_init_delayable(&dwork_, workHandler);
}

int ReadClass::init()
{
    GpioStateCallback cb{};
    cb.fn  = &ReadClass::gpioCallbackThunk;
    cb.ctx = this;

    const int ret = gpio_.configure(cb);
    if (ret != 0) {
        LOG_ERR("IGpioInput::configure failed: %d", ret);
    }
    return ret;
}

void ReadClass::gpioCallbackThunk(void *ctx, bool /*isHigh*/)
{
    auto *self = static_cast<ReadClass *>(ctx);
    k_work_reschedule(&self->dwork_, K_MSEC(kDebounceMs));
}

void ReadClass::workHandler(k_work *work)
{
    k_work_delayable *dw   = CONTAINER_OF(work, k_work_delayable, work);
    ReadClass        *self = CONTAINER_OF(dw, ReadClass, dwork_);
    self->onWork();
}

void ReadClass::onWork()
{
    const bool state = gpio_.readPin();

    if (state == lastState_) {
        return;
    }
    lastState_ = state;

    LOG_INF("GPIO input: %s", state ? "HIGH" : "LOW");

    const GpioStateMsg msg{.isHigh = state};
    const int ret = zbus_chan_pub(&gpio_state_chan, &msg, K_NO_WAIT);

    LOG_INF("zbus_chan_pub ret=%d, isHigh=%d", ret, (int)state);

    if (ret != 0) {
        LOG_ERR("zbus_chan_pub failed: %d", ret);
    }
}
