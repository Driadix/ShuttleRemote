#include "EngineerMenuScreen.h"
#include "UI_Graph.h" 
#include "ScreenManager.h"
#include <cstring>

EngineerMenuScreen::EngineerMenuScreen() 
    : _menuList(EngineerMenuScreen::provideMenuItem, ITEM_COUNT, 5) {}

void EngineerMenuScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
}

void EngineerMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Параметры шаттла"); break;
        case 1: strcpy(buffer, "Параметры пульта"); break;
        case 2: strcpy(buffer, "Диагностика"); break;
        case 3: strcpy(buffer, "Статистика"); break;
        case 4: strcpy(buffer, "Калибровка"); break;
        case 5: strcpy(buffer, "Тест движения"); break;
        case 6: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

void EngineerMenuScreen::draw(U8G2& display) { 
    _menuList.draw(display, 0, 0); 
}

void EngineerMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) { ScreenManager::getInstance().pop(); return; }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(_menuList.getCursorIndex()) {
            case 0: ScreenManager::getInstance().push(&configEditorScreen); break;
            case 1: ScreenManager::getInstance().push(&remoteSettingsScreen); break;
            case 2: ScreenManager::getInstance().push(&debugInfoScreen); break;
            case 3: ScreenManager::getInstance().push(&statsScreen); break;
            case 4: DataManager::getInstance().sendCommand(SP::CMD_CALIBRATE); break;
            case 5: ScreenManager::getInstance().push(&movementScreen); break;
            case 6: ScreenManager::getInstance().pop(); break;
        }
        return;
    }
    if (_menuList.handleInput(event)) setDirty();
}

void EngineerMenuScreen::tick() {}