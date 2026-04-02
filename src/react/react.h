#pragma once

#include <zephyr/kernel.h>

/**
 * @file react.h
 * @brief ReactClass — LED output control driven by ZBus GPIO state messages.
 */

class ReactClass
{
public:
    explicit ReactClass();
    int  init();
    void zbusMsgEventHandler(bool isHigh);

private:
    static constexpr int32_t kBlinkPeriodMs{100};
    static constexpr int32_t kLowOnMs{500};
    static constexpr int     kBlinkToggleCount{6};

    k_work_delayable dwork_;
    bool             currentState_{false};
    int              blinkCount_{0};

    static void workHandler(k_work *work);
    void        process();
};