#include "EngineeringMenuScreen.h"
#include "UI_Graph.h" // for StatsScreen, DebugInfoScreen
#include "ScreenManager.h"
#include <cstdio>

EngineeringMenuScreen::EngineeringMenuScreen()
    : _statusBar(),
      _menuList(EngineeringMenuScreen::provideMenuItem, ENG_ITEM_COUNT, 4)
{
}

void EngineeringMenuScreen::onEnter() {
    DataManager::getInstance().requestConfig(SP::CFG_SHUTTLE_LEN);
    DataManager::getInstance().requestConfig(SP::CFG_WAIT_TIME);
    DataManager::getInstance().requestConfig(SP::CFG_MPR_OFFSET);
    DataManager::getInstance().requestConfig(SP::CFG_CHNL_OFFSET);
    EventBus::subscribe(this);
}

void EngineeringMenuScreen::onExit() {
    EventBus::unsubscribe(this);
}

void EngineeringMenuScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::CONFIG_UPDATED || event == SystemEvent::TELEMETRY_UPDATED) {
        setDirty();
    }
}

void EngineeringMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Calibration"); break;
        case 1: strcpy(buffer, "Debug Info"); break;
        case 2: strcpy(buffer, "Sys Settings"); break;
        case 3: strcpy(buffer, "Logging: Toggle"); break;
        case 4: strcpy(buffer, "Movement"); break;
        case 5: strcpy(buffer, "Change Channel"); break;
        case 6: snprintf(buffer, 32, "Shuttle Len: %d", DataManager::getInstance().getConfig(SP::CFG_SHUTTLE_LEN)); break;
        case 7: snprintf(buffer, 32, "Wait Time: %d", DataManager::getInstance().getConfig(SP::CFG_WAIT_TIME)); break;
        case 8: snprintf(buffer, 32, "MPR Offset: %d", DataManager::getInstance().getConfig(SP::CFG_MPR_OFFSET)); break;
        case 9: snprintf(buffer, 32, "Chnl Offset: %d", DataManager::getInstance().getConfig(SP::CFG_CHNL_OFFSET)); break;
        case 10: strcpy(buffer, "Stats"); break;
        case 11: strcpy(buffer, "Back"); break;
        default: buffer[0] = '\0'; break;
    }
}

void EngineeringMenuScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16);
}

void EngineeringMenuScreen::adjustValue(int idx, bool increase) {
    int32_t val;
    switch(idx) {
        case 6: // Shuttle Length
            val = DataManager::getInstance().getConfig(SP::CFG_SHUTTLE_LEN);
            val += (increase ? 200 : -200);
            if (val < 800) val = 1200; // Legacy wrap around?
            else if (val > 1200) val = 800; // Legacy wrap
            DataManager::getInstance().setConfig(SP::CFG_SHUTTLE_LEN, val);
            break;
        case 7: // Wait Time
            val = DataManager::getInstance().getConfig(SP::CFG_WAIT_TIME);
            val += (increase ? 1 : -1);
            if (val < 5) val = 5;
            if (val > 30) val = 30;
            DataManager::getInstance().setConfig(SP::CFG_WAIT_TIME, val);
            break;
        case 8: // MPR Offset
            val = DataManager::getInstance().getConfig(SP::CFG_MPR_OFFSET);
            val += (increase ? 10 : -10);
            if (val < -100) val = -100;
            if (val > 100) val = 100;
            DataManager::getInstance().setConfig(SP::CFG_MPR_OFFSET, val);
            break;
        case 9: // Chnl Offset
            val = DataManager::getInstance().getConfig(SP::CFG_CHNL_OFFSET);
            val += (increase ? 10 : -10);
            if (val < -100) val = -100;
            if (val > 100) val = 100;
            DataManager::getInstance().setConfig(SP::CFG_CHNL_OFFSET, val);
            break;
    }
    setDirty();
}

void EngineeringMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    int idx = _menuList.getCursorIndex();

    if (event == InputEvent::LIFT_UP_PRESS) {
        adjustValue(idx, true);
        return;
    }
    if (event == InputEvent::LIFT_DOWN_PRESS) {
        adjustValue(idx, false);
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(idx) {
            case 0: // Calibration
                 DataManager::getInstance().sendCommand(SP::CMD_CALIBRATE);
                 break;
            case 1: // Debug Info
                 ScreenManager::getInstance().push(&debugInfoScreen);
                 break;
            case 2: // Sys Settings
                 break;
            case 3: // Logging
                 // Toggle?
                 DataManager::getInstance().sendCommand(SP::CMD_LOG_MODE, 1);
                 break;
            case 4: // Movement
                 ScreenManager::getInstance().push(&movementScreen);
                 break;
            case 5: // Change Channel
                 ScreenManager::getInstance().push(&changeChannelScreen);
                 break;
            case 10: // Stats
                 ScreenManager::getInstance().push(&statsScreen);
                 break;
            case 11: // Back
                 ScreenManager::getInstance().pop();
                 break;
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void EngineeringMenuScreen::tick() {
}
