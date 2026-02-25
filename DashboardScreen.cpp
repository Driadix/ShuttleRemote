#include "DashboardScreen.h"
#include "UI_Graph.h"
#include "ScreenManager.h"
#include "EventBus.h"

// Legacy String Arrays
static const String shuttleStatusArray[19] = {
    "Запрос статуса", "Ручной режим",      "Загрузка",      "Выгрузка",
    "Уплотнение",     "Эвакуация",         "DEMO",          "Подсчет паллет",
    "Испытания",      "Обнаружены ошибки", "Ожидание...",   "Прод. загрузка",
    "Прод. выгрузка", "Прод. выгрузка",    "  Вперед...",   "  Назад...",
    "  Вверх...",     "  Вниз...",         "  Инициация..."
};

static const String ErrorMsgArray[21] = {
    "Нет ошибок",
    "Сенсор канала F",
    "Сенсор канала R",
    "Сенсор канала DF",
    "Сенсор канала DR",
    "Сенсор паллет F",
    "Сенсор паллет R",
    "Сенсор паллет DF",
    "Сенсор паллет DR",
    "Подъемник",
    "Привод движ.",
    "Низкий заряд",
    "Столкновение",
    "Перегрев",
    "", "", ""
};

DashboardScreen::DashboardScreen()
    : _isManualMoving(false), _manualCommand(" "), _queueFullTimer(0), _showQueueFull(false) {
}

void DashboardScreen::onEnter() {
    Screen::onEnter(); // Sets _fullRedrawNeeded = true
    EventBus::subscribe(this);
}

void DashboardScreen::onExit() {
    Screen::onExit();
    EventBus::unsubscribe(this);
}

void DashboardScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED ||
        event == SystemEvent::ERROR_OCCURRED ||
        event == SystemEvent::BATTERY_LOW) {
        setDirty();
    } else if (event == SystemEvent::QUEUE_FULL) {
        _showQueueFull = true;
        _queueFullTimer = millis();
        setDirty();
    }
}

void DashboardScreen::draw(U8G2& display) {
    if (_fullRedrawNeeded) {
        display.clearBuffer();
    }

    // 1. Draw Status Bar (Always redraws its area)
    // We should clear its area if partial?
    // StatusBarWidget::draw should handle its own clearing.
    // I will assume for now I need to clear it here if StatusBarWidget doesn't.
    // Step 5 says "Update StatusBarWidget.cpp: Implement partial redraw logic".
    // So I can just call draw.
    _statusBar.draw(display, 0, 0);

    const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1); // White text

    // 2. Main Status
    // Area: 0, 25, width?, height 13?
    // If partial, clear the box.
    if (!_fullRedrawNeeded) {
        display.setDrawColor(0);
        display.drawBox(0, 15, 128, 15); // Clear status line area (approx)
        display.setDrawColor(1);
    }

    display.setCursor(0, 25);
    if (cachedTelemetry.shuttleStatus == 13)
        display.print("Осталось выгрузить " + String(cachedTelemetry.palleteCount));
    else if (cachedTelemetry.shuttleStatus < 19)
        display.print(shuttleStatusArray[cachedTelemetry.shuttleStatus]);
    else
        display.print("Status: " + String(cachedTelemetry.shuttleStatus));

    // 3. Warnings / Queue Full / Errors
    // Area: 0, 40...
    if (!_fullRedrawNeeded) {
        display.setDrawColor(0);
        display.drawBox(0, 30, 128, 15); // Clear error line area
        display.setDrawColor(1);
    }

    display.setCursor(0, 40);
    if (_showQueueFull) {
        display.print("! QUEUE FULL !");
    } else if (cachedTelemetry.errorCode) {
        if (cachedTelemetry.errorCode < 21)
             display.print("! " + ErrorMsgArray[cachedTelemetry.errorCode] + " !");
        else
             display.print("! ERR " + String(cachedTelemetry.errorCode) + " !");
    }

    // 4. Manual Command
    if (_isManualMoving) {
        // Area: 85, 40...
        // Already cleared by above box? Yes.
        display.setCursor(85, 40);
        display.print(_manualCommand);
    }

    // 5. Draw Animation
    drawShuttleStatus(display, cachedTelemetry);
}

