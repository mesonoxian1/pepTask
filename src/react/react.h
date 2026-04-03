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
 #define LED_TOGGLE_COUNT       (6)   //!< Total LED toggle count for a 3-blink sequence (blinks wanted x 2)


//! LED state enumeration
typedef enum REACT_ledState_ENUM {
    REACT_ledState_OFF = 0, //!< LED state OFF
    REACT_ledState_ON,      //!< LED state ON
} REACT_ledState_E;

//! ReactClass body
class ReactClass
{
public:
    explicit ReactClass(GpioInterface_GpioOutput &gpio);
    ERR_TYPE_commonErr_E init();
    void zbusMsgEventHandler(bool isHigh);
#ifdef UNIT_TEST
   /*!
     * @brief   Test helper — immediately fires the pending work item.
     *
     * @details Exposes the internal k_work_delayable for unit tests without
     *          making it public in production builds.
     */
    void drainWorkForTest() { 
        drainDelayable(&dwork_); 
    }
#endif

private:
    static constexpr int32_t kBlinkPeriodMs{LED_BLINK_PERIOD_MS};   //!< LED toggle interval (ms)
    static constexpr int32_t kLowOnMs{LED_ON_IN_LOW_STATE_MS};      //!< Low-state LED on duration (ms)
    static constexpr int     kBlinkToggleCount{LED_TOGGLE_COUNT};   //!< Total toggles for blink sequence

    GpioInterface_GpioOutput &gpio_;                        //!< GPIO output abstraction
    k_work_delayable dwork_;                                //!< Delayable work item for LED sequencing
    bool             currentState_{false};                  //!< Currently applied GPIO logical state
    REACT_ledState_E previousLedState{REACT_ledState_OFF};  //!< Previously applied LED state 
    int              blinkCount_{0};                        //!< Remaining toggle count in current sequence

    static void workHandler(k_work *work);
    void        process();
};
