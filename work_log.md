# Refactoring Work Log - Task 1 Asynchronous state machine

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

### 2. Rx Dispatcher
- Implemented `handleRx()` function to read from `Serial2` non-blocking.
- Used `SP::ProtocolParser` to validate incoming frames (CRC check).
- Handled `MSG_ACK`: calls `processIncomingAck()` to clear pending jobs.
- Handled `MSG_HEARTBEAT`: updates global `cachedTelemetry` and legacy global variables (`shuttleStatus`, `shuttleBattery`, `errorcode`, `quant`) to maintain UI compatibility.
- Handled `MSG_SENSORS`: updates `cachedSensors` and legacy sensor variables (`sensor_channel_f`, etc.).

### 3. Tx Dispatcher & Retry Engine
- Implemented `processTxQueue()` state machine:
  - **IDLE**: Checks queue, sends the pre-serialized `txBuffer` directly, starts timeout timer, transitions to **WAITING_ACK**.
  - **WAITING_ACK**: Checks timeout (500ms). Retries up to 3 times using the cached `txBuffer`. Transitions to **TIMEOUT_ERROR** on failure.
  - **TIMEOUT_ERROR**: Resets to IDLE (currently drops failed job).
- Implemented helpers with immediate serialization:
  - `queueCommand`, `queueConfigSet`, `queueRequest`:
    - Return `bool` (true = success, false = full).
    - Serialize the frame (Header + Payload + CRC) immediately into `TxJob::txBuffer`.
    - Handle `seqNum` assignment at queue time.

### 4. Legacy Code Removal & Refactoring
- Completely rewrote `cmdSend()` to map legacy command IDs to queue functions.
- Replaced direct `Serial2.print` calls in `keypadEvent`.
- Removed blocking `GetSerial2Data()` function and its helpers.
- Updated call sites (`cmdSend`, `keypadEvent`) to check queue return values and set `showQueueFull` on failure.

### 5. System Interlocks & UI
- Added `isOtaUpdating` flag and `AsyncElegantOTA` hooks.
- Updated `loop()` sleep logic to defer `SetSleep()` if the transmission queue is busy.
- Updated display logic in `loop()` to show "! QUEUE FULL !" if the queue overflows.

## Remaining/Missing Items
- **Hardware Config**: The initial E32 configuration in `setup()` remains as raw bytes.
- **Display Logic**: The display still relies on global variables populated from `cachedTelemetry`.
- **Error Handling**: `TIMEOUT_ERROR` currently resets to IDLE without explicit UI error (only Queue Full has explicit UI).

## Verification
- Code compiles (conceptually) and structure is valid.
- Logic flow handles non-blocking operations.
- Critical sections (OTA, Sleep) are protected.
- Memory safety improved by removing unsafe payload union casting.


# Refactoring Work Log - Task 2 Transform UI

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
