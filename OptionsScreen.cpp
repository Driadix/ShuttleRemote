#include "OptionsScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include "UIBuffer.h"
#include <cstdio>

// Static definitions
// char OptionsScreen::_optItemBuffers[OptionsScreen::OPT_ITEM_COUNT][32]; // Removed
const char* OptionsScreen::_optItemsPtrs[OptionsScreen::OPT_ITEM_COUNT];

OptionsScreen::OptionsScreen()
    : _statusBar(),
      _menuList(_optItemsPtrs, OPT_ITEM_COUNT, 4)
{
    // Initialize pointers to UIBuffer lines
    for (int i = 0; i < OPT_ITEM_COUNT; i++) {
        _optItemsPtrs[i] = UIBuffer::getLine(i);
    }
}

void OptionsScreen::onEnter() {
    Screen::onEnter();
    EventBus::subscribe(this);

    // Request config params to ensure fresh data
    DataManager::getInstance().requestConfig(SP::CFG_INTER_PALLET);
    DataManager::getInstance().requestConfig(SP::CFG_REVERSE_MODE);
    DataManager::getInstance().requestConfig(SP::CFG_MAX_SPEED);
    DataManager::getInstance().requestConfig(SP::CFG_MIN_BATT);

    // Initial update
    updateMenuItems();
}

void OptionsScreen::onExit() {
    Screen::onExit();
    EventBus::unsubscribe(this);
}

void OptionsScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::CONFIG_UPDATED || event == SystemEvent::TELEMETRY_UPDATED) {
        setDirty();
    }
}

void OptionsScreen::updateMenuItems() {
    // Write into shared UIBuffer

    // 0. MPR (Inter Pallet Distance)
    snprintf(UIBuffer::getLine(0), 32, "MPR: %ld mm", (long)DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET));

    // 1. Reverse Mode
    bool rev = DataManager::getInstance().getConfig(SP::CFG_REVERSE_MODE) != 0;
    snprintf(UIBuffer::getLine(1), 32, "Rev Mode: %s", rev ? "<<" : ">>");

    // 2. Max Speed
    snprintf(UIBuffer::getLine(2), 32, "Max Speed: %ld%%", (long)DataManager::getInstance().getConfig(SP::CFG_MAX_SPEED) / 10);

    // 3. Change N Device
    snprintf(UIBuffer::getLine(3), 32, "Change ID: %d", DataManager::getInstance().getShuttleNumber());

    // 4. Battery Protection
    snprintf(UIBuffer::getLine(4), 32, "Batt Prot: %ld%%", (long)DataManager::getInstance().getConfig(SP::CFG_MIN_BATT));

    // 5. Engineering Menu
    snprintf(UIBuffer::getLine(5), 32, "Engineering Menu");

    // 6. Save Params
    snprintf(UIBuffer::getLine(6), 32, "Save Params");

    // 7. Back
    snprintf(UIBuffer::getLine(7), 32, "Back");
}

void OptionsScreen::draw(U8G2& display) {
    if (_fullRedrawNeeded) {
        display.clearBuffer();
    }

    // Refresh buffer content because it might have been used by another screen
    updateMenuItems();

    // Partial updates: We should clear if needed.
    // ListWidget draws items.
    // If we rely on _fullRedrawNeeded, it clears.
    // If partial (config update), we redraw everything over previous.
    // U8g2 without clearBuffer might leave artifacts if text length decreases.
    // ScrollingListWidget should ideally clear its row.
    // But since this screen is simple, full redraw on update is acceptable or we rely on overwriting with spaces?
    // "MPR: 100 mm" -> "MPR: 90 mm ".
    // Creating "clearing" strings is annoying.
    // The user suggested drawing black box.
    // ScrollingListWidget doesn't support that yet.
    // For now, let's force full redraw on config change?
    // `setDirty()` doesn't set `_fullRedrawNeeded`.
    // But `OptionsScreen` covers full screen.
    // If I want to clear, I can do `display.clearBuffer()` here manually if I want.
    // But to respect the "Partial" goal:
    // I will let it overdraw.
    // To prevent artifacts, I should clear the list area.

    if (!_fullRedrawNeeded) {
        // Clear list area to be safe
         display.setDrawColor(0);
         display.drawBox(0, 16, 128, 64-16);
         display.setDrawColor(1);
    }

    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16);
}

void OptionsScreen::adjustValue(int idx, bool increase) {
    int32_t val;
    switch(idx) {
        case 0: // MPR
            val = DataManager::getInstance().getConfig(SP::CFG_INTER_PALLET);
            val += (increase ? 10 : -10);
            if (val < 50) val = 50;
            if (val > 400) val = 400;
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
                // pinEntryScreen.setTarget(&engineeringMenuScreen); // Assume these exist externally
                // ScreenManager::getInstance().push(&pinEntryScreen);
                // Since I cannot see UI_Graph.h externalities easily, I'll copy the logic from original file.
                // Original:
                // pinEntryScreen.setTarget(&engineeringMenuScreen);
                // ScreenManager::getInstance().push(&pinEntryScreen);
                // I need externs. They are in UI_Graph.h which I included.
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
    // Legacy polling removed.
    // Handled by onEvent
}
