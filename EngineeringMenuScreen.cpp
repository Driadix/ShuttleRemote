#include "EngineeringMenuScreen.h"
#include "UI_Graph.h" 
#include "ScreenManager.h"
#include <cstdio>
#include <cstring>

EngineeringMenuScreen::EngineeringMenuScreen() : _menuList(EngineeringMenuScreen::provideMenuItem, ENG_ITEM_COUNT, 4) {}

void EngineeringMenuScreen::onEnter() {
    DataManager::getInstance().requestConfig(SP::CFG_SHUTTLE_LEN);
    DataManager::getInstance().requestConfig(SP::CFG_WAIT_TIME);
    DataManager::getInstance().requestConfig(SP::CFG_MPR_OFFSET);
    DataManager::getInstance().requestConfig(SP::CFG_CHNL_OFFSET);
    EventBus::subscribe(this);
}

void EngineeringMenuScreen::onExit() { EventBus::unsubscribe(this); }

void EngineeringMenuScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::CONFIG_UPDATED || event == SystemEvent::TELEMETRY_UPDATED) setDirty();
}

void EngineeringMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Калибровка"); break;
        case 1: strcpy(buffer, "Отладка"); break;
        case 2: strcpy(buffer, "Систем. настр."); break;
        case 3: strcpy(buffer, "Журналирование"); break;
        case 4: strcpy(buffer, "Движение"); break;
        case 5: strcpy(buffer, "Изменить канал"); break;
        case 6: snprintf(buffer, 64, "Длина шаттла: %-4d", DataManager::getInstance().getConfig(SP::CFG_SHUTTLE_LEN)); break;
        case 7: snprintf(buffer, 64, "Ожидание: %-3d", DataManager::getInstance().getConfig(SP::CFG_WAIT_TIME)); break;
        case 8: snprintf(buffer, 64, "Смещение МПР: %-4d", DataManager::getInstance().getConfig(SP::CFG_MPR_OFFSET)); break;
        case 9: snprintf(buffer, 64, "Смещ. канала: %-4d", DataManager::getInstance().getConfig(SP::CFG_CHNL_OFFSET)); break;
        case 10: strcpy(buffer, "Статистика"); break;
        case 11: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

// FIXED: Formatted multi-line to ensure the closing brace is never lost
void EngineeringMenuScreen::draw(U8G2& display) { 
    _menuList.draw(display, 0, 0); 
}

void EngineeringMenuScreen::adjustValue(int idx, bool increase) {
    int32_t val;
    switch(idx) {
        case 6: 
            val = DataManager::getInstance().getConfig(SP::CFG_SHUTTLE_LEN);
            val += (increase ? 200 : -200);
            if (val < 800) val = 1200; 
            else if (val > 1200) val = 800; 
            DataManager::getInstance().setConfig(SP::CFG_SHUTTLE_LEN, val);
            break;
        case 7: 
            val = DataManager::getInstance().getConfig(SP::CFG_WAIT_TIME);
            val += (increase ? 1 : -1);
            if (val < 5) val = 5;
            if (val > 30) val = 30;
            DataManager::getInstance().setConfig(SP::CFG_WAIT_TIME, val);
            break;
        case 8: 
            val = DataManager::getInstance().getConfig(SP::CFG_MPR_OFFSET);
            val += (increase ? 10 : -10);
            if (val < -100) val = -100;
            if (val > 100) val = 100;
            DataManager::getInstance().setConfig(SP::CFG_MPR_OFFSET, val);
            break;
        case 9: 
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
    if (event == InputEvent::BACK_PRESS) { ScreenManager::getInstance().pop(); return; }

    int idx = _menuList.getCursorIndex();
    if (event == InputEvent::LIFT_UP_PRESS) { adjustValue(idx, true); return; }
    if (event == InputEvent::LIFT_DOWN_PRESS) { adjustValue(idx, false); return; }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(idx) {
            case 0: DataManager::getInstance().sendCommand(SP::CMD_CALIBRATE); break;
            case 1: ScreenManager::getInstance().push(&debugInfoScreen); break;
            case 3: DataManager::getInstance().sendCommand(SP::CMD_LOG_MODE, 1); break;
            case 4: ScreenManager::getInstance().push(&movementScreen); break;
            case 5: ScreenManager::getInstance().push(&changeChannelScreen); break;
            case 10: ScreenManager::getInstance().push(&statsScreen); break;
            case 11: ScreenManager::getInstance().pop(); break;
        }
        return;
    }
    if (_menuList.handleInput(event)) setDirty();
}

void EngineeringMenuScreen::tick() {}