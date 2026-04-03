#pragma once

/**
 * @file zephyr_stubs.h
 * @brief Minimal host-side stubs for Zephyr kernel and ZBus primitives.
 *
 * Allows read.cpp and react.cpp to compile on a standard Linux host
 * without the Zephyr SDK. Real scheduling is not emulated — tests control
 * execution by calling drainDelayable() to fire work handlers on demand.
 */

#include <cstdint>
#include <cstddef>
#include <cstring>

#ifndef ENODEV
#define ENODEV 19
#endif

/* ============================================================
 * k_work / k_work_delayable
 * ============================================================ */

using k_work_handler_t = void (*)(struct k_work *);

struct k_work {
    k_work_handler_t handler{nullptr};
};

struct k_work_delayable {
    k_work   work{};
    int32_t  delayMs{0};
    bool     pending{false};
};

struct k_timeout_t { int64_t ticks{0}; };

#define K_NO_WAIT   (k_timeout_t{0})
#define K_MSEC(ms)  (k_timeout_t{static_cast<int64_t>(ms)})

inline void k_work_init(k_work *w, k_work_handler_t h)
    { w->handler = h; }

inline void k_work_submit(k_work *w)
    { if (w->handler) w->handler(w); }

inline void k_work_init_delayable(k_work_delayable *dw, k_work_handler_t h)
    { k_work_init(&dw->work, h); dw->pending = false; }

inline int k_work_reschedule(k_work_delayable *dw, k_timeout_t t)
    { dw->delayMs = static_cast<int32_t>(t.ticks); dw->pending = true; return 0; }

/**
 * @brief Test helper — fire a pending delayable work item immediately.
 *
 * Simulates the work-queue executing after the scheduled delay.
 * Call once per expected reschedule; loop if the handler reschedules itself.
 */
inline void drainDelayable(k_work_delayable *dw)
{
    while (dw->pending) {
        dw->pending = false;
        if (dw->work.handler) dw->work.handler(&dw->work);
    }
}

#define CONTAINER_OF(ptr, type, member) \
    reinterpret_cast<type*>(reinterpret_cast<char*>(ptr) - offsetof(type, member))

/* ============================================================
 * ZBus stubs
 * ============================================================ */

struct zbus_channel;
using zbus_obs_cb_t = void (*)(const struct zbus_channel *);

struct zbus_observer {
    zbus_obs_cb_t callback{nullptr};
};

/**
 * @brief Minimal ZBus channel stub.
 *
 * Stores the last published message and one registered observer.
 */
struct zbus_channel {
    void          *msgPtr{nullptr};
    std::size_t    msgSize{0};
    zbus_observer *observer{nullptr};
};

inline int zbus_chan_pub(zbus_channel *chan, const void *msg, k_timeout_t)
{
    if (chan->msgPtr && msg) std::memcpy(chan->msgPtr, msg, chan->msgSize);
    if (chan->observer && chan->observer->callback) chan->observer->callback(chan);
    return 0;
}

inline int zbus_chan_read(const zbus_channel *chan, void *dst, k_timeout_t)
{
    if (!chan->msgPtr || !dst) return -1;
    std::memcpy(dst, chan->msgPtr, chan->msgSize);
    return 0;
}

inline const void *zbus_chan_const_msg(const zbus_channel *chan)
    { return chan->msgPtr; }

inline int zbus_chan_add_obs(zbus_channel *chan, zbus_observer *obs, k_timeout_t)
    { chan->observer = obs; return 0; }

#define ZBUS_CHAN_DECLARE(name)      extern zbus_channel name
#define ZBUS_OBSERVERS_EMPTY
#define ZBUS_MSG_INIT(...)          {}

#define ZBUS_CHAN_DEFINE(name, msg_type, _v, _u, _obs, init_val)    \
    static msg_type _zbus_msg_##name init_val;                       \
    zbus_channel name {                                               \
        &_zbus_msg_##name,                                            \
        sizeof(msg_type),                                             \
        nullptr                                                       \
    }

#define ZBUS_LISTENER_DEFINE(name, cb)  zbus_observer name{cb}
#define ZBUS_OBS_DECLARE(name)          extern zbus_observer name

/* ============================================================
 * Logging stubs — no-ops on host
 * ============================================================ */
#define LOG_MODULE_REGISTER(...)  /* nothing */
#define LOG_INF(...)              /* nothing */
#define LOG_ERR(...)              /* nothing */
#define LOG_DBG(...)              /* nothing */
#define LOG_WRN(...)              /* nothing */