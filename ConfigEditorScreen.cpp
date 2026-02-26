#include "ConfigEditorScreen.h"
#include "ScreenManager.h"
#include <cstdio>
#include <cstring>

SP::FullConfigPacket ConfigEditorScreen::_localConfig;

ConfigEditorScreen::ConfigEditorScreen() 
    : _menuList(ConfigEditorScreen::provideMenuItem, ITEM_COUNT, 5), _requestTimer(0) {}

void ConfigEditorScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    if (!DataManager::getInstance().hasFullConfig()) {
        _requestTimer = millis();
        DataManager::getInstance().requestFullConfig();
    } else {
        _localConfig = DataManager::getInstance().getFullConfig();
    }
    EventBus::subscribe(this);
}

void ConfigEditorScreen::onExit() {
    EventBus::unsubscribe(this);
    // Force refetch on next engineer menu entry
    DataManager::getInstance().invalidateFullConfig(); 
}

void ConfigEditorScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::CONFIG_UPDATED) {
        _localConfig = DataManager::getInstance().getFullConfig();
        setDirty();
    } else if (event == SystemEvent::CONNECTION_LOST) {
        setDirty();
    }
}

bool ConfigEditorScreen::hasValidData() const {
    return DataManager::getInstance().hasFullConfig();
}

void ConfigEditorScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: snprintf(buffer, 64, "Режим: %s", _localConfig.fifoLifo ? "LIFO" : "FIFO"); break;
        case 1: snprintf(buffer, 64, "МПР: %d мм", _localConfig.interPallet); break;
        case 2: snprintf(buffer, 64, "Смещение МПР: %d", _localConfig.mprOffset); break;
        case 3: snprintf(buffer, 64, "Смещ. канала: %d", _localConfig.chnlOffset); break;
        case 4: snprintf(buffer, 64, "Длина шаттла: %d", _localConfig.shuttleLen); break;
        case 5: snprintf(buffer, 64, "Макс. скорость: %d%%", _localConfig.maxSpeed / 10); break;
        case 6: snprintf(buffer, 64, "Инверсия: %s", _localConfig.reverseMode ? "Вкл" : "Выкл"); break;
        case 7: snprintf(buffer, 64, "Защита АКБ: %d%%", _localConfig.minBatt); break;
        case 8: snprintf(buffer, 64, "Время ож.: %d с", _localConfig.waitTime); break;
        case 9: snprintf(buffer, 64, "Номер шаттла: %d", _localConfig.shuttleNumber); break;
        case 10: strcpy(buffer, "[ Отправить ]"); break;
        case 11: strcpy(buffer, "[ Сохранить в ПЗУ ]"); break;
        case 12: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

void ConfigEditorScreen::adjustValue(int idx, bool increase) {
    switch(idx) {
        case 0: _localConfig.fifoLifo = !_localConfig.fifoLifo; break;
        case 1: 
            _localConfig.interPallet += (increase ? 10 : -10);
            if (_localConfig.interPallet < 50) _localConfig.interPallet = 50;
            if (_localConfig.interPallet > 400) _localConfig.interPallet = 400;
            break;
        case 2:
            _localConfig.mprOffset += (increase ? 10 : -10);
            if (_localConfig.mprOffset < -100) _localConfig.mprOffset = -100;
            if (_localConfig.mprOffset > 100) _localConfig.mprOffset = 100;
            break;
        case 3:
            _localConfig.chnlOffset += (increase ? 10 : -10);
            if (_localConfig.chnlOffset < -100) _localConfig.chnlOffset = -100;
            if (_localConfig.chnlOffset > 100) _localConfig.chnlOffset = 100;
            break;
        case 4:
            if (increase) {
                if (_localConfig.shuttleLen == 800) _localConfig.shuttleLen = 1000;
                else if (_localConfig.shuttleLen == 1000) _localConfig.shuttleLen = 1200;
                else _localConfig.shuttleLen = 800;
            } else {
                if (_localConfig.shuttleLen == 1200) _localConfig.shuttleLen = 1000;
                else if (_localConfig.shuttleLen == 1000) _localConfig.shuttleLen = 800;
                else _localConfig.shuttleLen = 1200;
            }
            break;
        case 5:
            _localConfig.maxSpeed += (increase ? 50 : -50);
            if (_localConfig.maxSpeed < 200) _localConfig.maxSpeed = 200;
            if (_localConfig.maxSpeed > 1000) _localConfig.maxSpeed = 1000;
            break;
        case 6: _localConfig.reverseMode = !_localConfig.reverseMode; break;
        case 7: {
            int val = _localConfig.minBatt;
            val += (increase ? 1 : -1);
            if (val < 0) val = 0;
            if (val > 50) val = 50;
            _localConfig.minBatt = val;
            break;
        }
        case 8: {
            int val = _localConfig.waitTime;
            val += (increase ? 1 : -1);
            if (val < 5) val = 5;
            if (val > 30) val = 30;
            _localConfig.waitTime = val;
            break;
        }
        case 9: {
            int val = _localConfig.shuttleNumber;
            val += (increase ? 1 : -1);
            if (val < 1) val = 1;
            if (val > 32) val = 32;
            _localConfig.shuttleNumber = val;
            break;
        }
    }
    setDirty();
}

void ConfigEditorScreen::drawData(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);
    _menuList.draw(display, 0, 0);
}

void ConfigEditorScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); 
        return;
    }

    if (!hasValidData()) return; 

    int idx = _menuList.getCursorIndex();
    bool decrease = (event == InputEvent::LEFT_PRESS || event == InputEvent::KEY_B_PRESS);
    bool increase = (event == InputEvent::RIGHT_PRESS || event == InputEvent::KEY_A_PRESS);

    if (decrease || increase) {
        adjustValue(idx, increase);
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(idx) {
            case 10: 
                DataManager::getInstance().pushFullConfig(_localConfig);
                break;
            case 11: 
                DataManager::getInstance().sendCommand(SP::CMD_SAVE_EEPROM); 
                break;
            case 12: 
                ScreenManager::getInstance().pop(); 
                break;
        }
        return;
    }
    if (_menuList.handleInput(event)) setDirty();
}

void ConfigEditorScreen::tick() {
    if (!hasValidData() && millis() - _requestTimer > 3000) {
        if (DataManager::getInstance().isConnected()) {
             _requestTimer = millis();
             DataManager::getInstance().requestFullConfig(); 
        }
    }
}