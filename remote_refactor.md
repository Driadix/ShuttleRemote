Technical Specification: ESP32 Remote Controller Architecture Refactoring

Executive Summary
This document outlines the architectural refactoring for an ESP32-based remote controller display system. The current codebase relies on a monolithic, procedurally driven enum-based state machine (enum Page with 36+ states) tightly coupled with hardware polling and UI rendering. As the application scales to support complex telemetry, OTA updates, and dynamic configurations, this approach has become unmaintainable.

The Goal: Transition to an industrial-grade, decoupled, Object-Oriented (OO) architecture utilizing a "Screen Stack" pattern for the UI, an Event-Driven DataManager for the backend, and strict separation of concerns. The new architecture prioritizes RAM safety (static allocation), CPU efficiency (dirty-flag rendering), and infinite scalability.

Core Architectural Pillars & Constraints
The system will be strictly divided into three distinct layers. Code from one layer must not bypass boundaries to communicate with another.

Hardware Abstraction Layer (HAL): Manages raw peripheral interactions (U8g2 Display, Matrix Keypad, Serial2 UART, Deep Sleep/RTC pins).

Data & Communication Layer (Model): Manages the ShuttleProtocol, TX/RX queues, and maintains the single source of truth for telemetry, sensor data, and configuration.

View Management Layer (UI): Renders visual elements based the Model and translates user inputs into Data Layer commands.

Primary Constraints for Developers:

No Dynamic Allocation: To prevent heap fragmentation over long uptimes, dynamic memory allocation (new/malloc) is strictly prohibited for UI screens and widgets. All screens must be instantiated globally (e.g., static DashboardScreen dashboard;) and manipulated via pointers.

No Blocking UI Calls: Software SPI pushes to the 128x64 OLED take ~20-40ms. Unconditional continuous redrawing will starve the CPU and cause UART queue overflows. Rendering must be strictly dirty-flag driven.

View Management Layer (UI Framework)
3.1. The Screen Interface

Every view in the application will inherit from a pure virtual base class Screen.

virtual void onEnter(): Fired when pushed to the active stack. (Used for resetting cursors or requesting config data).

virtual void onExit(): Fired when popped.

virtual void draw(U8G2& display): Responsible for rendering. Only called if the screen is flagged as dirty.

virtual void handleInput(InputEvent event): Processes logical inputs (e.g., BTN_UP, BTN_OK).

virtual void tick(): Called at a fixed frequency (e.g., 20Hz) for evaluating logic, processing animation frames, and checking DataManager flags.

3.2. ScreenManager & Navigation History

A centralized controller maintaining an array-based stack (Screen* stack[5]) and a topIndex.

Pushing (push(Screen* s)): Adds a screen to the stack. Used for drilling down into menus (e.g., Main -> Options -> Engineering).

Popping (pop()): Removes the active screen, instantly returning to the previous screen. Because screens are statically allocated, the previous screen retains its exact state (e.g., scrolling cursor position).

Return to Root (popToRoot()): Instantly sets topIndex = 0 (Dashboard). Used as a global "Home" escape sequence from any deep menu.

Input Routing: Routes incoming key presses from the InputManager strictly to stack[topIndex]->handleInput().

3.3. The Dirty-Flag Render Cascade (Performance)

To avoid Software SPI bottlenecks, screen updates follow a strict cascade:

The Source: DataManager updates a payload (e.g., telemetry) and sets an internal flag (telemetryDirty = true).

The Observer: The ScreenManager calls stack[topIndex]->tick() every 50ms. The active screen checks DataManager::isTelemetryDirty().

