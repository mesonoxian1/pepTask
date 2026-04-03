#pragma once

#include "errTypes.h"
#include "gpioInterface.h"
#include <zephyr/kernel.h>

/**
 * @file react.h
 * @brief ReactClass — LED output control driven by ZBus GPIO state messages.
 */
 
//! LED state enumeration
typedef enum REACT_ledState_ENUM {
    REACT_ledState_OFF = 0, //!< LED state OFF
    REACT_ledState_ON,      //!< LED state ON
} REACT_ledState_E;

class ReactClass
{
public:
    explicit ReactClass(GpioInterface_GpioOutput &gpio);
    ERR_TYPE_commonErr_E init();
    void zbusMsgEventHandler(bool isHigh);

private:
    static constexpr int32_t kBlinkPeriodMs{100};
    static constexpr int32_t kLowOnMs{500};
    static constexpr int     kBlinkToggleCount{6};

    GpioInterface_GpioOutput &gpio_;
    k_work_delayable dwork_;
    bool             currentState_{false};
    REACT_ledState_E previousLedState{REACT_ledState_OFF};
    int              blinkCount_{0};

    static void workHandler(k_work *work);
    void        process();
};