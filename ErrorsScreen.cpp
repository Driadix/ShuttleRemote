#include "ErrorsScreen.h"
#include "ScreenManager.h"
#include "Lang_RU.h"
#include <cstdio>

// Static definitions
uint8_t ErrorsScreen::_activeErrorCodes[MAX_ERR_ITEMS];
uint8_t ErrorsScreen::_totalItems = 0;

ErrorsScreen::ErrorsScreen()
    : _statusBar(),
      _menuList(ErrorsScreen::provideErrorItem, 0, 4) // Initial count 0
{
}

void ErrorsScreen::provideErrorItem(uint8_t index, char* buffer) {
    if (index >= _totalItems) {
        buffer[0] = '\0';
        return;
    }

    // Check if it's Reset or Back (Last 2 items)
    if (index == _totalItems - 2) {
        strcpy(buffer, "Reset Errors");
        return;
    }
    if (index == _totalItems - 1) {
        strcpy(buffer, "Back");
        return;
    }

    // It's an error
    uint8_t code = _activeErrorCodes[index];
    if (code == 0) {
        strcpy(buffer, "No Errors");
    } else {
        snprintf(buffer, 32, "E%02d: %s", code, ERROR_STRINGS[code]);
    }
}

void ErrorsScreen::onEnter() {
    updateErrorList();
    EventBus::subscribe(this);
}

void ErrorsScreen::onExit() {
    EventBus::unsubscribe(this);
}

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

    if (!anyError) {
        _activeErrorCodes[idx++] = 0; // 0 means "No Errors"
    }

    // Always add "Reset Errors" and "Back"
    // We don't store them in activeErrorCodes, handled by provider logic
    // Just increment count for them

    // Store dummy values just in case or use index logic
    // provider logic uses `index == _totalItems - 2`.
    // So we just need to set _totalItems = idx + 2;

    _totalItems = idx + 2;

    // Total items = idx
    // Reconstruct list widget with new count
    uint8_t oldCursor = _menuList.getCursorIndex();

    _menuList = ScrollingListWidget(ErrorsScreen::provideErrorItem, _totalItems, 4);

    // Restore cursor if valid
    if (oldCursor < _totalItems) {
        _menuList.setCursorIndex(oldCursor);
    }
}

void ErrorsScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16);
}

void ErrorsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();

        // _totalItems includes errors + Reset + Back.
        // Reset is at _totalItems - 2.
        // Back is at _totalItems - 1.

        if (idx == _totalItems - 2) { // Reset
             DataManager::getInstance().sendCommand(SP::CMD_RESET_ERROR);
             // Request update? tick() will handle it.
        } else if (idx == _totalItems - 1) { // Back
             ScreenManager::getInstance().pop();
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void ErrorsScreen::tick() {
}
