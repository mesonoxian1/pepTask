#pragma once

#include "errTypes.h"
/**
 * @file gpioInterface.h
 * @brief Hardware GPIO abstractions.
 */
 
/*!
 * @brief   Callback used to notify consumers of a GPIO input state change.
 *
 * @details Wraps a plain function pointer and a context pointer to avoid
 *          any dependency on std::function or the heap.
 */
struct GpioInterface_GpioStateCallback
{
    /**
     * @brief Function to invoke on GPIO state change.
     *
     * @param ctx       Opaque context pointer set by the caller.
     * @param isHigh    TRUE if the pin is now logically high, FALSE if low.
     */
    void (*func) (void *ctx, bool isHigh) {nullptr};
    /*! Opaque context forwarded to fn on every invocation. */
    void *ctx {nullptr};

    /**
     * @brief Invoke the callback if set.
     * @param isHigh  [in] Current logical pin state.
     */
    void operator() (bool isHigh) const
    {
        if(func != nullptr) { 
            func(ctx, isHigh); 
        }
    }
    /**
     * @brief   Check whether a valid callback is set.
     * @return  TRUE if funnc is non-null.
     */
    explicit operator bool() const { 
        return func != nullptr; 
    }
};

/**
 * @brief   Abstract interface for a GPIO input pin.
 *
 * @details Implement with a real Zephyr driver or a test mock.
 *          ReadClass depends only on this interface.
 */
class GpioInterface_GpioInput {

public:
    virtual ~GpioInterface_GpioInput() = default;
    /**
     * @brief   Configure the pin as input pull-up and register an edge callback.
     *
     * @param   cb  Invoked from work-queue context on every logical state change.
     * @return  ERR_TYPES_commonErr_OK on success, ERR_TYPES_commonErr_FAIL on failure.
     */
    virtual ERR_TYPE_commonErr_E configure(GpioInterface_GpioStateCallback cb) = 0;
    /**
     * @brief   Read the current logical state of the input pin.
     * @return  TRUE if the pin is logically high, FALSE if low.
     */
    virtual bool readPin() const = 0;
};

/**
 * @brief   Abstract interface for a GPIO output pin.
 *
 * @details Implement with a real Zephyr driver or a test mock.
 *          ReactClass depends only on this interface.
 */
class GpioInterface_GpioOutput { 

public:
    virtual ~GpioInterface_GpioOutput() = default;
    /**
     * @brief   Configure the pin as a push-pull digital output.
     * @return  ERR_TYPES_commonErr_OK on success, ERR_TYPES_commonErr_FAIL on failure.
     */
    virtual ERR_TYPE_commonErr_E configure() = 0;
    /**
     * @brief Drive the output pin to a logical level.
     * @param state  TRUE = high (LED on), FALSE = low (LED off).
     */
    virtual void setPin(bool state) = 0;
};
