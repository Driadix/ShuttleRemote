#include "UnloadPalletsScreen.h"
#include "ScreenManager.h"

UnloadPalletsScreen::UnloadPalletsScreen()
    : _spinner(2, 0), _state(ENTRY), _stateTimer(0), _quantity(0)
{
}

void UnloadPalletsScreen::onEnter() {
    _state = ENTRY;
    _stateTimer = 0;
    _spinner = NumericSpinnerWidget(2, 0); // Reset
    _quantity = 0;
}

void UnloadPalletsScreen::draw(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    switch (_state) {
        case ENTRY:
            display.setCursor(0, 20);
            display.print("Unload N Pallets:");
            _spinner.draw(display, 40, 40);
            display.setCursor(0, 60);
            display.print("Back:7   OK:Enter");
            break;

        case SUCCESS:
            display.setCursor(0, 25);
            display.print("Unloading " + String(_quantity) + " pallets");
            display.setCursor(0, 40);
            display.print("Started...");
            break;

        case FAIL:
            display.setCursor(0, 25);
            display.print("Quantity " + String(_quantity));
            display.setCursor(0, 40);
            display.print("Invalid / Cancel");
            break;
    }
}

void UnloadPalletsScreen::handleInput(InputEvent event) {
    if (_state != ENTRY) return; // Ignore input during message

    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_LONG_PRESS) { // Legacy uses OK (long?) or short?
        // Legacy main loop key 'D' (OK) handled inputQuant logic.
        // Let's use OK_LONG_PRESS to confirm action to be safe, or OK_SHORT_PRESS is usually navigation.
        // NumericSpinner uses OK_SHORT_PRESS to switch digits.
        // So we MUST use OK_LONG_PRESS to confirm entry.

        _quantity = _spinner.getValue();
        if (_quantity > 0) {
            if (DataManager::getInstance().sendCommand(SP::CMD_LONG_UNLOAD_QTY, _quantity)) {
                _state = SUCCESS;
            } else {
                // Queue full logic handled by Dashboard usually, but here?
                // Maybe just show Fail or retry?
                // Or let DataManager show specific error via dirty flag?
                // But DataManager::sendCommand returns false if queue full.
                // We can treat it as FAIL for now or just stay in ENTRY.
                // Legacy shows QUEUE FULL.
                // Let's show FAIL with specialized message? No space.
                // Just stay in ENTRY and maybe blink?
                // Or transition to FAIL state temporarily.
                 _state = FAIL; // Generic fail
            }
        } else {
            _state = FAIL;
        }
        _stateTimer = millis();
        setDirty();
        return;
    }

    if (_spinner.handleInput(event)) {
        setDirty();
    }
}

void UnloadPalletsScreen::tick() {
    if (_state != ENTRY) {
        if (millis() - _stateTimer > 2000) {
            if (_state == SUCCESS) {
                ScreenManager::getInstance().popToRoot(); // Go back to Dashboard after success
            } else {
                _state = ENTRY; // Retry
                _spinner = NumericSpinnerWidget(2, 0); // Reset
                setDirty();
            }
        }
    }
}
