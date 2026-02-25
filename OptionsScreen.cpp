#include "OptionsScreen.h"
#include "UI_Graph.h" 
#include "ScreenManager.h"
#include <cstdio>

OptionsScreen::OptionsScreen() : _menuList(OptionsScreen::provideMenuItem, OPT_ITEM_COUNT, 4) {}

void OptionsScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    EventBus::subscribe(this);
    DataManager::getInstance().requestConfig(SP::CFG_INTER_PALLET);
    DataManager::getInstance().requestConfig(SP::CFG_REVERSE_MODE);
    DataManager::getInstance().requestConfig(SP::CFG_MAX_SPEED);
    DataManager::getInstance().requestConfig(SP::CFG_MIN_BATT);
}

void OptionsScreen::onExit() { EventBus::unsubscribe(this); }

void OptionsScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::CONFIG_UPDATED || event == SystemEvent::TELEMETRY_UPDATED) setDirty();
}

void OptionsScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: snprintf(buffer, 64, "МПР: %-4ld мм", (long)DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET)); break;
        case 1: {
            bool rev = DataManager::getInstance().getConfig(SP::CFG_REVERSE_MODE) != 0;
            snprintf(buffer, 64, "Инверсия: %-3s", rev ? "<<" : ">>");
            break;
        }
        case 2: snprintf(buffer, 64, "Макс. скорость: %-3ld%%", (long)DataManager::getInstance().getConfig(SP::CFG_MAX_SPEED) / 10); break;
        case 3: snprintf(buffer, 64, "Изменить N уст: %-3d", DataManager::getInstance().getShuttleNumber()); break;
        case 4: snprintf(buffer, 64, "Защита бат: %-3ld%%", (long)DataManager::getInstance().getConfig(SP::CFG_MIN_BATT)); break;
        case 5: strcpy(buffer, "Инженерное меню"); break;
        case 6: strcpy(buffer, "Сохр. параметры"); break;
        case 7: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

void OptionsScreen::draw(U8G2& display) {
    _menuList.draw(display, 0, 0);
}

void OptionsScreen::adjustValue(int idx, bool increase) {
    int32_t val;
    switch(idx) {
        case 0: 
            val = DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET);
            val += (increase ? 10 : -10);
            if (val < 50) val = 50;
            if (val > 400) val = 400;
            DataManager::getInstance().setConfig(SP::CFG_INTER_PALLET, val);
            break;
        case 1: 
            val = DataManager::getInstance().getConfig(SP::CFG_REVERSE_MODE);
            DataManager::getInstance().setConfig(SP::CFG_REVERSE_MODE, !val);
            break;
        case 2: 
            val = DataManager::getInstance().getConfig(SP::CFG_MAX_SPEED);
            val += (increase ? 50 : -50);
            if (val < 200) val = 200;
            if (val > 1000) val = 1000;
            DataManager::getInstance().setConfig(SP::CFG_MAX_SPEED, val);
            break;
        case 4: 
            val = DataManager::getInstance().getConfig(SP::CFG_MIN_BATT);
            val += (increase ? 1 : -1);
            if (val < 0) val = 0;
            if (val > 50) val = 50;
            DataManager::getInstance().setConfig(SP::CFG_MIN_BATT, val);
            break;
    }
    setDirty();
}

void OptionsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); return;
    }

    int idx = _menuList.getCursorIndex();
    if (event == InputEvent::LIFT_UP_PRESS) { adjustValue(idx, true); return; }
    if (event == InputEvent::LIFT_DOWN_PRESS) { adjustValue(idx, false); return; }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(idx) {
            case 1: adjustValue(idx, true); break;
            case 3: ScreenManager::getInstance().push(&changeShuttleNumScreen); break;
            case 5: 
                pinEntryScreen.setTarget(&engineeringMenuScreen);
                ScreenManager::getInstance().push(&pinEntryScreen);
                break;
            case 6: DataManager::getInstance().sendCommand(SP::CMD_SAVE_EEPROM); break;
            case 7: ScreenManager::getInstance().pop(); break;
        }
        return;
    }
    if (_menuList.handleInput(event)) setDirty();
}

void OptionsScreen::tick() {}