#include "DebugInfoScreen.h"
#include "ScreenManager.h"

DebugInfoScreen::DebugInfoScreen() : _pageIndex(0) {}

void DebugInfoScreen::onEnter() {
    _pageIndex = 0;
    DataManager::getInstance().setPollContext(DataManager::PollContext::DEBUG_SENSORS);
    EventBus::subscribe(this);
}

void DebugInfoScreen::onExit() {
    EventBus::unsubscribe(this);
}

void DebugInfoScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::SENSORS_UPDATED) {
        setDirty();
    }
}

void DebugInfoScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);

    const SP::SensorPacket& sensors = DataManager::getInstance().getSensors();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    if (_pageIndex == 0) {
        display.setCursor(2, 22);
        display.print("Forw. chnl dist: " + String(sensors.distanceF));
        display.setCursor(2, 32);
        display.print("Rvrs. chnl dist: " + String(sensors.distanceR));
        display.setCursor(2, 42);
        display.print("Forw. plt dist:  " + String(sensors.distancePltF));
        display.setCursor(2, 52);
        display.print("Rvrs. plt dist:  " + String(sensors.distancePltR));
        display.setCursor(2, 62);
        display.print("Encoder ang: " + String(sensors.angle));

        // Simple activity indicator
        static uint8_t count = 0;
        count++;
        if (count > 3) count = 0;
        // display.drawGlyph(120, 20, 0x25f7 - count); // Requires symbol font
        display.setCursor(120, 20);
        if (count == 0) display.print("-");
        else if (count == 1) display.print("\\");
        else if (count == 2) display.print("|");
        else display.print("/");
    } else {
        display.setFont(u8g2_font_6x13_t_cyrillic);
        for(int i=0; i<8; i++) {
            bool state = (sensors.hardwareFlags & (1 << i)) != 0;
            display.setCursor(2, 20 + i*10);
            if (i == 0) display.print(String(state) + " DATCHIK_F1");
            else display.print(String(state) + " SENSOR_" + String(i));
        }
    }

    // Page indicator
    display.setCursor(100, 60);
    display.print(String(_pageIndex + 1) + "/2");
}

void DebugInfoScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    // Toggle page with UP/DOWN or OK?
    // Legacy used UP/DOWN to change cursorPos (1 or 2).
    if (event == InputEvent::UP_PRESS) {
        if (_pageIndex > 0) _pageIndex--;
        setDirty();
    }
    if (event == InputEvent::DOWN_PRESS) {
        if (_pageIndex < 1) _pageIndex++;
        setDirty();
    }
}

void DebugInfoScreen::tick() {
    DataManager::getInstance().setPollContext(DataManager::PollContext::DEBUG_SENSORS);
}
