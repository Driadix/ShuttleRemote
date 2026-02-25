#include "ErrorsScreen.h"
#include "ScreenManager.h"
#include <cstdio>

// Static definitions
char ErrorsScreen::_errItemBuffers[ErrorsScreen::MAX_ERR_ITEMS][32];
const char* ErrorsScreen::_errItemsPtrs[ErrorsScreen::MAX_ERR_ITEMS];

static const char* ErrorMsgArray[16] = {
    "No Errors", // 0 is handled separately usually
    "Chnl Sensor F",
    "Chnl Sensor R",
    "Chnl Sensor DF",
    "Chnl Sensor DR",
    "Pallet Sensor F",
    "Pallet Sensor R",
    "Pallet Sensor DF",
    "Pallet Sensor DR",
    "Lifter Error",
    "Motor Drive",
    "Low Battery",
    "Collision",
    "Overheat",
    "Unknown 14",
    "Unknown 15"
};

ErrorsScreen::ErrorsScreen()
    : _statusBar(),
      _menuList(_errItemsPtrs, 0, 4), // Initial count 0
      _activeErrorCount(0)
{
    // Initialize pointers
    for (int i = 0; i < MAX_ERR_ITEMS; i++) {
        _errItemsPtrs[i] = _errItemBuffers[i];
    }
}

void ErrorsScreen::onEnter() {
    updateErrorList();
}

void ErrorsScreen::updateErrorList() {
    const auto& tel = DataManager::getInstance().getTelemetry();
    uint16_t err = tel.errorCode;

    int idx = 0;

    // Check bits 1-15 (Legacy skips bit 0 for display, bit 0 might be general error flag?)
    // Legacy: for (ec = 1; ec < 16; ec++)
    bool anyError = false;
    for (int i = 1; i < 16; i++) {
        if ((err >> i) & 1) {
             snprintf(_errItemBuffers[idx++], 32, "E%02d: %s", i, ErrorMsgArray[i]);
             anyError = true;
        }
    }

    if (!anyError) {
        snprintf(_errItemBuffers[idx++], 32, "No Errors");
    }

    _activeErrorCount = idx; // Count of error lines

    // Always add "Reset Errors" and "Back"
    snprintf(_errItemBuffers[idx++], 32, "Reset Errors");
    snprintf(_errItemBuffers[idx++], 32, "Back");

    // Total items = idx
    // Reconstruct list widget with new count
    uint8_t oldCursor = _menuList.getCursorIndex();

    _menuList = ScrollingListWidget(_errItemsPtrs, idx, 4);

    // Restore cursor if valid
    if (oldCursor < idx) {
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

        // _activeErrorCount is the number of error lines.
        // If "No Errors", _activeErrorCount is 1.
        // If 2 errors, _activeErrorCount is 2.

        // Reset Errors is at index _activeErrorCount.
        // Back is at index _activeErrorCount + 1.

        if (idx == _activeErrorCount) { // Reset
             DataManager::getInstance().sendCommand(SP::CMD_RESET_ERROR);
             // Request update? tick() will handle it.
        } else if (idx == _activeErrorCount + 1) { // Back
             ScreenManager::getInstance().pop();
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void ErrorsScreen::tick() {
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        updateErrorList();
        setDirty();
    }
}
