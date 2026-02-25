#include "ChangeShuttleNumScreen.h"
#include "ScreenManager.h"

ChangeShuttleNumScreen::ChangeShuttleNumScreen()
    : _spinner(2, 1)
{
}

void ChangeShuttleNumScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    _spinner = NumericSpinnerWidget(2, DataManager::getInstance().getShuttleNumber());
}

void ChangeShuttleNumScreen::draw(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    display.setCursor(0, 15);
    display.print("New Shuttle Num:");

    _spinner.draw(display, 40, 35);

    display.setCursor(0, 60);
    display.print("Back:7   OK:Save");
}

void ChangeShuttleNumScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_LONG_PRESS) {
        uint32_t val = _spinner.getValue();
        if (val > 32) val = 32;
        if (val < 1) val = 1;

        DataManager::getInstance().saveLocalShuttleNumber((uint8_t)val);

        ScreenManager::getInstance().popToRoot();
        return;
    }

    if (_spinner.handleInput(event)) {
        setDirty();
    }
}

void ChangeShuttleNumScreen::tick() {
}
