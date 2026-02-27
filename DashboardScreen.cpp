#include "DashboardScreen.h"
#include "UI_Graph.h"
#include "ScreenManager.h"
#include "EventBus.h"
#include "Lang_RU.h"
#include "Logger.h"
#include "DebugUtils.h"

DashboardScreen::DashboardScreen()
    : _isManualMoving(false), _manualCommand(" "), 
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
        event == SystemEvent::BATTERY_LOW ||
        event == SystemEvent::CONNECTION_LOST || 
        event == SystemEvent::CONNECTION_RESTORED ||
        event == SystemEvent::LOCAL_BATT_UPDATED) {
        setDirty();
    } else if (event == SystemEvent::CMD_DISPATCHED) {
        SP::CmdType cmd = DataManager::getInstance().getLastUserCommandType();
        _actionMsg = String(DebugUtils::getUICommandName((uint8_t)cmd));
        _actionStatus = "[ > ]";
        _actionMsgTimer = millis();
        setDirty();
    } else if (event == SystemEvent::CMD_ACKED) {
        // Kept for Reliable Commands (like SAVE_EEPROM)
        if (_actionMsg.length() > 0) {
            _actionStatus = "[ OK ]";
            _actionMsgTimer = millis();
            setDirty();
        }
    } else if (event == SystemEvent::CMD_FAILED) {
        // Kept for Reliable Commands
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
    display.setDrawColor(1);

    char buf[64];
    bool connected = DataManager::getInstance().isConnected();

    if (!connected) {
        if ((millis() / 500) % 2 == 0) {
            display.drawUTF8(15, 35, "Поиск шаттла...");
        }
    } else {
        const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();

        if (cachedTelemetry.shuttleStatus == 13) {
            snprintf(buf, sizeof(buf), TXT_LEFT_TO_UNLOAD, cachedTelemetry.palleteCount);
            display.drawUTF8(0, 25, buf);
        }
        else if (cachedTelemetry.shuttleStatus < 19) {
            display.drawUTF8(0, 25, SHUTTLE_STATUS_STRINGS[cachedTelemetry.shuttleStatus]);
        }
        else {
            snprintf(buf, sizeof(buf), "Status: %d", cachedTelemetry.shuttleStatus);
            display.drawUTF8(0, 25, buf);
        }

        if (cachedTelemetry.errorCode) {
            if (cachedTelemetry.errorCode < 16) {
                 snprintf(buf, sizeof(buf), "! %s !", ERROR_STRINGS[cachedTelemetry.errorCode]);
                 display.drawUTF8(0, 40, buf);
            }
            else {
                 snprintf(buf, sizeof(buf), TXT_ERR_PREFIX, cachedTelemetry.errorCode);
                 display.drawUTF8(0, 40, buf);
            }
        }

        if (_isManualMoving) {
            display.drawUTF8(85, 40, _manualCommand.c_str());
        }

        if (_actionMsg.length() > 0) {
            char actionBuf[64];
            snprintf(actionBuf, sizeof(actionBuf), "%s %s", _actionMsg.c_str(), _actionStatus.c_str());
            display.drawUTF8(0, 52, actionBuf);
        }

        drawShuttleStatus(display, cachedTelemetry);
    }

    display.setFont(u8g2_font_10x20_t_cyrillic);
    uint8_t shtNumToDraw = _isSelectingShuttle ? _tempShuttleNum : DataManager::getInstance().getTargetShuttleID();
    
    int xOffset = 30 * (shtNumToDraw - 1);
    if (shtNumToDraw > 4) xOffset = 100;

    if (!_isSelectingShuttle || _blinkState) {
        snprintf(buf, sizeof(buf), "|%d|", shtNumToDraw);
        display.drawUTF8(xOffset, 63, buf);
    }
}

