ESP32 Radio Controller (Pult) Refactoring Architecture

Target: ESP32 Firmware Upgrade to Industrial-Grade Binary Protocol
Objective: Transition from a synchronous, string-based polling loop to an asynchronous, deterministic, non-blocking state machine utilizing a shared binary protocol structure.

1. Remote Controller Feature & UI Architecture Analysis

The Pult is heavily menu-driven (managed via a state machine page enum). Below is the breakdown of its features, operational intent, and how it translates to the new protocol.

Core Pages & Control Flow

MAIN (Page 1):

Features: Displays Shuttle Status, Battery, LIFO/FIFO mode, Warn/Errors, Pallet Count, and handles immediate hotkeys.

Hotkeys Used: 8 (Move Right), 0 (Move Left), 9 (Lifter Down), E (Lifter Up), 6 (Stop/Manual Mode), D (Demo), 5 (Load), C (Unload).

Binary Refactor (Adaptive Polling): This page will no longer block to poll. It will strictly display data dynamically. The remote controller will poll with MSG_REQ_HEARTBEAT with a fast interval (e.g., 400ms). The shuttle will respond with the TelemetryPacket (we will add any missing payload fields if required). When navigating to any other menu, we will throttle our polling frequency (e.g., 1.0s or 1.5s) to preserve bandwidth while maintaining the link state.

MENU (Page 2) & PACKING_CONTROL (Page 13):

Features: Triggers high-level logistics routines.

Commands: Pallet Count (CMD_COUNT_PALLETS), Compacting Forward/Backward (CMD_COMPACT_F/R), LIFO/FIFO Toggle, Return to Origin (CMD_HOME).

UNLOAD_PALLETE (Pages 5, 6, 7):

Features: Allows continuous unloading of 'N' pallets.

Commands: Sends CMD_LONG_UNLOAD_QTY mapping to arg1 as the requested quantity.

OPTIONS (Page 8) & ENGINEERING MENU (Page 18):

Features: Deep configuration of shuttle parameters.

Commands: Max Speed, MPR (Inter-pallet distance), Battery Protection, Shuttle Length, Wait Time, MPR Offset, Channel Offset.

Binary Refactor: These will perfectly map to MSG_CONFIG_SET / MSG_CONFIG_GET using the ConfigParamID enum (e.g., CFG_INTER_PALLET). The Pult will cache a local ConfigPacket representation.

DEBUG_INFO (Page 19):

Features: Displays live sensor data (TOF channels, Pallet sensors, Encoder Angle).

Binary Refactor: In the new architecture, the Pult will actively request MSG_REQ_SENSORS only when the user is actively viewing this specific page, rendering data extracted safely from the incoming SensorPacket.

MOVEMENT (Pages 33, 34, 35):

Features: Move fixed distances (10cm to 500cm).

Binary Refactor: Will use CMD_MOVE_DIST_R and CMD_MOVE_DIST_F with arg1 safely packing the distance in mm.

2. Dead Code & Deprecation Plan (The "Not Needed" List)

During the deep-dive analysis, significant disconnects were found between what the ESP32 attempts to send and what the STM32 actually parses. To optimize memory and execution time, the following will be purged:

Orphaned Pult Commands (Sent by ESP32, Ignored by STM32):

"dTest2", "dTestR", "dTestG", "dPallQ", "dSttic", "dLevel", "dminLv", "dEngee" (Legacy Commands 32-38, 42).

Case 46 (Failed Binary Implementation): The ESP32 tries to send a raw byte array (DataSend), but the STM32 get_Cmd() function only parses ASCII strings via substring().

"dLg" (Status PGG toggle): ESP32 sends "dLg1", but the STM32 looks for "dGetLg".

Orphaned Shuttle Parsers (Parsed by STM32, Never sent by ESP32):

"dEvOn_", "dEvOff": Evacuation mode logic exists in the shuttle but is disconnected from the Pult UI.

"dWaitT": The Shuttle looks for this, but the Pult sends "dWtXXX" instead.

"dMprOf": Shuttle looks for this, but Pult sends "dMoXXX".

Broken Stats (Page 16):

The Pult defines uint32_t statistic[10], but the logic to populate it is currently broken. The new SecureStats (SRAM) implementation and MSG_STATS packet on the STM32 will cleanly replace this.

3. Deep Protocol & Architecture Strategy

A. Data Population & Cache Strategy (The Presentation Decoupling)

