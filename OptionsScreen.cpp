#include "OptionsScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include <cstdio>

// Static definitions
char OptionsScreen::_optItemBuffers[OptionsScreen::OPT_ITEM_COUNT][32];
const char* OptionsScreen::_optItemsPtrs[OptionsScreen::OPT_ITEM_COUNT];

OptionsScreen::OptionsScreen()
    : _statusBar(),
      _menuList(_optItemsPtrs, OPT_ITEM_COUNT, 4)
{
    for (int i = 0; i < OPT_ITEM_COUNT; i++) {
        _optItemsPtrs[i] = _optItemBuffers[i];
    }
}

void OptionsScreen::onEnter() {
    // Request config params to ensure fresh data
    DataManager::getInstance().requestConfig(SP::CFG_INTER_PALLET);
    DataManager::getInstance().requestConfig(SP::CFG_REVERSE_MODE);
    DataManager::getInstance().requestConfig(SP::CFG_MAX_SPEED);
    DataManager::getInstance().requestConfig(SP::CFG_MIN_BATT);
    updateMenuItems();
}

void OptionsScreen::updateMenuItems() {
    // 0. MPR (Inter Pallet Distance)
    snprintf(_optItemBuffers[0], 32, "MPR: %d mm", DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET));

    // 1. Reverse Mode
    bool rev = DataManager::getInstance().getConfig(SP::CFG_REVERSE_MODE) != 0;
    snprintf(_optItemBuffers[1], 32, "Rev Mode: %s", rev ? "<<" : ">>");

    // 2. Max Speed (Stored as ~0-96, displayed as percentage?)
    // Legacy: speedset / 10. If speedset is 1000 -> 100%.
    // Protocol sends speedset directly.
    // Let's just show raw value or assume it's scaled.
    // Legacy: `speedset` is 1000 max. Display `speedset/10`.
    // DataManager stores `value` as int32_t.
    snprintf(_optItemBuffers[2], 32, "Max Speed: %d%%", DataManager::getInstance().getConfig(SP::CFG_MAX_SPEED) / 10);

    // 3. Change N Device
    snprintf(_optItemBuffers[3], 32, "Change ID: %d", DataManager::getInstance().getShuttleNumber());

    // 4. Battery Protection
    snprintf(_optItemBuffers[4], 32, "Batt Prot: %d%%", DataManager::getInstance().getConfig(SP::CFG_MIN_BATT));

    // 5. Engineering Menu
    snprintf(_optItemBuffers[5], 32, "Engineering Menu");

    // 6. Save Params
    snprintf(_optItemBuffers[6], 32, "Save Params");

    // 7. Back
    snprintf(_optItemBuffers[7], 32, "Back");
}

void OptionsScreen::draw(U8G2& display) {
    updateMenuItems();
    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16);
}

void OptionsScreen::adjustValue(int idx, bool increase) {
    int32_t val;
    switch(idx) {
        case 0: // MPR
            val = DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET);
            val += (increase ? 10 : -10);
            if (val < 50) val = 50; // Legacy min
            if (val > 400) val = 400; // Legacy max
            DataManager::getInstance().setConfig(SP::CFG_INTER_PALLET, val);
            break;

        case 1: // Reverse Mode
            val = DataManager::getInstance().getConfig(SP::CFG_REVERSE_MODE);
            val = !val; // Toggle
            DataManager::getInstance().setConfig(SP::CFG_REVERSE_MODE, val);
            break;

        case 2: // Max Speed
            val = DataManager::getInstance().getConfig(SP::CFG_MAX_SPEED);
            val += (increase ? 50 : -50);
            if (val < 200) val = 200;
            if (val > 1000) val = 1000;
            DataManager::getInstance().setConfig(SP::CFG_MAX_SPEED, val);
            break;

        case 4: // Batt Prot
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
        ScreenManager::getInstance().pop();
        return;
    }

    int idx = _menuList.getCursorIndex();

    // Adjust values with 'E' (UP) and '9' (DOWN) keys (LIFT_UP/DOWN)
    if (event == InputEvent::LIFT_UP_PRESS) {
        adjustValue(idx, true);
        return;
    }
    if (event == InputEvent::LIFT_DOWN_PRESS) {
        adjustValue(idx, false);
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        switch(idx) {
            case 1: // Toggle Reverse Mode
                adjustValue(idx, true);
                break;
            case 3: // Change ID
                ScreenManager::getInstance().push(&changeShuttleNumScreen);
                break;
            case 5: // Engineering Menu
                pinEntryScreen.setTarget(&engineeringMenuScreen);
                ScreenManager::getInstance().push(&pinEntryScreen);
                break;
            case 6: // Save Params
                DataManager::getInstance().sendCommand(SP::CMD_SAVE_EEPROM);
                break;
            case 7: // Back
                ScreenManager::getInstance().pop();
                break;
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void OptionsScreen::tick() {
    if (DataManager::getInstance().consumeConfigDirtyFlag()) {
        setDirty();
    }
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        setDirty();
    }
}