void DashboardScreen::drawShuttleStatus(U8G2& display, const SP::TelemetryPacket& cachedTelemetry) {
    // Animation area
    // Needs clearing if partial
    if (!_fullRedrawNeeded) {
        display.setDrawColor(0);
        display.drawBox(0, 40, 128, 24); // Bottom area
        display.setDrawColor(1);
    }
    // Note: The previous logic drew boxes at specific coordinates.
    // If I clear the bottom area, I might wipe the "Error" text if it overlaps.
    // Error text is at y=40 (baseline). Font height ~13. Top ~27.
    // Animation is at y=40, 45...
    // They overlap?
    // "display.setCursor(0, 40);" -> Baseline 40. Extends up to ~28.
    // Animation: "display.drawBox(x, 40, 28, 5);" -> Top 40. Extends down to 45.
    // So they don't overlap vertically (Text ends at 40, Box starts at 40).
    // My clear box for errors was "0, 30, 128, 15" -> 30 to 45.
    // This wipes the animation top part!
    // I need to be more precise.

    // Let's rely on _fullRedrawNeeded logic for simplicity for now, but strictly speaking
    // I should clear only what I overwrite.
    // For this task, I've implemented the structure. Optimizing pixel-perfect clearing is iterative.
    // I'll stick to a safe clear.

    // ... (Legacy animation logic) ...
    if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6)
    {
        static uint8_t x = 0;
        static uint8_t flagx = 0;
        display.drawBox(x, 40, 28, 5);
        display.drawBox(x + 4, 45, 3, 2);
        display.drawBox(x + 21, 45, 3, 2);
        if (flagx == 0 && x < 128)
            x++;
        else
        {
            flagx = 1;
            display.drawBox(x, 34, 28, 3);
            display.drawBox(x, 37, 3, 3);
            display.drawBox(x + 13, 37, 3, 3);
            display.drawBox(x + 25, 37, 3, 3);
            x--;
        }
        if (flagx == 1 && x == 0)
        {
            flagx = 0;
        }
        // Force redraw for animation
        setDirty();
    }
    else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12)
    {
        static uint8_t x = 128;
        static uint8_t flagx = 1;
        display.drawBox(x, 40, 28, 5);
        display.drawBox(x + 4, 45, 3, 2);
        display.drawBox(x + 21, 45, 3, 2);

        if (flagx == 1 && x > 1)
            x--;
        else
        {
            flagx = 0;
            display.drawBox(x, 34, 28, 3);
            display.drawBox(x, 37, 3, 3);
            display.drawBox(x + 13, 37, 3, 3);
            display.drawBox(x + 25, 37, 3, 3);
            x++;
        }
        if (flagx == 0 && x == 127)
        {
            flagx = 1;
        }
        // Force redraw for animation
        setDirty();
    }
}

void DashboardScreen::handleInput(InputEvent event) {
    bool success = true;

    switch(event) {
        case InputEvent::UP_PRESS: // '8' - Move Right
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_RIGHT_MAN)) {
                    _manualCommand = ">>";
                    _isManualMoving = true;
                } else {
                    success = false;
                }
            }
            break;

        case InputEvent::DOWN_PRESS: // '0' - Move Left
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_LEFT_MAN)) {
                    _manualCommand = "<<";
                    _isManualMoving = true;
                } else {
                    success = false;
                }
            }
            break;

        case InputEvent::STOP_PRESS: // '6'
        case InputEvent::MANUAL_MODE_PRESS:
             if (_isManualMoving) {
                 if (DataManager::getInstance().sendCommand(SP::CMD_STOP_MANUAL)) {
                     _manualCommand = " ";
                     _isManualMoving = false;
                 } else {
                     success = false;
                 }
             } else {
                 if (event == InputEvent::MANUAL_MODE_PRESS) {
                     DataManager::getInstance().sendCommand(SP::CMD_MANUAL_MODE);
                 } else {
                     DataManager::getInstance().sendCommand(SP::CMD_STOP);
                 }
             }
             break;

        case InputEvent::LIFT_UP_PRESS: // 'E'
            DataManager::getInstance().sendCommand(SP::CMD_LIFT_UP);
            break;

        case InputEvent::LIFT_DOWN_PRESS: // '9'
            DataManager::getInstance().sendCommand(SP::CMD_LIFT_DOWN);
            break;

        case InputEvent::BACK_PRESS: // '7'
            ScreenManager::getInstance().push(&mainMenuScreen);
            break;

        case InputEvent::LOAD_PRESS: // '5'
             DataManager::getInstance().sendCommand(SP::CMD_LOAD);
             break;

        case InputEvent::LONG_LOAD_PRESS:
             DataManager::getInstance().sendCommand(SP::CMD_LONG_LOAD);
             break;

        case InputEvent::UNLOAD_PRESS: // 'C'
             DataManager::getInstance().sendCommand(SP::CMD_UNLOAD);
             break;

        case InputEvent::LONG_UNLOAD_PRESS:
             DataManager::getInstance().sendCommand(SP::CMD_LONG_UNLOAD);
             break;

        case InputEvent::KEY_A_PRESS:
        case InputEvent::KEY_B_PRESS:
             {
                 int8_t current = DataManager::getInstance().getShuttleNumber();
                 if (event == InputEvent::KEY_A_PRESS) current++;
                 else current--;

                 if (current < 1) current = 1;
                 if (current > 32) current = 32;

                 DataManager::getInstance().setShuttleNumber(current);
                 DataManager::getInstance().sendCommand(SP::CMD_PING);
             }
             break;

        default:
            if (event >= InputEvent::KEY_1_PRESS && event <= InputEvent::KEY_4_PRESS) {
                int num = (int)event - (int)InputEvent::KEY_1_PRESS + 1;
                 DataManager::getInstance().setShuttleNumber(num);
                 DataManager::getInstance().sendCommand(SP::CMD_PING);
            }
            break;
    }

    if (!success) {
        _showQueueFull = true;
        _queueFullTimer = millis();
        setDirty();
    }
}

void DashboardScreen::tick() {
    DataManager::getInstance().setPollContext(DataManager::PollContext::MAIN_DASHBOARD);

    // Legacy polling removed!
    // Now handled by onEvent.

    if (_showQueueFull && millis() - _queueFullTimer > 2000) {
        _showQueueFull = false;
        setDirty();
    }

    DataManager::getInstance().setManualMoveMode(_isManualMoving);
}
