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
#ifdef UNIT_TEST
    friend class ReadClassTest;
#endif

public:
    explicit ReadClass(GpioInterface_GpioInput &gpio);
    ERR_TYPE_commonErr_E init();
#ifdef UNIT_TEST
    k_work_delayable dwork_;  //!< Exposed for unit testing via drainDelayable()
#endif
private:
    static constexpr int32_t kDebounceMs {DEBOUNCE_TOUT_MS};

    GpioInterface_GpioInput &gpio_;
    bool             lastState_{false};
#ifndef UNIT_TEST
    k_work_delayable dwork_;
#endif
    static void gpioCallback(void *ctx, bool isHigh);
    static void workHandler(k_work *work);
    void process();
};
