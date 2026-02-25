#include "ErrorsScreen.h"
#include "ScreenManager.h"
#include "Lang_RU.h"
#include <cstdio>

uint8_t ErrorsScreen::_activeErrorCodes[MAX_ERR_ITEMS];
uint8_t ErrorsScreen::_totalItems = 0;

ErrorsScreen::ErrorsScreen() : _menuList(ErrorsScreen::provideErrorItem, 0, 4) {}

void ErrorsScreen::provideErrorItem(uint8_t index, char* buffer) {
    if (index >= _totalItems) { buffer[0] = '\0'; return; }
    if (index == _totalItems - 2) { strcpy(buffer, "Сброс ошибок"); return; }
    if (index == _totalItems - 1) { strcpy(buffer, "Назад"); return; }

    uint8_t code = _activeErrorCodes[index];
    if (code == 0) { strcpy(buffer, TXT_NO_ERRORS); } 
    else { snprintf(buffer, 64, "E%02d: %s", code, ERROR_STRINGS[code]); }
}

void ErrorsScreen::onEnter() { updateErrorList(); EventBus::subscribe(this); }
void ErrorsScreen::onExit() { EventBus::unsubscribe(this); }

void ErrorsScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED || event == SystemEvent::ERROR_OCCURRED) {
        updateErrorList();
        setDirty();
    }
}

void ErrorsScreen::updateErrorList() {
    const auto& tel = DataManager::getInstance().getTelemetry();
    uint16_t err = tel.errorCode;

    int idx = 0;
    bool anyError = false;
    for (int i = 1; i < 16; i++) {
        if ((err >> i) & 1) {
             _activeErrorCodes[idx++] = i;
             anyError = true;
        }
    }

    if (!anyError) _activeErrorCodes[idx++] = 0;
    _totalItems = idx + 2;

    uint8_t oldCursor = _menuList.getCursorIndex();
    _menuList = ScrollingListWidget(ErrorsScreen::provideErrorItem, _totalItems, 4);
    if (oldCursor < _totalItems) _menuList.setCursorIndex(oldCursor);
}

void ErrorsScreen::draw(U8G2& display) { _menuList.draw(display, 0, 0); }

void ErrorsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) { ScreenManager::getInstance().pop(); return; }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        if (idx == _totalItems - 2) { 
             DataManager::getInstance().sendCommand(SP::CMD_RESET_ERROR);
        } else if (idx == _totalItems - 1) { 
             ScreenManager::getInstance().pop();
        }
        return;
    }
    if (_menuList.handleInput(event)) setDirty();
}

void ErrorsScreen::tick() {}