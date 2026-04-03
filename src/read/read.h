#pragma once

#include "errTypes.h"
#include "gpioInterface.h"
#include <zephyr/kernel.h>

/**
 * @file read.h
 * @brief ReadClass — GPIO input sensing with debounce and ZBus publish.
 */

#define DEBOUNCE_TOUT_MS    (20) //!< Debounce timeout for the button

class ReadClass
{
public:
    explicit ReadClass(GpioInterface_GpioInput &gpio);
    ERR_TYPE_commonErr_E init();

private:
    static constexpr int32_t kDebounceMs {DEBOUNCE_TOUT_MS};

    GpioInterface_GpioInput &gpio_;
    k_work_delayable dwork_;
    bool             lastState_{false};

    static void gpioCallback(void *ctx, bool isHigh);
    static void workHandler(k_work *work);
    void process();
};
