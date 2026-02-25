#include "DashboardScreen.h"
#include "UI_Graph.h"
#include "ScreenManager.h"
#include "EventBus.h"
#include "Lang_RU.h"

DashboardScreen::DashboardScreen()
    : _isManualMoving(false), _manualCommand(" "), _queueFullTimer(0), _showQueueFull(false), _animX(0), _animFlagX(0), _lastAnimTick(0) {
}

void DashboardScreen::onEnter() {
    Screen::onEnter();
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
    _statusBar.draw(display, 0, 0);

    const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1); // White text

    // 2. Main Status
    display.setCursor(0, 25);
    if (cachedTelemetry.shuttleStatus == 13)
        display.print("Осталось выгрузить " + String(cachedTelemetry.palleteCount));
    else if (cachedTelemetry.shuttleStatus < 19)
        display.print(SHUTTLE_STATUS_STRINGS[cachedTelemetry.shuttleStatus]);
    else
        display.print("Status: " + String(cachedTelemetry.shuttleStatus));

    // 3. Warnings / Queue Full / Errors
    display.setCursor(0, 40);
    if (_showQueueFull) {
        display.print("! QUEUE FULL !");
    } else if (cachedTelemetry.errorCode) {
        if (cachedTelemetry.errorCode < 16)
             display.print(String("! ") + ERROR_STRINGS[cachedTelemetry.errorCode] + " !");
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
    if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6)
    {
        display.drawBox(_animX, 40, 28, 5);
        display.drawBox(_animX + 4, 45, 3, 2);
        display.drawBox(_animX + 21, 45, 3, 2);

        if (_animFlagX == 1) {
            display.drawBox(_animX, 34, 28, 3);
            display.drawBox(_animX, 37, 3, 3);
            display.drawBox(_animX + 13, 37, 3, 3);
            display.drawBox(_animX + 25, 37, 3, 3);
        }
    }
    else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12)
    {
        display.drawBox(_animX, 40, 28, 5);
        display.drawBox(_animX + 4, 45, 3, 2);
        display.drawBox(_animX + 21, 45, 3, 2);

        if (_animFlagX == 0) {
            display.drawBox(_animX, 34, 28, 3);
            display.drawBox(_animX, 37, 3, 3);
            display.drawBox(_animX + 13, 37, 3, 3);
            display.drawBox(_animX + 25, 37, 3, 3);
        }
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

    // Animation Tick (e.g., 50ms for smooth 20fps)
    if (millis() - _lastAnimTick > 50) {
        _lastAnimTick = millis();
        bool animChanged = false;

        const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();
        if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6) {
            // Forward animation logic
            if (_animFlagX == 0 && _animX < 128) {
                _animX++;
            } else {
                _animFlagX = 1;
                _animX--;
            }
            if (_animFlagX == 1 && _animX == 0) {
                _animFlagX = 0;
            }
            animChanged = true;
        } else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12) {
            // Reverse animation logic
            if (_animFlagX == 1 && _animX > 1) {
                _animX--;
            } else {
                _animFlagX = 0;
                _animX++;
            }
            if (_animFlagX == 0 && _animX == 127) {
                _animFlagX = 1;
            }
            animChanged = true;
        }

        if (animChanged) setDirty();
    }
}
