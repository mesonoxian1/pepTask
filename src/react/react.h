#pragma once

#include "errTypes.h"
#include "gpioInterface.h"
#include <zephyr/kernel.h>

/**
 * @file react.h
 * @brief ReactClass — LED output control driven by ZBus GPIO state messages.
 */

 #define LED_BLINK_PERIOD_MS    (100) //!< LED blink interval in ms
 #define LED_ON_IN_LOW_STATE_MS (500) //!< Time that LED will stay turner ON if input is low
 #define LED_TOGGLE_COUNT       (6u)  //!< Total LED toggle count for a 3-blink sequence (blinks wanted x 2)


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
#ifdef UNIT_TEST
    void drainWorkForTest() { drainDelayable(&dwork_); }
#endif

private:
    static constexpr int32_t kBlinkPeriodMs{LED_BLINK_PERIOD_MS};
    static constexpr int32_t kLowOnMs{LED_ON_IN_LOW_STATE_MS};
    static constexpr int     kBlinkToggleCount{LED_TOGGLE_COUNT};

    GpioInterface_GpioOutput &gpio_;
    k_work_delayable dwork_;
    bool             currentState_{false};
    REACT_ledState_E previousLedState{REACT_ledState_OFF};
    int              blinkCount_{0};

    static void workHandler(k_work *work);
    void        process();
};