# Phase 1: Foundation (HAL) – Input & Power Management Refactor

## Overview
This refactoring phase establishes a sterile boundary between hardware and application logic by introducing an abstraction layer for Input and Power management.

## Changes

### 1. Logical Input Events
- **File:** `InputEvents.h`
- **Description:** Defined an enum class `InputEvent` representing logical actions (e.g., `UP_PRESS`, `OK_SHORT_PRESS`, `STOP_PRESS`) instead of raw key codes.
- **Benefits:** Decouples the application from physical key mapping. Allows for easier remapping or supporting different input devices.

### 2. Input Manager
- **Files:** `InputManager.h`, `InputManager.cpp`
- **Description:**
    - Encapsulates the `Keypad` library and its configuration (rows, cols, pins).
    - Implements a polling-based state machine in `update()` to detect Short and Long presses.
    - Exposes a simple `getNextEvent()` API for the main loop.
- **Usage:**
    - `InputManager::init()` in `setup()`.
    - `InputManager::update()` in `loop()`.
    - `InputManager::getNextEvent()` to retrieve events.

### 3. Power Controller
- **Files:** `PowerController.h`, `PowerController.cpp`
- **Description:**
    - Centralizes power management logic.
    - Handles RTC GPIO initialization and deep sleep entry (`enterDeepSleep`).
    - Tracks idle time via `feedWatchdog()` and `tick()`.
    - Provides `preventSleep()` for critical operations like OTA.
- **Usage:**
    - `PowerController::init()` in `setup()`.
    - `PowerController::tick()` in `loop()`.
    - `PowerController::feedWatchdog()` whenever user activity is detected.

### 4. Main Application Cleanup
- **File:** `PultV1_0_2.ino`
- **Description:**
    - Removed global `Keypad` object and pin arrays.
    - Removed legacy `keypadEvent` function and monolithic switch statement.
    - Removed legacy `SetSleep` function and manual idle checks in `loop`.
    - Integrated `InputManager` and `PowerController`.
    - Currently, the main loop prints detected input events to Serial for verification, effectively disabling the old UI interaction logic.

# Phase 2: Data Abstraction (Model) – The DataManager

## Overview
This phase centralizes all state management, protocol parsing, and UART communication into a single `DataManager` class. This eliminates global variables for telemetry and configuration and enforces a strict separation between the UI and the data layer.

## Changes

### 1. DataManager Singleton
- **Files:** `DataManager.h`, `DataManager.cpp`
- **Description:**
    - Implemented as a Singleton (`DataManager::getInstance()`).
    - Encapsulates `ShuttleProtocol.h` usage, `TxJob` queue, and `ProtocolParser`.
    - Manages all data structs (`TelemetryPacket`, `SensorPacket`, `StatsPacket`, `Config`).
    - Implements atomic "dirty flags" for UI rendering optimization.

### 2. Command Gateway & Polling Engine
- **Features:**
    - **Command Gateway:** `sendCommand(SP::CmdType cmd, ...)` handles packet construction and queuing. Returns `false` if queue is full.
    - **Context-Aware Polling:** `tick()` automatically handles heartbeats and sensor requests based on the active `PollContext` (MAIN, DEBUG, STATS).
    - **Manual Mode Logic:** Encapsulated "Dead-Man" heartbeat logic within `tick()` when manual mode is active.

### 3. Main Application Refactor
- **File:** `PultV1_0_2.ino`
- **Description:**
    - Removed all legacy global variables (`txQueue`, `cachedTelemetry`, `parser`, etc.).
    - Removed raw `Serial2` handling functions (`handleRx`, `processTxQueue`).
    - `loop()` now calls `DataManager::getInstance().tick()` and updates polling context based on `page`.
    - `cmdSend` and `keypadEvent` now use `DataManager` API instead of direct queue manipulation.
    - UI rendering reads data via read-only getters (`getTelemetry()`, etc.).

## Verification
- **Headless Mode:** The system prints telemetry updates to Serial when dirty flags are consumed.
- **Queue Management:** The "QUEUE FULL" UI warning logic is preserved using the return value of `sendCommand`.
- **Polling:** Heartbeats are automatically sent at 400ms (Main) or 1500ms (Background) without cluttering the main loop.

# Phase 3: UI Framework Core (The Screen Stack)

## Overview
This phase implements the core UI framework, replacing the legacy enum-based navigation with an Object-Oriented "Screen Stack" architecture. This ensures memory safety (static allocation), CPU efficiency (dirty-flag rendering), and modularity.

## Changes

### 1. The Screen Interface
- **File:** `Screen.h`
- **Description:** Abstract base class for all UI views.
- **Key Methods:**
    - `onEnter()`: Reset state, request configs.
    - `onExit()`: Cleanup.
    - `draw(U8G2& display)`: Render logic (pure virtual).
    - `handleInput(InputEvent)`: Input processing (pure virtual).
    - `tick()`: Logical updates (animations, data polling).
- **Optimization:** Implements a `_needsRedraw` flag. `draw()` is only called when this flag is true.

### 2. The Screen Manager
- **Files:** `ScreenManager.h`, `ScreenManager.cpp`
- **Description:** Singleton controller managing the navigation stack.
- **Stack:** Static array `Screen* _stack[5]` (No dynamic allocation).
- **Navigation:**
    - `push(Screen*)`: Adds a screen to the stack.
    - `pop()`: Removes the top screen, returning to the previous one.
    - `popToRoot()`: Instantly returns to the bottom-most screen (Dashboard).
- **Render Cascade:**
    - `tick(U8G2& display)` is called in the main loop.
    - It calls `currentScreen->tick()` for logic.
    - It checks `currentScreen->needsRedraw()`.
    - If dirty, it clears the buffer, calls `draw()`, and sends the buffer to the OLED.

## Architecture Notes
- **Zero Allocation:** All screens must be instantiated as global/static objects. The `ScreenManager` only stores pointers.
- **Input Routing:** Inputs flow from `InputManager` -> `ScreenManager` -> `Active Screen`.
- **Integration:** The `ScreenManager` is ready to be integrated into `PultV1_0_2.ino`. The legacy switch-case logic remains active until individual screens are ported to the new interface.

## Next Steps
- **Phase 4:** Develop reusable Widgets (`ScrollingListWidget`, `NumericSpinnerWidget`).
- **Phase 5:** Port the Dashboard and Menu screens to the new `Screen` classes.
