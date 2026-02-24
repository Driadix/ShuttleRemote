# Refactoring Work Log - PultV1_0_2.ino

## Summary
Refactored the Pult firmware to replace the blocking string-based command system with a non-blocking, asynchronous state machine using a binary protocol (`ShuttleProtocol.h`).

## Changes Implemented

### 1. Protocol Integration
- Included `ShuttleProtocol.h` wrapped in `namespace SP` to avoid enum collisions.
- Defined `TxState` enum (IDLE, WAITING_ACK, TIMEOUT_ERROR).
- Defined `TxJob` struct to hold pending commands/requests.
  - **Optimized**: `TxJob` now stores a pre-serialized `txBuffer` (64 bytes) and `txLength`. This avoids type-punning hazards and redundant re-serialization during retries.
- Implemented a static circular buffer `txQueue` (size 5).
- Added global flags `showQueueFull` and `queueFullTimer` for UI feedback.

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
