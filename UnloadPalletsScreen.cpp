#include "UnloadPalletsScreen.h"
#include "ScreenManager.h"

UnloadPalletsScreen::UnloadPalletsScreen()
    : _spinner(2, 0), _state(ENTRY), _stateTimer(0), _quantity(0)
{
}

void UnloadPalletsScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    _state = ENTRY;
    _stateTimer = 0;
    _spinner = NumericSpinnerWidget(2, 0);
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
    if (_state != ENTRY) return;

    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_LONG_PRESS) {
        _quantity = _spinner.getValue();
        if (_quantity > 0) {
            if (DataManager::getInstance().sendCommand(SP::CMD_LONG_UNLOAD_QTY, _quantity)) {
                _state = SUCCESS;
            } else {
                 _state = FAIL;
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
                ScreenManager::getInstance().popToRoot();
            } else {
                _state = ENTRY;
                _spinner = NumericSpinnerWidget(2, 0);
                setDirty();
            }
        }
    }
}