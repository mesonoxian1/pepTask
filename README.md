# GPIO ZBus Application - PeP task

Zephyr RTOS application implementing GPIO input sensing and LED output control
using a publish/subscribe architecture via ZBus.

---

## Folder Structure

```
app/
├── CMakeLists.txt              # Zephyr app build configuration
├── prj.conf                    # Kconfig options (GPIO, ZBus, logging, C++)
├── app.overlay                 # Device tree overlay — defines button0 and led0 nodes
│
├── include/
│   ├── errTypes.h              # Common error type enum (ERR_TYPE_commonErr_E)
│   ├── gpioInterface.h         # Abstract interfaces: GpioInterface_GpioInput,
│   │                           # GpioInterface_GpioOutput, GpioStateCallback
│   └── zbusTopics.h            # ZBus channel declaration and GpioStateMsg definition
│
├── src/
│   ├── main.cpp                # Entry point — ZBus channel definition, concrete Zephyr
│   │                           # GPIO implementations (ZephyrGpioInput/Output),
│   │                           # application object wiring, simulation thread
│   ├── read/
│   │   ├── read.h              # ReadClass declaration
│   │   └── read.cpp            # ReadClass implementation — GPIO input sensing,
│   │                           # debounce via k_work_delayable, ZBus publish
│   └── react/
│       ├── react.h             # ReactClass declaration
│       └── react.cpp           # ReactClass implementation — ZBus observer, LED blink
|                               # and hold sequences via k_work_delayable
│
└── tests/
    ├── CMakeLists.txt          # Standalone GTest/GMock build (no Zephyr SDK needed)
    ├── zbus_chan_stub.cpp      # gpio_state_chan and react_listener definitions for 
    |                           # the test build (replaces main.cpp)
    ├── gpio_mocks.h            # MockGpioInput and MockGpioOutput via GMock
    ├── test_read_class.cpp     # ReadClass unit tests
    ├── test_react_class.cpp    # ReactClass unit tests
    └── stubs/
        └── include/
            ├── zephyr_stubs.h             # Host-side stubs for k_work, ZBus, logging
            └── zephyr/
                ├── kernel.h               # Shim → zephyr_stubs.h
                ├── logging/log.h          # Shim → zephyr_stubs.h
                └── zbus/zbus.h            # Shim → zephyr_stubs.h
```

---

## Prerequisites

### Zephyr & west

Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

```bash
# Install west
pip install west

# Initialise workspace
west init ~/zephyrproject
cd ~/zephyrproject
west update

# Install Python dependencies
pip install -r zephyr/scripts/requirements.txt
```

**Tested with Zephyr v4.4.0-rc1.**

### Python virtual environment (recommended)

```bash
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install west
```

### GTest 

No manual installation needed — GTest and GMock are downloaded automatically
via CMake `FetchContent` on first build. Requires internet access and CMake 3.20+.

---

## Building & Running

### Zephyr app (native_sim)

```bash
cd ~/zephyrproject/app

# Clean build
west build -b native_sim -p always

# Incremental build
west build -b native_sim

# Run
./build/zephyr/zephyr.exe
```

### GTest unit tests

```bash
cd ~/zephyrproject/app/tests

# First run — downloads GTest from GitHub
cmake -B build -S .
cmake --build build

# Run all tests
./build/gpio_tests

# Run specific test suite
./build/gpio_tests --gtest_filter="ReadClassTest*"
./build/gpio_tests --gtest_filter="ReactClassTest*"

# Verbose output
./build/gpio_tests --gtest_output=verbose
```

> **Note:** Tests are built and run from the `tests/` directory using a plain
> CMake project — no Zephyr SDK or `west` required.

---

## Simulation Thread

The application includes a simulation thread that automatically toggles the
emulated GPIO input to exercise the full application flow on `native_sim`
without physical hardware.

It is controlled by a flag in `src/main.cpp`:

```cpp
//! Define as true to enable the simulation thread
#define SIMULATION_THREAD_ENABLED   (false)
```

When set to `true`, the sim thread drives the emulated button input every
3 seconds, alternating between press (logical HIGH) and release (logical LOW).
This triggers the full chain: GPIO edge → debounce → ZBus publish →
ReactClass → LED blink/hold sequence.

Set to `false` for production builds or when using the Zephyr shell to
inject GPIO events manually.

---

## Architecture Overview

```
GPIO state change (hardware / emulated)
    │
    ▼
ZephyrGpioInput (main.cpp)
    │  ISR → k_work → GpioStateCallback
    ▼
ReadClass (src/read/)
    │  debounce via k_work_delayable
    │  zbus_chan_pub → gpio_state_chan
    ▼
ReactClass (src/react/)
    │  ZBus observer callback → k_work_delayable
    ▼
ZephyrGpioOutput (main.cpp)
    │  setPin() → LED blink / hold
    ▼
LED output
```

Hardware access is decoupled behind `GpioInterface_GpioInput` and
`GpioInterface_GpioOutput` in `include/gpioInterface.h`. `ReadClass` and
`ReactClass` depend only on these interfaces — enabling unit testing via
`MockGpioInput` and `MockGpioOutput` without any real hardware or Zephyr SDK.