#include "DashboardScreen.h"
#include "UI_Graph.h"
#include "ScreenManager.h"
#include "EventBus.h"
#include "Lang_RU.h"
#include "Logger.h"
#include "DebugUtils.h"

DashboardScreen::DashboardScreen()
    : _isManualMoving(false), _manualCommand(" "), _queueFullTimer(0), _showQueueFull(false), 
      _tempShuttleNum(1), _isSelectingShuttle(false), _blinkState(false), _shuttleSelectTimer(0), _lastBlinkTick(0),
      _animX(0), _animFlagX(0), _lastAnimTick(0), _actionMsg(""), _actionStatus(""), _actionMsgTimer(0) {
}

void DashboardScreen::onEnter() {
    Screen::onEnter();
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::ACTIVE_TELEMETRY);
    _isSelectingShuttle = false;
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
    } else if (event == SystemEvent::CMD_DISPATCHED) {
        SP::CmdType cmd = DataManager::getInstance().getLastUserCommandType();
        _actionMsg = String(DebugUtils::getUICommandName((uint8_t)cmd));
        _actionStatus = "[ > ]";
        _actionMsgTimer = millis();
        setDirty();
    } else if (event == SystemEvent::CMD_ACKED) {
        if (_actionMsg.length() > 0) {
            _actionStatus = "[ OK ]";
            _actionMsgTimer = millis();
            setDirty();
        }
    } else if (event == SystemEvent::CMD_FAILED) {
        if (_actionMsg.length() > 0) {
            _actionStatus = "[ X ]";
            _actionMsgTimer = millis();
            setDirty();
        }
    }
}

void DashboardScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1); // White text

    char buf[64];

    if (!DataManager::getInstance().isConnected()) {
        display.drawStr(0, 25, "Подключение..."); // Connecting...
        // Hide movement animations when disconnected
    } else {
        const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();

        // 2. Main Status
        if (cachedTelemetry.shuttleStatus == 13) {
            snprintf(buf, sizeof(buf), TXT_LEFT_TO_UNLOAD, cachedTelemetry.palleteCount);
            display.drawStr(0, 25, buf);
        }
        else if (cachedTelemetry.shuttleStatus < 19) {
            display.drawStr(0, 25, SHUTTLE_STATUS_STRINGS[cachedTelemetry.shuttleStatus]);
        }
        else {
            snprintf(buf, sizeof(buf), "Status: %d", cachedTelemetry.shuttleStatus);
            display.drawStr(0, 25, buf);
        }

        // 3. Warnings / Queue Full / Errors
        if (_showQueueFull) {
            display.drawStr(0, 40, TXT_QUEUE_FULL);
        } else if (cachedTelemetry.errorCode) {
            if (cachedTelemetry.errorCode < 16) {
                 snprintf(buf, sizeof(buf), "! %s !", ERROR_STRINGS[cachedTelemetry.errorCode]);
                 display.drawStr(0, 40, buf);
            }
            else {
                 snprintf(buf, sizeof(buf), TXT_ERR_PREFIX, cachedTelemetry.errorCode);
                 display.drawStr(0, 40, buf);
            }
        }

        // 4. Manual Command
        if (_isManualMoving) {
            display.drawStr(85, 40, _manualCommand.c_str());
        }

        // 5. Draw Movement Animation
        drawShuttleStatus(display, cachedTelemetry);
    }

    // ACK Status / Action Bar
    if (_actionMsg.length() > 0) {
        char actionBuf[64];
        snprintf(actionBuf, sizeof(actionBuf), "%s %s", _actionMsg.c_str(), _actionStatus.c_str());
        display.drawStr(0, 52, actionBuf);
    }
    // Fallback: If no action message but waiting for ACK (e.g. background task?)
    // The user requested to replace static space with transient bar.
    // If we are waiting for ACK but it wasn't a user command (e.g. heartbeat), we probably shouldn't show it here to keep it clean.

    // 6. BOTTOM SHUTTLE INDICATOR (Legacy moving number)
    display.setFont(u8g2_font_10x20_t_cyrillic);
    uint8_t shtNumToDraw = _isSelectingShuttle ? _tempShuttleNum : DataManager::getInstance().getShuttleNumber();
    
    // Calculate layout identical to old legacy behavior
    int xOffset = 30 * (shtNumToDraw - 1);
    if (shtNumToDraw > 4) {
        xOffset = 100; // Right aligned if number > 4
    }

    if (!_isSelectingShuttle || _blinkState) {
        snprintf(buf, sizeof(buf), "|%d|", shtNumToDraw);
        display.drawStr(xOffset, 63, buf);
    }
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
        case InputEvent::UP_PRESS: // '8'
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_RIGHT_MAN)) {
                    _manualCommand = ">>";
                    _isManualMoving = true;
                } else { success = false; }
            }
            break;

        case InputEvent::DOWN_PRESS: // '0'
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_LEFT_MAN)) {
                    _manualCommand = "<<";
                    _isManualMoving = true;
                } else { success = false; }
            }
            break;

        case InputEvent::STOP_PRESS: // '6'
        case InputEvent::MANUAL_MODE_PRESS:
             if (_isManualMoving) {
                 if (DataManager::getInstance().sendCommand(SP::CMD_STOP_MANUAL)) {
                     _manualCommand = " ";
                     _isManualMoving = false;
                 } else { success = false; }
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

        case InputEvent::BACK_PRESS: // '7'
            ScreenManager::getInstance().push(&mainMenuScreen);
            break;

        // --- SHUTTLE QUICK SELECTION LOGIC ---
        case InputEvent::OK_SHORT_PRESS:
        case InputEvent::OK_LONG_PRESS: // 'D'
            if (_isSelectingShuttle) {
                DataManager::getInstance().saveLocalShuttleNumber(_tempShuttleNum);
                _isSelectingShuttle = false;
                // No PING needed, heartbeat handles it
                setDirty();
            }
            break;

        case InputEvent::KEY_A_PRESS:
        case InputEvent::KEY_B_PRESS:
        case InputEvent::KEY_1_PRESS:
        case InputEvent::KEY_2_PRESS:
        case InputEvent::KEY_3_PRESS:
        case InputEvent::KEY_4_PRESS:
             if (!_isSelectingShuttle) {
                 _tempShuttleNum = DataManager::getInstance().getShuttleNumber();
             }
             _isSelectingShuttle = true;
             _shuttleSelectTimer = millis();
             _blinkState = true;
             
             if (event == InputEvent::KEY_A_PRESS) {
                 _tempShuttleNum++;
             } else if (event == InputEvent::KEY_B_PRESS) {
                 _tempShuttleNum--;
             } else {
                 _tempShuttleNum = (int)event - (int)InputEvent::KEY_1_PRESS + 1;
             }

             if (_tempShuttleNum < 1) _tempShuttleNum = 1;
             if (_tempShuttleNum > 32) _tempShuttleNum = 32;
             
             setDirty();
             break;

        default:
            break;
    }

    if (!success) {
        LOG_W("UI", "Input %s ignored: Command Failed / Queue Full.", DebugUtils::getEventName(event));
        _showQueueFull = true;
        _queueFullTimer = millis();
        setDirty();
    }
}

