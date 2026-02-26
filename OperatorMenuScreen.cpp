#include "OperatorMenuScreen.h"
#include "UI_Graph.h" 
#include "ScreenManager.h"
#include <cstring> 

OperatorMenuScreen::OperatorMenuScreen()
    : _menuList(OperatorMenuScreen::provideMenuItem, ITEM_COUNT, 5) {}

void OperatorMenuScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
}

void OperatorMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Выгрузка N палет"); break;
        case 1: strcpy(buffer, "Уплотнение"); break;
        case 2: strcpy(buffer, "Подсчет палет"); break;
        case 3: strcpy(buffer, "Эвакуация"); break;
        case 4: strcpy(buffer, "Возврат домой"); break;
        case 5: strcpy(buffer, "Активные ошибки"); break;
        case 6: strcpy(buffer, "Инженерное меню"); break;
        case 7: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

void OperatorMenuScreen::draw(U8G2& display) {
    _menuList.draw(display, 0, 0); 
}

void OperatorMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); 
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        switch(idx) {
            case 0: ScreenManager::getInstance().push(&unloadPalletsScreen); break;
            case 1: ScreenManager::getInstance().push(&compactionMenuScreen); break;
            case 2: DataManager::getInstance().sendCommand(SP::CMD_COUNT_PALLETS); break;
            case 3: DataManager::getInstance().sendCommand(SP::CMD_EVACUATE_ON); break;
            case 4: DataManager::getInstance().sendCommand(SP::CMD_HOME); break;
            case 5: ScreenManager::getInstance().push(&errorsScreen); break;
            case 6: 
                pinEntryScreen.setTarget(&engineerMenuScreen);
                ScreenManager::getInstance().push(&pinEntryScreen);
                break;
            case 7: ScreenManager::getInstance().pop(); break;
        }
        return;
    }

    if (_menuList.handleInput(event)) setDirty();
}

void OperatorMenuScreen::tick() {}