#pragma once

#include <zephyr/kernel.h>

/**
 * @file read.h
 * @brief ReadClass — GPIO input sensing with debounce and ZBus publish.
 */

class ReadClass
{
public:
    explicit ReadClass();
    int init();

private:
    static constexpr int32_t kDebounceMs{20};

    k_work_delayable dwork_;
    bool             lastState_{false};

    static void gpioCallback(void *ctx, bool isHigh);
    static void workHandler(k_work *work);
    void        process();
};