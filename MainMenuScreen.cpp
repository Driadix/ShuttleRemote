#include "MainMenuScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include <cstdio> // For snprintf

// Static member definitions
char MainMenuScreen::_menuItemBuffers[MainMenuScreen::MENU_ITEM_COUNT][32];
const char* MainMenuScreen::_menuItemsPtrs[MainMenuScreen::MENU_ITEM_COUNT];

MainMenuScreen::MainMenuScreen()
    : _statusBar(),
      _menuList(_menuItemsPtrs, MENU_ITEM_COUNT, 4) // visibleItems=4
{
    // Initialize static pointers (only once ideally, but constructor is called once for static instance)
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        _menuItemsPtrs[i] = _menuItemBuffers[i];
    }
}

void MainMenuScreen::onEnter() {
    updateMenuItems();
    // Reset cursor to top? Legacy doesn't, but maybe good UX.
    // _menuList.setCursorIndex(0);
}

void MainMenuScreen::updateMenuItems() {
    // 0. Options
    snprintf(_menuItemBuffers[0], 32, "Settings"); // "Настройки"

    // 1. Errors
    snprintf(_menuItemBuffers[1], 32, "Errors");

    // 2. Unload Pallets
    snprintf(_menuItemBuffers[2], 32, "Unload Pallets");

    // 3. Engineering Menu (Secured)
    snprintf(_menuItemBuffers[3], 32, "Engineering Menu");

    // 4. Pallet Count (Dynamic)
    const auto& tel = DataManager::getInstance().getTelemetry();
    snprintf(_menuItemBuffers[4], 32, "Pallet Count: %d", tel.palleteCount);

    // 5. Compact
    snprintf(_menuItemBuffers[5], 32, "Compact");

    // 6. Mode (Dynamic)
    bool fifoLifo = (tel.stateFlags & 0x20) != 0; // 0x20 check from legacy
    snprintf(_menuItemBuffers[6], 32, "Mode: %s", fifoLifo ? "LIFO" : "FIFO");

    // 7. Home
    snprintf(_menuItemBuffers[7], 32, "Return Home");

    // 8. Back
    snprintf(_menuItemBuffers[8], 32, "Back");
}

void MainMenuScreen::draw(U8G2& display) {
    updateMenuItems(); // Ensure dynamic strings are fresh

    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16); // Below status bar
}

void MainMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); // Back to Dashboard
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        switch(idx) {
            case 0: // Settings -> OptionsScreen
                ScreenManager::getInstance().push(&optionsScreen);
                break;
            case 1: // Errors -> ErrorsScreen
                ScreenManager::getInstance().push(&errorsScreen);
                break;
            case 2: // Unload -> UnloadPalletsScreen
                ScreenManager::getInstance().push(&unloadPalletsScreen);
                break;
            case 3: // Engineering Menu -> PinEntryScreen -> EngineeringMenuScreen
                pinEntryScreen.setTarget(&engineeringMenuScreen);
                ScreenManager::getInstance().push(&pinEntryScreen);
                break;
            case 4: // Pallet Count
                 DataManager::getInstance().sendCommand(SP::CMD_COUNT_PALLETS);
                 break;
            case 5: // Compact
                 // For now, trigger CMD_COMPACT_F (assuming forward)
                 DataManager::getInstance().sendCommand(SP::CMD_COMPACT_F);
                 break;
            case 6: // Mode
                 // Toggle FIFO/LIFO
                 {
                    bool newMode = !((DataManager::getInstance().getTelemetry().stateFlags & 0x20) != 0);
                    DataManager::getInstance().setConfig(SP::CFG_FIFO_LIFO, newMode ? 1 : 0);
                    setDirty();
                 }
                 break;
            case 7: // Return Home
                 DataManager::getInstance().sendCommand(SP::CMD_HOME);
                 break;
            case 8: // Back
                 ScreenManager::getInstance().pop();
                 break;
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void MainMenuScreen::tick() {
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        setDirty();
    }
}