To prevent screen flickering and eliminate redundant OLED SPI traffic, we will implement a "Dirty Flag" caching system. The Presentation Layer (UI) will be entirely decoupled from the Transport Layer (UART).

The Local Cache: We will maintain volatile structs in the ESP32 memory: TelemetryPacket cachedTelemetry;, SensorPacket cachedSensors;.

Change Detection (memcmp): When the ProtocolParser successfully validates a new frame, it will compare the new payload against the cache using a rapid memcmp().

The Dirty Flag: If differences are found, the new struct overwrites the cache, and a global boolean bool isDisplayDirty = true; is set.

Keypad Events: Any valid keypad press will also trigger isDisplayDirty = true; to ensure instant UI responsiveness (e.g., cursor moving).

B. Decoupled, Asynchronous State Machine (Command & ACK Handling)

Industrial systems do not block waiting for responses. We will implement a tracked transmission queue.

Command Structuring: When an action is required (e.g., "Unload"), the UI calls an asynchronous helper: queueCommand(CMD_UNLOAD, 0).

State Management: The comms manager maintains an internal state: IDLE, WAITING_ACK, ERROR_TIMEOUT.

Transmission & Tracking: queueCommand packs the data, calculates the CRC, assigns a unique seq number, records lastTxTime = millis(), and transitions to WAITING_ACK.

Retry Logic (Robustness): If WAITING_ACK persists for > 500ms without receiving the matching MSG_ACK, the ESP32 will auto-retry the packet up to 3 times before setting an ERROR_TIMEOUT state (which flags the UI to show a "Comms Error").

Success: Upon receiving MSG_ACK with result == 0 matching the sent sequence, the queue is cleared, and the state returns to IDLE.

C. Manual Control "Dead-Man's Switch" via Heartbeat

The current CMD_MOVE_RIGHT_MAN relies on an unstable "ngPing" string. This will be replaced with a highly robust RTOS-safe mechanism:

Pult Side: Upon holding the '8' (Right) key, the Pult sends a one-time CMD_MOVE_RIGHT_MAN. As long as the key registers as HOLD or rapidly PRESSED, the Pult guarantees a MSG_REQ_HEARTBEAT is fired every 200ms. When released, it fires CMD_STOP_MANUAL.

Shuttle Side (Safety Fallback): The STM32 begins movement. A hardware or software timer resets every time any valid protocol frame with a matching CRC is received on the UART. If this timer exceeds 500ms, the Shuttle assumes total loss of radio link and executes a forced hardware motor_Stop().

D. Display Update Strategy (10Hz Fixed Framerate)

The OLED update routine will be removed from the immediate execution flow of button presses or serial reads.

In the main loop(), a non-blocking timer runs at exactly 100ms intervals (10 FPS).

It evaluates: if (isDisplayDirty || forceRedraw).

If true, it renders the current active page using the latest cachedTelemetry / cachedSensors variables and sets isDisplayDirty = false.

This guarantees the UI is completely fluid, immune to UART blocking, and prevents I2C/SPI bus bottlenecking from spamming the u8g2 buffer.

4. Crucial Pre-Refactor Considerations ("The Missing Things")

Before writing code, we must architect around the following physical constraints of the ESP32 and the Pult hardware:

E32 LoRa Module Configuration Separation:

The current code sends raw bytes (0xC0...) to configure the E32 radio module on boot. This hardware configuration phase must be explicitly separated from the ShuttleProtocol phase. We will build an initRadio() routine that ensures the module is fully initialized before the ProtocolParser is allowed to process bytes.

NVS (Preferences) vs. EEPROM:

The ESP32 relies on a deprecated EEPROM.h wrapper which simulates EEPROM in flash memory. This is highly susceptible to wear and corruption on ESP32. We will migrate to the native Preferences.h (Non-Volatile Storage / NVS), which provides robust, key-value mapped wear-leveling.

OTA (Over-The-Air) Update Safety:

The Pult utilizes AsyncElegantOTA. During a firmware upload, heavy UART traffic and OLED screen updates can cause Watchdog timeouts (WDT) and core panics. We must implement a state toggle bool isOtaUpdating. If true, the system suspends all UART transmissions, halts the OLED refresh, and focuses 100% on the web server task.


Sleep Mode State Management:

The Pult enters deep sleep after displayOffInterval. The async command queue must be verified as IDLE before allowing sleep. If a command is actively WAITING_ACK, the sleep timer must be temporarily deferred to prevent going to sleep exactly as the user issues a critical command like CMD_STOP.