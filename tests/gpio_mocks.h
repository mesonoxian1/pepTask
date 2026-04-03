#pragma once

#include "gpioInterface.h"
#include <gmock/gmock.h>

/**
 * @file gpio_mocks.h
 * @brief GMock implementations of GpioInterface_GpioInput and GpioInterface_GpioOutput.
 */

/**
 * @brief Mock GPIO input.
 *
 * @details configure() is intercepted by GMock and also stores the callback
 *          so tests can simulate pin edges via simulateEdge().
 */
class MockGpioInput : public GpioInterface_GpioInput
{
public:
    MOCK_METHOD(ERR_TYPE_commonErr_E, configure,
                (GpioInterface_GpioStateCallback cb), (override));
    MOCK_METHOD(bool, readPin, (), (const, override));

    /**
     * @brief Capture the callback while returning OK.
     *
     * Use in SetUp:
     * @code
     *   ON_CALL(mockInput, configure(_))
     *       .WillByDefault([&](GpioInterface_GpioStateCallback cb) {
     *           return mockInput.captureAndReturn(cb);
     *       });
     * @endcode
     */
    ERR_TYPE_commonErr_E captureAndReturn(GpioInterface_GpioStateCallback cb)
    {
        storedCb_ = cb;
        return ERR_TYPE_commonErr_OK;
    }

    /**
     * @brief Simulate a GPIO edge event.
     *
     * Invokes the callback captured by captureAndReturn().
     *
     * @param isHigh  Logical level to report to ReadClass.
     */
    void simulateEdge(bool isHigh)
    {
        if (storedCb_) storedCb_(isHigh);
    }

private:
    GpioInterface_GpioStateCallback storedCb_{};
};

/**
 * @brief Mock GPIO output.
 */
class MockGpioOutput : public GpioInterface_GpioOutput
{
public:
    MOCK_METHOD(ERR_TYPE_commonErr_E, configure, (), (override));
    MOCK_METHOD(void, setPin, (bool state), (override));
};
