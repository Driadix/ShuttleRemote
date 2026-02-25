#include "ChangeChannelScreen.h"
#include "ScreenManager.h"
#include "UI_Graph.h" // For MainMenuScreen? Or just pop to main.

// Helper to update radio (implemented in PultV1_0_2.ino)
extern void setRadioChannel(uint8_t ch);

ChangeChannelScreen::ChangeChannelScreen()
    : _spinner(3, 0) // Max 100? Legacy has 2 digits for channel? "dDm" but channel is usually 0-100.
    // Legacy: tempChannelNumber < 100.
    // So 2 digits enough? But 100 is 3 digits.
{
}

void ChangeChannelScreen::onEnter() {
    // Current channel?
    // DataManager doesn't expose channel. It's radio config.
    // Let's default to 0 or try to read?
    // Legacy reads `configArray[4]`.
    // We can assume user enters new value from 0.
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
        // Save
        uint32_t val = _spinner.getValue();
        if (val > 100) val = 100;
        if (val < 1) val = 1; // Legacy > 1 checks

        setRadioChannel((uint8_t)val);

        // Go back to Main Dashboard or Menu?
        // Legacy goes to MAIN.
        ScreenManager::getInstance().popToRoot();
        return;
    }

    if (_spinner.handleInput(event)) {
        setDirty();
    }
}

void ChangeChannelScreen::tick() {
    // No polling needed
}
