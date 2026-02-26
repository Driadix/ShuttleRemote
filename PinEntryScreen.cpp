#include "PinEntryScreen.h"
#include "DataManager.h"
#include "ScreenManager.h"

PinEntryScreen::PinEntryScreen()
    : _spinner(4, 0), _targetScreen(nullptr), _showAccessDenied(false), _accessDeniedTimer(0), PIN_CODE(1441)
{
}

void PinEntryScreen::setTarget(Screen* target) {
    _targetScreen = target;
}

void PinEntryScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    _showAccessDenied = false;
    _accessDeniedTimer = 0;
    // Reset spinner
    _spinner = NumericSpinnerWidget(4, 0);
}

void PinEntryScreen::draw(U8G2& display) {
    if (_showAccessDenied) {
        display.setFont(u8g2_font_6x13_t_cyrillic);
        display.setDrawColor(1);
        display.setCursor(10, 30);
        display.print("ДОСТУП ЗАПРЕЩЕН");
        return;
    }

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);
    display.setCursor(10, 15);
    display.print("ВВЕДИТЕ PIN:");

    _spinner.draw(display, 20, 35);
}

void PinEntryScreen::handleInput(InputEvent event) {
    if (_showAccessDenied) return;

    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_LONG_PRESS) {
        if (_spinner.getValue() == PIN_CODE) {
            ScreenManager::getInstance().pop(); // Remove PinEntry
            if (_targetScreen) {
                ScreenManager::getInstance().push(_targetScreen);
            }
        } else {
            _showAccessDenied = true;
            _accessDeniedTimer = millis();
            setDirty();
        }
        return;
    }

    // Delegate to spinner
    if (_spinner.handleInput(event)) {
        setDirty();
    }
}

void PinEntryScreen::tick() {
    if (_showAccessDenied) {
        if (millis() - _accessDeniedTimer > 2000) {
            _showAccessDenied = false;
            _spinner = NumericSpinnerWidget(4, 0); // Reset
            setDirty();
        }
    }
}
