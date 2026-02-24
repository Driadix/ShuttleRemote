# Refactoring Work Log

## Step 1: Implement Fixed Framerate Timer & Dirty Flag
- **Objective:** Transform UI into deterministic, non-blocking view (10 FPS max).
- **Actions Taken:**
    - Added `uint32_t lastDisplayUpdate = 0;` and `bool isDisplayDirty = true;` to globals.
    - Modified `loop()` to wrap UI rendering logic in a 100ms timer block (`millis() - lastDisplayUpdate >= 100`).
    - Guarded rendering with `if (isDisplayDirty || showQueueFull)`.
    - Updated `keypadEvent()` to set `isDisplayDirty = true` on user interaction.
    - Deprecated `displayUpdate` flag.

## Step 2: Implement Change Detection via memcmp
- **Objective:** Only redraw if data changes.
- **Actions Taken:**
    - Modified `handleRx()` to use `memcmp` for `MSG_HEARTBEAT` and `MSG_SENSORS`.
    - `cachedTelemetry` and `cachedSensors` are only updated if the incoming payload differs from the cache.
    - Sets `isDisplayDirty = true` only upon change.
    - Removed manual assignments to legacy global variables inside `handleRx`.

## Step 3: Purge Legacy Globals & Refactor UI
- **Objective:** Eliminate outdated variables and use binary protocol structures.
- **Actions Taken:**
    - Removed/Commented out legacy globals: `shuttleStatus`, `shuttleBattery`, `errorcode`, `warncode`, `quant`, `sensor_channel_f/r`, `sensor_pallete_F/R`, `DATCHIK_...`.
    - Replaced `quant` with `inputQuant` for user input logic (e.g., Unload Pallet page).
    - Refactored UI rendering in `loop()` to access `cachedTelemetry` (e.g., `cachedTelemetry.shuttleStatus`, `cachedTelemetry.batteryCharge`) and `cachedSensors`.
    - Updated logic in `keypadEvent()`:
        - Replaced `fifolifo_mode` toggle with direct `queueConfigSet` based on `cachedTelemetry.stateFlags`.
        - Replaced `quant` usage with `inputQuant` and `cachedTelemetry.palleteCount` where appropriate.
    - Updated `cmdSend()`:
        - `CMD_UNLOAD_PALLET_BY_NUMBER` now uses `inputQuant`.
        - `CMD_FIFO_LIFO` logic moved to `keypadEvent` (case removed/commented in `cmdSend` to avoid global dependency).
    - Removed legacy "driving" logic for unloading (`if (quant ...)` block in `loop`) as the Shuttle now handles `CMD_LONG_UNLOAD_QTY` natively.

## Missing / To Do
- **Compilation Verification:** As the environment lacks `arduino-cli`, changes are verified via code reading. Real compilation test is required.
- **Legacy Logic:** Some legacy logic (e.g. `shuttleStatus == 10` manual counting in `case 'C'`) was commented out or replaced with tentative logic. Needs verification on hardware.
- **Warning Codes:** Legacy `warncode` was removed. Error display now relies on `cachedTelemetry.errorCode`. Verify if specific warnings from `WarnMsgArray` need to be mapped to `errorCode` or `MSG_LOG`.
