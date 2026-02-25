#include "MainMenuScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include <cstdio> // For snprintf

MainMenuScreen::MainMenuScreen()
    : _statusBar(),
      _menuList(MainMenuScreen::provideMenuItem, MENU_ITEM_COUNT, 4)
{
}

void MainMenuScreen::onEnter() {
    EventBus::subscribe(this);
}

void MainMenuScreen::onExit() {
    EventBus::unsubscribe(this);
}

void MainMenuScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED || event == SystemEvent::CONFIG_UPDATED) {
        setDirty();
    }
}

void MainMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Settings"); break;
        case 1: strcpy(buffer, "Errors"); break;
        case 2: strcpy(buffer, "Unload Pallets"); break;
        case 3: strcpy(buffer, "Engineering Menu"); break;
        case 4:
            snprintf(buffer, 32, "Pallet Count: %d", DataManager::getInstance().getTelemetry().palleteCount);
            break;
        case 5: strcpy(buffer, "Compact"); break;
        case 6: {
            bool fifoLifo = (DataManager::getInstance().getTelemetry().stateFlags & 0x20) != 0;
            snprintf(buffer, 32, "Mode: %s", fifoLifo ? "LIFO" : "FIFO");
            break;
        }
        case 7: strcpy(buffer, "Return Home"); break;
        case 8: strcpy(buffer, "Back"); break;
        default: buffer[0] = '\0'; break;
    }
}

void MainMenuScreen::draw(U8G2& display) {
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
}