void DashboardScreen::tick() {
    _statusBar.tick();
    if (_statusBar.needsRedraw()) {
        setDirty();
        _statusBar.clearDirty();
    }
    // DataManager::getInstance().setPollingMode(DataManager::PollingMode::ACTIVE_TELEMETRY); // Already set onEnter

    if (_showQueueFull && millis() - _queueFullTimer > 2000) {
        _showQueueFull = false;
        setDirty();
    }

    // Action Bar Expiration
    if (_actionMsg.length() > 0) {
        uint32_t elapsed = millis() - _actionMsgTimer;
        if (_actionStatus == "[ OK ]") {
            if (elapsed > 1500) {
                _actionMsg = "";
                _actionStatus = "";
                setDirty();
            }
        } else if (_actionStatus == "[ X ]") {
            if (elapsed > 3000) {
                _actionMsg = "";
                _actionStatus = "";
                setDirty();
            }
        }
    }

    // Shuttle selection timeout and blinking
    if (_isSelectingShuttle) {
        if (millis() - _shuttleSelectTimer > 5000) {
            _isSelectingShuttle = false; // Timeout, revert
            setDirty();
        }
        if (millis() - _lastBlinkTick > 250) {
            _lastBlinkTick = millis();
            _blinkState = !_blinkState;
            setDirty();
        }
    }

    DataManager::getInstance().setManualMoveMode(_isManualMoving);

    // Movement Animation Tick (50ms)
    if (millis() - _lastAnimTick > 50) {
        _lastAnimTick = millis();
        bool animChanged = false;

        const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();
        if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6) {
            if (_animFlagX == 0 && _animX < 128) {
                _animX++;
            } else {
                _animFlagX = 1;
                _animX--;
            }
            if (_animFlagX == 1 && _animX == 0) _animFlagX = 0;
            animChanged = true;
        } else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12) {
            if (_animFlagX == 1 && _animX > 1) {
                _animX--;
            } else {
                _animFlagX = 0;
                _animX++;
            }
            if (_animFlagX == 0 && _animX == 127) _animFlagX = 1;
            animChanged = true;
        }

        if (animChanged && !_isSelectingShuttle) setDirty(); 
    }
}