#include "ChangeChannelScreen.h"
#include "ScreenManager.h"

extern void setRadioChannel(uint8_t ch);

ChangeChannelScreen::ChangeChannelScreen()
    : _spinner(3, 0)
{
}

void ChangeChannelScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::IDLE_KEEPALIVE);
    _spinner = NumericSpinnerWidget(3, 0);
}

void ChangeChannelScreen::draw(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    display.setCursor(0, 15);
    display.print("Enter New Channel:");

    _spinner.draw(display, 40, 35);

    display.setCursor(0, 60);
    display.print("Back:7   OK:Save");
}

void ChangeChannelScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_LONG_PRESS) {
        uint32_t val = _spinner.getValue();
        if (val > 100) val = 100;
        if (val < 1) val = 1;

        setRadioChannel((uint8_t)val);

        ScreenManager::getInstance().popToRoot();
        return;
    }

    if (_spinner.handleInput(event)) {
        setDirty();
    }
}

void ChangeChannelScreen::tick() {
}