The Interception: If true, the screen updates its local variables, sets its own _needsRedraw = true, and tells the DataManager to clear its flag. (If the active screen doesn't care about telemetry, it ignores the flag, preventing unnecessary redraws).

The Renderer: The ScreenManager's main loop checks if stack[topIndex]->_needsRedraw is true. If so, it clears the buffer, calls draw(), pushes to the OLED, and resets the flag.

3.4. Advanced Navigation: The Target Pointer Pattern (PIN Security)

Instead of hardcoding PIN logic into various menus or tracking state via generic enums (pageAfterPin), the system uses a reusable PinEntryScreen utilizing a "Target Pointer".

Implementation: The PinEntryScreen contains a Screen* targetScreen member.

Usage: Before pushing the PIN screen, the originating menu configures it: pinScreen.setTarget(&engineeringMenu); screenManager.push(&pinScreen);.

Execution: The user enters the code. If correct, the PinEntryScreen calls screenManager.pop(); screenManager.push(targetScreen);. If incorrect, it clears the entry and shows a brief "Access Denied" modal.

Benefit: Any screen in the system can be secured dynamically with two lines of code without duplicating security logic.

3.5. Reusable UI Widgets (Composition)

Screens will compose UI using static, reusable widgets rather than raw draw coordinates:

ScrollingListWidget: Accepts an array of strings. Manages cursorPos, pagination (handling lists larger than the screen), and highlighting.

NumericSpinnerWidget: A multi-digit data entry widget (PIN codes, shuttle IDs, pallet quantities). Handles digit wrapping and cursor shifting.

StatusBarWidget: Persistent banner displaying Battery %, FIFO/LIFO, Shuttle ID. Reads directly from the DataManager.

ModalDialogWidget: A pop-up overlay for confirmations or temporary error messages (auto-dismissible via timers).

Data & Communication Layer: DataManager
The DataManager encapsulates UART data, entirely replacing global structs.

4.1. Single Source of Truth & Safe Access

Maintains private instances of TelemetryPacket, SensorPacket, StatsPacket, and configurations.

Exposes const getter methods to prevent UI from accidentally modifying state.

4.2. Command Gateway & The TX Queue

All UI actions route through here: DataManager::getInstance().sendCommand(CMD_LIFT_UP).

Queue Saturation Handling: If the TX queue is full, the command is immediately rejected. The DataManager flags SystemAlert::QUEUE_FULL. The ScreenManager observes this flag and overlays a "QUEUE FULL" warning, bypassing the need for individual screens to handle failure logic.

System Services
5.1. InputManager

Decouples raw Keypad matrix polling from UI logic.

Maps physical keys to logical actions (e.g., 'A' -> BTN_UP).

Handles debouncing and distinct BTN_ACTION_LONG events cleanly.

5.2. PowerController

Monitors an InputManager::getIdleTime() counter.

If idle time exceeds the threshold, the TX Queue is empty, and no active manual commands are running, it orchestrates the rtc_gpio holds and initiates esp_deep_sleep_start().

Known Problems & Mitigation Strategies
Asynchronous Data Loading ("Flash of Old Data"): * Solution: Implement DataBinding. EngineeringMenuScreen::onEnter() registers a request for configs. The ScreenManager forces a "Loading..." modal. Only when the DataManager signals the requested data has arrived does the modal close and the menu render.

Keypad Interruptions during OTA: * Solution: During OTA, a global isOTAActive flag is set. The ScreenManager forces an OtaProgressScreen, overrides the PowerController (preventing sleep), and drops all inputs from the InputManager.

Implementation Roadmap (Atomic Steps for Developer)
Refactoring must be executed in atomic phases to maintain a bootable system at all times:

Phase 1: Foundation (HAL). Implement InputManager and PowerController. Verify deep sleep and debouncing work without display logic.

Phase 2: Data Abstraction (Model). Implement DataManager. Port ShuttleProtocol parsing inside. Validate TX/RX queues and dirty flags via serial logging.

Phase 3: UI Framework Core. Implement Screen base class, ScreenManager, and the render cascade loop.

Phase 4: Widget Library. Develop and unit-test ScrollingListWidget and NumericSpinnerWidget using dummy data.

Phase 5: Screen Migration. Iteratively port enum Page archetypes into static Screen classes. Start with DashboardScreen, then PinEntryScreen, then the Menu screens.

Phase 6: Final Cleanup. Delete enum Page, remove inline sleep logic, and clean the primary loop().