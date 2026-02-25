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

## Verification
- **Input:** Pressing keys should now output logical event IDs to the Serial monitor.
- **Power:** The system should automatically enter deep sleep after 25 seconds of inactivity. Pressing any key should wake it (via RTC GPIO wakeup) or reset the idle timer if awake.

## Next Steps
- Re-implement the UI navigation logic using the new `InputEvent` system (Phase 2).
- Map `InputEvent`s to specific application commands and page transitions.