void DashboardScreen::drawShuttleStatus(U8G2& display, const SP::TelemetryPacket& cachedTelemetry) {
    if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6) {
        display.drawBox(_animX, 40, 28, 5);
        display.drawBox(_animX + 4, 45, 3, 2);
        display.drawBox(_animX + 21, 45, 3, 2);
        if (_animFlagX == 1) {
            display.drawBox(_animX, 34, 28, 3);
            display.drawBox(_animX, 37, 3, 3);
            display.drawBox(_animX + 13, 37, 3, 3);
            display.drawBox(_animX + 25, 37, 3, 3);
        }
    } else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12) {
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
    if (!DataManager::getInstance().isConnected()) {
        if (event != InputEvent::BACK_PRESS && 
            event != InputEvent::KEY_A_PRESS && 
            event != InputEvent::KEY_B_PRESS &&
            event != InputEvent::KEY_1_PRESS &&
            event != InputEvent::KEY_2_PRESS &&
            event != InputEvent::KEY_3_PRESS &&
            event != InputEvent::KEY_4_PRESS &&
            event != InputEvent::OK_SHORT_PRESS &&
            event != InputEvent::OK_LONG_PRESS) {
            return; 
        }
    }

    // Commands fire instantly over the volatile link
    switch(event) {
        case InputEvent::UP_PRESS: // '8'
            if (!_isManualMoving) {
                DataManager::getInstance().sendCommand(SP::CMD_MOVE_RIGHT_MAN);
                _manualCommand = ">>";
                _isManualMoving = true;
            }
            break;

        case InputEvent::DOWN_PRESS: // '0'
            if (!_isManualMoving) {
                DataManager::getInstance().sendCommand(SP::CMD_MOVE_LEFT_MAN);
                _manualCommand = "<<";
                _isManualMoving = true;
            }
            break;

        case InputEvent::STOP_PRESS:
        case InputEvent::MANUAL_MODE_PRESS:
             if (_isManualMoving) {
                 DataManager::getInstance().sendCommand(SP::CMD_STOP_MANUAL);
                 _manualCommand = " ";
                 _isManualMoving = false;
             } else {
                 if (event == InputEvent::MANUAL_MODE_PRESS) DataManager::getInstance().sendCommand(SP::CMD_MANUAL_MODE);
                 else DataManager::getInstance().sendCommand(SP::CMD_STOP);
             }
             break;

        case InputEvent::LIFT_UP_PRESS: DataManager::getInstance().sendCommand(SP::CMD_LIFT_UP); break;
        case InputEvent::LIFT_DOWN_PRESS: DataManager::getInstance().sendCommand(SP::CMD_LIFT_DOWN); break;
        case InputEvent::LOAD_PRESS: DataManager::getInstance().sendCommand(SP::CMD_LOAD); break;
        case InputEvent::LONG_LOAD_PRESS: DataManager::getInstance().sendCommand(SP::CMD_LONG_LOAD); break;
        case InputEvent::UNLOAD_PRESS: DataManager::getInstance().sendCommand(SP::CMD_UNLOAD); break;
        case InputEvent::LONG_UNLOAD_PRESS: DataManager::getInstance().sendCommand(SP::CMD_LONG_UNLOAD); break;

        case InputEvent::BACK_PRESS: // '7'
            ScreenManager::getInstance().push(&operatorMenuScreen);
            break;

        case InputEvent::OK_SHORT_PRESS:
        case InputEvent::OK_LONG_PRESS: // 'D'
            if (_isSelectingShuttle) {
                DataManager::getInstance().saveLocalShuttleNumber(_tempShuttleNum);
                _isSelectingShuttle = false;
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
                 _tempShuttleNum = DataManager::getInstance().getTargetShuttleID();
             }
             _isSelectingShuttle = true;
             _shuttleSelectTimer = millis();
             _blinkState = true;
             
             if (event == InputEvent::KEY_A_PRESS) _tempShuttleNum++;
             else if (event == InputEvent::KEY_B_PRESS) _tempShuttleNum--;
             else _tempShuttleNum = (int)event - (int)InputEvent::KEY_1_PRESS + 1;

             if (_tempShuttleNum < 1) _tempShuttleNum = 1;
             if (_tempShuttleNum > 32) _tempShuttleNum = 32;
             
             setDirty();
             break;
        default: break;
    }
}

void DashboardScreen::tick() {
    _statusBar.tick();
    if (_statusBar.needsRedraw()) {
        setDirty();
        _statusBar.clearDirty();
    }

    if (!DataManager::getInstance().isConnected()) {
        static uint32_t lastSearchBlink = 0;
        if (millis() - lastSearchBlink > 500) {
            lastSearchBlink = millis();
            setDirty();
        }
    }

    if (_actionMsg.length() > 0) {
        uint32_t elapsed = millis() - _actionMsgTimer;
        // Volatile Action Mode: Clears [ > ] gracefully since no physical ACK will arrive
        if (_actionStatus == "[ > ]" && elapsed > 800) {
            _actionMsg = "";
            _actionStatus = "";
            setDirty();
        } else if (_actionStatus == "[ OK ]" && elapsed > 1500) {
            _actionMsg = "";
            _actionStatus = "";
            setDirty();
        } else if (_actionStatus == "[ X ]" && elapsed > 3000) {
            _actionMsg = "";
            _actionStatus = "";
            setDirty();
        }
    }

    if (_isSelectingShuttle) {
        if (millis() - _shuttleSelectTimer > 5000) {
            _isSelectingShuttle = false;
            setDirty();
        }
        if (millis() - _lastBlinkTick > 250) {
            _lastBlinkTick = millis();
            _blinkState = !_blinkState;
            setDirty();
        }
    }

    DataManager::getInstance().setManualMoveMode(_isManualMoving);

    if (millis() - _lastAnimTick > 50) {
        _lastAnimTick = millis();
        bool animChanged = false;

        const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();
        if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6) {
            if (_animFlagX == 0 && _animX < 128) _animX++;
            else { _animFlagX = 1; _animX--; }
            if (_animFlagX == 1 && _animX == 0) _animFlagX = 0;
            animChanged = true;
        } else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12) {
            if (_animFlagX == 1 && _animX > 1) _animX--;
            else { _animFlagX = 0; _animX++; }
            if (_animFlagX == 0 && _animX == 127) _animFlagX = 1;
            animChanged = true;
        }

        if (animChanged && !_isSelectingShuttle) setDirty(); 
    }
}