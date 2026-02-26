#include "CompactionMenuScreen.h"
#include "ScreenManager.h"
#include <cstring>

CompactionMenuScreen::CompactionMenuScreen() 
    : _menuList(CompactionMenuScreen::provideMenuItem, 3, 3) {}

void CompactionMenuScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
}

void CompactionMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Вперед"); break;
        case 1: strcpy(buffer, "Назад"); break;
        case 2: strcpy(buffer, "Отмена"); break;
        default: buffer[0] = '\0'; break;
    }
}

void CompactionMenuScreen::draw(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);
    display.drawUTF8(0, 15, "Уплотнение:");
    _menuList.draw(display, 0, 25);
}

void CompactionMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); 
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        if (idx == 0) DataManager::getInstance().sendCommand(SP::CMD_COMPACT_F);
        else if (idx == 1) DataManager::getInstance().sendCommand(SP::CMD_COMPACT_R);
        
        ScreenManager::getInstance().popToRoot(); // Return to dashboard after execution
        return;
    }

    if (_menuList.handleInput(event)) setDirty();
}

void CompactionMenuScreen::tick() {}