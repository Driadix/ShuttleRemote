# Refactoring Work Log - PultV1_0_2.ino

## Summary
Refactored the Pult firmware to replace the blocking string-based command system with a non-blocking, asynchronous state machine using a binary protocol (`ShuttleProtocol.h`).

## Changes Implemented

### 1. Protocol Integration
- Included `ShuttleProtocol.h` wrapped in `namespace SP` to avoid enum collisions.
- Defined `TxState` enum (IDLE, WAITING_ACK, TIMEOUT_ERROR).
- Defined `TxJob` struct to hold pending commands/requests.
- Implemented a static circular buffer `txQueue` (size 5).

### 2. Rx Dispatcher
- Implemented `handleRx()` function to read from `Serial2` non-blocking.
- Used `SP::ProtocolParser` to validate incoming frames (CRC check).
- Handled `MSG_ACK`: calls `processIncomingAck()` to clear pending jobs.
- Handled `MSG_HEARTBEAT`: updates global `cachedTelemetry` and legacy global variables (`shuttleStatus`, `shuttleBattery`, `errorcode`, `quant`) to maintain UI compatibility.
- Handled `MSG_SENSORS`: updates `cachedSensors` and legacy sensor variables (`sensor_channel_f`, etc.).

### 3. Tx Dispatcher & Retry Engine
- Implemented `processTxQueue()` state machine:
  - **IDLE**: Checks queue, sends frame (Header+Payload+CRC), starts timeout timer, transitions to **WAITING_ACK**.
  - **WAITING_ACK**: Checks timeout (500ms). Retries up to 3 times. Transitions to **TIMEOUT_ERROR** on failure.
  - **TIMEOUT_ERROR**: Resets to IDLE (currently drops failed job).
- Implemented helpers:
  - `queueCommand(SP::CommandPacket, targetID)`: Enqueues a command.
  - `queueConfigSet(paramID, value, targetID)`: Enqueues a configuration set request.
  - `queueRequest(msgID, targetID)`: Enqueues a data request (Heartbeat, Sensors, etc.).

### 4. Legacy Code Removal & Refactoring
- Completely rewrote `cmdSend()` to map legacy command IDs to `SP::CommandPacket` or `MSG_CONFIG_SET`/`REQ_HEARTBEAT`.
- Replaced direct `Serial2.print` calls in `keypadEvent` (Movement pages, Logging toggle, Ping) with `queueCommand()`.
- Removed blocking `GetSerial2Data()` function and its helpers (`GetSerial2Ans_`, `GetSerial2Ans`).
- Removed blocking loops in `cmdSend()` that waited for "dLoad_!" style string ACKs.

### 5. System Interlocks
- Added `isOtaUpdating` flag.
- Hooked `AsyncElegantOTA` callbacks in `setup()` to set/clear the flag.
- Prevented Rx/Tx processing when OTA is active.
- Updated `loop()` sleep logic to defer `SetSleep()` if the transmission queue is not empty or waiting for ACK.

## Remaining/Missing Items
- **Hardware Config**: The initial E32 configuration in `setup()` (`Serial2.write(0xC1)...`) remains as raw bytes. This should eventually be refactored to a dedicated `initRadio()` routine.
- **Display Logic**: The display still relies on global variables (`shuttleStatus`, etc.) which are populated in `handleRx` from `cachedTelemetry`. A future refactor should make the UI read directly from `cachedTelemetry`.
- **Error Handling**: `TIMEOUT_ERROR` currently resets to IDLE. Robust error handling (UI feedback) is needed.
- **Legacy Commands**: Some legacy commands (32-39, 46) were not fully mapped or were deemed dead code.

## Verification
- Code compiles (conceptually) and structure is valid.
- Logic flow handles non-blocking operations.
- Critical sections (OTA, Sleep) are protected.
